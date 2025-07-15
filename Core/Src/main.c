/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main prog  ATC_Init(&lora, &hlpuart1, 512, "LoRaWAN");
  ATC_SetEvents(&lora, events);
  
  // Initialize I2C and scan for sensors
  printf("DEBUG: Initializing I2C sensors\n");
  scan_i2c_bus(); // Check what devices exist
  sensirion_i2c_hal_init(); // Initialize sensirion I2C layerbody
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
#include "dma.h"
#include "i2c.h"
#include "usart.h"
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
	DEVICE_SLEEP,
	COLLECT_DATA,
//	LORAWAN_MODULE_ERROR,
} LoRaWAN_State_t;
volatile LoRaWAN_State_t lorawan_state = COLLECT_DATA;

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
    printf("DEBUG: Data response received: %s\n", str);
    // You can parse downlink data here if needed
}

const ATC_EventTypeDef events[] = {

	/*JOINING CALLBACKS*/
    { "JOIN: [OK]",      cb_JOIN_SUCCESS        },
    { "JOIN: [FAIL]",    cb_NOT_JOINED          },
    { "ERROR 81",        cb_NOT_JOINED          },

	/* DATA TX/RX CALLBACKS */
//	{ "TX: [",           cb_DATA_SENT           }, //Datasheet Section: 5.2.10 TX
    { "RX: W",           cb_DATA_RESPONSE       },

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
  /* USER CODE BEGIN 2 */
  ATC_Init(&lora, &hlpuart1, 512, "LoRaWAN");
  ATC_SetEvents(&lora, events);
  scan_i2c_bus(); // Check what devices exist
  sensirion_i2c_hal_init();

  const char *dev_eui = "0025CA00000055EE";
  const char *app_eui = "0025CA00000055EE";
  const char *app_key = "2B7E151628AED2A6ABF7158809CF4F3C";
  if (lorawan_configure(&lora, dev_eui, app_eui, app_key)) {
      printf("LoRaWAN configuration successful\n");
  } else {
      printf("LoRaWAN configuration failed\n");
  }

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  printf("DEBUG: Entering main loop\n");
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  // Validate lora handle before calling ATC_Loop
	  if (lora.hUart == NULL) {
		  printf("ERROR: lora.hUart is NULL!\n");
		  Error_Handler();
	  }
	  if (lora.pRxBuff == NULL) {
		  printf("ERROR: lora.pRxBuff is NULL!\n");
		  Error_Handler();
	  }
	  
	  printf("DEBUG: About to call ATC_Loop\n");
	  ATC_Loop(&lora);
	  printf("DEBUG: ATC_Loop completed successfully\n");
	  switch (lorawan_state) {
	  case LORAWAN_NOT_JOINED:
		{
			LoRaWAN_Error_t join_result = join_network(&lora);
			if (join_result == LORAWAN_OK) {
				lorawan_state = LORAWAN_JOINING;
				printf("DEBUG: Join command sent successfully\n");
			} else {
				printf("ERROR: Join command failed with error %d\n", join_result);
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
		  resp = ATC_SendReceive(&lora, "ATI 3001\r\n", 200, CONNECTION_STATUS, 2000, 2, "\r0\n", "\r1\n");
		  if (CONNECTION_STATUS == 0)
		  {
			  lorawan_state = LORAWAN_NOT_JOINED;
			  break;
		  }

		  char* ATSEND_Result = NULL;
		  int a = 12;
		  resp = ATC_SendReceive(&lora, "AT+SEND \"AA\"\r\n", 200, &ATSEND_Result, 2000, 2, "OK");
		  if (resp == 0) {
			  lorawan_state = LORAWAN_DATA_SENDING;
			  printf("DEBUG: Send command accepted\n");
		  } else {
			  lorawan_state = LORAWAN_NOT_JOINED;
		  }
	  break;
	  case LORAWAN_DATA_SENDING:
      break;
	  case DEVICE_SLEEP:
		  // Eren can do sleep, no clue how, expecially with the atc lib.
		  HAL_Delay(10000); //simulate 10 second sleep
		  lorawan_state = COLLECT_DATA;
	  break;
	  case COLLECT_DATA:
		  printf("DEBUG: COLLECT_DATA state - starting sensor operations\n");
		  
		  // Call sensor initialization and reading
		  int sensor_result = sensor_init_and_read();
		  if (sensor_result == NO_ERROR) {
			  printf("DEBUG: Sensor data collected successfully\n");
		  } else {
			  printf("ERROR: Sensor data collection failed with error %d\n", sensor_result);
		  }
		  
		  HAL_Delay(1000); // Small delay
		  lorawan_state = LORAWAN_JOINED; // Go back to send data
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPUART1|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
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
