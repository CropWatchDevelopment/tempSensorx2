#include "sensirion.h"
#include "main.h"
#include "sensirion_common.h"
#include "sensirion_i2c_hal.h"
#include "sht4x_i2c.h"

extern I2C_HandleTypeDef hi2c1;

// Variable definitions (declared as extern in the header)
bool has_sensor_1 = false;
bool has_sensor_2 = false;
uint16_t temp_ticks_1 = 0;
uint16_t hum_ticks_1 = 0;
uint16_t temp_ticks_2 = 0;
uint16_t hum_ticks_2 = 0;
int16_t error = 0;

void scan_i2c_bus(void)
{
    has_sensor_1 = false;
    has_sensor_2 = false;

    uint8_t addr;
    HAL_Delay(100);

    for (addr = 3; addr < 0x78; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK) {
            if (addr == 0x44)
                has_sensor_1 = true;
            if (addr == 0x46)
                has_sensor_2 = true;
        }
    }
}

int sensor_init_and_read(void)
{
    if (!has_sensor_1 && !has_sensor_2) {
        return -200;
    }

    error = NO_ERROR;

    if (has_sensor_1)
    {
        sht4x_init(SHT43_I2C_ADDR_44);
        sht4x_soft_reset();
        sensirion_i2c_hal_sleep_usec(10000);
        sht4x_init(SHT43_I2C_ADDR_44);
        error = sht4x_measure_high_precision_ticks(&temp_ticks_1, &hum_ticks_1);
    }

    if (error)
    {
            return -200;
    }

    if (has_sensor_2)
    {
        sht4x_init(SHT40_I2C_ADDR_46);
        sht4x_soft_reset();
        sensirion_i2c_hal_sleep_usec(10000);
        sht4x_init(SHT40_I2C_ADDR_46);
        error = sht4x_measure_high_precision_ticks(&temp_ticks_2, &hum_ticks_2);
    }

    if (error)
    {
        return -200;
    }

    // Correct so no negative numbers may ever be returned
    temp_ticks_1 = (temp_ticks_1 / 100) + 55;
    temp_ticks_2 = (temp_ticks_2 / 100) + 55;

    hum_ticks_1 = (hum_ticks_1 / 1000);

    // if the value is still less than -55 (as it is less than 0 now) something is very wrong
    // lets just return too cold here and error out
    if (temp_ticks_1 < 0 || temp_ticks_2 < 0) return -255;

    return 0;
}
