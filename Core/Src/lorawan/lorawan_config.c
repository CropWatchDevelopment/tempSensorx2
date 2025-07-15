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


#include "lorawan_config.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "atc.h" // Assuming ATC_HandleTypeDef and ATC_SendReceive are defined here

// Constants for AT commands and parameters
#define AT_RESPONSE_BUFFER_SIZE 256
#define DEV_EUI_LENGTH 16
#define APP_EUI_LENGTH 16
#define APP_KEY_LENGTH 32
#define AS923_1_FREQ_MIN 923200000 // Hz
#define AS923_1_FREQ_MAX 923400000 // Hz
#define DEFAULT_FREQ 923200000      // Hz, default for TTN Japan AS923-1
#define JAPAN_REGION 9              // AS923-1 for Japan
#define TTN_SUBBAND_CHANNEL 0       // Channel 0 for TTN AS923-1
#define TX_POWER 11                 // dBm, ensures EIRP ≤ 13 dBm with ≤2 dBi antenna
#define DATA_RATE 0                 // DR0
#define JOIN_TIMEOUT_MS 10000       // 10 seconds for join response

// Error codes
typedef enum {
    LORAWAN_OK = 0,
    LORAWAN_ERR_DEV_EUI,
    LORAWAN_ERR_APP_EUI,
    LORAWAN_ERR_APP_KEY,
    LORAWAN_ERR_FREQ_CHECK,
    LORAWAN_ERR_AT_COMMAND,
    LORAWAN_ERR_SAVE_RESET,
    LORAWAN_ERR_JOIN
} LoRaWAN_Error_t;

// Function prototypes
static bool validate_hex_string(const char *str, size_t expected_len);
static LoRaWAN_Error_t set_dev_eui(ATC_HandleTypeDef *lora, const char *dev_eui);
static LoRaWAN_Error_t set_app_eui(ATC_HandleTypeDef *lora, const char *app_eui);
static LoRaWAN_Error_t set_app_key(ATC_HandleTypeDef *lora, const char *app_key);
static LoRaWAN_Error_t configure_region_and_channel(ATC_HandleTypeDef *lora);
static LoRaWAN_Error_t check_and_set_frequency(ATC_HandleTypeDef *lora);
static LoRaWAN_Error_t configure_lorawan_params(ATC_HandleTypeDef *lora);
static LoRaWAN_Error_t save_and_reset(ATC_HandleTypeDef *lora);
static LoRaWAN_Error_t set_adr(ATC_HandleTypeDef *lora, bool enable);
static LoRaWAN_Error_t set_otaa(ATC_HandleTypeDef *lora, bool enable);
static LoRaWAN_Error_t set_class_a(ATC_HandleTypeDef *lora);
static LoRaWAN_Error_t set_data_rate(ATC_HandleTypeDef *lora, int dr);
static LoRaWAN_Error_t set_tx_power(ATC_HandleTypeDef *lora, int power);
static LoRaWAN_Error_t factor_reset(ATC_HandleTypeDef *lora);
static LoRaWAN_Error_t join_network(ATC_HandleTypeDef *lora);

/**
 * @brief Configures the RM126x LoRaWAN module for Japan (AS923-1) with TTN and initiates network join.
 * @param lora Pointer to the ATC handle for communication with the module.
 * @param dev_eui Device EUI (16 hex characters).
 * @param app_eui Application EUI (16 hex characters).
 * @param app_key Application key (32 hex characters).
 * @return true if configuration and join are successful, false otherwise.
 */
