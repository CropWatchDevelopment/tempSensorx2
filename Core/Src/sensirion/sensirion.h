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

// Function prototypes
void scan_i2c_bus(void);
bool sensor_init_and_read(void);

#endif
