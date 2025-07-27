/* Core/Inc/battery.h
 *
 * Exposes:
 *  • ReadBatteryVoltage()  — raw voltage in volts, or –1.0f on error
 *  • GetBatteryLevel()     — voltage + percentage, returns status
 */

#ifndef BATTERY_H
#define BATTERY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32l0xx_hal.h"   // for HAL_StatusTypeDef, GPIO macros, etc.

/* Status codes for GetBatteryLevel() */
typedef enum {
    BATTERY_OK       = 0,
    BATTERY_ADC_ERROR = -1,
    BATTERY_TIMEOUT   = -2,
    BATTERY_ERROR     = -3
} Battery_Status_t;

/**
 * @brief  Read the battery voltage (halved by divider + buffered).
 * @return VBAT in volts, or –1.0f on error.
 */
float ReadBatteryVoltage(void);

/**
 * @brief  Read VBAT and compute a 0–100% percentage.
 * @param  out_v     pointer to float → battery voltage [V]
 * @param  out_pct   pointer to uint8_t → battery % (0…100)
 * @return BATTERY_OK or BATTERY_ERROR
 */
Battery_Status_t GetBatteryLevel(uint32_t *out_v, uint8_t *out_pct);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_H */
