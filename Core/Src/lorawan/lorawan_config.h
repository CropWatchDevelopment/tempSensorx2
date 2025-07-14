// lorawan_config.h
// Header for LoRaWAN module configuration

#ifndef LORAWAN_CONFIG_H
#define LORAWAN_CONFIG_H

#include <stdbool.h>
#include "atc.h"

/**
 * @brief  Configure the LoRaWAN module with predefined settings.
 * @param  lora  Pointer to an initialized ATC handle
 * @return true on success, false on any AT command failure
 */
bool lorawan_configure(ATC_HandleTypeDef *lora, const char *dev_eui, const char *app_eui, const char *app_key);

#endif // LORAWAN_CONFIG_H
