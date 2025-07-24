/*
 * lorawan_serial.c
 *
 * See lorawan_serial.h for a high level description.  This file
 * implements a lightweight UART receive/transmit driver that is
 * compatible with deep sleep on the STM32L0 series.  It uses
 * interrupt driven reception so that DMA can remain disabled while
 * sleeping; DMA cannot wake the MCU from STOP mode because the
 * hardware wake‑up logic only monitors the UART receiver line when
 * DMA is idle.  Before entering low power the driver configures the
 * wake‑up source to the start bit of the next character.  When a
 * start bit is detected the MCU wakes up and reception continues
 * where it left off.
 */

#include "lorawan_serial.h"
#include <string.h>

static void clear_buffers(LoRaSerial_HandleTypeDef *h)
{
    h->head = h->tail = 0;
    h->linePos = 0;
    memset(h->lineBuf, 0, sizeof(h->lineBuf));
}

HAL_StatusTypeDef LoRaSerial_Init(LoRaSerial_HandleTypeDef *h,
                                  UART_HandleTypeDef *huart,
                                  uint8_t *rxBuffer,
                                  uint16_t bufferSize,
                                  LoRaLineCallback cb)
{
    if (!h || !huart || !rxBuffer || bufferSize == 0) {
        return HAL_ERROR;
    }
    h->huart      = huart;
    h->rxBuffer   = rxBuffer;
    h->bufferSize = bufferSize;
    h->lineCallback = cb;
    h->inLowPower = 0;
    clear_buffers(h);
    /* Start an interrupt based reception of a single byte.  We do
     * single byte reads so that the RXNE interrupt fires on every
     * character; this avoids DMA and allows the UART to wake the MCU
     * on start bit when in STOP mode.  The received byte will be
     * placed into h->rxByte and the HAL will call HAL_UART_RxCpltCallback().
     */
    return HAL_UART_Receive_IT(h->huart, &h->rxByte, 1);
}

HAL_StatusTypeDef LoRaSerial_Send(LoRaSerial_HandleTypeDef *h,
                                  const char *data)
{
    if (!h || !data) {
        return HAL_ERROR;
    }
    size_t len = strlen(data);
    /* Use blocking transmit; the application can wrap this in a
     * critical section or adjust the timeout as desired. */
    return HAL_UART_Transmit(h->huart, (uint8_t*)data, (uint16_t)len, HAL_MAX_DELAY);
}

/* Internal helper to push a byte into the ring buffer */
static void push_byte(LoRaSerial_HandleTypeDef *h, uint8_t ch)
{
    uint16_t next = (h->head + 1) % h->bufferSize;
    if (next != h->tail) {
        h->rxBuffer[h->head] = ch;
        h->head = next;
    }
    /* else overflow – drop the byte */
}

/* Called from the HAL UART RX complete interrupt handler.  You must
 * forward the HAL callback to this function from HAL_UART_RxCpltCallback()
 * in your code. */
void LoRaSerial_RxCpltCallback(LoRaSerial_HandleTypeDef *h,
                               UART_HandleTypeDef *huart)
{
    if (!h || h->huart != huart) {
        return;
    }
    uint8_t ch = h->rxByte;
    /* Store the character in the ring buffer */
    push_byte(h, ch);
    /* Build up a line until we see '\n' or '\r' */
    if (h->linePos < sizeof(h->lineBuf) - 1) {
        if (ch != '\r' && ch != '\n') {
            h->lineBuf[h->linePos++] = (char)ch;
        }
        /* On newline or carriage return, terminate and call callback */
        if (ch == '\r' || ch == '\n') {
            h->lineBuf[h->linePos] = '\0';
            if (h->linePos > 0 && h->lineCallback) {
                h->lineCallback(h->lineBuf);
            }
            /* Reset line buffer for next message */
            h->linePos = 0;
            memset(h->lineBuf, 0, sizeof(h->lineBuf));
        }
    } else {
        /* Buffer full – reset */
        h->linePos = 0;
        memset(h->lineBuf, 0, sizeof(h->lineBuf));
    }
    /* Restart reception of next byte, unless in low power */
    if (!h->inLowPower) {
        HAL_UART_Receive_IT(h->huart, &h->rxByte, 1);
    }
}

HAL_StatusTypeDef LoRaSerial_EnterLowPower(LoRaSerial_HandleTypeDef *h)
{
    if (!h) {
        return HAL_ERROR;
    }
    if (h->inLowPower) {
        return HAL_OK; /* already in low power */
    }
    h->inLowPower = 1;
    /* Abort any ongoing reception so that the UART can be reconfigured */
    HAL_UART_AbortReceive(h->huart);
    /* Flush buffers so we start clean after waking */
    clear_buffers(h);
    /* Configure the wake‑up source to the start bit.  See
     * HAL_UARTEx_StopModeWakeUpSourceConfig() in stm32l0xx_hal_uart_ex.c
     * for details. */
    UART_WakeUpTypeDef wakeup;
    wakeup.WakeUpEvent = UART_WAKEUP_ON_STARTBIT;
    wakeup.AddressLength = UART_ADDRESS_DETECT_7B;
    wakeup.Address = 0; /* unused for start bit mode */
    if (HAL_UARTEx_StopModeWakeUpSourceConfig(h->huart, wakeup) != HAL_OK) {
        return HAL_ERROR;
    }
    /* Enable the wakeup interrupt (WUF bit) */
    __HAL_UART_ENABLE_IT(h->huart, UART_IT_WUF);
    /* Enable Stop mode on the UART */
    if (HAL_UARTEx_EnableStopMode(h->huart) != HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef LoRaSerial_ExitLowPower(LoRaSerial_HandleTypeDef *h)
{
    if (!h) {
        return HAL_ERROR;
    }
    if (!h->inLowPower) {
        return HAL_OK; /* not in low power */
    }
    h->inLowPower = 0;
    /* Disable wakeup interrupt and Stop mode */
    __HAL_UART_DISABLE_IT(h->huart, UART_IT_WUF);
    if (HAL_UARTEx_DisableStopMode(h->huart) != HAL_OK) {
        return HAL_ERROR;
    }
    /* Clear any lingering wakeup flag */
    __HAL_UART_CLEAR_FLAG(h->huart, UART_CLEAR_WUF);
    /* Restart reception */
    return HAL_UART_Receive_IT(h->huart, &h->rxByte, 1);
}