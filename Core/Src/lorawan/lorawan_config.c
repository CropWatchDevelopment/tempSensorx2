#include "lorawan_config.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "atc.h"

// ----------------------------------------------- CONFIGURE LORAWAN
//  ATC_SendReceive(&lora, "AT%S 500=\"0025CA00000055F70025CA00000055F7\"\r\n", 100, NULL, 200, 1, "OK");
//  ATC_SendReceive(&lora, "ATS 600=0\r\n", 100, NULL, 200, 2, "OK"); /* 2) ADR off */
//  ATC_SendReceive(&lora, "ATS 602=1\r\n", 100, NULL, 200, 2, "OK");  /* OTAA activation */
//  ATC_SendReceive(&lora, "ATS 603=0\r\n", 100, NULL, 200, 2, "OK");
//  ATC_SendReceive(&lora, "ATS 611=9\r\n", 100, NULL, 200, 2, "OK"); /* 3) Region AS923-1 (Japan) */
//  ATC_SendReceive(&lora, "ATS 713=0\r\n", 100, NULL, 200, 2, "OK");  /* Static DR → DR0 (lowest) */
//  ATC_SendReceive(&lora, "ATS 714=12\r\n", 100, NULL, 200, 2, "OK");  /* Static TX power → 12 dBm (max) */
//  ATC_SendReceive(&lora, "AT&W\r\n", 100, NULL, 200, 2, "OK");  /* Save settings */
//  ATC_SendReceive(&lora, "ATZ\r\n", 100, NULL, 200, 2, "OK");  /* Apply settings */

#define AT_RESPONSE_BUFFER_SIZE 256
#define DEV_EUI_LENGTH           16
#define APP_EUI_LENGTH           16
#define APP_KEY_LENGTH           32
#define AS923_1_FREQ_MIN   923200000
#define AS923_1_FREQ_MAX   923400000
#define DEFAULT_FREQ       923200000
#define JAPAN_REGION       9
#define TTN_SUBBAND_CHANNEL 0
#define TX_POWER           11
#define DATA_RATE          0
#define JOIN_TIMEOUT_MS    10000

static LoRaWAN_Error_t set_dev_eui(ATC_HandleTypeDef *lora, const char *dev_eui);
static LoRaWAN_Error_t set_app_eui(ATC_HandleTypeDef *lora, const char *app_eui);
static LoRaWAN_Error_t set_app_key(ATC_HandleTypeDef *lora, const char *app_key);
static LoRaWAN_Error_t configure_region_and_channel(ATC_HandleTypeDef *lora);
static LoRaWAN_Error_t check_and_set_frequency(ATC_HandleTypeDef *lora);
static LoRaWAN_Error_t save_and_reset(ATC_HandleTypeDef *lora);
static LoRaWAN_Error_t set_adr(ATC_HandleTypeDef *lora, bool enable);
static LoRaWAN_Error_t set_otaa(ATC_HandleTypeDef *lora, bool enable);
static LoRaWAN_Error_t set_class_a(ATC_HandleTypeDef *lora);
static LoRaWAN_Error_t set_data_rate(ATC_HandleTypeDef *lora, int dr);
static LoRaWAN_Error_t set_tx_power(ATC_HandleTypeDef *lora, int power);
static LoRaWAN_Error_t factor_reset(ATC_HandleTypeDef *lora);

bool lorawan_configure(ATC_HandleTypeDef *lora, const char *dev_eui, const char *app_eui, const char *app_key) {
    LoRaWAN_Error_t err;

    if ((err = set_dev_eui(lora, dev_eui)) != LORAWAN_OK ||
        (err = set_app_eui(lora, app_eui)) != LORAWAN_OK ||
        (err = set_app_key(lora, app_key)) != LORAWAN_OK) {
        return false;
    }

    if ((err = set_class_a(lora)) != LORAWAN_OK) {
    	return false;
    }

    if ((err = configure_region_and_channel(lora)) != LORAWAN_OK) {
        return false;
    }

    if ((err = check_and_set_frequency(lora)) != LORAWAN_OK) {
        return false;
    }

    if ((err = save_and_reset(lora)) != LORAWAN_OK) {
        return false;
    }

    return true;
}

