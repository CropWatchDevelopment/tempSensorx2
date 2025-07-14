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
#include "atc.h"

bool lorawan_configure(ATC_HandleTypeDef *lora) {
    int resp;

    // 1) Set AppKey (DevEUI||DevEUI)
    resp = ATC_SendReceive(lora,
        "AT%S 500=\"00000000000000000000000000000000\"\r\n", // Pass this in...
        100, NULL, 200, 1, "OK");
    if (resp < 0) return false;

    // 2) ADR off
    resp = ATC_SendReceive(lora,
        "ATS 600=0\r\n",
        100, NULL, 200, 1, "OK");
    if (resp < 0) return false;

    // 3) OTAA activation
    resp = ATC_SendReceive(lora,
        "ATS 602=1\r\n",
        100, NULL, 200, 1, "OK");
    if (resp < 0) return false;

    // 4) Class A
    resp = ATC_SendReceive(lora,
        "ATS 603=0\r\n",
        100, NULL, 200, 1, "OK");
    if (resp < 0) return false;

    // 5) Region AS923-1 (Japan)
    resp = ATC_SendReceive(lora,
        "ATS 611=9\r\n",
        100, NULL, 200, 1, "OK");
    if (resp < 0) return false;

    // 6) Static DR → DR0
    resp = ATC_SendReceive(lora,
        "ATS 713=0\r\n",
        100, NULL, 200, 1, "OK");
    if (resp < 0) return false;

    // 7) Static TX Power → 12 dBm
    resp = ATC_SendReceive(lora,
        "ATS 714=12\r\n",
        100, NULL, 200, 1, "OK");
    if (resp < 0) return false;

    // 8) Save settings
    resp = ATC_SendReceive(lora,
        "AT&W\r\n",
        100, NULL, 200, 1, "OK");
    if (resp < 0) return false;

    // 9) Apply settings (warm reset)
    resp = ATC_SendReceive(lora,
        "ATZ\r\n",
        100, NULL, 200, 1, "OK");
    if (resp < 0) return false;

    return true;
}
