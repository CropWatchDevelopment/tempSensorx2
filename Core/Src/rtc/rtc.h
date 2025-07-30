/*
 * rtc.h
 *
 *  Created on: Jul 29, 2025
 *      Author: kevin
 *      Description: RTC calibration and timing functions for LSI oscillator
 */

#ifndef RTC_RTC_H_
#define RTC_RTC_H_

#include "main.h"
#include "stm32l0xx_hal.h"
#include "stm32l0xx_hal_rtc.h"

/* RTC Calibration Constants */
#define RTC_LSI_NOMINAL_FREQ        37000   // Nominal LSI frequency in Hz
#define RTC_CALIB_PULSES_PER_PPM    488     // 488 ppm correction per 512 pulses
#define RTC_CALIB_MAX_PULSES        511     // Maximum calibration pulses
#define RTC_CALIB_PERIOD_SEC        32      // Calibration period in seconds

/* Wake-up Timer Constants */
#define RTC_WAKEUP_INTERVAL_SEC     600     // 10 minutes in seconds
#define RTC_PRESCALER_ASYNC         127     // RTC AsynchPrediv
#define RTC_PRESCALER_SYNC          255     // RTC SynchPrediv

/* Function Prototypes */

/**
 * @brief Initialize RTC wake-up timer with calibration
 * @param None
 * @retval None
 */
void RTC_WakeUp_Init_Calibrated(void);

/**
 * @brief Calibrate RTC using smooth calibration for LSI inaccuracy
 * @param error_ppm: Measured timing error in parts per million
 *                  Positive = timer too slow (LSI too slow)
 *                  Negative = timer too fast (LSI too fast)
 * @retval None
 */
void RTC_Calibrate_LSI(int32_t error_ppm);

/**
 * @brief Measure LSI frequency using TIM21 (if available)
 * @param None
 * @retval Measured LSI frequency in Hz
 */
uint32_t RTC_Measure_LSI_Frequency(void);

/**
 * @brief Calculate optimal wake-up counter value based on LSI frequency
 * @param lsi_freq: LSI frequency in Hz
 * @param interval_sec: Desired wake-up interval in seconds
 * @retval Wake-up counter value
 */
uint32_t RTC_Calculate_WakeUp_Counter(uint32_t lsi_freq, uint32_t interval_sec);

/**
 * @brief Get current RTC calibration status
 * @param None
 * @retval Calibration status string for debugging
 */
const char* RTC_Get_Calibration_Status(void);

/**
 * @brief Apply factory calibration values if available
 * @param None
 * @retval None
 */
void RTC_Apply_Factory_Calibration(void);

/**
 * @brief Test RTC timing accuracy over a short period
 * @param test_duration_sec: Test duration in seconds
 * @retval Measured error in ppm
 */
int32_t RTC_Test_Timing_Accuracy(uint32_t test_duration_sec);

/**
 * @brief Get recommended calibration values for common scenarios
 * @param None
 * @retval None
 */
void RTC_Print_Calibration_Guide(void);

/**
 * @brief Apply calibration with a specific error value
 * @param error_ppm: Error in parts per million to correct
 * @retval None
 */
void RTC_Apply_Calibration_Value(int32_t error_ppm);

#endif /* RTC_RTC_H_ */