static LoRaWAN_Error_t set_dev_eui(ATC_HandleTypeDef *lora, const char *dev_eui) {
    char  command[64];
    char *resp_str = NULL;
    int   resp = ATC_SendReceive(lora, "AT%S 501?\r\n", 100, &resp_str, 200, 2,
                                 "0000000000000000", "OK");
    if (resp == 2) return LORAWAN_OK;
    if (resp != 1) return LORAWAN_ERR_DEV_EUI;
    snprintf(command, sizeof(command), "AT%%S 501=\"%s\"\r\n", dev_eui);
    ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return LORAWAN_OK;
}

static LoRaWAN_Error_t set_app_eui(ATC_HandleTypeDef *lora, const char *app_eui) {
    char command[64];
    char *resp_str = NULL;
    int resp = ATC_SendReceive(lora, "AT%S 502?\r\n", 100, &resp_str, 200, 2,
                               "0000000000000000", "OK");
    if (resp == 2) return LORAWAN_OK;
    if (resp != 1) return LORAWAN_ERR_APP_EUI;
    snprintf(command, sizeof(command), "AT%%S 502=\"%s\"\r\n", app_eui);
    ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return LORAWAN_OK;
}

static LoRaWAN_Error_t set_app_key(ATC_HandleTypeDef *lora, const char *app_key) {
    char  command[96];
    char *resp_str = NULL;
    int   resp = ATC_SendReceive(lora, "AT%S 500?\r\n", 100, &resp_str, 200, 2,
                                 "00000000000000000000000000000000", "OK");
    if (resp == 2) return LORAWAN_OK;
    if (resp != 1) return LORAWAN_ERR_APP_KEY;
    snprintf(command, sizeof(command), "AT%%S 500=\"%s\"\r\n", app_key);
    ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return LORAWAN_OK;
}

static LoRaWAN_Error_t configure_region_and_channel(ATC_HandleTypeDef *lora) {
    char  command[32];
    char *resp_str = NULL;
    int   resp = ATC_SendReceive(lora, "AT%S 611?\r\n", 100, &resp_str, 200, 2,
                                 "9", "OK");
    if (resp == 2) return LORAWAN_OK;
    if (resp != 1) return LORAWAN_ERR_AT_COMMAND;
    snprintf(command, sizeof(command), "AT%%S 611=%d\r\n", JAPAN_REGION);
    ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return LORAWAN_OK;
}

static LoRaWAN_Error_t check_and_set_frequency(ATC_HandleTypeDef *lora) {
    char  command[32];
    char *resp_str = NULL;
    int   resp = ATC_SendReceive(lora, "AT%S 605?\r\n", 100, &resp_str, 200, 2,
                                 "923200000", "OK");
    if (resp == 2) return LORAWAN_OK;
    if (resp != 1) return LORAWAN_ERR_AT_COMMAND;
    snprintf(command, sizeof(command), "AT%%S 605=%u\r\n", DEFAULT_FREQ);
    ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return LORAWAN_OK;
}

static LoRaWAN_Error_t set_adr(ATC_HandleTypeDef *lora, bool enable) {
    char  command[32];
    snprintf(command, sizeof(command), "AT%%S 600=%d\r\n", enable);
    int resp = ATC_SendReceive(lora, "AT%S 600?\r\n", 100, NULL, 200, 1,
                               enable ? "1" : "0");
    if (resp == 2) return LORAWAN_OK;
    if (resp != 1) return LORAWAN_ERR_AT_COMMAND;
    ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return LORAWAN_OK;
}


