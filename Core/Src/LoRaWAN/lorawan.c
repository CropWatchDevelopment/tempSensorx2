/* lorawan.c - Clean, valid C implementation for LoRaWAN AT commands */
#include "lorawan.h"
#include <string.h>
#include <stdio.h>

extern void ConsolePrintf(const char *format, ...);

// Send an AT command and optionally check for an expected substring
static int ATC_SendReceive(
    ATC_HandleTypeDef *lora,
    const char *command,
    uint32_t command_len,
    char *response,
    uint32_t response_size,
    uint32_t timeout_ms,
    const char *expected_response
) {
    if (!lora || !lora->huart || !command || command_len == 0) {
        return -1;
    }

    // Transmit command
    HAL_StatusTypeDef status = HAL_UART_Transmit(
        lora->huart,
        (uint8_t *)command,
        command_len,
        timeout_ms
    );
    if (status != HAL_OK) {
        ConsolePrintf("ATC_SendReceive: TX failed (status=%d) for '%.*s'\r\n",
                      (int)status,
                      (int)command_len,
                      command);
        return -2;
    }

    // Receive response
    if (response && response_size > 0) {
        uint16_t rx_len = 0;
        memset(response, 0, response_size);
        status = HAL_UARTEx_ReceiveToIdle(
            lora->huart,
            (uint8_t *)response,
            response_size - 1,
            &rx_len,
            timeout_ms
        );
        ConsolePrintf("ATC_SendReceive: RX status=%d, rx_len=%u\r\n",
                      (int)status,
                      (unsigned)rx_len);
        if (rx_len) {
            ConsolePrintf("ATC_SendReceive: RX '%.*s'\r\n",
                          (int)rx_len,
                          response);
        }
        if (status != HAL_OK) {
            return -4;
        }
    }

    // Validate expected substring
    if (expected_response && response) {
        if (!strstr(response, expected_response)) {
            return -3;
        }
    }

    return 0;
}

// Wake module: send "AT" and parse the combined WAKE+OK or OK banner
static LoRaWAN_Error_t wake_module(
    ATC_HandleTypeDef *lora,
    char *response,
    uint32_t response_size,
    uint32_t timeout_ms
) {
    // Flush UART RX FIFO
    __HAL_UART_CLEAR_OREFLAG(lora->huart);
    __HAL_UART_CLEAR_IDLEFLAG(lora->huart);
    uint8_t dummy;
    while (__HAL_UART_GET_FLAG(lora->huart, UART_FLAG_RXNE)) {
        HAL_UART_Receive(lora->huart, &dummy, 1, 10);
    }

    // Send AT and read banner
    const char *wake_cmd = "AT\r\n";
    ConsolePrintf("wake_module: sending AT and capturing banner\r\n");
    int rc = ATC_SendReceive(
        lora,
        wake_cmd,
        (uint32_t)strlen(wake_cmd),
        response,
        response_size,
        timeout_ms,
        NULL
    );
    if (rc != 0) {
        ConsolePrintf("wake_module: AT transmit error (rc=%d)\r\n", rc);
        return LORAWAN_ERROR_COMMUNICATION;
    }

    // Give module time to send the rest of the banner
    HAL_Delay(100);

    // response already holds full banner
    ConsolePrintf("wake_module: banner='%s'\r\n", response);

    // Check for WAKE or OK
    if (strstr(response, "WAKE") || strstr(response, "OK")) {
        return LORAWAN_ERROR_OK;
    }
    return LORAWAN_ERROR_COMMUNICATION;
}

