/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ****************************************************************************	  case LORAWAN_DATA_SENDING:
			// Start timer when entering this state
			if (data_sending_start_time == 0) {
				data_sending_start_time = HAL_GetTick();
			}

			// Check if 10 seconds have passed
			if ((HAL_GetTick() - data_sending_start_time) >= 10000) {
				data_sending_start_time = 0; // Reset timer
				lorawan_state = ENTER_SLEEP_MODE;  // Go to sleep mode
			}
		break;ntion
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
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "rtc.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "atc.h"
#include "stdbool.h"
#include <stdint.h>
#include <stdio.h>     // for printf, atoi
#include <string.h>    // for strchr
#include <stdlib.h>    // for atoi
/*LoRaWAN Config stuff*/
#include "lorawan/lorawan_config.h"
/*Sensirion SHT4x*/
#include "sensirion/sensirion_common.h"
#include "sensirion/sensirion_i2c_hal.h"
#include "sensirion/sht4x_i2c.h"
#include "sensirion/sensirion.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LORAWAN_JOIN_TIMEOUT_MS ((uint32_t)10000)  // 15 seconds, I think....
#define sensirion_hal_sleep_us sensirion_i2c_hal_sleep_usec
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint32_t join_start_time = 0;
ATC_HandleTypeDef lora;
bool AWAKE = false;
int resp = 0;
char respBuf[64];
bool updated = false;
int rcv = 0;
int loraWAKE = 0;
int loraErr = 0;

uint32_t data_sending_start_time = 0; // Add timer variable for data sending

// LoRaWAN transmission status
typedef enum {
    TX_STATUS_UNKNOWN,
    TX_STATUS_ACK,
    TX_STATUS_FAIL,
    TX_STATUS_SENT
} TX_Status_t;
volatile TX_Status_t last_tx_status = TX_STATUS_UNKNOWN;

typedef enum {
    LORAWAN_NOT_JOINED,
    LORAWAN_JOINING,
    LORAWAN_JOINED,
    LORAWAN_DATA_RECEIVED,
	LORAWAN_DATA_SENDING,
	ENTER_SLEEP_MODE,
	DEVICE_SLEEP,
	COLLECT_DATA,
//	LORAWAN_MODULE_ERROR,
} LoRaWAN_State_t;
volatile LoRaWAN_State_t lorawan_state = COLLECT_DATA;

// --- Add for STOP mode sleep management ---
volatile uint8_t sleep_cycles = 0;
volatile uint8_t sleep_wakeup_flag = 0;

void cb_WAKE(const char* str)
{
	AWAKE = true;
	// Start a timer to determine when the module will sleep again
}

void cb_JOIN_SUCCESS(const char* str)
{
	lorawan_state = LORAWAN_JOINED;
}
void cb_NOT_JOINED(const char* str)
{
	lorawan_state = LORAWAN_NOT_JOINED;
}
void cb_DATA_RESPONSE(const char* str)
{
	HAL_Delay(5000);
  // Check if this is a TX confirmation
  if (strstr(str, "TX:") != NULL) {
    lorawan_state = ENTER_SLEEP_MODE;
  }
  // You can parse downlink data here if needed
}

const ATC_EventTypeDef events[] = {

	/*JOINING CALLBACKS*/
    { "JOIN: [OK]",      cb_JOIN_SUCCESS        },
    { "JOIN: [FAIL]",    cb_NOT_JOINED          },
    { "ERROR 81",        cb_NOT_JOINED          },

	/* DATA TX/RX CALLBACKS */
	{ "TX: [",           cb_DATA_RESPONSE       }, //Datasheet Section: 5.2.10 TX
    // "RX: W:",          cb_DATA_RESPONSE       },
	{ "ADRX: DR:",       cb_DATA_RESPONSE       },

	/* MODULE STATUS CALLBACKS */
    { "WAKE",            cb_WAKE                }, //Datasheet Section: 5.2.11 WAKE
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == LPUART1)
	{
		ATC_IdleLineCallback(&lora, Size);
	}
}

