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

    // Wake Module from Sleep (if it is asleep)
    ATC_SendReceive(lora, "AT\r\n", 4, response, response_size, timeout_ms, expected_response);
    HAL_Delay(300);

    char* isConnectedResponse = NULL;
    int isConnectedStatus = ATC_SendReceive(lora, "ATI 3001\r\n", 10, isConnectedResponse, response_size, 300, "1");
    if (isConnectedStatus == 0)
    {
    	LoRaWAN_Join(lora);
    }

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

LoRaWAN_Error_t LoRaWAN_SendHex(ATC_HandleTypeDef *lora, const uint8_t *payload, size_t length)
{
    if (!lora || !lora->huart || !payload || length == 0) {
        return LORAWAN_ERROR_INVALID_PARAM;
    }

    char hex[length * 2 + 1];
    for (size_t i = 0; i < length; ++i) {
        sprintf(&hex[i * 2], "%02X", payload[i]);
    }
    hex[length * 2] = '\0';

    char command[length * 2 + 12];
    snprintf(command, sizeof(command), "AT+SEND \"%s\"\r\n", hex);

    char response[64];
    return send_data_and_get_response(lora, command, response, sizeof(response), 5000, "OK");
}

LoRaWAN_Error_t LoRaWAN_Set_Battery(ATC_HandleTypeDef *lora, int level)
{
    // 1. Validate parameters
    if (lora == NULL || lora->huart == NULL) {
        return LORAWAN_ERROR_INVALID_PARAM;
    }
    if (level < 0 || level > 100) {
        return LORAWAN_ERROR_INVALID_PARAM;
    }

    // 2. Map percentage (0-100) to LoRaWAN battery status (1-254)
    //    0% maps to 1 (minimum)
    //    100% maps to 254 (maximum)
    //    Formula: battery_status = 1 + (level * 253) / 100
    int battery_status;
    if (level == 0) {
        battery_status = 1;  // Minimum battery level
    } else {
        battery_status = 1 + (level * 253) / 100;
        // Ensure we don't exceed 254
        if (battery_status > 254) {
            battery_status = 254;
        }
    }

    // 3. Build the AT command string
    char command[16];
    int n = snprintf(command, sizeof(command), "AT+BAT=%d\r\n", battery_status);
    if (n < 0 || n >= (int)sizeof(command)) {
        // formatting error or truncated
        return LORAWAN_ERROR_INVALID_PARAM;
    }

    // 4. Send and wait for "OK"
    char response[64];
    return send_data_and_get_response(
        lora,
        command,
        response,
        sizeof(response),
        5000,      // timeout in ms
        "OK"       // expected response
    );
}

LoRaWAN_Error_t LoRaWAN_Set_Battery_Status(ATC_HandleTypeDef *lora, uint8_t battery_percentage, int measurement_success)
{
    // 1. Validate parameters
    if (lora == NULL || lora->huart == NULL) {
        return LORAWAN_ERROR_INVALID_PARAM;
    }

    int battery_status;
    
    // 2. Determine battery status based on measurement success
    if (!measurement_success) {
        // Battery measurement failed - use 255 (unable to measure)
        battery_status = 255;
    } else {
        // Battery measurement succeeded - map percentage (0-100) to LoRaWAN battery status (1-254)
        if (battery_percentage == 0) {
            battery_status = 1;  // Minimum battery level
        } else if (battery_percentage >= 100) {
            battery_status = 254;  // Maximum battery level
        } else {
            battery_status = 1 + (battery_percentage * 253) / 100;
        }
    }

    // 3. Build the AT command string
    char command[16];

    int n = snprintf(command, sizeof(command), "AT+BAT=%d\r\n", battery_status);
    if (n < 0 || n >= (int)sizeof(command)) {
        // formatting error or truncated
        return LORAWAN_ERROR_INVALID_PARAM;
    }

    // 4. Send and wait for "OK"
    char response[64];
    return send_data_and_get_response(
        lora,
        command,
        response,
        sizeof(response),
        5000,      // timeout in ms
        "OK"       // expected response
    );
}