bool lorawan_configure(ATC_HandleTypeDef *lora, const char *dev_eui, const char *app_eui, const char *app_key) {
    LoRaWAN_Error_t err;

    factor_reset(lora);

    printf("DEBUG: lorawan_configure called with lora handle: %p\n", (void*)lora);
    if (lora == NULL) {
        printf("ERROR: lora handle is NULL!\n");
        return false;
    }
    if (lora->hUart == NULL) {
        printf("ERROR: lora->hUart is NULL!\n");
        return false;
    }
    printf("DEBUG: lora->hUart = %p, Name = %s\n", (void*)lora->hUart, lora->Name);

    // Validate input parameters
    if (!validate_hex_string(dev_eui, DEV_EUI_LENGTH) ||
        !validate_hex_string(app_eui, APP_EUI_LENGTH) ||
        !validate_hex_string(app_key, APP_KEY_LENGTH)) {
        printf("Invalid EUI or AppKey format\n");
        return false;
    }

    // Configure DevEUI, AppEUI, and AppKey
    if ((err = set_dev_eui(lora, dev_eui)) != LORAWAN_OK ||
        (err = set_app_eui(lora, app_eui)) != LORAWAN_OK ||
        (err = set_app_key(lora, app_key)) != LORAWAN_OK) {
        printf("Error setting EUIs or AppKey: %d\n", err);
        return false;
    }

    // Configure region and sub-band
    if ((err = configure_region_and_channel(lora)) != LORAWAN_OK) {
        printf("Error configuring region/channel: %d\n", err);
        return false;
    }

    // Check and set frequency
    if ((err = check_and_set_frequency(lora)) != LORAWAN_OK) {
        printf("Error checking/setting frequency: %d\n", err);
        return false;
    }

    // Configure LoRaWAN parameters (ADR, OTAA, Class, DR, TX Power)
    if ((err = configure_lorawan_params(lora)) != LORAWAN_OK) {
        printf("Error configuring LoRaWAN parameters: %d\n", err);
        return false;
    }

    // Save settings and reset
    if ((err = save_and_reset(lora)) != LORAWAN_OK) {
        printf("Error saving/resetting: %d\n", err);
        return false;
    }

    // Attempt to join the network
    if ((err = join_network(lora)) != LORAWAN_OK) {
        printf("Error joining network: %d\n", err);
        return false;
    }

    return true;
}

/**
 * @brief Validates that a string is hexadecimal and of the expected length.
 * @param str The string to validate.
 * @param expected_len The expected length of the string.
 * @return true if the string is valid hexadecimal and matches the length, false otherwise.
 */