// this should be default!
static LoRaWAN_Error_t set_otaa(ATC_HandleTypeDef *lora, bool enable) {
    char  command[32];
    snprintf(command, sizeof(command), "AT%%S 602=%d\r\n", enable);
    int resp = ATC_SendReceive(lora, "AT%S 602?\r\n", 100, NULL, 200, 1,
                               enable ? "1" : "0");
    if (resp == 2) return LORAWAN_OK;
    if (resp != 1) return LORAWAN_ERR_AT_COMMAND;
    ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return LORAWAN_OK;
}

static LoRaWAN_Error_t set_class_a(ATC_HandleTypeDef *lora) {
    int resp = ATC_SendReceive(lora, "AT%S 603?\r\n", 100, NULL, 200, 2, "0", "OK");
    if (resp == 2) return LORAWAN_OK;
    if (resp != 1) return LORAWAN_ERR_AT_COMMAND;
    ATC_SendReceive(lora, "AT%S 603=0\r\n", 100, NULL, 200, 1, "OK");
    return LORAWAN_OK;
}

static LoRaWAN_Error_t set_data_rate(ATC_HandleTypeDef *lora, int dr) {
    char  command[32];
    char buf[4];
    char *resp_str = NULL;
    snprintf(buf, sizeof(buf), "%d", dr);
    int resp = ATC_SendReceive(lora, "AT%S 713?\r\n", 100, NULL, 200, 2, buf, "OK");
    if (resp == 2) return LORAWAN_OK;
    if (resp != 1) return LORAWAN_ERR_AT_COMMAND;
    snprintf(command, sizeof(command), "AT%%S 713=%d\r\n", dr);
    ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return LORAWAN_OK;
}

static LoRaWAN_Error_t set_tx_power(ATC_HandleTypeDef *lora, int power) {
    char  command[32];
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", power);
    int resp = ATC_SendReceive(lora, "AT%S 714?\r\n", 100, NULL, 200, 2, buf, "OK");
    if (resp == 2) return LORAWAN_OK;
    if (resp != 1) return LORAWAN_ERR_AT_COMMAND;
    snprintf(command, sizeof(command), "AT%%S 714=%d\r\n", power);
    ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return LORAWAN_OK;
}

LoRaWAN_Error_t join_network(ATC_HandleTypeDef *lora) {
    char  response[AT_RESPONSE_BUFFER_SIZE];
    char *response_ptr = response;
    int   resp = ATC_SendReceive(lora, "AT+JOIN\r\n", 100, &response_ptr, JOIN_TIMEOUT_MS, 1, "OK");
    if (resp < 0) return LORAWAN_ERR_AT_COMMAND;
    return LORAWAN_OK;
}

static LoRaWAN_Error_t factor_reset(ATC_HandleTypeDef *lora) {
    int resp = ATC_SendReceive(lora, "AT&F\r\n", 100, NULL, 2000, 1, "OK");
    if (resp < 0) {
        resp = ATC_SendReceive(lora, "AT+RESET\r\n", 100, NULL, 2000, 1, "OK");
    }
    return (resp < 0) ? LORAWAN_ERR_AT_COMMAND : LORAWAN_OK;
}

static LoRaWAN_Error_t save_and_reset(ATC_HandleTypeDef *lora) {
    int resp = ATC_SendReceive(lora, "AT&W\r\n", 100, NULL, 200, 1, "OK");
    if (resp < 0) return LORAWAN_ERR_SAVE_RESET;
    resp = ATC_SendReceive(lora, "ATZ\r\n", 100, NULL, 200, 1, "OK");
    if (resp < 0) return LORAWAN_ERR_SAVE_RESET;
    return LORAWAN_OK;
}

void to_hex_str(uint32_t value, uint8_t width, char *output) {
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = width - 1; i >= 0; --i) {
        output[i] = hex_chars[value & 0xF];
        value >>= 4;
    }
    output[width] = '\0';
}

void format_at_send_cmd(uint32_t data, uint8_t hex_digits, char *out_buf) {
    char hex_str[33];
    to_hex_str(data, hex_digits, hex_str);
    strcpy(out_buf, "AT+SEND \"");
    strcat(out_buf, hex_str);
    strcat(out_buf, "\"\r\n");
}
