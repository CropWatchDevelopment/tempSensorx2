/* USER CODE BEGIN Header */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "rtc.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "atc.h"
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
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void enter_low_power_mode(void)
{
    // Configure all GPIOs to analog mode (lowest power)
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_All;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    
    // Configure ALL GPIO ports as analog
    __HAL_RCC_GPIOA_CLK_ENABLE();
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    __HAL_RCC_GPIOA_CLK_DISABLE();
    
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    __HAL_RCC_GPIOB_CLK_DISABLE();
    
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    __HAL_RCC_GPIOC_CLK_DISABLE();
    
    // Add the missing GPIO ports
    __HAL_RCC_GPIOD_CLK_ENABLE();
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    __HAL_RCC_GPIOD_CLK_DISABLE();
    
    __HAL_RCC_GPIOH_CLK_ENABLE();
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
    __HAL_RCC_GPIOH_CLK_DISABLE();
    
    // Disable peripheral clocks
    __HAL_RCC_I2C1_CLK_DISABLE();
    __HAL_RCC_LPUART1_CLK_DISABLE();
    __HAL_RCC_DMA1_CLK_DISABLE();
    
    // Disable ADC clock if enabled
    __HAL_RCC_ADC1_CLK_DISABLE();
    
    // Disable TIM clocks if any are enabled
    __HAL_RCC_TIM2_CLK_DISABLE();
    __HAL_RCC_TIM21_CLK_DISABLE();
    __HAL_RCC_TIM22_CLK_DISABLE();
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
    
    // Reinitialize GPIOs
    MX_GPIO_Init();
//    MX_I2C1_Init();
    MX_LPUART1_UART_Init();
    MX_DMA_Init();
}


void cb_WAKE(const char* str)
{
  __NOP();
}

const ATC_EventTypeDef events[] = {
	/* MODULE STATUS CALLBACKS */
    { "WAKE",            cb_WAKE                },
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
  MX_RTC_Init();
  MX_LPUART1_UART_Init();
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
  resp = ATC_SendReceive(&lora, "AT\r\n", 200, &ATSEND_Result, 2000, 2, "OK", "ERROR");
  while(1)
    {
	  ATC_Loop(&lora);
//  	  /* Enter low power configuration */
//  	  enter_low_power_mode();
//
//  	  /* Suspend the SysTick interrupt */
//  	      HAL_SuspendTick();
//
//  	      /* Clear any previous wake-up flag */
//  	      __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
//
//  	      /* Enter STOP mode with low power regulator */
//  	      HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
//
//  	      /* MCU is now sleeping here until the RTC wakeup fires */
//
//  	      /* Resume SysTick */
//  	      HAL_ResumeTick();
//
//  	      /* Reconfigure the clocks */
//  	      SystemClock_Config();
//
//  	      /* Exit low power mode and restore peripherals */
//  	      exit_low_power_mode();
//
//  	      /* --- at this point you can do whatever you need on wakeup --- */
//  	      /* e.g. toggle an LED to prove we woke up: */
//  	      HAL_GPIO_TogglePin(I2C_ENABLE_GPIO_Port, I2C_ENABLE_Pin);
//  	      HAL_Delay(5000);
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPUART1|RCC_PERIPHCLK_RTC;
  PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
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
