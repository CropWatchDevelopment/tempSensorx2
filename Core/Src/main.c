/* USER CODE BEGIN Header */
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
#include <stdbool.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
ATC_HandleTypeDef lora;
bool joined = false;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void enter_low_power_mode(void);
void exit_low_power_mode(void);
/* USER CODE END PFP */



/********************************************************************************************
;	Function:		HARDWARE_PWR_SleepOptimisation
;	Description:	Set Unused Pins for Low Power Optimization
;	Inputs: 	 	Nothing
;	Returns:		Nothing
*********************************************************************************************/
void HARDWARE_PWR_SleepOptimisation( void )
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
	// Enable GPIOB clock
	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_InitStruct.Pin = GPIO_PIN_0  | GPIO_PIN_1 | GPIO_PIN_5  ;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5 , GPIO_PIN_RESET);


	/* Set Pins as Analog Inputs */
	GPIO_InitStruct.Pin 			= GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7  | GPIO_PIN_8  | GPIO_PIN_9  | GPIO_PIN_10  | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_15;
	GPIO_InitStruct.Mode 			= GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull 			= GPIO_NOPULL;
	GPIO_InitStruct.Speed 			= GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 			= GPIO_PIN_2 |GPIO_PIN_3 | GPIO_PIN_4  | GPIO_PIN_6 | GPIO_PIN_7  | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12  | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	GPIO_InitStruct.Mode 			= GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull 			= GPIO_NOPULL;
	GPIO_InitStruct.Speed 			= GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin 			= GPIO_PIN_0 | GPIO_PIN_1 |  GPIO_PIN_2 |GPIO_PIN_3 | GPIO_PIN_4  | GPIO_PIN_5  |  GPIO_PIN_6 | GPIO_PIN_7  | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12  | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	GPIO_InitStruct.Mode 			= GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull 			= GPIO_NOPULL;
	GPIO_InitStruct.Speed 			= GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

}

/* Add RTC wake-up interrupt handler */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
  /* Clear wake-up flag */
  __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(hrtc, RTC_FLAG_WUTF);
}


/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void enter_low_power_mode(void)
{
	HARDWARE_PWR_SleepOptimisation();

	/* Configure RTC wake-up timer for 60 seconds */
	if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 59, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK)
	{
	Error_Handler();
	}

	// Enter Stop Mode with low power regulator
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

	// When we wake up, execution continues here
	SystemClock_Config();
	exit_low_power_mode();
}

void exit_low_power_mode(void)
{
    // Re-enable peripheral clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_LPUART1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    
    // Reinitialize GPIOs (this will restore PB5 to its normal configuration)
    MX_GPIO_Init();
    MX_LPUART1_UART_Init();
    MX_DMA_Init();
}


void cb_WAKE(const char* str)
{
  __NOP();
}
void cb_OK(const char* str)
{
  __NOP();
}
void cb_JOIN(const char* str)
{
	switch (str[7]) {
		case 'O':
			joined = true;
			break;
		case 'F':
			joined = false;
			break;
		default:
			__NOP();
	}
	HAL_Delay(500);
}

const ATC_EventTypeDef events[] = {
	/* MODULE STATUS CALLBACKS */
    { "WAKE",            cb_WAKE                },
	{ "OK",              cb_OK                  },
	{ "JOIN: [",         cb_JOIN                },
	{ NULL,              NULL                   }  // CRITICAL: Add this terminator!
};
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
  // Initialize the ATC handle before using it
  ATC_Init(&lora, &hlpuart1, 512, "LoRaWAN"); // Adjust buffer size as needed
  ATC_SetEvents(&lora, events);
  HAL_Delay(10000);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  int resp = 0;
  char* ATSEND_Result = NULL;
  uint32_t last_command_time = 0;

  while(1)
  {
      ATC_Loop(&lora);
      
      // Send AT+JOIN every 10 seconds if not successful
      if (HAL_GetTick() - last_command_time > 10000 && !joined)
      {
    	  resp = ATC_SendReceive(&lora, "AT\r\n", 1000, &ATSEND_Result, 3000, 1, "OK");
          resp = ATC_SendReceive(&lora, "AT+JOIN\r\n", 1000, &ATSEND_Result, 3000, 1, "OK");
          last_command_time = HAL_GetTick();
          
          // Add breakpoint here to check resp value
          __NOP();
      }
      if (joined)
      {
    	  enter_low_power_mode();
      }
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  
  // Change to Scale 3 for lowest power:
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  
  // Also change MSI range to match Scale 3 requirements
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_0; // 65.536 kHz (matches your low power config)

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
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
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == LPUART1)
  {
    ATC_IdleLineCallback(&lora, Size);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == LPUART1)
  {
    // Handle UART errors
    __HAL_UART_CLEAR_FLAG(huart, 0xFFFFFFFF);
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
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
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
