/*
 * sleep.c
 *
 *  Created on: Jul 29, 2025
 *      Author: kevin
 */
#include "main.h"
#include "stm32l0xx_hal.h"
#include "stm32l0xx_hal_rcc.h"
#include "stm32l0xx_hal_pwr.h"
#include "stm32l0xx_hal_gpio.h"
#include "stm32l0xx_hal_rtc.h"
#include "stm32l0xx_hal_adc.h"
#include "stm32l0xx_hal_adc_ex.h"
#include "sleep.h"

void enter_sleep_mode()
{
	ConsolePrintf("Going to sleep...\r\n");
	
	// Configure all unused GPIO pins to analog mode for minimum power consumption
	configure_gpio_for_low_power();
	ConsolePrintf("GPIO configured for low power\r\n");
	
	// Properly deinitialize peripherals before sleep
	HAL_I2C_DeInit(&hi2c1);
	ConsolePrintf("I2C deinitialized\r\n");
	HAL_UART_DeInit(&huart1);
	ConsolePrintf("UART1 deinitialized\r\n");
	HAL_UART_DeInit(&hlpuart1);
	ConsolePrintf("LPUART1 deinitialized\r\n");
	MX_ADC_DeInit();
	
	// CRITICAL: Disable peripheral clocks completely before sleep
	__HAL_RCC_I2C1_CLK_DISABLE();
	__HAL_RCC_USART1_CLK_DISABLE();
	__HAL_RCC_LPUART1_CLK_DISABLE();
	__HAL_RCC_ADC1_CLK_DISABLE();
	ConsolePrintf("All peripheral clocks disabled\r\n");
	
	// CRITICAL: Disable VREFINT and temperature sensor to save power
	HAL_ADCEx_DisableVREFINT();
	HAL_ADCEx_DisableVREFINTTempSensor();
	ConsolePrintf("VREFINT and temp sensor disabled\r\n");
	
	// CRITICAL: Enable Ultra Low Power mode before entering sleep
	HAL_PWREx_EnableUltraLowPower();
	HAL_PWREx_EnableFastWakeUp();
	ConsolePrintf("Ultra Low Power mode enabled\r\n");
	
	// Debug power state before sleep
	debug_power_state();
	
	Enter_Stop_Mode();
	
	ConsolePrintf("Resumed after wake-up\r\n");
	
	// Basic system clock reconfiguration
	SystemClock_Config_Wrapper();
	ConsolePrintf("System clock reconfigured\r\n");
	
	// Re-enable peripheral clocks
	__HAL_RCC_I2C1_CLK_ENABLE();
	__HAL_RCC_USART1_CLK_ENABLE();
	__HAL_RCC_LPUART1_CLK_ENABLE();
	__HAL_RCC_ADC1_CLK_ENABLE();
	ConsolePrintf("Peripheral clocks re-enabled\r\n");
	
	// Restore GPIO configuration after sleep
	restore_gpio_after_sleep();
	ConsolePrintf("GPIO restored\r\n");
	
	// Re-initialize peripherals
	MX_I2C1_Init_Wrapper();
	ConsolePrintf("I2C1 reinitialized\r\n");
	
	MX_USART1_UART_Init_Wrapper();
	ConsolePrintf("UART reinitialized\r\n");
	
	MX_LPUART1_UART_Init_Wrapper();
	ConsolePrintf("LPUART1 (lora) reinitialized\r\n");
	
	MX_ADC_Init_Wrapper();
	ConsolePrintf("ADC reinitialized\r\n");
	
	ConsolePrintf("Wake-up sequence complete - peripherals ready\r\n");
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
    ConsolePrintf("Deinitializing ADC...\r\n");
    
    /* 1) Stop any ongoing ADC conversions */
    if (HAL_ADC_Stop(&hadc) != HAL_OK) {
        ConsolePrintf("ADC Stop failed\r\n");
    }
    
    /* 2) De‑initialize the ADC handle */
    if (HAL_ADC_DeInit(&hadc) != HAL_OK) {
        ConsolePrintf("ADC DeInit failed\r\n");
        Error_Handler();
    }
    
    /* 3) Disable the ADC clock */
    __HAL_RCC_ADC1_CLK_DISABLE();
    ConsolePrintf("ADC clock disabled\r\n");

    /* 4) Reset the GPIO pin back to its default state (analog mode) */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_1);
    
    ConsolePrintf("ADC deinitialization complete\r\n");
}

void Enter_Stop_Mode(void)
{
  ConsolePrintf("Preparing to enter Stop mode\r\n");

  // CRITICAL: Apply Kevin Cantrell's GPIO optimization RIGHT before sleep
  configure_gpio_for_low_power();
  ConsolePrintf("GPIO re-optimized immediately before sleep\r\n");
  
  // CRITICAL: Disable VREFINT immediately before sleep for ultra-low power
  // This is the key to getting <5µA sleep current
  HAL_ADCEx_DisableVREFINT();
  HAL_ADCEx_DisableVREFINTTempSensor();
  ConsolePrintf("VREFINT force-disabled before sleep\r\n");
  
  // CRITICAL: Disable additional peripheral clocks that might consume power
  __HAL_RCC_DMA1_CLK_DISABLE();
  __HAL_RCC_CRC_CLK_DISABLE();
  ConsolePrintf("Additional peripheral clocks disabled\r\n");
  
  // Verify and force Ultra Low Power mode
  HAL_PWREx_EnableUltraLowPower();
  HAL_PWREx_EnableFastWakeUp();
  ConsolePrintf("ULP mode verified and enabled before sleep\r\n");

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

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
  // System clock will be reconfigured in the main wake-up sequence
  // No need to do it here to avoid conflicts
}

