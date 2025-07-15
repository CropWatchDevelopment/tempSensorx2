#include "sensirion.h"
#include "main.h"
#include "gpio.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"
#include "sht4x_i2c.h"

// Variable definitions (these were declared as extern in the header)
bool has_sensor_1 = false;
bool has_sensor_2 = false;
uint16_t temp_ticks_1 = 0;
uint16_t hum_ticks_1 = 0;
uint16_t temp_ticks_2 = 0;
uint16_t hum_ticks_2 = 0;
int16_t error = 0;

void scan_i2c_bus(void)
{
	printf("DEBUG: Starting I2C scan\n");
	
	// Reset sensor flags
	has_sensor_1 = false;
	has_sensor_2 = false;
	
	HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_SET);
	printf("DEBUG: I2C power enabled\n");
    HAL_Delay(100); // let bus settle

    uint8_t addr;
    for (addr = 3; addr < 0x78; addr++)
    {
        // HAL expects 8-bit address = 7-bit << 1
        if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK)
        {
        	printf("DEBUG: Found device at address 0x%02X\n", addr);
        	// SHT4x sensors use 7-bit addresses 0x44 and 0x46
        	if (addr == 0x44) {
        		has_sensor_1 = true;
        		printf("DEBUG: Sensor 1 detected at 0x44\n");
        	}
        	if (addr == 0x46) {
        		has_sensor_2 = true;
        		printf("DEBUG: Sensor 2 detected at 0x46\n");
        	}
        }
    }
    HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_RESET);
    printf("DEBUG: I2C scan complete - Sensor1: %s, Sensor2: %s\n", 
           has_sensor_1 ? "found" : "not found",
           has_sensor_2 ? "found" : "not found");
}

int sensor_init_and_read(void)
{
	printf("DEBUG: Starting sensor initialization and reading\n");
	if (!has_sensor_1 && !has_sensor_2) {
		printf("DEBUG: No sensors detected, returning -1\n");
		return -1;
	}
	
	HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_SET);
	printf("DEBUG: I2C power enabled for sensor reading\n");
	error = NO_ERROR;
	HAL_Delay(100); // Let power stabilize

	// --- Read From Sensor A (0x44) ---
	if (has_sensor_1)
	{
		printf("DEBUG: Initializing sensor 1 (0x44)\n");
		sht4x_init(SHT43_I2C_ADDR_44);
		printf("DEBUG: Performing soft reset on sensor 1\n");
		sht4x_soft_reset();
		sensirion_i2c_hal_sleep_usec(10000);
		printf("DEBUG: Re-initializing sensor 1 after reset\n");
		sht4x_init(SHT43_I2C_ADDR_44);
		printf("DEBUG: Reading measurement from sensor 1\n");
		error = sht4x_measure_high_precision_ticks(&temp_ticks_1, &hum_ticks_1);
		if (error != NO_ERROR) {
			printf("ERROR: Sensor 1 measurement failed with error %d\n", error);
		} else {
			printf("DEBUG: Sensor 1 measurement successful - Temp: %u, Hum: %u\n", temp_ticks_1, hum_ticks_1);
		}
	}

	// --- Read From Sensor B (0x46) ---
	if (has_sensor_2)
	{
		printf("DEBUG: Initializing sensor 2 (0x46)\n");
		sht4x_init(SHT40_I2C_ADDR_46);
		printf("DEBUG: Performing soft reset on sensor 2\n");
		sht4x_soft_reset();
		sensirion_i2c_hal_sleep_usec(10000);
		printf("DEBUG: Re-initializing sensor 2 after reset\n");
		sht4x_init(SHT40_I2C_ADDR_46);
		printf("DEBUG: Reading measurement from sensor 2\n");
		error = sht4x_measure_high_precision_ticks(&temp_ticks_2, &hum_ticks_2);
		if (error != NO_ERROR) {
			printf("ERROR: Sensor 2 measurement failed with error %d\n", error);
		} else {
			printf("DEBUG: Sensor 2 measurement successful - Temp: %u, Hum: %u\n", temp_ticks_2, hum_ticks_2);
		}
	}

	HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_RESET);
	printf("DEBUG: I2C power disabled, sensor reading complete\n");

	return error;
}
