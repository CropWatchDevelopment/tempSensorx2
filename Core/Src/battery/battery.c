/* Core/Src/battery/battery.c */

#include "battery.h"
#include "main.h"    // for HAL_GPIO_WritePin, HAL_Delay

// bring in your ADC handle from main.c
extern ADC_HandleTypeDef hadc;

// PMOS gate control pin (adjust to your actual port/pin)
#define VBAT_MEAS_EN_GPIO_Port   GPIOB
#define VBAT_MEAS_EN_Pin         GPIO_PIN_0

// ADC & divider constants
#define ADC_REF_VOLTAGE        3.3f
#define ADC_RESOLUTION         4095.0f   // 12‑bit: 0…4095
#define VOLTAGE_DIVIDER_RATIO  2.0f      // two 1 MΩ resistors in divider
#define VBAT_SETTLE_DELAY_MS   10U

float ReadBatteryVoltage(void)
{
    HAL_StatusTypeDef status;
    uint32_t raw;

    // 1) Enable the divider
    HAL_GPIO_WritePin(VBAT_MEAS_EN_GPIO_Port, VBAT_MEAS_EN_Pin, GPIO_PIN_SET);

    // 2) Wait for the divider node to settle
    HAL_Delay(VBAT_SETTLE_DELAY_MS);

    // 3) Start ADC conversion on your handle 'hadc'
    status = HAL_ADC_Start(&hadc);
    if (status != HAL_OK) goto fail;

    status = HAL_ADC_PollForConversion(&hadc, 10);
    if (status != HAL_OK) goto fail;

    raw = HAL_ADC_GetValue(&hadc);
    HAL_ADC_Stop(&hadc);

    // 4) Disable the divider
    HAL_GPIO_WritePin(VBAT_MEAS_EN_GPIO_Port, VBAT_MEAS_EN_Pin, GPIO_PIN_RESET);

    // 5) Convert ADC code → volts, then scale up by divider ratio
    float v_adc = ((float)raw / ADC_RESOLUTION) * ADC_REF_VOLTAGE;
    return v_adc * VOLTAGE_DIVIDER_RATIO;

fail:
    // Ensure divider is off on error
    HAL_GPIO_WritePin(VBAT_MEAS_EN_GPIO_Port, VBAT_MEAS_EN_Pin, GPIO_PIN_RESET);
    return -1.0f;
}

Battery_Status_t GetBatteryLevel(uint32_t *out_v, uint8_t *out_pct)
{
    if (!out_v || !out_pct) return BATTERY_ERROR;

    float v = ReadBatteryVoltage();
    if (v < 0.0f) return BATTERY_ERROR;

    *out_v = v;

    // Example curve: 3.0 V→0%, 4.2 V→100%
    const float V_MIN = 3.0f, V_MAX = 4.2f;
    float pct = (v - V_MIN) / (V_MAX - V_MIN) * 100.0f;
    if (pct < 0.0f)   pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    *out_pct = (uint8_t)(pct + 0.5f);

    return BATTERY_OK;
}



