/*
 * sleep.h
 *
 *  Created on: Jul 29, 2025
 *      Author: kevin
 */

#ifndef SRC_SLEEP_SLEEP_H_
#define SRC_SLEEP_SLEEP_H_

void enter_sleep_mode();
void Enter_Stop_Mode();
void RTC_WakeUp_Init(void);
void MX_ADC_DeInit(void);
void configure_gpio_for_low_power(void);
void restore_gpio_after_sleep(void);
void debug_power_state(void);
void post_wake_power_optimization(void);

#endif /* SRC_SLEEP_SLEEP_H_ */
