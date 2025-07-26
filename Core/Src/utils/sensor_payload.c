/*
 * sensor_payload.c
 *
 *  Created on: Jul 26, 2025
 *      Author: kevin
 */


#include "sensor_payload.h"
#include "hex_utils.h"

bool build_sensor_hex_payload(uint16_t temp_ticks,
                              uint16_t hum_ticks,
                              char    *hex_buf,
                              size_t   hex_buf_len,
                              size_t  *out_hex_len)
{
    // We use fixed‑point: raw ticks are in milli‑units.
    // Add 55 °C → 55 000 milli‑units, then convert to centi‑units (hundredths) by /10.
    const int32_t OFFSET_MILLI = 55 * 1000;  // +55°C

    // 1) Temperature: (temp_ticks + OFFSET_MILLI) milli‑°C → hundredths
    int32_t t_milli = (int32_t)temp_ticks + OFFSET_MILLI;
    if (t_milli < 0) {
        return false;  // underflow
    }
    uint32_t temp_cent = (uint32_t)((t_milli + 5) / 10);

    // 2) Humidity: no offset, just milli‑% → hundredths
    int32_t h_milli = (int32_t)hum_ticks;
    if (h_milli < 0) {
        return false;
    }
    uint32_t hum_cent = (uint32_t)((h_milli + 5) / 10);

    // 3) Hex‑encode [ temp_cent, hum_cent ] each in 4 digits
    uint32_t data[2] = { temp_cent, hum_cent };
    const uint8_t WIDTH = 4;           // 4 hex digits per value
    size_t needed = 2 * WIDTH + 1;     // +1 for '\0'

    if (hex_buf_len < needed) {
        return false;  // too small
    }
    if (!uint32_array_to_hex(data, 2, WIDTH, hex_buf, hex_buf_len)) {
        return false;
    }
    if (out_hex_len) {
        *out_hex_len = 2 * WIDTH;
    }
    return true;
}
