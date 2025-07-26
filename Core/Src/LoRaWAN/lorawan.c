#include "lorawan.h"
#include <string.h>
#include <stdio.h>

static int ATC_SendReceive(ATC_HandleTypeDef *lora, const char *command, uint32_t command_len, char *response, uint32_t response_size, uint32_t timeout_ms, const char *expected_response)
{
    if (lora == NULL || lora->huart == NULL || command == NULL || command_len == 0) {
        return -1;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit(lora->huart, (uint8_t *)command, command_len, timeout_ms);
    if (status != HAL_OK) {
        return -2;
    }

    if (response != NULL && response_size > 0) {
        uint16_t rx_len = 0;
        memset(response, 0, response_size);
        status = HAL_UARTEx_ReceiveToIdle(lora->huart, (uint8_t *)response, response_size - 1, &rx_len, timeout_ms);
        if (status != HAL_OK) {
            return -4;
        }
        response[rx_len] = '\0';
    }

    if (expected_response && response) {
        if (!strstr(response, expected_response)) {
            return -3;
        }
    }

    return 0;
}

LoRaWAN_Error_t send_data_and_get_response(ATC_HandleTypeDef *lora, const char *data, char *response, uint32_t response_size, uint32_t timeout_ms, const char *expected_response)
{
    if (!lora || !lora->huart || !data || !response || response_size == 0) {
        return LORAWAN_ERROR_INVALID_PARAM;
    }

    ATC_SendReceive(lora, "AT\r\n", 4, response, response_size, timeout_ms, expected_response);
    HAL_Delay(300);
    int result = ATC_SendReceive(lora, data, strlen(data), response, response_size, timeout_ms, expected_response);

    if (result == -1) {
        return LORAWAN_ERROR_INVALID_PARAM;
    } else if (result == -2) {
        return LORAWAN_ERROR_COMMUNICATION;
    } else if (result == -3) {
        return LORAWAN_ERROR_UNEXPECTED_RESPONSE;
    } else if (result == -4) {
        return LORAWAN_ERROR_TIMEOUT;
    }

    return LORAWAN_ERROR_OK;
}

LoRaWAN_Error_t LoRaWAN_Join(ATC_HandleTypeDef *lora)
{
    char response[256];
    LoRaWAN_Error_t status = send_data_and_get_response(lora, "AT+JOIN\r\n", response, sizeof(response), 10000, "OK");
    if (status != LORAWAN_ERROR_OK) {
        return status;
    }

    memset(response, 0, sizeof(response));
    uint16_t rx_len = 0;
    HAL_StatusTypeDef hal_status = HAL_UARTEx_ReceiveToIdle(lora->huart, (uint8_t *)response, sizeof(response) - 1, &rx_len, 10000);
    if (hal_status != HAL_OK) {
        return LORAWAN_ERROR_TIMEOUT;
    }
    response[rx_len] = '\0';

    if (strstr(response, "JOINED")) {
        return LORAWAN_ERROR_OK;
    } else if (strstr(response, "JOIN FAILED")) {
        return LORAWAN_ERROR_NOT_JOINED;
    }
    return LORAWAN_ERROR_UNEXPECTED_RESPONSE;
}

LoRaWAN_Error_t LoRaWAN_SendHex(ATC_HandleTypeDef *lora, const uint8_t *payload, size_t length, int fPort)
{
    if (!lora || !lora->huart || !payload || length == 0 || fPort < 1 || fPort > 198) {
        return LORAWAN_ERROR_INVALID_PARAM;
    }

    // Step 1: Set the fPort using ATS
    char port_command[32];
    snprintf(port_command, sizeof(port_command), "ATS 629=%d\r\n", fPort);

    char response[64];
    LoRaWAN_Error_t port_status = send_data_and_get_response(lora, port_command, response, sizeof(response), 3000, "OK");
    if (port_status != LORAWAN_ERROR_OK) {
        return port_status;
    }

    // Step 2: Convert payload to hex
    char hex[length * 2 + 1];
    for (size_t i = 0; i < length; ++i) {
        sprintf(&hex[i * 2], "%02X", payload[i]);
    }
    hex[length * 2] = '\0';

    // Step 3: Build AT+SEND command
    char command[length * 2 + 16];
    snprintf(command, sizeof(command), "AT+SEND \"%s\"\r\n", hex);

    // Step 4: Send the command
    return send_data_and_get_response(lora, command, response, sizeof(response), 5000, "OK");
}

