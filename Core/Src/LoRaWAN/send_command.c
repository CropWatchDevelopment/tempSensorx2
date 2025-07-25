// send_command.c
#include "send_command.h"
#include "../Utils/hex_utils.h"
#include <string.h>

bool build_at_send_command(const uint32_t *nums,
                           size_t count,
                           uint8_t width,
                           char *cmd_buf,
                           size_t buf_len,
                           size_t *out_len)
{
    const char prefix[] = "AT+SEND \"";
    const char suffix[] = "\"\r\n";
    size_t pre = sizeof(prefix) - 1;
    size_t suf = sizeof(suffix) - 1;
    size_t hex_len = count * (size_t)width;
    size_t need = pre + hex_len + suf;

    if (width == 0 || width > 8 || buf_len < need + 1) {
        return false;
    }

    // 1) copy prefix
    memcpy(cmd_buf, prefix, pre);

    // 2) hex‑encode directly into the buffer after the prefix
    if (!uint32_array_to_hex(nums,
                             count,
                             width,
                             cmd_buf + pre,
                             hex_len + 1))
    {
        return false;
    }

    // 3) append suffix
    memcpy(cmd_buf + pre + hex_len, suffix, suf);

    // 4) null‑terminate (optional, for safety)
    cmd_buf[pre + hex_len + suf] = '\0';

    if (out_len) {
        *out_len = need;
    }
    return true;
}
