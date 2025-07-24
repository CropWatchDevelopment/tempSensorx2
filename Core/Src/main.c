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
#include "lorawan/lorawan_serial.h"
#include "atc.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
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
static uint8_t loraRxBuf[128];
LoRaSerial_HandleTypeDef hLora;
ATC_HandleTypeDef lora;  // ATC handle for compatibility
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

// LoRaSerial callback for handling received lines
void LoraLineHandler(const char *line)
{
    // Handle JOIN responses
    if (strstr(line, "JOIN: ") != NULL) {
        if (strstr(line, "JOIN: OK") != NULL) {
            joined = true;
            device_state = DEVICE_COLLECT_DATA;
        } else if (strstr(line, "JOIN: FAILED") != NULL) {
            joined = false;
        }
    }
    // Add other AT command response handling as needed
}

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

    // Send the debug message using HAL directly
    HAL_UART_Transmit(&huart1, (uint8_t*)buffer, strlen(buffer), 1000);
}

void enter_low_power_mode(void)
{
	Debug_Print("Going to sleep\r\n");

	// Wait for UART transmission to complete before entering low power
	while (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_TC) == RESET);
	while (__HAL_UART_GET_FLAG(&hlpuart1, UART_FLAG_TC) == RESET);
	
	// Wait for LPUART to be idle and ready
	while (__HAL_UART_GET_FLAG(&hlpuart1, UART_FLAG_BUSY) == SET);
	while (__HAL_UART_GET_FLAG(&hlpuart1, UART_FLAG_REACK) == RESET);

	// Prepare LoRa serial for low power mode
	LoRaSerial_EnterLowPower(&hLora);

	// De-init I2C and debug UART to avoid spurious interrupts
	HAL_I2C_DeInit(&hi2c1);
	HAL_UART_DeInit(&huart1);

	// Disable EXTI interrupts to prevent GPIO-related wake-ups
	HAL_NVIC_DisableIRQ(EXTI0_1_IRQn);
	HAL_NVIC_DisableIRQ(EXTI2_3_IRQn);
	HAL_NVIC_DisableIRQ(EXTI4_15_IRQn);

	// Disable DMA interrupts that could cause wake-ups
	HAL_NVIC_DisableIRQ(DMA1_Channel2_3_IRQn);
	HAL_NVIC_DisableIRQ(DMA1_Channel4_5_6_7_IRQn);

	// Disable UART interrupts except for LPUART1 wake-up
	HAL_NVIC_DisableIRQ(USART1_IRQn);

	// Configure RTC wake-up timer first, then deactivate any existing timer
	if (HAL_RTCEx_DeactivateWakeUpTimer(&hrtc) != HAL_OK)
	{
		Error_Handler();
	}

	/* Configure RTC wake-up timer for 60 seconds */
	if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 59, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK)
	{
		Error_Handler();
	}

	// Clear all pending interrupts to prevent spurious wake-ups
	__HAL_RCC_CLEAR_RESET_FLAGS();
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);  // Clear wake-up flag
	NVIC_ClearPendingIRQ(RNG_LPUART1_IRQn);
	NVIC_ClearPendingIRQ(USART1_IRQn);
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_ClearPendingIRQ(DMA1_Channel2_3_IRQn);
	NVIC_ClearPendingIRQ(DMA1_Channel4_5_6_7_IRQn);
	__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);

	// Clear any UART error flags
	__HAL_UART_CLEAR_FLAG(&hlpuart1, UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF);

	// Disable unused peripheral clocks to reduce power consumption
	__HAL_RCC_I2C1_CLK_DISABLE();
	__HAL_RCC_USART1_CLK_DISABLE();
	__HAL_RCC_DMA1_CLK_DISABLE();

	// Disable debug in Stop mode to save power
	HAL_DBGMCU_DisableDBGStopMode();

	// Suspend SysTick to prevent it from waking the MCU
	HAL_SuspendTick();

	// Enter Stop Mode with low power regulator
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

	// When we wake up, execution continues here
	HAL_ResumeTick();
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
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    
    // Reinitialize GPIOs and peripherals
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_I2C1_Init();
    MX_LPUART1_UART_Init();
    MX_USART1_UART_Init();
    MX_RTC_Init();
    
    // Re-enable interrupts that were disabled
    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
    HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
    HAL_NVIC_EnableIRQ(DMA1_Channel4_5_6_7_IRQn);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    
    // Exit low power mode for LoRa serial
    LoRaSerial_ExitLowPower(&hLora);
    
    // Reinitialize ATC handle for compatibility
    ATC_Init(&lora, &hlpuart1, 512, "LoRaWAN");
    
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
  // Initialize the LoRaSerial driver
  LoRaSerial_Init(&hLora, &hlpuart1, loraRxBuf, sizeof(loraRxBuf), LoraLineHandler);
  
  // Initialize the ATC handle for compatibility with existing code
  ATC_Init(&lora, &hlpuart1, 512, "LoRaWAN");

  Debug_Print("RDY\r\n");

  HAL_Delay(10000); // This makes it easier to debug, don't remove
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t last_command_time = 0;

  Debug_Print("DeviceState=Sleep\r\n");
  HAL_Delay(100); // This makes it easier to debug, don't remove
  device_state = DEVICE_SLEEP;

  while(1)
  {
      // Use LoRaSerial driver for sending commands instead of ATC
      // ATC_Loop(&lora); // Commented out - we're using LoRaSerial now
	  Debug_Print("loop!!!\r\n");
      HAL_Delay(100); // This makes it easier to debug, don't remove
      switch (device_state)
      {
      case LORAWAN_JOIN:
          if (HAL_GetTick() - last_command_time > 10000 && !joined)
          {
        	  // Send JOIN command using LoRaSerial instead of ATC
              Debug_Print("Sending JOIN command\r\n");
        	  LoRaSerial_Send(&hLora, "AT\r\n"); // Wake the LoRa Module
              HAL_Delay(100);
              LoRaSerial_Send(&hLora, "AT+JOIN\r\n");
              last_command_time = HAL_GetTick();
          }
	  break;
	  case DEVICE_SLEEP:
		  enter_low_power_mode();
	  break;
	  case DEVICE_COLLECT_DATA:
		  // Send data using LoRaSerial instead of ATC
		  Debug_Print("Sending data...\r\n");
		  LoRaSerial_Send(&hLora, "AT\r\n");
          HAL_Delay(100);
		  LoRaSerial_Send(&hLora, "AT+SEND \"AA\"\r\n");
		  HAL_Delay(100);  // Wait for command to be processed
		  device_state = DEVICE_SLEEP;
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
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
  PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_HSI;  // Use HSI for wake-up capability
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  
  // Ensure HSI remains active during STOP mode for LPUART wake-up
  __HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_STOP_WAKEUPCLOCK_HSI);
}

/* USER CODE BEGIN 4 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
  // RTC Alarm callback - system will wake up from STOP mode
  // No code needed here, just waking up is enough
  __NOP();
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
  // RTC Wake-up Timer callback - system will wake up from STOP mode after 60 seconds
  // This is the intended wake-up source
  __NOP();
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
