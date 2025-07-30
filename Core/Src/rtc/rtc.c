/*
 * rtc.c
 *
 *  Created on: Jul 29, 2025
 *      Author: kevin
 *      Description: RTC calibration and timing functions for LSI oscillator
 */

#include "rtc.h"
#include <stdio.h>
#include <stdlib.h>

/* External variables */
extern RTC_HandleTypeDef hrtc;

/* Private variables */
static int32_t current_calibration_ppm = 0;
static uint32_t measured_lsi_freq = RTC_LSI_NOMINAL_FREQ;
static char calibration_status[64] = "Not calibrated";

/* Function to print debug messages - assumes ConsolePrintf is available */
extern void ConsolePrintf(const char *format, ...);

/**
 * @brief Initialize RTC wake-up timer with calibration
 */
void RTC_WakeUp_Init_Calibrated(void)
{
    ConsolePrintf("Starting RTC Wake-Up Timer with calibration...\r\n");

    // Disable the Wake-Up Timer before configuring
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
    ConsolePrintf("RTC Wake-Up Timer disabled\r\n");

    // Apply any existing calibration
    RTC_Apply_Factory_Calibration();

    // Measure LSI frequency if possible
    measured_lsi_freq = RTC_Measure_LSI_Frequency();
    ConsolePrintf("Using LSI frequency: %lu Hz\r\n", measured_lsi_freq);

    // Calculate optimal wake-up counter
    uint32_t wakeup_counter = RTC_Calculate_WakeUp_Counter(measured_lsi_freq, RTC_WAKEUP_INTERVAL_SEC);
    ConsolePrintf("Calculated wake-up counter: %lu\r\n", wakeup_counter);

    // Configure Wake-Up Timer
    if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, wakeup_counter, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK)
    {
        ConsolePrintf("ERROR: Failed to set RTC Wake-Up Timer\r\n");
        Error_Handler();
    }

    // Enable RTC Wake-Up interrupt in NVIC
    HAL_NVIC_SetPriority(RTC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_IRQn);

    ConsolePrintf("RTC Wake-Up Timer configured for %d minutes\r\n", RTC_WAKEUP_INTERVAL_SEC / 60);
    ConsolePrintf("Calibration status: %s\r\n", RTC_Get_Calibration_Status());
}

/**
 * @brief Calibrate RTC using smooth calibration for LSI inaccuracy
 */
void RTC_Calibrate_LSI(int32_t error_ppm)
{
    ConsolePrintf("Starting RTC LSI calibration with error: %ld ppm\r\n", error_ppm);
    
    if (error_ppm == 0) {
        ConsolePrintf("No calibration needed (error = 0 ppm)\r\n");
        snprintf(calibration_status, sizeof(calibration_status), "No calibration (0 ppm)");
        return;
    }

    // Calculate pulse correction
    // 512 pulses ≈ 488 ppm correction
    uint32_t pulse_correction = (uint32_t)((abs(error_ppm) * 512) / RTC_CALIB_PULSES_PER_PPM);
    
    // Limit to maximum correction
    if (pulse_correction > RTC_CALIB_MAX_PULSES) {
        pulse_correction = RTC_CALIB_MAX_PULSES;
        ConsolePrintf("WARNING: Limiting correction to maximum %d pulses\r\n", RTC_CALIB_MAX_PULSES);
    }

    uint32_t smooth_calib_plus;
    
    // Determine calibration direction
    if (error_ppm > 0) {
        // Timer is too long - LSI too slow - subtract pulses to speed up
        smooth_calib_plus = RTC_SMOOTHCALIB_PLUSPULSES_RESET;
        ConsolePrintf("LSI too slow, subtracting %lu pulses\r\n", pulse_correction);
    } else {
        // Timer is too short - LSI too fast - add pulses to slow down  
        smooth_calib_plus = RTC_SMOOTHCALIB_PLUSPULSES_SET;
        ConsolePrintf("LSI too fast, adding %lu pulses\r\n", pulse_correction);
    }

    // Apply the smooth calibration
    if (HAL_RTCEx_SetSmoothCalib(&hrtc, 
                               RTC_SMOOTHCALIB_PERIOD_32SEC,
                               smooth_calib_plus, 
                               pulse_correction) == HAL_OK) {
        current_calibration_ppm = error_ppm;
        snprintf(calibration_status, sizeof(calibration_status), 
                "%s%lu pulses (%ld ppm)", 
                (smooth_calib_plus == RTC_SMOOTHCALIB_PLUSPULSES_SET) ? "+" : "-",
                pulse_correction, error_ppm);
        ConsolePrintf("RTC smooth calibration applied successfully\r\n");
    } else {
        ConsolePrintf("ERROR: Failed to apply RTC smooth calibration\r\n");
        snprintf(calibration_status, sizeof(calibration_status), "Calibration failed");
    }
}

/**
 * @brief Measure LSI frequency using system tick (simplified approach)
 */
uint32_t RTC_Measure_LSI_Frequency(void)
{
    // This is a simplified implementation
    // In a full implementation, you would use TIM21 to measure LSI against HSI
    // For now, we'll use the nominal frequency with some variation based on temperature
    
    ConsolePrintf("Using nominal LSI frequency (advanced measurement not implemented)\r\n");
    
    // You could add temperature compensation here if you have temperature data
    // LSI typically varies by about ±1%/°C
    
    return RTC_LSI_NOMINAL_FREQ;
}

/**
 * @brief Calculate optimal wake-up counter value based on LSI frequency
 */
