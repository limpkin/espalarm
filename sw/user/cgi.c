/*
Some random cgi routines. Used in the LED example and the page that returns the entire
flash as a binary. Also handles the hit counter on the main page.
*/

/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */


#include <esp8266.h>
#include "user_main.h"
#include "cgi.h"
#include "pwm.h"
#include "io.h"


//cause I can't be bothered to write an ioGetLed()
static char currLedState=0;

//Cgi that turns the LED on or off according to the 'led' param in the POST data
int ICACHE_FLASH_ATTR cgiLed(HttpdConnData *connData) {
	int len;
	char buff[1024];
	
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len=httpdFindArg(connData->post->buff, "redval", buff, sizeof(buff));
	if (len>0) {
		uint32_t dutyc = atoi(buff);
		pwm_set_duty(dutyc * MAX_DUTY_C / 100, LED_R_CHANNEL);
	}

	len=httpdFindArg(connData->post->buff, "blueval", buff, sizeof(buff));
	if (len>0) {
		uint32_t dutyc = atoi(buff);
		pwm_set_duty(dutyc * MAX_DUTY_C / 100, LED_B_CHANNEL);
	}

	len=httpdFindArg(connData->post->buff, "greenval", buff, sizeof(buff));
	if (len>0) {
		uint32_t dutyc = atoi(buff);
		pwm_set_duty(dutyc * MAX_DUTY_C / 100, LED_G_CHANNEL);
		pwm_start();
	}

	httpdRedirect(connData, "led.tpl");
	return HTTPD_CGI_DONE;
}


//Alarm Cgi
int ICACHE_FLASH_ATTR cgiAlarm(HttpdConnData *connData) {
	int len;
	char buff[1024];
	bool alarmdays[7];
	uint32_t alarmtime[2];
	
	if (connData->conn==NULL) {
		//Connection aborted. Clean up.
		return HTTPD_CGI_DONE;
	}

	len=httpdFindArg(connData->post->buff, "monday", buff, sizeof(buff));
	if (len>0) 
	{
		alarmdays[1] = true;
	}
	else
	{
		alarmdays[1] = false;
	}

	len=httpdFindArg(connData->post->buff, "tuesday", buff, sizeof(buff));
	if (len>0) 
	{
		alarmdays[2] = true;
	}
	else
	{
		alarmdays[2] = false;
	}

	len=httpdFindArg(connData->post->buff, "wednesday", buff, sizeof(buff));
	if (len>0) 
	{
		alarmdays[3] = true;
	}
	else
	{
		alarmdays[3] = false;
	}

	len=httpdFindArg(connData->post->buff, "thursday", buff, sizeof(buff));
	if (len>0) 
	{
		alarmdays[4] = true;
	}
	else
	{
		alarmdays[4] = false;
	}

	len=httpdFindArg(connData->post->buff, "friday", buff, sizeof(buff));
	if (len>0) 
	{
		alarmdays[5] = true;
	}
	else
	{
		alarmdays[5] = false;
	}

	len=httpdFindArg(connData->post->buff, "saturday", buff, sizeof(buff));
	if (len>0) 
	{
		alarmdays[6] = true;
	}
	else
	{
		alarmdays[6] = false;
	}

	len=httpdFindArg(connData->post->buff, "sunday", buff, sizeof(buff));
	if (len>0) 
	{
		alarmdays[0] = true;
	}
	else
	{
		alarmdays[0] = false;
	}

	for(uint8_t i = 0; i < 7; i++)
		os_printf("%02d\r\n", alarmdays[i]);
	
	len=httpdFindArg(connData->post->buff, "hours", buff, sizeof(buff));
	if (len>0) 
	{
		alarmtime[0] = atoi(buff);
	}
	
	len=httpdFindArg(connData->post->buff, "minutes", buff, sizeof(buff));
	if (len>0) 
	{
		alarmtime[1] = atoi(buff);
	}

	store_alarm_settings(alarmdays, alarmtime);

	httpdRedirect(connData, "alarm.tpl");
	return HTTPD_CGI_DONE;
}