static bool validate_hex_string(const char *str, size_t expected_len) {
    if (str == NULL || strlen(str) != expected_len) {
        return false;
    }
    for (size_t i = 0; i < expected_len; i++) {
        if (!((str[i] >= '0' && str[i] <= '9') ||
              (str[i] >= 'A' && str[i] <= 'F') ||
              (str[i] >= 'a' && str[i] <= 'f'))) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Sets the Device EUI (DevEUI) if not already set or invalid.
 * @param lora Pointer to the ATC handle for communication.
 * @param dev_eui Device EUI (16 hex characters).
 * @return LORAWAN_OK on success, LORAWAN_ERR_DEV_EUI or LORAWAN_ERR_AT_COMMAND on failure.
 */
static LoRaWAN_Error_t set_dev_eui(ATC_HandleTypeDef *lora, const char *dev_eui) {
    printf("DEBUG: set_dev_eui called\n");
    char response[AT_RESPONSE_BUFFER_SIZE];
    char *response_ptr = response;
    printf("DEBUG: About to call ATC_SendReceive for DevEUI query\n");
    int resp = ATC_SendReceive(lora, "ATS 501?\r\n", 100, &response_ptr, 200, 1, "OK");
    printf("DEBUG: ATC_SendReceive returned %d\n", resp);
    if (resp < 0) return LORAWAN_ERR_AT_COMMAND;

    if (strstr(response, "0000000000000000") != NULL) {
        char command[64];
        snprintf(command, sizeof(command), "ATS 501=\"%s\"\r\n", dev_eui);
        resp = ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
        if (resp < 0) return LORAWAN_ERR_DEV_EUI;
    }
    return LORAWAN_OK;
}

/**
 * @brief Sets the Application EUI (AppEUI/JoinEUI).
 * @param lora Pointer to the ATC handle for communication.
 * @param app_eui Application EUI (16 hex characters).
 * @return LORAWAN_OK on success, LORAWAN_ERR_APP_EUI on failure.
 */
static LoRaWAN_Error_t set_app_eui(ATC_HandleTypeDef *lora, const char *app_eui) {
    char command[64];
    snprintf(command, sizeof(command), "ATS 502=\"%s\"\r\n", app_eui);
    int resp = ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return (resp < 0) ? LORAWAN_ERR_APP_EUI : LORAWAN_OK;
}

/**
 * @brief Sets the Application Key (AppKey).
 * @param lora Pointer to the ATC handle for communication.
 * @param app_key Application key (32 hex characters).
 * @return LORAWAN_OK on success, LORAWAN_ERR_APP_KEY on failure.
 */
static LoRaWAN_Error_t set_app_key(ATC_HandleTypeDef *lora, const char *app_key) {
    char command[96];
    snprintf(command, sizeof(command), "ATS 500=\"%s\"\r\n", app_key);
    int resp = ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return (resp < 0) ? LORAWAN_ERR_APP_KEY : LORAWAN_OK;
}

/**
 * @brief Configures the region and sub-band for Japan (AS923-1) with TTN.
 * @param lora Pointer to the ATC handle for communication.
 * @return LORAWAN_OK on success, LORAWAN_ERR_AT_COMMAND on failure.
 */
static LoRaWAN_Error_t configure_region_and_channel(ATC_HandleTypeDef *lora) {
    char command[32];
    int resp;

    // Set region to AS923-1 (Japan)
    snprintf(command, sizeof(command), "ATS 611=%d\r\n", JAPAN_REGION);
    resp = ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    if (resp < 0) return LORAWAN_ERR_AT_COMMAND;

    // Set sub-band channel for TTN
//    snprintf(command, sizeof(command), "ATS 606=%d\r\n", TTN_SUBBAND_CHANNEL);
//    resp = ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
//    if (resp < 0) return LORAWAN_ERR_AT_COMMAND;

    return LORAWAN_OK;
}

/**
 * @brief Checks and sets the frequency to ensure compliance with AS923-1 (923.2–923.4 MHz).
 * @param lora Pointer to the ATC handle for communication.
 * @return LORAWAN_OK on success, LORAWAN_ERR_AT_COMMAND or LORAWAN_ERR_FREQ_CHECK on failure.
 */
static LoRaWAN_Error_t check_and_set_frequency(ATC_HandleTypeDef *lora) {
    char response[AT_RESPONSE_BUFFER_SIZE];
    char *response_ptr = response;
    int resp = ATC_SendReceive(lora, "ATS 605?\r\n", 100, &response_ptr, 200, 1, "OK");
    if (resp < 0) return LORAWAN_ERR_AT_COMMAND;

    // Parse frequency (assuming response contains numeric Hz value)
    unsigned long freq = atol(response);
    if (freq < AS923_1_FREQ_MIN || freq > AS923_1_FREQ_MAX) {
        char command[32];
        snprintf(command, sizeof(command), "ATS 605=%u\r\n", DEFAULT_FREQ);
        resp = ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
        if (resp < 0) return LORAWAN_ERR_FREQ_CHECK;
    }
    return LORAWAN_OK;
}

/**
 * @brief Configures LoRaWAN parameters (ADR, OTAA, Class, Data Rate, TX Power).
 * @param lora Pointer to the ATC handle for communication.
 * @return LORAWAN_OK on success, or an error code from a failed sub-function.
 */
static LoRaWAN_Error_t configure_lorawan_params(ATC_HandleTypeDef *lora) {
    LoRaWAN_Error_t err;

    // Disable ADR
    if ((err = set_adr(lora, false)) != LORAWAN_OK) return err;

    // Enable OTAA
    if ((err = set_otaa(lora, true)) != LORAWAN_OK) return err;

    // Set Class A
    if ((err = set_class_a(lora)) != LORAWAN_OK) return err;

    // Set static data rate (DR0)
    if ((err = set_data_rate(lora, DATA_RATE)) != LORAWAN_OK) return err;

    // Set TX power (11 dBm for Japan compliance)
    if ((err = set_tx_power(lora, TX_POWER)) != LORAWAN_OK) return err;

    return LORAWAN_OK;
}

/**
 * @brief Disables or enables Adaptive Data Rate (ADR).
 * @param lora Pointer to the ATC handle for communication.
 * @param enable true to enable ADR, false to disable.
 * @return LORAWAN_OK on success, LORAWAN_ERR_AT_COMMAND on failure.
 */
static LoRaWAN_Error_t set_adr(ATC_HandleTypeDef *lora, bool enable) {
    char command[32];
    snprintf(command, sizeof(command), "ATS 600=%d\r\n", enable ? 1 : 0);
    int resp = ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return (resp < 0) ? LORAWAN_ERR_AT_COMMAND : LORAWAN_OK;
}

/**
 * @brief Enables or disables Over-The-Air Activation (OTAA).
 * @param lora Pointer to the ATC handle for communication.
 * @param enable true to enable OTAA, false to disable.
 * @return LORAWAN_OK on success, LORAWAN_ERR_AT_COMMAND on failure.
 */
static LoRaWAN_Error_t set_otaa(ATC_HandleTypeDef *lora, bool enable) {
    char command[32];
    snprintf(command, sizeof(command), "ATS 602=%d\r\n", enable ? 1 : 0);
    int resp = ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return (resp < 0) ? LORAWAN_ERR_AT_COMMAND : LORAWAN_OK;
}

/**
 * @brief Sets the LoRaWAN device to Class A.
 * @param lora Pointer to the ATC handle for communication.
 * @return LORAWAN_OK on success, LORAWAN_ERR_AT_COMMAND on failure.
 */
static LoRaWAN_Error_t set_class_a(ATC_HandleTypeDef *lora) {
    int resp = ATC_SendReceive(lora, "ATS 603=0\r\n", 100, NULL, 200, 1, "OK");
    return (resp < 0) ? LORAWAN_ERR_AT_COMMAND : LORAWAN_OK;
}

/**
 * @brief Sets the static data rate for LoRaWAN communication.
 * @param lora Pointer to the ATC handle for communication.
 * @param dr Data rate to set (e.g., 0 for DR0).
 * @return LORAWAN_OK on success, LORAWAN_ERR_AT_COMMAND on failure.
 */
static LoRaWAN_Error_t set_data_rate(ATC_HandleTypeDef *lora, int dr) {
    char command[32];
    snprintf(command, sizeof(command), "ATS 713=%d\r\n", dr);
    int resp = ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return (resp < 0) ? LORAWAN_ERR_AT_COMMAND : LORAWAN_OK;
}

/**
 * @brief Sets the TX power for LoRaWAN transmission.
 * @param lora Pointer to the ATC handle for communication.
 * @param power TX power in dBm (e.g., 11 for Japan compliance).
 * @return LORAWAN_OK on success, LORAWAN_ERR_AT_COMMAND on failure.
 */
static LoRaWAN_Error_t set_tx_power(ATC_HandleTypeDef *lora, int power) {
    char command[32];
    snprintf(command, sizeof(command), "ATS 714=%d\r\n", power);
    int resp = ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return (resp < 0) ? LORAWAN_ERR_AT_COMMAND : LORAWAN_OK;
}

/**
 * @brief Initiates the LoRaWAN network join process using OTAA.
 * @param lora Pointer to the ATC handle for communication.
 * @return LORAWAN_OK on successful join, LORAWAN_ERR_JOIN or LORAWAN_ERR_AT_COMMAND on failure.
 */
static LoRaWAN_Error_t join_network(ATC_HandleTypeDef *lora) {
    char response[AT_RESPONSE_BUFFER_SIZE];
    char *response_ptr = response;
    int resp = ATC_SendReceive(lora, "AT+JOIN\r\n", 100, &response_ptr, JOIN_TIMEOUT_MS, 1, "OK");
    if (resp < 0) return LORAWAN_ERR_AT_COMMAND;

    // Check for join success (expecting a response like "+JOIN: Joined" or similar)
    if (strstr(response, "Joined") == NULL) {
        return LORAWAN_ERR_JOIN;
    }

    return LORAWAN_OK;
}

/**
 * @brief Sets all writable parameters to default values and clears the trusted device bond database then performs a warm reset.
 * @param lora Pointer to the ATC handle for communication.
 * @return LORAWAN_OK on success AFTER the reboot, LORAWAN_ERR_AT_COMMAND on failure.
 */
static LoRaWAN_Error_t factor_reset(ATC_HandleTypeDef *lora) {
    char command[32];
    int resp = ATC_SendReceive(lora, command, 100, NULL, 200, 1, "OK");
    return (resp < 0) ? LORAWAN_ERR_AT_COMMAND : LORAWAN_OK;
}

/**
 * @brief Saves settings to non-volatile memory and performs a warm reset.
 * @param lora Pointer to the ATC handle for communication.
 * @return LORAWAN_OK on success, LORAWAN_ERR_SAVE_RESET on failure.
 */
static LoRaWAN_Error_t save_and_reset(ATC_HandleTypeDef *lora) {
    int resp;

    // Save settings to non-volatile memory
    resp = ATC_SendReceive(lora, "AT&W\r\n", 100, NULL, 200, 1, "OK");
    if (resp < 0) return LORAWAN_ERR_SAVE_RESET;

    // Perform warm reset
    resp = ATC_SendReceive(lora, "ATZ\r\n", 100, NULL, 200, 1, "OK");
    if (resp < 0) return LORAWAN_ERR_SAVE_RESET;

    return LORAWAN_OK;
}
