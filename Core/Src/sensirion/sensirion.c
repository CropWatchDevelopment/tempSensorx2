#include "sensirion.h"
#include "main.h"
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
	// Reset sensor flags
	has_sensor_1 = false;
	has_sensor_2 = false;
	
	HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_SET);
    uint8_t addr;
    HAL_Delay(100); // let bus settle

    for (addr = 3; addr < 0x78; addr++)
    {
        // HAL expects 8-bit address = 7-bit << 1
        if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK)
        {
        	// SHT4x sensors use 7-bit addresses 0x44 and 0x46
        	if (addr == 0x44) {
        		has_sensor_1 = true;
        	}
        	if (addr == 0x46) {
        		has_sensor_2 = true;
        	}
        }
    }
    HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_RESET);
}

int sensor_init_and_read(void)
{
	if (!has_sensor_1 && !has_sensor_2) {
		return -1;
	}
	
	HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_SET);
	error = NO_ERROR;
	HAL_Delay(100); // Let power stabilize

	// --- Read From Sensor A (0x44) ---
	if (has_sensor_1)
	{
		sht4x_init(SHT43_I2C_ADDR_44);
		sht4x_soft_reset();
		sensirion_i2c_hal_sleep_usec(10000);
		sht4x_init(SHT43_I2C_ADDR_44);
		error = sht4x_measure_high_precision_ticks(&temp_ticks_1, &hum_ticks_1);
	}

	// --- Read From Sensor B (0x46) ---
	if (has_sensor_2)
	{
		sht4x_init(SHT40_I2C_ADDR_46);
		sht4x_soft_reset();
		sensirion_i2c_hal_sleep_usec(10000);
		sht4x_init(SHT40_I2C_ADDR_46);
		error = sht4x_measure_high_precision_ticks(&temp_ticks_2, &hum_ticks_2);
	}

	HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_RESET);

	if (error) return (-200);
	return error;
}
