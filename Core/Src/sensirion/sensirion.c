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
uint8_t temp_delta;
uint8_t hum_delta;
int16_t i2c_error_code = 0;

void scan_i2c_bus(void)
{
	// we re-set these to false because we want to check this every time for safety
    has_sensor_1 = false;
    has_sensor_2 = false;
    
    ConsolePrintf("Testing sensor 1 at address 0x44 (I2C addr 0x88)...\r\n");
    HAL_StatusTypeDef result1 = HAL_I2C_IsDeviceReady(&hi2c1, 68 << 1, 1, 10);
    ConsolePrintf("Sensor 1 result: %s (code: %d)\r\n", 
                  (result1 == HAL_OK) ? "HAL_OK" : 
                  (result1 == HAL_ERROR) ? "HAL_ERROR" :
                  (result1 == HAL_BUSY) ? "HAL_BUSY" : 
                  (result1 == HAL_TIMEOUT) ? "HAL_TIMEOUT" : "UNKNOWN", result1);
    if (result1 == HAL_OK) has_sensor_1 = true;
    
    ConsolePrintf("Testing sensor 2 at address 0x46 (I2C addr 0x8C)...\r\n");
    HAL_StatusTypeDef result2 = HAL_I2C_IsDeviceReady(&hi2c1, 70 << 1, 1, 10);
    ConsolePrintf("Sensor 2 result: %s (code: %d)\r\n", 
                  (result2 == HAL_OK) ? "HAL_OK" : 
                  (result2 == HAL_ERROR) ? "HAL_ERROR" :
                  (result2 == HAL_BUSY) ? "HAL_BUSY" : 
                  (result2 == HAL_TIMEOUT) ? "HAL_TIMEOUT" : "UNKNOWN", result2);
    if (result2 == HAL_OK) has_sensor_2 = true;

    ConsolePrintf("Sensor scan: Sensor 1 (0x44): %s, Sensor 2 (0x46): %s\r\n", 
                  has_sensor_1 ? "FOUND" : "NOT FOUND", 
                  has_sensor_2 ? "FOUND" : "NOT FOUND");
    return;
}

I2C_Error_t sensor_init_and_read(void)
{
	ConsolePrintf("Enabling I2C power...\r\n");
	HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_SET);
	ConsolePrintf("I2C_ENABLE pin set to HIGH (PB5)\r\n");
	
	// Read back the pin state to confirm it's actually set
	GPIO_PinState pin_state = HAL_GPIO_ReadPin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin);
	ConsolePrintf("I2C_ENABLE pin readback: %s\r\n", (pin_state == GPIO_PIN_SET) ? "HIGH" : "LOW");
	
	HAL_Delay(400); // WE NEED THIS FOR IC POWER UP!!! IT TAKES 300mS to competely power up!!!
	ConsolePrintf("I2C power stabilized, scanning for sensors...\r\n");
	
	// Test I2C bus by scanning a wider range of addresses
	ConsolePrintf("Performing extended I2C bus scan...\r\n");
	int devices_found = 0;
	for (uint8_t addr = 0x08; addr < 0x78; addr++) {
		if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK) {
			ConsolePrintf("I2C device found at address 0x%02X\r\n", addr);
			devices_found++;
		}
	}
	ConsolePrintf("Total I2C devices found: %d\r\n", devices_found);
	
	scan_i2c_bus();
    if (!has_sensor_1 && !has_sensor_2) {
    	i2c_error_code = NO_SENSORS_FOUND;
    	HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_RESET);
        return I2C_ERROR_BOTH_SENSORS_NOT_RESPONDING;
    }

    i2c_error_code = NO_ERROR;

    if (has_sensor_1) {
        ConsolePrintf("Reading sensor 1 (SHT43 at 0x44)...\r\n");
        sht4x_init(SHT43_I2C_ADDR_44);
        sht4x_soft_reset();
        sensirion_i2c_hal_sleep_usec(10000);
        sht4x_init(SHT43_I2C_ADDR_44);
        i2c_error_code = sht4x_measure_high_precision_ticks(&temp_ticks_1, &hum_ticks_1);
        ConsolePrintf("Sensor 1 raw: temp_ticks=%u, hum_ticks=%u\r\n", temp_ticks_1, hum_ticks_1);
    }

    if (has_sensor_2) {
        ConsolePrintf("Reading sensor 2 (SHT40 at 0x46)...\r\n");
        sht4x_init(SHT40_I2C_ADDR_46);
        sht4x_soft_reset();
        sensirion_i2c_hal_sleep_usec(10000);
        sht4x_init(SHT40_I2C_ADDR_46);
        i2c_error_code = sht4x_measure_high_precision_ticks(&temp_ticks_2, &hum_ticks_2);
        ConsolePrintf("Sensor 2 raw: temp_ticks=%u, hum_ticks=%u\r\n", temp_ticks_2, hum_ticks_2);
    }
    ConsolePrintf("Disabling I2C power...\r\n");
    HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_RESET);

             calculated_temp   = (uint16_t)((temp_ticks_1 + 5U) / 10U);
    uint16_t calculated_temp_2 = (uint16_t)((temp_ticks_2 + 5U) / 10U);

    ConsolePrintf("Temperature readings: Sensor 1: %u.%u°C, Sensor 2: %u.%u°C\r\n", 
                  calculated_temp / 10, calculated_temp % 10,
                  calculated_temp_2 / 10, calculated_temp_2 % 10);

             calculated_hum    = (uint8_t)(hum_ticks_1 / 1000U);
    uint8_t  calculated_hum_2  = (uint8_t)(hum_ticks_2 / 1000U);

    ConsolePrintf("Humidity readings: Sensor 1: %u%%, Sensor 2: %u%%\r\n", 
                  calculated_hum, calculated_hum_2);

             temp_delta        = (uint8_t)abs(calculated_temp - calculated_temp_2);
             hum_delta         = (uint8_t)abs(calculated_hum  - calculated_hum_2);

    ConsolePrintf("Sensor deltas: Temp delta: %u.%u°C, Hum delta: %u%%\r\n", 
                  temp_delta / 10, temp_delta % 10, hum_delta);

    if (temp_delta > 200) {
        ConsolePrintf("WARNING: Temperature difference too large (%u.%u°C > 20.0°C)\r\n", 
                      temp_delta / 10, temp_delta % 10);
        return I2C_ERROR_SENSORS_TOO_DIFFERENT;
    }
    if (i2c_error_code) return false;

    return I2C_ERROR_OK;
}
