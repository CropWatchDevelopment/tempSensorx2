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
ATC_HandleTypeDef dbg;
bool joined = false;

typedef enum {
	DEVICE_SLEEP,
	DEVICE_COLLECT_DATA,
	LORAWAN_JOIN,
} Device_State_t;
volatile Device_State_t device_state = LORAWAN_JOIN;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void SystemClock_Config(void);
void enter_low_power_mode(void);
void exit_low_power_mode(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
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
			device_state = DEVICE_COLLECT_DATA;
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
    { "WAKE",            cb_WAKE                }, // i will remove this too
	{ "OK",              cb_OK                  }, // I will remove this later as it is not imporant to know this
	{ "JOIN: [",         cb_JOIN                },
	{ NULL,              NULL                   }  // CRITICAL: Add this terminator!
};

void Get_RTC_Timestamp(char *buffer, uint16_t buffer_size) {
    RTC_DateTypeDef sDate = {0};
    RTC_TimeTypeDef sTime = {0};

    // Get the current time and date
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    // Format the timestamp as [YYYY-MM-DD HH:MM:SS]
    snprintf(buffer, buffer_size, "[20%02d-%02d-%02d %02d:%02d:%02d] ",
             sDate.Year, sDate.Month, sDate.Date,
             sTime.Hours, sTime.Minutes, sTime.Seconds);
}

void Debug_Print(const char *message) {
    char buffer[256]; // Adjust size as needed
    char timestamp[32];

    // Get the timestamp
    Get_RTC_Timestamp(timestamp, sizeof(timestamp));

    // Combine timestamp and message
    snprintf(buffer, sizeof(buffer), "%s%s", timestamp, message);

    // Send the debug message
    ATC_SendReceive(&dbg, buffer, 1000, NULL, 3000, 1, "OK");
}

void enter_low_power_mode(void)
{
	Debug_Print("Going to sleep\r\n");

	// De-init I2C
	HAL_I2C_DeInit(&hi2c1);
	HAL_UART_DeInit(&huart1);

//	HARDWARE_PWR_SleepOptimisation(); // WHERE DID THIS GO????

	if (HAL_RTCEx_DeactivateWakeUpTimer(&hrtc) != HAL_OK)
	    {
	        Error_Handler();
	    }

	    /* Configure RTC wake‑up timer for 60 seconds */
	    if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 59, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK)
	    {
	        Error_Handler();
	    }

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
    MX_DMA_Init();                              // I did this because it is in the same order as it was generated
    MX_I2C1_Init();
    MX_LPUART1_UART_Init();
    MX_USART1_UART_Init();
    MX_RTC_Init();
    ATC_Init(&lora, &hlpuart1, 512, "LoRaWAN"); // this SHOULD make the ATC serial commands workagain
    ATC_Init(&dbg, &huart1, 512, "Debug");
    ATC_SetEvents(&lora, events);               // Setup all async events again
    device_state = DEVICE_COLLECT_DATA;

	Debug_Print("Waking UP!\r\n");

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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  // Initialize the ATC handle before using it
  ATC_Init(&lora, &hlpuart1, 512, "LoRaWAN"); // Adjust buffer size as needed
  ATC_Init(&dbg, &huart1, 512, "Debug");
  ATC_SetEvents(&lora, events);

  Debug_Print("RDY\r\n");

  HAL_Delay(10000); // This makes it easier to debug, don't remove
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  char* ATSEND_Result = NULL;
  uint32_t last_command_time = 0;

  Debug_Print("DeviceState=Sleep\r\n");
  HAL_Delay(100); // This makes it easier to debug, don't remove
  device_state = DEVICE_SLEEP;

  while(1)
  {
     // ATC_Loop(&lora);
	  Debug_Print("loop!!!\r\n");
      HAL_Delay(100); // This makes it easier to debug, don't remove
      switch (device_state)
      {
      case LORAWAN_JOIN:
//          if (HAL_GetTick() - last_command_time > 10000 && !joined)
//          {
//        	  // this always works
//        	  ATC_SendReceive(&lora, "AT\r\n", 1000, &ATSEND_Result, 3000, 1, "OK"); // This wakes the LoRa Module, it can be anything, so AT is best
//              ATC_SendReceive(&lora, "AT+JOIN\r\n", 1000, &ATSEND_Result, 3000, 1, "OK");
//              last_command_time = HAL_GetTick();
//          }
	  break;
	  case DEVICE_SLEEP:
		  enter_low_power_mode();
	  break;
	  case DEVICE_COLLECT_DATA:
//		  ATC_SendReceive(&dbg, "Sending data...\r\n", 1000, NULL, 3000, 1, "OK");
//		  // what sucks is when the device sleeps, I loose debugger session, so i can't get a break point here after sleep!
//		  ATC_SendReceive(&lora, "AT\r\n", 1000, &ATSEND_Result, 3000, 1, "OK");
//		  ATC_SendReceive(&lora, "AT+SEND \"AA\"\r\n", 1000, &ATSEND_Result, 3000, 1, "OK");
//		  device_state = DEVICE_SLEEP;
	  break;
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

/* USER CODE BEGIN 4 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  // RTC Alarm callback - system will wake up from STOP mode
  // No code needed here, just waking up is enough
  __NOP();

}

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