//Template code for the led page.
int ICACHE_FLASH_ATTR tplLed(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return HTTPD_CGI_DONE;

	os_strcpy(buff, "Unknown");
	if (os_strcmp(token, "ledstate")==0) {
		if (currLedState) {
			os_strcpy(buff, "on");
		} else {
			os_strcpy(buff, "off");
		}
		httpdSend(connData, buff, -1);
		return HTTPD_CGI_DONE;
	}

	if (os_strcmp(token, "redpwm")==0) {
		os_sprintf(buff, "%d", ((uint32_t)pwm_get_duty(LED_R_CHANNEL))*45*100/(PWM_PER*1000));
		httpdSend(connData, buff, -1);
		return HTTPD_CGI_DONE;
	}

	if (os_strcmp(token, "bluepwm")==0) {
		os_sprintf(buff, "%d", ((uint32_t)pwm_get_duty(LED_B_CHANNEL))*45*100/(PWM_PER*1000));
		httpdSend(connData, buff, -1);
		return HTTPD_CGI_DONE;
	}

	if (os_strcmp(token, "greenpwm")==0) {
		os_sprintf(buff, "%d", ((uint32_t)pwm_get_duty(LED_G_CHANNEL))*45*100/(PWM_PER*1000));
		httpdSend(connData, buff, -1);
		return HTTPD_CGI_DONE;
	}

	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}

//Template code for the alarm page.
int ICACHE_FLASH_ATTR tplAlarm(HttpdConnData *connData, char *token, void **arg) {
	bool* alarmdays;
	uint32_t* alarmtime;
	char buff[128];
	if (token==NULL) return HTTPD_CGI_DONE;

	os_strcpy(buff, "Unknown");
	alarmdays = get_alarmdays();
	alarmtime = get_alarmtime();

	if (os_strcmp(token, "monday") == 0)
	{
		if (alarmdays[1]) {
			os_strcpy(buff, "checked");
		} else {
			os_strcpy(buff, "");
		}
		httpdSend(connData, buff, -1);
		return HTTPD_CGI_DONE;
	}

	if (os_strcmp(token, "tuesday") == 0)
	{
		if (alarmdays[2]) {
			os_strcpy(buff, "checked");
		} else {
			os_strcpy(buff, "");
		}
		httpdSend(connData, buff, -1);
		return HTTPD_CGI_DONE;
	}

	if (os_strcmp(token, "wednesday") == 0)
	{
		if (alarmdays[3]) {
			os_strcpy(buff, "checked");
		} else {
			os_strcpy(buff, "");
		}
		httpdSend(connData, buff, -1);
		return HTTPD_CGI_DONE;
	}

	if (os_strcmp(token, "thursday") == 0)
	{
		if (alarmdays[4]) {
			os_strcpy(buff, "checked");
		} else {
			os_strcpy(buff, "");
		}
		httpdSend(connData, buff, -1);
		return HTTPD_CGI_DONE;
	}

	if (os_strcmp(token, "friday") == 0)
	{
		if (alarmdays[5]) {
			os_strcpy(buff, "checked");
		} else {
			os_strcpy(buff, "");
		}
		httpdSend(connData, buff, -1);
		return HTTPD_CGI_DONE;
	}

	if (os_strcmp(token, "saturday") == 0)
	{
		if (alarmdays[6]) {
			os_strcpy(buff, "checked");
		} else {
			os_strcpy(buff, "");
		}
		httpdSend(connData, buff, -1);
		return HTTPD_CGI_DONE;
	}

	if (os_strcmp(token, "sunday") == 0)
	{
		if (alarmdays[0]) {
			os_strcpy(buff, "checked");
		} else {
			os_strcpy(buff, "");
		}
		httpdSend(connData, buff, -1);
		return HTTPD_CGI_DONE;
	}

	if (os_strcmp(token, "hours")==0) {
		os_sprintf(buff, "%d", alarmtime[0]);
		httpdSend(connData, buff, -1);
		return HTTPD_CGI_DONE;
	}

	if (os_strcmp(token, "minutes")==0) {
		os_sprintf(buff, "%d", alarmtime[1]);
		httpdSend(connData, buff, -1);
		return HTTPD_CGI_DONE;
	}

	if (os_strcmp(token, "curtime")==0) {
		httpdSend(connData, main_get_datetime(), -1);
		return HTTPD_CGI_DONE;
	}

	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}

static long hitCounter=0;

//Template code for the counter on the index page.
int ICACHE_FLASH_ATTR tplCounter(HttpdConnData *connData, char *token, void **arg) {
	char buff[128];
	if (token==NULL) return HTTPD_CGI_DONE;

	if (os_strcmp(token, "counter")==0) {
		hitCounter++;
		os_sprintf(buff, "%ld", hitCounter);
		httpdSend(connData, buff, -1);
		return HTTPD_CGI_DONE;
	}

	if (os_strcmp(token, "curtime")==0) {
		httpdSend(connData, main_get_datetime(), -1);
		return HTTPD_CGI_DONE;
	}

	httpdSend(connData, buff, -1);
	return HTTPD_CGI_DONE;
}
