#ifndef SENSIRION_H
#define SENSIRION_H

#include <stdint.h>
#include <stdbool.h>

// External variable declarations (not definitions)
extern bool has_sensor_1;
extern bool has_sensor_2;
extern uint16_t calculated_temp;
extern uint8_t calculated_hum;
extern int16_t i2c_error_code;

typedef enum {
    I2C_ERROR_OK = 0,
	I2C_ERROR_SENSOR_A_NOT_RESPONDING = -1,
	I2C_ERROR_SENSOR_B_NOT_RESPONDING = -2,
    I2C_ERROR_BOTH_SENSORS_NOT_RESPONDING = -3,
    I2C_ERROR_SENSORS_TOO_DIFFERENT = -4,
} I2C_Error_t;

// Function prototypes
void scan_i2c_bus(void);
I2C_Error_t sensor_init_and_read(void);

#endif
