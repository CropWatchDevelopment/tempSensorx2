/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h> // For bool, true, false
#include "sensirion/sensirion.h"
#include "LoRaWAN/lorawan.h"
#include "utils/sensor_payload.h"
#include "stm32l0xx_hal.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define AT_RESPONSE_BUFFER_SIZE 256
#define DEV_EUI_LENGTH 16
#define APP_EUI_LENGTH 16
#define APP_KEY_LENGTH 32
#define AS923_1_FREQ_MIN 923200000
#define AS923_1_FREQ_MAX 923400000
#define DEFAULT_FREQ 923200000
#define JAPAN_REGION 9
#define TTN_SUBBAND_CHANNEL 0
#define TX_POWER 11
#define DATA_RATE 0
#define JOIN_TIMEOUT_MS 10000

// LORAWAN DEFINES
#define AT_CMD_PREFIX "AT+SEND \""
#define AT_CMD_SUFFIX "\"\r\n"

// I2C DEFINES
#define SHT40_I2C_ADDR (0x44 << 1)        // SHT40 I2C address (shifted for HAL)
#define SHT40_MEASURE_HIGH_PRECISION 0xFD // High-precision measurement command
#define TIMEOUT 100                       // I2C timeout in ms

// Read Battery Voltage
#define VREFINT_CAL_ADDR   ((uint16_t*)0x1FF80078U)
#define ADC_MAX            4095U       // 12‑bit full scale


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart1;

RTC_HandleTypeDef hrtc;