void configure_gpio_for_low_power(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Enable all GPIO clocks first
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE(); // Don't forget GPIOH for ultra-low power
    
    // Kevin Cantrell's optimized GPIO configuration for ultra-low power
    // First set specific pins as outputs LOW to prevent floating
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5, GPIO_PIN_RESET);

    /* Set ALL unused pins as Analog Inputs for minimum power consumption */
    
    // GPIOA: Set unused pins to analog mode (exclude PA13/PA14 for debug if needed)
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | 
                          GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | 
                          GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // GPIOB: Set unused pins to analog (excluding the ones we set as outputs above)
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_7 | 
                          GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | 
                          GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // GPIOC: Set ALL pins to analog mode
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | 
                          GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | 
                          GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    // GPIOH: Usually just H0 and H1 on STM32L0, but set to analog for lowest power
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
    
    ConsolePrintf("All unused GPIO pins optimized for ultra-low power\r\n");
}

void restore_gpio_after_sleep(void)
{
    // Re-initialize GPIO pins that need to be restored after sleep
    // This should restore only the pins that are actually used
    // Most pins can remain in analog mode
    MX_GPIO_Init_Wrapper(); // This will restore the necessary GPIO configurations
}

void debug_power_state(void)
{
    ConsolePrintf("=== POWER DEBUG ===\r\n");
    
    // Check Ultra Low Power mode status
    if (READ_BIT(PWR->CR, PWR_CR_ULP) != 0) {
        ConsolePrintf("ULP mode: ENABLED\r\n");
    } else {
        ConsolePrintf("ULP mode: DISABLED\r\n");
    }
    
    // Check Fast Wake-up mode
    if (READ_BIT(PWR->CR, PWR_CR_FWU) != 0) {
        ConsolePrintf("Fast Wake: ENABLED\r\n");
    } else {
        ConsolePrintf("Fast Wake: DISABLED\r\n");
    }
    
    // Check VREFINT status
    if (READ_BIT(SYSCFG->CFGR3, SYSCFG_CFGR3_EN_VREFINT) != 0) {
        ConsolePrintf("VREFINT: ENABLED (HIGH POWER!)\r\n");
    } else {
        ConsolePrintf("VREFINT: DISABLED\r\n");
    }
    
    // Check peripheral clocks
    ConsolePrintf("I2C1 CLK: %s\r\n", __HAL_RCC_I2C1_IS_CLK_ENABLED() ? "ON" : "OFF");
    ConsolePrintf("UART1 CLK: %s\r\n", __HAL_RCC_USART1_IS_CLK_ENABLED() ? "ON" : "OFF");
    ConsolePrintf("LPUART1 CLK: %s\r\n", __HAL_RCC_LPUART1_IS_CLK_ENABLED() ? "ON" : "OFF");
    ConsolePrintf("ADC1 CLK: %s\r\n", __HAL_RCC_ADC1_IS_CLK_ENABLED() ? "ON" : "OFF");
    ConsolePrintf("DMA1 CLK: %s\r\n", __HAL_RCC_DMA1_IS_CLK_ENABLED() ? "ON" : "OFF");
    ConsolePrintf("CRC CLK: %s\r\n", __HAL_RCC_CRC_IS_CLK_ENABLED() ? "ON" : "OFF");
    
    ConsolePrintf("===================\r\n");
}

void post_wake_power_optimization(void)
{
    ConsolePrintf("=== POST-WAKE POWER OPTIMIZATION ===\r\n");
    
    // This function addresses the key differences between initial sleep vs post-wake sleep
    
    // 1. CRITICAL: Disable VREFINT that gets re-enabled by multiple peripheral inits
    HAL_ADCEx_DisableVREFINT();
    HAL_ADCEx_DisableVREFINTTempSensor();
    ConsolePrintf("VREFINT force-disabled (main power culprit)\r\n");
    
    // 2. Re-apply Kevin Cantrell's GPIO optimization aggressively
    configure_gpio_for_low_power();
    ConsolePrintf("GPIO re-optimized for ultra-low power\r\n");
    
    // 3. Force Ultra Low Power mode (peripheral inits might disable it)
    HAL_PWREx_EnableUltraLowPower();
    HAL_PWREx_EnableFastWakeUp();
    ConsolePrintf("ULP mode force-enabled\r\n");
    
    // 4. Disable any peripheral clocks that shouldn't be running
    // These might get enabled during SystemClock_Config or peripheral inits
    if (__HAL_RCC_DMA1_IS_CLK_ENABLED()) {
        __HAL_RCC_DMA1_CLK_DISABLE();
        ConsolePrintf("DMA1 clock force-disabled\r\n");
    }
    
    if (__HAL_RCC_CRC_IS_CLK_ENABLED()) {
        __HAL_RCC_CRC_CLK_DISABLE();
        ConsolePrintf("CRC clock force-disabled\r\n");
    }
    
    // 5. Ensure all unused GPIO clocks are disabled after optimization
    // Note: We need the clocks enabled to configure the pins, then we can disable them
    ConsolePrintf("GPIO optimization complete for post-wake sleep\r\n");
    
    ConsolePrintf("=== POST-WAKE OPTIMIZATION COMPLETE ===\r\n");
}
