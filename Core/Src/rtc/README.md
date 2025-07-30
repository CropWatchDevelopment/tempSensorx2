# RTC Calibration System - Ultra-Low Power Optimized

This system provides accurate timing for the 10-minute sleep intervals while maintaining ultra-low power consumption (<1µA during sleep) by using MSI clock and RTC smooth calibration.

## Power vs Accuracy Trade-off

**Ultra-Low Power Mode (Current Implementation):**
- Sleep Current: <1µA (microamps) 
- System Clock: MSI 4.194 MHz
- Timing Accuracy: Moderate (calibrated with RTC smooth calibration)
- Battery Life: Maximum

**High Accuracy Mode (Not Recommended for Battery Operation):**
- Sleep Current: ~8mA (milliamps) - 8000x higher!
- System Clock: HSI+PLL 32 MHz  
- Timing Accuracy: Excellent
- Battery Life: 1/8000th of ultra-low power mode

## Quick Start

1. **Initial Setup**: System configured for ultra-low power with acceptable timing accuracy
2. **Measure Your Device**: Time several 10-minute sleep cycles with a stopwatch
3. **Calculate Error**: Use the formula: `error_ppm = ((actual_seconds - 600) / 600) * 1,000,000`
4. **Apply Calibration**: Update `factory_error_ppm` in `rtc.c`

## Example Usage

```c
// In your initialization code:
RTC_Print_Calibration_Guide();        // Shows calibration instructions
RTC_WakeUp_Init_Calibrated();         // Initialize with current calibration

// To apply a specific calibration (after measurement):
RTC_Apply_Calibration_Value(-5000);   // If timer is 5000 ppm too fast
```

## Common Calibration Values

| Measured Time | Error (ppm) | Calibration Code |
|---------------|-------------|------------------|
| 603 seconds   | +5000       | `RTC_Apply_Calibration_Value(5000)` |
| 597 seconds   | -5000       | `RTC_Apply_Calibration_Value(-5000)` |
| 610 seconds   | +16667      | `RTC_Apply_Calibration_Value(16667)` |
| 590 seconds   | -16667      | `RTC_Apply_Calibration_Value(-16667)` |

## Calibration Process

1. **Flash with no calibration**:
   ```c
   RTC_Apply_Calibration_Value(0);  // No calibration
   ```

2. **Measure timing**: Use a stopwatch to time multiple 10-minute cycles

3. **Calculate error**: 
   - If timer takes 603 seconds: `error = ((603-600)/600) * 1000000 = +5000 ppm`
   - If timer takes 597 seconds: `error = ((597-600)/600) * 1000000 = -5000 ppm`

4. **Apply correction**: Update code and reflash:
   ```c
   RTC_Apply_Calibration_Value(5000);  // Your calculated error
   ```

5. **Verify**: Test again to confirm improved accuracy

## Advanced Features

- **Ultra-Low Power Sleep**: Extreme power optimizations for <1µA sleep current
- **LSI Frequency Measurement**: Automatic LSI frequency detection (placeholder for future implementation)  
- **Temperature Compensation**: Framework for temperature-based calibration adjustments
- **Factory Calibration**: Store calibration values for production devices
- **GPIO Power Management**: All unused pins configured as analog inputs for minimum leakage

## Power Consumption Targets

| Mode | Current | Description |
|------|---------|-------------|
| Sleep (Ultra-Low Power) | <1µA | All peripherals off, MSI stopped, RTC only |
| Active (MSI 4.194 MHz) | ~1-2mA | During sensor reading and LoRaWAN transmission |
| Active (HSI+PLL 32 MHz) | ~8mA | NOT RECOMMENDED for battery operation |

## Troubleshooting High Power Consumption

If your device consumes >10µA during sleep:

1. **Check VREFINT**: Ensure `HAL_ADCEx_DisableVREFINT()` is called
2. **Check Temperature Sensor**: Ensure `HAL_ADCEx_DisableVREFINTTempSensor()` is called  
3. **Check GPIO Configuration**: All unused pins should be analog inputs with no pull
4. **Check System Clock**: Should be MSI, not HSI+PLL during sleep
5. **Check Peripheral Clocks**: All unused clocks should be disabled

## Technical Details

- **Calibration Range**: ±511 pulses = ±488 ppm maximum correction
- **Resolution**: ~0.95 ppm per pulse (488 ppm / 512 pulses)
- **Update Period**: Calibration applied every 32 seconds
- **LSI Nominal**: 37 kHz (actual range: 32-44 kHz)

## Functions Available

- `RTC_WakeUp_Init_Calibrated()` - Initialize with calibration
- `RTC_Print_Calibration_Guide()` - Show calibration instructions
- `RTC_Apply_Calibration_Value(ppm)` - Apply specific calibration
- `RTC_Get_Calibration_Status()` - Get current calibration info

## Integration Notes

The RTC calibration system automatically:
- Calculates optimal wake-up counter values
- Applies smooth calibration corrections
- Provides debug output for monitoring
- Handles re-initialization after sleep/wake cycles

Simply replace your old `RTC_WakeUp_Init()` calls with `RTC_WakeUp_Init_Calibrated()` to use the enhanced timing system.
