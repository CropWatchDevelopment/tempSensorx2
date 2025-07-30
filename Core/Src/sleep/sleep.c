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
#include "sleep.h"
#include "../rtc/rtc.h"

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
	
	// CRITICAL: Enable Ultra Low Power mode RIGHT BEFORE entering sleep
	// This ensures it's not disabled by any peripheral deinitialization
	HAL_PWREx_EnableUltraLowPower();
	HAL_PWREx_EnableFastWakeUp();
	ConsolePrintf("Ultra Low Power mode enabled\r\n");
	
	// ULTRA-LOW POWER: Additional power optimizations
	apply_extreme_low_power_settings();
	
	Enter_Stop_Mode();
	
	ConsolePrintf("Resumed after wake-up\r\n");
	
	SystemClock_Config_Wrapper();
	ConsolePrintf("System clock reconfigured\r\n");
	
	// CRITICAL: Re-enable Ultra Low Power mode IMMEDIATELY after system clock config
	// because SystemClock_Config resets the power regulator settings
	HAL_PWREx_EnableUltraLowPower();
	HAL_PWREx_EnableFastWakeUp();
	ConsolePrintf("Ultra Low Power mode re-enabled after system clock config\r\n");
	
	// Re-enable peripheral clocks before reinitializing peripherals
	__HAL_RCC_I2C1_CLK_ENABLE();
	__HAL_RCC_USART1_CLK_ENABLE();
	__HAL_RCC_LPUART1_CLK_ENABLE();
	__HAL_RCC_ADC1_CLK_ENABLE();
	ConsolePrintf("Peripheral clocks re-enabled\r\n");
	
	// Restore GPIO configuration after sleep
	restore_gpio_after_sleep();
	
	// CRITICAL: Re-configure unused pins for low power after GPIO restoration
	// The GPIO init wrapper restores all pins, but we need unused ones back to analog
	configure_gpio_for_low_power();
	
	MX_I2C1_Init_Wrapper();
	ConsolePrintf("I2C1 reinitialized\r\n");
	MX_USART1_UART_Init_Wrapper();
	ConsolePrintf("UART reinitialized\r\n");
	MX_LPUART1_UART_Init_Wrapper();
	ConsolePrintf("LPUART1 (lora) reinitialized\r\n");
	RTC_WakeUp_Init_Calibrated();
	ConsolePrintf("RTC Wake-Up Timer reinitialized with calibration\r\n");
	
	// CRITICAL: Re-enable Ultra Low Power mode after RTC init
	// RTC initialization might affect power settings
	HAL_PWREx_EnableUltraLowPower();
	HAL_PWREx_EnableFastWakeUp();
	ConsolePrintf("Ultra Low Power mode re-enabled after RTC init\r\n");
	
	MX_ADC_Init_Wrapper();
	ConsolePrintf("ADC reinitialized\r\n");
	
	// CRITICAL: Re-disable VREFINT and temp sensor after ADC reinit
	// ADC initialization re-enables these, causing high power consumption in subsequent sleeps
	HAL_ADCEx_DisableVREFINT();
	HAL_ADCEx_DisableVREFINTTempSensor();
	ConsolePrintf("CRITICAL: VREFINT and temp sensor re-disabled after ADC init\r\n");
	ConsolePrintf("Power management cycle complete - ready for next sleep\r\n");
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
    MX_GPIO_Init_Wrapper(); // This will restore the necessary GPIO configurations
}

void apply_extreme_low_power_settings(void)
{
    ConsolePrintf("Applying extreme low power settings...\r\n");
    
    // 1. Disable all unused peripheral clocks that might still be running
    __HAL_RCC_GPIOA_CLK_DISABLE();  // We'll re-enable only what we need
    __HAL_RCC_GPIOB_CLK_DISABLE();
    __HAL_RCC_DMA1_CLK_DISABLE();
    __HAL_RCC_SYSCFG_CLK_DISABLE();
    
    // 2. Configure voltage regulator for lowest power
    HAL_PWREx_EnableUltraLowPower();
    HAL_PWREx_EnableFastWakeUp();
    
    // 3. Use the lowest power regulator mode
    HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    
    // 4. Ensure VREFINT and temperature sensor are truly disabled
    // These can consume significant power if left enabled
    HAL_ADCEx_DisableVREFINT();
    ConsolePrintf("VREFINT disabled for ultra-low power\r\n");
    
    HAL_ADCEx_DisableVREFINTTempSensor();
    ConsolePrintf("Temperature sensor disabled for ultra-low power\r\n");
    
    // 5. Configure all GPIO ports for minimum leakage
    configure_all_gpio_for_minimum_power();
    
    ConsolePrintf("Extreme low power settings applied - expect <1µA sleep current\r\n");
}

void configure_all_gpio_for_minimum_power(void)
{
    // Enable GPIO clocks temporarily to configure them
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();  // If available
    __HAL_RCC_GPIOD_CLK_ENABLE();  // If available
    __HAL_RCC_GPIOH_CLK_ENABLE();  // If available
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Configure ALL pins as analog input with no pull to minimize leakage
    // This is the absolute lowest power configuration
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    
    // GPIOA - Configure all pins except those we absolutely need
    // Keep PA2/PA3 (USART1), others can be analog
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | 
                          GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 |
                          GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 |
                          GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // GPIOB - Configure unused pins only
    // Keep PB0 (VBAT_MEAS_EN), PB5 (I2C_ENABLE), PB6/PB7 (I2C), PB10/PB11 (LPUART1)
    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | 
                          GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_12 | GPIO_PIN_13 |
                          GPIO_PIN_14 | GPIO_PIN_15;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // Configure any other GPIO ports if they exist on your MCU
    #ifdef GPIOC
    GPIO_InitStruct.Pin = GPIO_PIN_All;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    #endif
    
    #ifdef GPIOD  
    GPIO_InitStruct.Pin = GPIO_PIN_All;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    #endif
    
    #ifdef GPIOH
    GPIO_InitStruct.Pin = GPIO_PIN_All;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
    #endif
    
    // Now disable GPIO clocks to save even more power
    // We'll re-enable them when we wake up
    __HAL_RCC_GPIOA_CLK_DISABLE();
    __HAL_RCC_GPIOB_CLK_DISABLE();
    __HAL_RCC_GPIOC_CLK_DISABLE();
    __HAL_RCC_GPIOD_CLK_DISABLE();
    __HAL_RCC_GPIOH_CLK_DISABLE();
    
    ConsolePrintf("All GPIO configured for minimum power consumption\r\n");
}