uint32_t RTC_Calculate_WakeUp_Counter(uint32_t lsi_freq, uint32_t interval_sec)
{
    // Calculate CK_SPRE frequency: LSI / ((AsynchPrediv + 1) * (SynchPrediv + 1))
    float ck_spre_freq = (float)lsi_freq / ((RTC_PRESCALER_ASYNC + 1) * (RTC_PRESCALER_SYNC + 1));
    
    ConsolePrintf("CK_SPRE frequency: %.3f Hz\r\n", ck_spre_freq);
    
    // Calculate wake-up counter for desired interval
    // Subtract 1 because counter counts from N down to 0
    uint32_t wakeup_counter = (uint32_t)((interval_sec * ck_spre_freq) - 1.0f);
    
    // Ensure we don't exceed 16-bit limit
    if (wakeup_counter > 0xFFFF) {
        wakeup_counter = 0xFFFF;
        ConsolePrintf("WARNING: Counter limited to 16-bit maximum\r\n");
    }
    
    return wakeup_counter;
}

/**
 * @brief Get current RTC calibration status
 */
const char* RTC_Get_Calibration_Status(void)
{
    return calibration_status;
}

/**
 * @brief Apply factory calibration values if available
 */
void RTC_Apply_Factory_Calibration(void)
{
    // CALIBRATION SETTING: Change this value based on your measurements
    // This is optimized for ultra-low power operation with MSI at 4.194 MHz
    // Examples:
    //   0     = No calibration (default)
    //   +5000 = Timer too slow by 5000 ppm (timer takes 603 sec instead of 600)
    //   -5000 = Timer too fast by 5000 ppm (timer takes 597 sec instead of 600)
    //
    // NOTE: With MSI clock, timing accuracy is reduced compared to HSI+PLL,
    // but power consumption during sleep is dramatically lower (<1µA vs 8mA)
    int32_t factory_error_ppm = 0;  // <<<< CHANGE THIS VALUE AFTER MEASUREMENT
    
    if (factory_error_ppm != 0) {
        ConsolePrintf("Applying low-power calibration: %ld ppm\r\n", factory_error_ppm);
        ConsolePrintf("NOTE: Using MSI for ultra-low power (<1µA sleep current)\r\n");
        RTC_Calibrate_LSI(factory_error_ppm);
    } else {
        ConsolePrintf("Using default timing optimized for ultra-low power\r\n");
        ConsolePrintf("MSI clock: Better power efficiency, acceptable timing accuracy\r\n");
        ConsolePrintf("To calibrate: measure actual 10-min sleep time and update factory_error_ppm\r\n");
    }
}

/**
 * @brief Test RTC timing accuracy over a short period
 */
int32_t RTC_Test_Timing_Accuracy(uint32_t test_duration_sec)
{
    ConsolePrintf("Starting %lu second timing accuracy test...\r\n", test_duration_sec);
    
    // This would require a precise external reference
    // For now, return 0 indicating no measurable error
    // In practice, you would:
    // 1. Start a test timer
    // 2. Compare against system tick or external reference
    // 3. Calculate the error in ppm
    
    ConsolePrintf("Timing test not implemented - requires external reference\r\n");
    return 0;
}

/**
 * @brief Manual calibration helper function
 * Call this function with your measured timing error
 */
void RTC_Manual_Calibration(uint32_t expected_sec, uint32_t actual_sec)
{
    ConsolePrintf("Manual calibration: Expected %lu sec, Actual %lu sec\r\n", 
                 expected_sec, actual_sec);
    
    if (expected_sec == actual_sec) {
        ConsolePrintf("No calibration needed - timing is accurate\r\n");
        return;
    }
    
    // Calculate error in ppm
    int32_t error_ppm = ((int32_t)actual_sec - (int32_t)expected_sec) * 1000000L / (int32_t)expected_sec;
    
    ConsolePrintf("Calculated timing error: %ld ppm\r\n", error_ppm);
    
    // Apply calibration
    RTC_Calibrate_LSI(error_ppm);
}

/**
 * @brief Get recommended calibration values for common scenarios
 */
void RTC_Print_Calibration_Guide(void)
{
    ConsolePrintf("\r\n=== RTC Calibration Guide ===\r\n");
    ConsolePrintf("1. Flash code with RTC_Calibrate_LSI(0) - no calibration\r\n");
    ConsolePrintf("2. Time your 10-minute sleep cycles with a stopwatch\r\n");
    ConsolePrintf("3. Calculate error: error_ppm = ((actual_sec - 600) / 600) * 1000000\r\n");
    ConsolePrintf("4. Update code with RTC_Calibrate_LSI(error_ppm) and reflash\r\n");
    ConsolePrintf("\r\nCommon corrections:\r\n");
    ConsolePrintf("- If timer takes 603 sec: error = +5000 ppm (too slow)\r\n");
    ConsolePrintf("- If timer takes 597 sec: error = -5000 ppm (too fast)\r\n");
    ConsolePrintf("- If timer takes 610 sec: error = +16667 ppm (very slow)\r\n");
    ConsolePrintf("- If timer takes 590 sec: error = -16667 ppm (very fast)\r\n");
    ConsolePrintf("============================\r\n\r\n");
}

/**
 * @brief Apply calibration with a specific error value
 * @param error_ppm: Error in parts per million to correct
 * @retval None
 */
void RTC_Apply_Calibration_Value(int32_t error_ppm)
{
    if (error_ppm == 0) {
        ConsolePrintf("Applying no calibration (0 ppm error)\r\n");
    } else {
        ConsolePrintf("Applying calibration correction: %ld ppm\r\n", error_ppm);
    }
    
    RTC_Calibrate_LSI(error_ppm);
}
