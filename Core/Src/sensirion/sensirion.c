#include "sensirion.h"
#include "main.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"
#include "sht4x_i2c.h"
#include <stdlib.h>

extern I2C_HandleTypeDef hi2c1;

// Variable definitions (declared as extern in the header)
bool has_sensor_1 = false;
bool has_sensor_2 = false;
uint16_t temp_ticks_1 = 0;
uint16_t hum_ticks_1 = 0;
uint16_t temp_ticks_2 = 0;
uint16_t hum_ticks_2 = 0;
uint16_t calculated_temp;
uint8_t  calculated_hum;
int16_t i2c_error_code = 0;

void scan_i2c_bus(void)
{
	// we re-set these to false because we want to check this every time for safety
    has_sensor_1 = false;
    has_sensor_2 = false;
    if (HAL_I2C_IsDeviceReady(&hi2c1, 68 << 1, 1, 10) == HAL_OK) has_sensor_1 = true;
    if (HAL_I2C_IsDeviceReady(&hi2c1, 70 << 1, 1, 10) == HAL_OK) has_sensor_2 = true;
    return;
}

I2C_Error_t sensor_init_and_read(void)
{
	HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_SET);
	scan_i2c_bus();
    if (!has_sensor_1 && !has_sensor_2) {
    	i2c_error_code = NO_SENSORS_FOUND;
    	HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_RESET);
        return I2C_ERROR_BOTH_SENSORS_NOT_RESPONDING;
    }

    i2c_error_code = NO_ERROR;
    HAL_Delay(100);

    if (has_sensor_1) {
        sht4x_init(SHT43_I2C_ADDR_44);
        sht4x_soft_reset();
        sensirion_i2c_hal_sleep_usec(10000);
        sht4x_init(SHT43_I2C_ADDR_44);
        i2c_error_code = sht4x_measure_high_precision_ticks(&temp_ticks_1, &hum_ticks_1);
    }

    if (has_sensor_2) {
        sht4x_init(SHT40_I2C_ADDR_46);
        sht4x_soft_reset();
        sensirion_i2c_hal_sleep_usec(10000);
        sht4x_init(SHT40_I2C_ADDR_46);
        i2c_error_code = sht4x_measure_high_precision_ticks(&temp_ticks_2, &hum_ticks_2);
    }
    HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_RESET);

             calculated_temp   = (uint16_t)((temp_ticks_1 + 5U) / 10U);
    uint16_t calculated_temp_2 = (uint16_t)((temp_ticks_2 + 5U) / 10U);

             calculated_hum    = (uint8_t)(hum_ticks_1 / 1000U);
    uint8_t  calculated_hum_2  = (uint8_t)(hum_ticks_2 / 1000U);

    // Determine diffs between temps and hums
    uint8_t temp_delta = (uint8_t)abs(calculated_temp - calculated_temp_2);
//    uint8_t hum_delta  = (uint8_t)abs(calculated_hum  - calculated_hum_2);

    if (temp_delta > 2) return I2C_ERROR_SENSORS_TOO_DIFFERENT;
    if (i2c_error_code) return false;

    return I2C_ERROR_OK;
}
