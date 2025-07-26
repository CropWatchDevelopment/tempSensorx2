/*
 * hex_utils.c
 *
 *  Created on: Jul 26, 2025
 *      Author: kevin
 */


// hex_utils.c
#include "hex_utils.h"

bool uint32_array_to_hex(const uint32_t *nums,
                         size_t count,
                         uint8_t width,
                         char *out_str,
                         size_t out_len)
{
    if (width == 0 || width > 8) {
        return false;
    }
    size_t needed = count * (size_t)width + 1;
    if (out_len < needed) {
        return false;
    }

    char *p = out_str;
    for (size_t i = 0; i < count; ++i) {
        uint32_t v = nums[i];
        for (int shift = (int)(width - 1) * 4; shift >= 0; shift -= 4) {
            uint8_t nibble = (v >> shift) & 0xF;
            *p++ = (nibble < 10) ? ('0' + nibble)
                                 : ('A' + nibble - 10);
        }
    }
    *p = '\0';
    return true;
}
