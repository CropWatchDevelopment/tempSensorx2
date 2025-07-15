// lorawan_config.h
// Header for LoRaWAN module configuration

#ifndef LORAWAN_CONFIG_H
#define LORAWAN_CONFIG_H

#include <stdbool.h>
#include "atc.h"

// Error codes
typedef enum {
    LORAWAN_OK = 0,
    LORAWAN_ERR_DEV_EUI,
    LORAWAN_ERR_APP_EUI,
    LORAWAN_ERR_APP_KEY,
    LORAWAN_ERR_FREQ_CHECK,
    LORAWAN_ERR_AT_COMMAND,
    LORAWAN_ERR_SAVE_RESET,
    LORAWAN_ERR_JOIN
} LoRaWAN_Error_t;

/**
 * @brief  Configure the LoRaWAN module with predefined settings.
 * @param  lora  Pointer to an initialized ATC handle
 * @return true on success, false on any AT command failure
 */
bool lorawan_configure(ATC_HandleTypeDef *lora, const char *dev_eui, const char *app_eui, const char *app_key);

/**
 * @brief  Initiates the LoRaWAN network join process using OTAA.
 * @param  lora  Pointer to an initialized ATC handle
 * @return LORAWAN_OK on success, error code on failure
 */
LoRaWAN_Error_t join_network(ATC_HandleTypeDef *lora);

#endif // LORAWAN_CONFIG_H
