#ifndef LORAWAN_H
#define LORAWAN_H

#include "main.h"
#include <stdint.h>
#include <stddef.h>

typedef enum {
    LORAWAN_ERROR_OK = 0,
    LORAWAN_ERROR_INVALID_PARAM = -1,
    LORAWAN_ERROR_COMMUNICATION = -2,
    LORAWAN_ERROR_UNEXPECTED_RESPONSE = -3,
    LORAWAN_ERROR_TIMEOUT = -4,
    LORAWAN_ERROR_NOT_JOINED = -5
} LoRaWAN_Error_t;

typedef struct {
    UART_HandleTypeDef *huart;
} ATC_HandleTypeDef;

LoRaWAN_Error_t send_data_and_get_response(ATC_HandleTypeDef *lora, const char *data, char *response, uint32_t response_size, uint32_t timeout_ms, const char *expected_response);
LoRaWAN_Error_t LoRaWAN_Join(ATC_HandleTypeDef *lora);
LoRaWAN_Error_t LoRaWAN_SendHex(ATC_HandleTypeDef *lora, const uint8_t *payload, size_t length);

// Setup commands
LoRaWAN_Error_t LoRaWAN_Set_Battery(ATC_HandleTypeDef *lora, int battery);
LoRaWAN_Error_t LoRaWAN_Set_Battery_Status(ATC_HandleTypeDef *lora, uint8_t battery_percentage, int measurement_success);

#endif // LORAWAN_H
