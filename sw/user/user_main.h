//////////////////////////////////////////////////
// Simple NTP client for ESP8266.
// Copyright 2015 Richard A Burton
// richardaburton@gmail.com
// See license.txt for license terms.
//////////////////////////////////////////////////

#ifndef __USERMAIN_H__
#define __USERMAIN_H__

// PWM output pins
#define PWM_CHANNEL	3
#define PWM_0_OUT_IO_MUX PERIPHS_IO_MUX_MTDI_U
#define PWM_0_OUT_IO_NUM 12
#define PWM_0_OUT_IO_FUNC  FUNC_GPIO12
#define PWM_1_OUT_IO_MUX PERIPHS_IO_MUX_MTCK_U
#define PWM_1_OUT_IO_NUM 13
#define PWM_1_OUT_IO_FUNC  FUNC_GPIO13
#define PWM_2_OUT_IO_MUX PERIPHS_IO_MUX_MTMS_U
#define PWM_2_OUT_IO_NUM 14
#define PWM_2_OUT_IO_FUNC  FUNC_GPIO14

#define LED_R_CHANNEL	2		// Led channel
#define LED_G_CHANNEL	0		// Led channel
#define LED_B_CHANNEL	1		// Led channel
#define PWM_PER		5000		// Approximately 200Hz
#define MAX_DUTY_C	PWM_PER*1000/45 // Cf SDK, approx 111111

#define ALARM_ANIM_MS	33
#define ALARM_ANIM_FADE_INC	50

char* ICACHE_FLASH_ATTR main_get_datetime(void);
char* ICACHE_FLASH_ATTR main_get_time(void);
void ICACHE_FLASH_ATTR store_alarm_settings(bool* days, uint32_t* time);
bool* ICACHE_FLASH_ATTR get_alarmdays(void);
uint32_t* ICACHE_FLASH_ATTR get_alarmtime(void);

#endif
