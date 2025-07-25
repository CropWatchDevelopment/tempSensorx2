#include <stm32l0xx_hal.h>
#include "sensirion_common.h"
#include "sensirion_config.h"
#include "sensirion_i2c_hal.h"

extern I2C_HandleTypeDef hi2c1;

uint8_t msgbuf[256];

void sensirion_i2c_hal_init(void) {
    /* I2C is initialized elsewhere */
}

void sensirion_i2c_hal_free(void) {
    /* nothing to free */
}

int8_t sensirion_i2c_hal_read(uint8_t address, uint8_t* data, uint8_t count) {
    return (int8_t)HAL_I2C_Master_Receive(&hi2c1, (uint16_t)(address << 1), data, count, 100);
}

int8_t sensirion_i2c_hal_write(uint8_t address, const uint8_t* data, uint8_t count) {
    return (int8_t)HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(address << 1), (uint8_t*)data, count, 100);
}

void sensirion_i2c_hal_sleep_usec(uint32_t useconds) {
    uint32_t msec = useconds / 1000;
    if (useconds % 1000 > 0) {
        msec++;
    }
    if (HAL_GetHalVersion() < 0x01010100) {
        msec++;
    }
    HAL_Delay(msec);
}
