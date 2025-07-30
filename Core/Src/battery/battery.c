/* Core/Src/battery/battery.c */

#include "battery.h"
#include "main.h"    // for HAL_GPIO_WritePin, HAL_Delay

extern ADC_HandleTypeDef hadc;

// PMOS gate control pin
#define VBAT_MEAS_EN_GPIO_Port   GPIOB
#define VBAT_MEAS_EN_Pin         GPIO_PIN_0

// ADC & divider constants (integer math!)
#define ADC_REF_VOLTAGE_MV    3300U    // 3.3 V → 3300 mV
#define ADC_MAX_COUNT         4095U    // 12‑bit ADC: 0…4095
#define DIVIDER_RATIO_NUM     2U       // VBAT is halved by two equal resistors → multiply back by 2
#define DIVIDER_RATIO_DEN     1U
#define VBAT_SETTLE_DELAY_MS  10U

// Battery percentage curve (in mV)
#define V_MIN_MV 3000U    // 3.0 V → 0%
#define V_MAX_MV 4200U    // 4.2 V → 100%

int32_t ReadBatteryVoltage(void)
{
    HAL_StatusTypeDef status;
    uint32_t raw;

    // 1) Enable the divider
    HAL_GPIO_WritePin(VBAT_MEAS_EN_GPIO_Port, VBAT_MEAS_EN_Pin, GPIO_PIN_SET);

    // 2) Wait for it to settle
    HAL_Delay(VBAT_SETTLE_DELAY_MS);

    // 3) Sample ADC
    status = HAL_ADC_Start(&hadc);
    if (status != HAL_OK) goto fail;
    status = HAL_ADC_PollForConversion(&hadc, 10);
    if (status != HAL_OK) goto fail;
    raw = HAL_ADC_GetValue(&hadc);
    HAL_ADC_Stop(&hadc);

    // 4) Turn divider off
    HAL_GPIO_WritePin(VBAT_MEAS_EN_GPIO_Port, VBAT_MEAS_EN_Pin, GPIO_PIN_RESET);

    // 5) Compute VBAT in mV:
    //    VBAT = (raw/ADC_MAX_COUNT) * ADC_REF_VOLTAGE_MV * (DIVIDER_RATIO_NUM/DIVIDER_RATIO_DEN)
    //    = (raw * ADC_REF_VOLTAGE_MV * DIVIDER_RATIO_NUM + denom/2) / denom  (rounded)
    const uint32_t denom = ADC_MAX_COUNT * DIVIDER_RATIO_DEN;
    uint32_t numerator = raw * ADC_REF_VOLTAGE_MV * DIVIDER_RATIO_NUM + (denom/2U);
    uint32_t vbat_mV = numerator / denom;

    return (int32_t)vbat_mV;

fail:
    HAL_GPIO_WritePin(VBAT_MEAS_EN_GPIO_Port, VBAT_MEAS_EN_Pin, GPIO_PIN_RESET);
    return -1;  // ADC error indicator
}

Battery_Status_t GetBatteryLevel(uint32_t *out_v, uint8_t *out_pct)
{
    if (out_v == NULL || out_pct == NULL) {
        return BATTERY_ERROR;
    }

    int32_t v_mV = ReadBatteryVoltage();
    if (v_mV < 0) {
        return BATTERY_ADC_ERROR;
    }

    *out_v = (uint32_t)v_mV;
    ConsolePrintf("Input Voltage: %lu.%03lu V\r\n", (uint32_t)v_mV / 1000, (uint32_t)v_mV % 1000);

    if ((uint32_t)v_mV <= V_MIN_MV) {
        *out_pct = 0;
    }
    else if ((uint32_t)v_mV >= V_MAX_MV) {
        *out_pct = 100;
    }
    else {
        uint32_t range = V_MAX_MV - V_MIN_MV;
        // pct = ((v_mV - V_MIN_MV) / range) * 100
        // = ( (v_mV - V_MIN_MV)*100 + range/2 ) / range
        uint32_t numerator = (uint32_t)(v_mV - V_MIN_MV) * 100U + (range/2U);
        uint32_t pct = numerator / range;
        *out_pct = (uint8_t)pct;
    }

    return BATTERY_OK;
}
