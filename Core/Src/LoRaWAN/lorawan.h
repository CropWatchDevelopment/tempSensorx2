#ifndef LORAWAN_H
#define LORAWAN_H

#include "main.h"
#include <stdint.h>
#include <stddef.h>

#define APPLICATION_PORT_PARAM 629

typedef enum {
    LORAWAN_ERROR_OK = 0,
    LORAWAN_ERROR_INVALID_PARAM = -1,
    LORAWAN_ERROR_COMMUNICATION = -2,
    LORAWAN_ERROR_UNEXPECTED_RESPONSE = -3,
    LORAWAN_ERROR_TIMEOUT = -4,
    LORAWAN_ERROR_NOT_JOINED = -5
} LoRaWAN_Error_t;

typedef enum {
	JOIN_TIMEOUT_MS = 10000,
} LoRaWAN_JOIN_t;

typedef struct {
    UART_HandleTypeDef *huart;
} ATC_HandleTypeDef;

LoRaWAN_Error_t send_data_and_get_response(ATC_HandleTypeDef *lora, const char *data, char *response, uint32_t response_size, uint32_t timeout_ms, const char *expected_response);
LoRaWAN_Error_t LoRaWAN_Join(ATC_HandleTypeDef *lora);
LoRaWAN_Error_t LoRaWAN_Join_Status(ATC_HandleTypeDef *lora);
LoRaWAN_Error_t LoRaWAN_SetPort(ATC_HandleTypeDef *lora, uint8_t port);
LoRaWAN_Error_t LoRaWAN_SendHex(ATC_HandleTypeDef *lora, const uint8_t *payload, size_t length);
LoRaWAN_Error_t LoRaWAN_SendHexOnPort(ATC_HandleTypeDef *lora, uint8_t port, const uint8_t *payload, size_t length);

LoRaWAN_Error_t LoRaWAN_Set_Battery(ATC_HandleTypeDef *lora, uint8_t batteryStatus);
LoRaWAN_Error_t LoRaWAN_UpdateBattery(ATC_HandleTypeDef *lora);


// Setup commands
LoRaWAN_Error_t LoRaWAN_Set_Battery_Status(ATC_HandleTypeDef *lora, uint8_t battery_percentage, int measurement_success);

#endif // LORAWAN_H