/* USER CODE BEGIN PV */
ATC_HandleTypeDef lora;
int send_count = 0;
uint32_t battery_value_adc;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_RTC_Init(void);
static void MX_LPUART1_UART_Init(void);
static void MX_ADC_Init(void);
/* USER CODE BEGIN PFP */
void ConsolePrintf(const char *format, ...);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void RTC_IRQHandler(void)
{
  HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
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

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
  // Reconfigure system clock after wake-up
  SystemClock_Config();

  // Print message
  ConsolePrintf("Woke up at %s\r\n", "1-minute interval");
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













/**
 * @brief  Read VBAT (mV) via 1 MΩ:1 MΩ divider + unity buffer, no FPU
 * @retval Battery voltage in millivolts (rounded)
 */
uint32_t ReadBattery_mV(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    uint32_t raw_vref, raw_div;
    uint32_t Vdda_mV, Vnode_mV, VBAT_mV;

    // 1) Enable internal reference measurement
    HAL_ADCEx_EnableVREFINT();

    // 2) Sample VREFINT to compute true VDDA
    sConfig.Channel = ADC_CHANNEL_VREFINT;
    sConfig.Rank    = ADC_RANK_CHANNEL_NUMBER;
    HAL_ADC_ConfigChannel(&hadc, &sConfig);

    HAL_ADC_Start(&hadc);
    HAL_ADC_PollForConversion(&hadc, HAL_MAX_DELAY);
    raw_vref = HAL_ADC_GetValue(&hadc);
    HAL_ADC_Stop(&hadc);

    // Vdda = 3000 mV × VREFINT_CAL / raw_vref  (calibrated at 3.0 V)
    Vdda_mV = (3000UL * (*VREFINT_CAL_ADDR) + raw_vref/2U) / raw_vref;

    // 3) Turn on your divider/buffer and let it settle
    HAL_GPIO_WritePin(GPIOB, VBAT_MEAS_EN_Pin, GPIO_PIN_SET);
    HAL_Delay(300);

    // 4) Sample the divider output (VBAT/2)
    sConfig.Channel = ADC_CHANNEL_4;              // ← adjust to your sense pin
    sConfig.Rank    = ADC_RANK_CHANNEL_NUMBER;
    HAL_ADC_ConfigChannel(&hadc, &sConfig);

    HAL_ADC_Start(&hadc);
    HAL_ADC_PollForConversion(&hadc, HAL_MAX_DELAY);
    raw_div = HAL_ADC_GetValue(&hadc);
    HAL_ADC_Stop(&hadc);

    // 5) Turn divider off
    HAL_GPIO_WritePin(GPIOB, VBAT_MEAS_EN_Pin, GPIO_PIN_RESET);

    // 6) Compute node voltage in mV and undo the 1:1 divider
    //    Vnode_mV = raw_div/4095 * Vdda_mV
    Vnode_mV = ((raw_div * Vdda_mV) + (ADC_MAX/2U)) / ADC_MAX;
    VBAT_mV  = Vnode_mV * 2U;  // since divider ratio is 0.5

    return VBAT_mV;  // e.g. 3700 → 3.700 V
}

















/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  HAL_Delay(10000);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  MX_RTC_Init();
  MX_LPUART1_UART_Init();
  MX_ADC_Init();
  /* USER CODE BEGIN 2 */
  RTC_WakeUp_Init();

//  /* Scan the I2C bus and read sensors once at startup */
//  scan_i2c_bus();
//  sensor_init_and_read();
//  LoRaWAN_Join(&lora);

  HAL_GPIO_WritePin(GPIOB, VBAT_MEAS_EN_Pin, GPIO_PIN_SET);
  HAL_Delay(300);                          // let the divider & buffer settle
  uint32_t batt = ReadBattery_mV();
	HAL_GPIO_WritePin(GPIOB, VBAT_MEAS_EN_Pin, GPIO_PIN_RESET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  ConsolePrintf("Entering main loop\r\n");
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    ConsolePrintf("Going to sleep...\r\n");

    HAL_I2C_DeInit(&hi2c1);
    HAL_UART_DeInit(&huart1);
    // De-init LPUART1 (LoRaWAN UART)
    HAL_UART_DeInit(&hlpuart1);

    // Disable LPUART wake-up from Stop mode
    __HAL_UART_DISABLE_IT(&hlpuart1, UART_IT_RXNE);                    // Disable RXNE interrupt
    __HAL_UART_DISABLE_IT(&hlpuart1, UART_IT_IDLE);                    // Disable IDLE interrupt
    __HAL_UART_CLEAR_FLAG(&hlpuart1, UART_FLAG_RXNE | UART_FLAG_IDLE); // Clear any pending flags

    __HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE);                    // Disable RXNE interrupt
    __HAL_UART_DISABLE_IT(&huart1, UART_IT_IDLE);                    // Disable IDLE interrupt
    __HAL_UART_CLEAR_FLAG(&huart1, UART_FLAG_RXNE | UART_FLAG_IDLE); // Clear any pending flags

    // Enter Stop mode
    Enter_Stop_Mode(); // Wakes up via RTC interrupt

    // === Code resumes after wake-up ===
    ConsolePrintf("Resumed after wake-up\r\n");

    // Reconfigure clocks
    SystemClock_Config();
    ConsolePrintf("System clock reconfigured\r\n");

    // Reinit I2C peripheral
    MX_I2C1_Init();
    ConsolePrintf("I2C1 reinitialized\r\n");

    // Reinit UART
    MX_USART1_UART_Init();
    ConsolePrintf("UART reinitialized\r\n");

    MX_LPUART1_UART_Init();
    ConsolePrintf("LPUART1 (lora) reinitialized\r\n");

    // Reinit WakeUp timer (MUST be outside the callback!)
    RTC_WakeUp_Init();
    ConsolePrintf("RTC Wake-Up Timer reinitialized\r\n");


    HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_SET);
    HAL_Delay(300); // Short Delay to let voltage stabalize
    scan_i2c_bus();
    bool i2c_success = sensor_init_and_read();
    HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_RESET);

    if (i2c_success)
    {
		uint8_t payload[3];
		payload[0] = (uint8_t)(calculated_temp >> 8);     // high byte
		payload[1] = (uint8_t)(calculated_temp & 0xFF);   // low byte
		payload[2] = calculated_hum;
		LoRaWAN_SendHex(&lora, payload, 3);
    }
    else
    {
//    	LoRaWAN_SendHex(&lora, , 3);
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_5;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_LPUART1
                              |RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_RTC;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.OversamplingMode = DISABLE;
  hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.SamplingTime = ADC_SAMPLETIME_160CYCLES_5;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerFrequencyMode = ENABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_VREFINT;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00000608;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */
  lora.huart = &hlpuart1;
  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  ConsolePrintf("USART1 (DBG) Cnfigured\r\n");
  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, VBAT_MEAS_EN_Pin|I2C_ENABLE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : VBAT_MEAS_EN_Pin I2C_ENABLE_Pin */
  GPIO_InitStruct.Pin = VBAT_MEAS_EN_Pin|I2C_ENABLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void ConsolePrintf(const char *format, ...)
{
  char buffer[128];          // Buffer for the original message
  char timestamp_buffer[32]; // Buffer for timestamp
  char final_buffer[160];    // Combined buffer (timestamp + message)

  // Get time and date from RTC
  RTC_DateTypeDef date;
  RTC_TimeTypeDef time;
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

  // Format timestamp as [YYYY-MM-DD HH:MM:SS]
  snprintf(timestamp_buffer, sizeof(timestamp_buffer), "[20%02d-%02d-%02d %02d:%02d:%02d] ",
           date.Year, date.Month, date.Date,
           time.Hours, time.Minutes, time.Seconds);

  // Format the original message
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  // Combine timestamp and message
  snprintf(final_buffer, sizeof(final_buffer), "%s%s", timestamp_buffer, buffer);

  // Transmit the combined message
  HAL_UART_Transmit(&huart1, (uint8_t *)final_buffer, strlen(final_buffer), HAL_MAX_DELAY);
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
