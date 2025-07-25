// send_command.h
#ifndef SEND_COMMAND_H
#define SEND_COMMAND_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief  Build an AT+SEND command with zero‑padded hex data.
 *
 * Format is:  AT+SEND "<hex…>"\r\n
 *
 * @param nums        Array of uint32_t values.
 * @param count       Number of elements in nums.
 * @param width       Hex digits per value (1…8).
 * @param cmd_buf     Caller‐allocated buffer for the full AT command.
 * @param buf_len     Size of cmd_buf in bytes.
 * @param out_len     [out] Number of bytes in the built command (excluding any final '\0').
 * @return true on success, false if buf_len is too small or width invalid.
 */
bool build_at_send_command(const uint32_t *nums,
                           size_t count,
                           uint8_t width,
                           char *cmd_buf,
                           size_t buf_len,
                           size_t *out_len);

#endif // SEND_COMMAND_H