LoRaWAN_Error_t send_data_and_get_response(
    ATC_HandleTypeDef *lora,
    const char *data,
    char *response,
    uint32_t response_size,
    uint32_t timeout_ms,
    const char *expected_response
) {
    if (!lora || !lora->huart || !data || !response || response_size == 0) {
        return LORAWAN_ERROR_INVALID_PARAM;
    }

    // Wake the module
    LoRaWAN_Error_t wake_status = wake_module(
        lora,
        response,
        response_size,
        timeout_ms
    );
    if (wake_status != LORAWAN_ERROR_OK) {
        return wake_status;
    }

    HAL_Delay(50);
    ConsolePrintf("send_data_and_get_response: sending '%s'\r\n", data);

    // Send real command and validate response
    int res = ATC_SendReceive(
        lora,
        data,
        (uint32_t)strlen(data),
        response,
        response_size,
        timeout_ms,
        expected_response
    );
    switch (res) {
        case -1: return LORAWAN_ERROR_INVALID_PARAM;
        case -2: return LORAWAN_ERROR_COMMUNICATION;
        case -3: return LORAWAN_ERROR_UNEXPECTED_RESPONSE;
        case -4: return LORAWAN_ERROR_TIMEOUT;
        default:  return LORAWAN_ERROR_OK;
    }
}

LoRaWAN_Error_t LoRaWAN_Join(ATC_HandleTypeDef *lora) {
    char response[256] = {0};
    ConsolePrintf("LoRaWAN_Join: start\r\n");
    LoRaWAN_Error_t status = send_data_and_get_response(
        lora,
        "AT+JOIN\r\n",
        response,
        sizeof(response),
        10000,
        "OK"
    );
    if (status != LORAWAN_ERROR_OK) {
        return status;
    }

    // Wait for JOINED or JOIN FAILED
    memset(response, 0, sizeof(response));
    uint16_t rx_len = 0;
    if (HAL_UARTEx_ReceiveToIdle(
            lora->huart,
            (uint8_t *)response,
            sizeof(response) - 1,
            &rx_len,
            10000
        ) == HAL_OK) {
        response[rx_len] = '\0';
    }
    ConsolePrintf("LoRaWAN_Join: async banner '%s'\r\n", response);
    if (strstr(response, "JOINED")) {
        return LORAWAN_ERROR_OK;
    }
    if (strstr(response, "JOIN FAILED")) {
        return LORAWAN_ERROR_NOT_JOINED;
    }
    return LORAWAN_ERROR_UNEXPECTED_RESPONSE;
}

LoRaWAN_Error_t LoRaWAN_SendHex(
    ATC_HandleTypeDef *lora,
    const uint8_t *payload,
    size_t length
) {
    if (!lora || !lora->huart || !payload || length == 0) {
        return LORAWAN_ERROR_INVALID_PARAM;
    }
    // Convert payload to hex string
    char hex[2 * length + 1];
    for (size_t i = 0; i < length; ++i) {
        sprintf(&hex[2 * i], "%02X", payload[i]);
    }
    hex[2 * length] = '\0';

    // Build AT+SEND command
    char cmd[2 * length + 12];
    snprintf(cmd, sizeof(cmd), "AT+SEND \"%s\"\r\n", hex);

    // Local response buffer
    char response[64] = {0};
    return send_data_and_get_response(
        lora,
        cmd,
        response,
        sizeof(response),
        5000,
        "OK"
    );
}

LoRaWAN_Error_t LoRaWAN_Join_Status(ATC_HandleTypeDef *lora) {
    char resp[32] = {0};
    LoRaWAN_Error_t st = send_data_and_get_response(
        lora,
        "ATI 3001\r\n",
        resp,
        sizeof(resp),
        300,
        "1"
    );
    return (st == LORAWAN_ERROR_OK && strstr(resp, "1"))
        ? LORAWAN_ERROR_OK
        : LORAWAN_ERROR_NOT_JOINED;
}

LoRaWAN_Error_t LoRaWAN_Set_Battery(
    ATC_HandleTypeDef *lora,
    int level
) {
    if (!lora || !lora->huart || level < 0 || level > 100) {
        return LORAWAN_ERROR_INVALID_PARAM;
    }
    char cmd[16];
    int len = snprintf(cmd, sizeof(cmd), "AT+BAT=%d\r\n", level);
    if (len < 0 || len >= (int)sizeof(cmd)) {
        return LORAWAN_ERROR_INVALID_PARAM;
    }
    char response[64] = {0};
    return send_data_and_get_response(
        lora,
        cmd,
        response,
        sizeof(response),
        5000,
        "OK"
    );
}