/**
 * @brief RTC Wake-up Timer Event Callback
 * This function is called when the RTC wake-up timer expires
 */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    // Set flag to indicate wakeup
    sleep_wakeup_flag = 1;
    if (sleep_cycles > 0) sleep_cycles--;
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

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_LPUART1_UART_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  // Initialize ATC LoRa handle
  lora.hUart = &hlpuart1;
  lora.psEvents = (ATC_EventTypeDef*)events;
  lora.Events = sizeof(events) / sizeof(events[0]);
  lora.Size = 256;  // Buffer size
  lora.pRxBuff = malloc(lora.Size);
  lora.pReadBuff = malloc(lora.Size);
  lora.RxIndex = 0;
  lora.RespCount = 0;
  
  // Initialize response pointers to NULL
  for(int i = 0; i < 16; i++) {  // ATC_RESP_MAX is typically 16
    lora.ppResp[i] = NULL;
  }

	//CONFIGURE LORAWAN STUFF:
	lorawan_configure(
			&lora,
			"0025CA00000055EE",
			"0025CA00000055EE",
			"2B7E151628AED2A6ABF7158809CF4F3C"
	);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  ATC_Loop(&lora);
	  switch (lorawan_state) {
	  case LORAWAN_NOT_JOINED:
		{
			LoRaWAN_Error_t join_result = join_network(&lora);
			if (join_result == LORAWAN_OK) {
				lorawan_state = LORAWAN_JOINING;
			} else {
				// Could implement retry logic here
			}
		}
		break;
	  case LORAWAN_JOINING:
		// Wait for join callback to change state
		break;
	  case LORAWAN_JOINED:
		  // Ready to send data
		  last_tx_status = TX_STATUS_UNKNOWN; // Reset status before sending

		  char* CONNECTION_STATUS = NULL;
		  resp = ATC_SendReceive(&lora, "ATI 3001\r\n", 200, &CONNECTION_STATUS, 2000, 2, "0\r", "1");
		  if (CONNECTION_STATUS == 0)
		  {
			  lorawan_state = LORAWAN_NOT_JOINED;
			  break;
		  }

		  // Create AT command with sensor data
		  char at_command[64];
		  uint16_t sensor_val = temp_ticks_2; // Use sensor data
		  format_at_send_cmd(sensor_val, 4, at_command);

		  char* ATSEND_Result = NULL;
		  resp = ATC_SendReceive(&lora, at_command, 200, &ATSEND_Result, 2000, 2, "OK\r", "ERROR");
		  if (resp == 1) {
			  lorawan_state = LORAWAN_DATA_SENDING;
		  } else {
			  lorawan_state = LORAWAN_NOT_JOINED;
		  }
	  break;
	  case LORAWAN_DATA_SENDING:
			// Start timer when entering this state
			if (data_sending_start_time == 0) {
				data_sending_start_time = HAL_GetTick();
			}

			// Check if 10 seconds have passed
			if ((HAL_GetTick() - data_sending_start_time) >= 10000) {
				data_sending_start_time = 0; // Reset timer
				lorawan_state = ENTER_SLEEP_MODE;  // Go directly to sleep, not DEVICE_SLEEP
			}
		break;
	  case LORAWAN_DATA_RECEIVED:
		  // Handle received downlink data (this is only for actual downlink messages)
		  lorawan_state = ENTER_SLEEP_MODE;
		  break;
	  case ENTER_SLEEP_MODE:
		  ATC_SendReceive(&lora, "ATI 42\r\n", 200, &CONNECTION_STATUS, 2000, 5, "0", "1", "2", "3", "OK");
		  ATC_SendReceive(&lora, "AT+SLEEP\r\n", 200, &CONNECTION_STATUS, 2000, 1, "OK");
		  ATC_SendReceive(&lora, "AT+SLEEP\r\n", 200, &CONNECTION_STATUS, 2000, 1, "OK");
		  HAL_SuspendTick();
		  if (sleep_cycles == 0) {
			  sleep_cycles = 8; // 8 x 16s = ~128s
		  }
		  sleep_wakeup_flag = 0;
		  HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 10000, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
		  while (sleep_cycles > 0) {
			  sleep_wakeup_flag = 0;
			  HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
			  SystemClock_Config();
			  while (!sleep_wakeup_flag) { __WFI(); }
		  }
		  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
		  HAL_ResumeTick();
		  lorawan_state = DEVICE_SLEEP;
		  break;
	  case DEVICE_SLEEP:
		  lorawan_state = COLLECT_DATA;
	  break;
	  case COLLECT_DATA:
		  // Scan for sensors and read data
		  scan_i2c_bus();
		  if (has_sensor_1 || has_sensor_2) {
			  sensor_init_and_read();
		  }
		  lorawan_state = LORAWAN_JOINED;
		  break;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPUART1|RCC_PERIPHCLK_I2C1
                              |RCC_PERIPHCLK_RTC;
  PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
