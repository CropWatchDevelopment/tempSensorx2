/*
 * sleep.c
 *
 *  Created on: Jul 29, 2025
 *      Author: kevin
 */
#include "main.h"
#include "stm32l0xx_hal.h"
#include "stm32l0xx_hal_rcc.h"
#include "sleep.h"

void enter_sleep_mode()
{
	ConsolePrintf("Going to sleep...\r\n");
	
	// Disable peripheral clocks during sleep mode for maximum power saving
	__HAL_RCC_ADC1_CLK_SLEEP_DISABLE();
	__HAL_RCC_I2C1_CLK_SLEEP_DISABLE(); 
	__HAL_RCC_USART1_CLK_SLEEP_DISABLE();
	__HAL_RCC_LPUART1_CLK_SLEEP_DISABLE();
	
	// CRITICAL: Disable GPIO clocks during sleep mode for maximum power saving
	__HAL_RCC_GPIOA_CLK_SLEEP_DISABLE();
	__HAL_RCC_GPIOB_CLK_SLEEP_DISABLE();
	
	// Disable other power-hungry peripherals during sleep
	__HAL_RCC_DMA1_CLK_SLEEP_DISABLE();
	__HAL_RCC_SYSCFG_CLK_SLEEP_DISABLE();
	
	// Enable Ultra Low Power mode (disables VREFINT during stop mode)
	HAL_PWREx_EnableUltraLowPower();
	// Enable fast wake-up to reduce VREFINT startup time
	HAL_PWREx_EnableFastWakeUp();
	
	// CRITICAL: Disable debug interface in sleep mode to save power
	__HAL_RCC_DBGMCU_CLK_SLEEP_DISABLE();
	
	// Configure all unused GPIO pins to analog mode for minimum power consumption
	configure_gpio_for_low_power();
	
	HAL_I2C_DeInit(&hi2c1);
	HAL_UART_DeInit(&huart1);
	HAL_UART_DeInit(&hlpuart1);
	MX_ADC_DeInit();
	__HAL_UART_DISABLE_IT(&hlpuart1, UART_IT_RXNE);
	__HAL_UART_DISABLE_IT(&hlpuart1, UART_IT_IDLE);
	__HAL_UART_CLEAR_FLAG(&hlpuart1, UART_FLAG_RXNE | UART_FLAG_IDLE);
	__HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE);
	__HAL_UART_DISABLE_IT(&huart1, UART_IT_IDLE);
	__HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE | UART_FLAG_IDLE);
	
	Enter_Stop_Mode();
	
	ConsolePrintf("Resumed after wake-up\r\n");
	
	// Re-enable peripheral clocks after wake-up
	__HAL_RCC_ADC1_CLK_SLEEP_ENABLE();
	__HAL_RCC_I2C1_CLK_SLEEP_ENABLE(); 
	__HAL_RCC_USART1_CLK_SLEEP_ENABLE();
	__HAL_RCC_LPUART1_CLK_SLEEP_ENABLE();
	
	// Re-enable GPIO clocks after wake-up
	__HAL_RCC_GPIOA_CLK_SLEEP_ENABLE();
	__HAL_RCC_GPIOB_CLK_SLEEP_ENABLE();
	
	// Re-enable other peripherals after wake-up
	__HAL_RCC_DMA1_CLK_SLEEP_ENABLE();
	__HAL_RCC_SYSCFG_CLK_SLEEP_ENABLE();
	__HAL_RCC_DBGMCU_CLK_SLEEP_ENABLE();
	
	SystemClock_Config();
	ConsolePrintf("System clock reconfigured\r\n");
	
	// Restore GPIO configuration after sleep
	restore_gpio_after_sleep();
	
	MX_I2C1_Init();
	ConsolePrintf("I2C1 reinitialized\r\n");
	MX_USART1_UART_Init();
	ConsolePrintf("UART reinitialized\r\n");
	MX_LPUART1_UART_Init();
	ConsolePrintf("LPUART1 (lora) reinitialized\r\n");
	RTC_WakeUp_Init();
	ConsolePrintf("RTC Wake-Up Timer reinitialized\r\n");
	MX_ADC_Init();
	ConsolePrintf("ADC reinitialized\r\n");
}

void RTC_WakeUp_Init(void)
{
  ConsolePrintf("Starting RTC Wake-Up Timer configuration\r\n");

  // Disable the Wake-Up Timer before configuring
  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
  ConsolePrintf("RTC Wake-Up Timer disabled\r\n");

  // Configure Wake-Up Timer for 60 seconds using LSI (~40 kHz)
  // With AsynchPrediv = 127, SynchPrediv = 255: CK_SPRE = 40,000 / (128 * 256) = ~1.22 Hz
  // For ~60 seconds: WakeUpCounter = (60 * 1.22) - 1 = ~72
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 59, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK)
  {
    ConsolePrintf("RTC Wake-Up Timer Init Failed\r\n");
  }
  else
  {
    ConsolePrintf("RTC Wake-Up Timer Initialized for ~60 seconds\r\n");
  }

  // Enable RTC Wake-Up interrupt in NVIC
  HAL_NVIC_SetPriority(RTC_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(RTC_IRQn);
  ConsolePrintf("RTC Wake-Up interrupt enabled in NVIC\r\n");
}

void MX_ADC_DeInit(void)
{
    /* 1) De‑initialize the ADC handle */
    if (HAL_ADC_DeInit(&hadc) != HAL_OK) {
        Error_Handler();
    }
    /* 2) Disable the ADC clock */
    __HAL_RCC_ADC1_CLK_DISABLE();

    /* 3) Reset the GPIO pin back to its default state
       (PB1 was configured as analog in MX_GPIO_Init) */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_1);
}

void Enter_Stop_Mode(void)
{
  ConsolePrintf("Preparing to enter Stop mode\r\n");

  // Clear Wake-Up flag
  __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
  ConsolePrintf("RTC Wake-Up flag cleared\r\n");

  // Enter Stop mode (low-power mode)
  ConsolePrintf("Entering Stop mode\r\n");
  /* Suspend SysTick to prevent it from waking up the MCU immediately */
  HAL_SuspendTick();
  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
  /* Resume SysTick after waking up */
  HAL_ResumeTick();
  ConsolePrintf("Exited Stop mode\r\n");
}

void RTC_IRQHandler(void)
{
  HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
  // Reconfigure system clock after wake-up
  SystemClock_Config();
}

void configure_gpio_for_low_power(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Configure all unused GPIO pins to analog mode to minimize leakage current
    // This is CRITICAL for achieving lowest power consumption
    
    // GPIOA: Set unused pins to analog mode (except PA0 - VBAT_MEAS, PA2/PA3 - USART1, PA9/PA10 if used)
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | 
                          GPIO_PIN_8 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // GPIOB: Set unused pins to analog mode (except PB0 - VBAT_MEAS_EN, PB1 - ADC, PB5 - I2C_ENABLE, PB6/PB7 - I2C1, PB10/PB11 - LPUART1)
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_8 | 
                          GPIO_PIN_9 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void restore_gpio_after_sleep(void)
{
    // Re-initialize GPIO pins that need to be restored after sleep
    // This should restore only the pins that are actually used
    // Most pins can remain in analog mode
    MX_GPIO_Init(); // This will restore the necessary GPIO configurations
}
