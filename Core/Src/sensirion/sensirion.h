#ifndef SENSIRION_H
#define SENSIRION_H

#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"

// External variable declarations (not definitions)
extern bool has_sensor_1;
extern bool has_sensor_2;
extern uint16_t temp_ticks_1;
extern uint16_t hum_ticks_1;
extern uint16_t temp_ticks_2;
extern uint16_t hum_ticks_2;
extern int16_t error;

// Function prototypes
void scan_i2c_bus(void);
int sensor_init_and_read(void);

#endif
