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
int resp = 0;
char respBuf[64];
bool updated = false;
int rcv = 0;
int loraWAKE = 0;
int loraErr = 0;

// i2c variables
int16_t error = NO_ERROR;
bool has_sensor_1 = false;
uint16_t temp_ticks_1;
uint16_t hum_ticks_1;

bool has_sensor_2 = false;
uint16_t temp_ticks_2;
uint16_t hum_ticks_2;

typedef enum {
    LORAWAN_NOT_JOINED,
    LORAWAN_JOINING,
    LORAWAN_JOINED,
    LORAWAN_DATA_RECEIVED,
	LORAWAN_DATA_RESPONSE,
	LORAWAN_DATA_SENDING,
	DEVICE_SLEEP,
//	LORAWAN_MODULE_ERROR,
} LoRaWAN_State_t;
volatile LoRaWAN_State_t lorawan_state = LORAWAN_NOT_JOINED;

void cbWAKE(const char* str)
{
	loraWAKE = 1;
}
void cb_NOT_JOINED(const char* str)
{
	lorawan_state = LORAWAN_NOT_JOINED;
}
void cb_JOIN_SUCCESS(const char* str)
{
	lorawan_state = LORAWAN_JOINED;
}
//void cbError(const char* str)
//{
//	lorawan_state = LORAWAN_MODULE_ERROR;
//}
void cb_DATA_SENT(const char* str)
{
	lorawan_state = LORAWAN_DATA_RECEIVED;
}
void cb_DATA_RESPONSE(const char* str)
{
	lorawan_state = LORAWAN_DATA_RESPONSE;
}
void cbDebug(const char* str) {
    // print each byte in hex so you can see those hidden prefix chars
    for (const char*p=str; *p; p++) printf("%02X ", (unsigned char)*p);
    printf("\n");
}
const ATC_EventTypeDef events[] = {
	// { "",       cbDebug    },  // match everything
    { "JOIN:",        cb_JOIN_SUCCESS   },  // catches “JOIN: [OK], …”
//  	{ "\nCONNECT \n", cb_JOIN_SUCCESS   },
    { "ERROR 81",     cb_NOT_JOINED     },  // join-fail code
	{ "TX: [",        cb_DATA_SENT      },
    { "[RX]:",        cb_DATA_RESPONSE  },  // downlink received
    { "WAKE",         cbWAKE            },  // UART wake message
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
//  scan_i2c_bus(); // Check what devices exist
//  sensirion_i2c_hal_init();
  ATC_HandleTypeDef lora; // Initialize your ATC handle
  const char *dev_eui = "0025CAFEFFEFFE2"; // Replace with your DevEUI
  const char *app_eui = "0025CA0000000000"; // Replace with your AppEUI
  const char *app_key = "0025CA00000000000000000000000000"; // Replace with your AppKey

  if (lorawan_configure(&lora, dev_eui, app_eui, app_key)) {
      printf("LoRaWAN configuration successful\n");
  } else {
      printf("LoRaWAN configuration failed\n");
  }

  char *verLine = NULL;
  int ret = ATC_SendReceive(&lora,
      "ATI 3\r\n",
      /*txTimeout=*/100,
      &verLine,          // will point at the match for pattern[0] or pattern[1]
      /*rxTimeout=*/200,
      /*Items=*/2,       // we’re passing two patterns
      "\n",              // pattern[0]: the newline before "127..."
      "OK"               // pattern[1]: the final OK
  );

  if (ret == 1 && verLine) {
      // ret==1 means we matched pattern[0], i.e. the newline
      // skip that newline:
      char *v = verLine + 1;
      // strip trailing CR if present
      char *cr = strchr(v, '\r');
      if (cr) *cr = '\0';
      printf("Firmware version: %s\n", v);
  }
  else if (ret == 2) {
      // we matched "OK" first (unlikely), you could still parse lora.pReadBuff
  }
  else {
      // handle timeout / error
  }
  __NOP();
  /* USER CODE END 2 */
  char *connection_result = NULL;
  ATC_SendReceive(&lora,
  						  "AT+JOIN\r\n",
  						  100,
  						  &connection_result,
  						  500,
  						  1,
  						  "OK");
  		  lorawan_state = LORAWAN_JOINING;
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  ATC_Loop(&lora);
//	  switch (lorawan_state) {
//	  case LORAWAN_NOT_JOINED:
//		{
//		  char *connection_result = NULL;
//		  join_start_time = HAL_GetTick();  // ← timestamp first
//		  ATC_SendReceive(&lora,
//						  "AT+JOIN\r\n",
//						  100,
//						  &connection_result,
//						  500,
//						  1,
//						  "OK");
//		  lorawan_state = LORAWAN_JOINING;
//		}
//		break;
//		  case LORAWAN_JOINING:
//			  if ((HAL_GetTick() - join_start_time) >= LORAWAN_JOIN_TIMEOUT_MS)
//			          {
//			            lorawan_state = LORAWAN_NOT_JOINED; // retry
//			          }
//			  break;
//		  case LORAWAN_JOINED:
//			  // Ready to send data
//			  resp = ATC_SendReceive(&lora, "AT+SEND \"AA\"\r\n", 200, NULL, 2000, 2, "OK");
//			  lorawan_state = LORAWAN_DATA_SENDING;
//			  break;
//		  case LORAWAN_DATA_RESPONSE:
//			  // Do something with the response data...
//			  lorawan_state = DEVICE_SLEEP;
//			  break;
//		  case LORAWAN_DATA_SENDING:
//			  ATC_Loop(&lora);
//			  break;
//		  case LORAWAN_DATA_RECEIVED:
//			  // Data successfully sent/received, go back to joined or repeat
//			  HAL_Delay(10000);
//			  lorawan_state = LORAWAN_JOINED;
//			  break;
//		  case DEVICE_SLEEP:
//
//			  break;
//		  case LORAWAN_MODULE_ERROR:
//			  char *connection_status = NULL;
//			  resp = ATC_SendReceive(&lora, "ATI 3001\r\n", 100, &connection_status, 200, 2, "\r\nOK");
//			  if (resp > 0 && connection_status)
//			  {
//				  int status = atoi(connection_status);
//				  if (status == 0) lorawan_state = LORAWAN_NOT_JOINED; // Retry join
//				  if (status == 1) lorawan_state = LORAWAN_JOINED;
//			  }
//			  break;
//	  }
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


//void scan_i2c_bus(void)
//{
//	HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_SET);
//    uint8_t addr;
//    HAL_Delay(100); // let bus settle
//
//    for (addr = 3; addr < 0x78; addr++)
//    {
//        // HAL expects 8-bit address = 7-bit << 1
//        if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK)
//        {
//        	if (addr == 68) has_sensor_1 = true;
//        	if (addr == 70) has_sensor_2 = true;
//        }
//    }
//    HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_RESET);
//}

//int sensor_init_and_read(void)
//{
//	if (!has_sensor_1 && !has_sensor_2) return -1;
//	HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_SET);
//	error = NO_ERROR;
//	HAL_Delay(100); // Let power stabalize
//
//	// --- Read From Sensor A ---
//	if (has_sensor_1)
//	{
//		sht4x_init(SHT43_I2C_ADDR_44);
//		sht4x_soft_reset();
//		sensirion_i2c_hal_sleep_usec(10000);
//		sht4x_init(SHT43_I2C_ADDR_44);
//		error = sht4x_measure_high_precision_ticks(&temp_ticks_1, &hum_ticks_1);
//	}
//
//	// --- Read From Sensor B ---
//	if (has_sensor_2)
//	{
//		sht4x_init(SHT40_I2C_ADDR_44);
//		sht4x_soft_reset();
//		sensirion_i2c_hal_sleep_usec(10000);
//		sht4x_init(SHT40_I2C_ADDR_44);
//		error = sht4x_measure_high_precision_ticks(&temp_ticks_2, &hum_ticks_2);
//	}
//
//
//	HAL_GPIO_WritePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin, GPIO_PIN_RESET);
//
//	return error;
//}

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
