/*
 * hex_utils.h
 *
 *  Created on: Jul 26, 2025
 *      Author: kevin
 */


// hex_utils.h
#ifndef HEX_UTILS_H
#define HEX_UTILS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

bool uint32_array_to_hex(const uint32_t *nums,
                         size_t count,
                         uint8_t width,
                         char *out_str,
                         size_t out_len);

#endif // HEX_UTILS_H
