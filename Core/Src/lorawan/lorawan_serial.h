/*
 * lorawan_serial.h
 *
 * A small reusable driver for interacting with an AT‑command based LoRaWAN
 * module over LPUART1 on the STM32L0 series.  This driver avoids using
 * DMA for reception so that the microcontroller can safely enter STOP or
 * standby low‑power modes and wake up when a start bit is detected on
 * the RX line.  It maintains a simple ring buffer for bytes received
 * and can invoke a user supplied callback whenever a complete line of
 * text (terminated by '\n' or '\r') is received.  Before entering
 * low‑power mode you must call LoRaSerial_EnterLowPower() to configure
 * the wake‑up source and abort any outstanding reception.  After the
 * device wakes up you must call LoRaSerial_ExitLowPower() to restore
 * normal operation.
 *
 * This driver is intentionally minimal: it performs no parsing of the
 * AT replies and leaves protocol handling to your application.  It
 * simply provides an interrupt driven UART interface that is friendly
 * to deep sleep.  See lorawan_serial.c for implementation details.
 */

#ifndef LORAWAN_SERIAL_H
#define LORAWAN_SERIAL_H

#include "stm32l0xx_hal.h"

/*
 * Prototype for the callback invoked whenever a complete line has
 * been received.  The supplied string is null terminated and lives
 * within an internal buffer; it is only valid until the next line is
 * received so copy it if you need to retain the contents.  This
 * callback is executed from the context of the UART RX interrupt so it
 * should do minimal processing – ideally just set flags or copy data
 * into a queue – and return quickly.
 */
typedef void (*LoRaLineCallback)(const char *line);

/*
 * Handle structure for the serial driver.  Your application should
 * allocate one of these and pass it to LoRaSerial_Init().
 */
typedef struct {
    UART_HandleTypeDef *huart;      /* pointer to the HAL UART handle */
    uint8_t *rxBuffer;              /* circular buffer for incoming bytes */
    uint16_t bufferSize;            /* size of rxBuffer */
    volatile uint16_t head;         /* head index in the ring buffer */
    volatile uint16_t tail;         /* tail index in the ring buffer */
    LoRaLineCallback lineCallback;  /* called when a full line is received */
    char lineBuf[256];              /* temporary buffer for assembling lines */
    uint16_t linePos;               /* current position in lineBuf */
    uint8_t rxByte;                 /* single byte buffer for interrupt reception */
    uint8_t inLowPower;             /* flag indicating low power mode is active */
} LoRaSerial_HandleTypeDef;

/*
 * Initialise the LoRaSerial driver.  Supply the underlying HAL UART
 * handle (e.g. &hlpuart1), an externally allocated byte buffer for
 * reception, its size, and a pointer to a callback function.  The
 * buffer size should be large enough to hold bursts of incoming data
 * between wakeups; 64–256 bytes is reasonable for AT replies.
 */
HAL_StatusTypeDef LoRaSerial_Init(LoRaSerial_HandleTypeDef *h,
                                  UART_HandleTypeDef *huart,
                                  uint8_t *rxBuffer,
                                  uint16_t bufferSize,
                                  LoRaLineCallback cb);

/*
 * Transmit a null terminated command string to the LoRa module.  This
 * function blocks until all bytes are sent; if you need
 * non‑blocking transmission consider using HAL_UART_Transmit_IT().
 */
HAL_StatusTypeDef LoRaSerial_Send(LoRaSerial_HandleTypeDef *h,
                                  const char *data);

/*
 * To be called from your application's implementation of
 * HAL_UART_RxCpltCallback().  Pass the pointer to your LoRaSerial
 * handle and the UART handle from the callback so the driver can
 * confirm the interrupt belongs to the configured UART.  This function
 * returns void because the HAL callback prototype does not permit a
 * return value.
 */
void LoRaSerial_RxCpltCallback(LoRaSerial_HandleTypeDef *h,
                               UART_HandleTypeDef *huart);

/*
 * Prepare the UART to enter STOP or standby low‑power mode.  This
 * aborts any ongoing reception, flushes the line buffer and configures
 * the wake‑up source to trigger on a start bit.  It enables the
 * UART WUF interrupt and enables Stop mode on the UART.  After
 * calling this function your application should configure the power
 * regulator and call HAL_PWR_EnterSTOPMode() or similar.  If the
 * device is already in low power mode this does nothing and returns
 * HAL_OK.
 */
HAL_StatusTypeDef LoRaSerial_EnterLowPower(LoRaSerial_HandleTypeDef *h);

/*
 * Restore normal UART operation after waking from STOP or standby mode.
 * This disables the wake‑up interrupt and Stop mode on the UART,
 * flushes the ring buffer and restarts interrupt based reception.
 * Should be called immediately after returning from HAL_PWR_EnterSTOPMode().
 */
HAL_StatusTypeDef LoRaSerial_ExitLowPower(LoRaSerial_HandleTypeDef *h);

#endif /* LORAWAN_SERIAL_H */