/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain 
 * this notice you can do whatever you want with this stuff. If we meet some day, 
 * and you think this stuff is worth it, you can buy me a beer in return. 
 * ----------------------------------------------------------------------------
 */

/*
This is example code for the esphttpd library. It's a small-ish demo showing off 
the server, including WiFi connection management capabilities, some IO and
some pictures of cats.
*/

#include <esp8266.h>
#include "httpd.h"
#include "io.h"
#include "httpdespfs.h"
#include "cgi.h"
#include "cgiwifi.h"
#include "cgiflash.h"
#include "stdout.h"
#include "auth.h"
#include "espfs.h"
#include "captdns.h"
#include "webpages-espfs.h"
#include "cgiwebsocket.h"
#include <time.h>
#include "pwm.h"
#include "sntp.h"
#include "gpio16.h"
#include "user_main.h"

static struct tm* current_time_dt;
static char string_current_datetime[100];
static char string_current_time[100];
static time_t current_timestamp;
static os_timer_t timer_second;
static os_timer_t timer_alarmanim;
typedef struct tm tm_struct;
static bool alarm_discard = false;
static bool time_fetched = false;
static bool time_fetched_light = false;
static bool stored_alarmdays[7];
static uint32_t stored_alarmtime[2];
static uint32_t stored_alarmcolor[3] = {0,0,100};
static uint32_t stored_prep[3];
static bool alarm_in_progress = false;
static bool alarm_ack = false;

//The example can print out the heap use every 3 seconds. You can use this to catch memory leaks.
//#define SHOW_HEAP_USE

//Function that tells the authentication system what users/passwords live on the system.
//This is disabled in the default build; if you want to try it, enable the authBasic line in
//the builtInUrls below.
int ICACHE_FLASH_ATTR myPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen) {
	if (no==0) {
		os_strcpy(user, "admin");
		os_strcpy(pass, "s3cr3t");
		return 1;
//Add more users this way. Check against incrementing no for each user added.
//	} else if (no==1) {
//		os_strcpy(user, "user1");
//		os_strcpy(pass, "something");
//		return 1;
	}
	return 0;
}

static ETSTimer websockTimer;

//Broadcast the uptime in seconds every second over connected websockets
static void ICACHE_FLASH_ATTR websockTimerCb(void *arg) {
	static int ctr=0;
	char buff[128];
	ctr++;
	os_sprintf(buff, "Up for %d minutes %d seconds!\n", ctr/60, ctr%60);
	//os_printf("Up for %d minutes %d seconds!\n", ctr/60, ctr%60);
	cgiWebsockBroadcast("/websocket/ws.cgi", buff, os_strlen(buff), WEBSOCK_FLAG_NONE);
}

//On reception of a message, send "You sent: " plus whatever the other side sent
void myWebsocketRecv(Websock *ws, char *data, int len, int flags) {
	int i;
	char buff[128];
	os_sprintf(buff, "You sent: ");
	for (i=0; i<len; i++) buff[i+10]=data[i];
	buff[i+10]=0;
	cgiWebsocketSend(ws, buff, os_strlen(buff), WEBSOCK_FLAG_NONE);
}

//Websocket connected. Install reception handler and send welcome message.
void myWebsocketConnect(Websock *ws) {
	ws->recvCb=myWebsocketRecv;
	cgiWebsocketSend(ws, "Hi, Websocket!", 14, WEBSOCK_FLAG_NONE);
}


#ifdef ESPFS_POS
CgiUploadFlashDef uploadParams={
	.type=CGIFLASH_TYPE_ESPFS,
	.fw1Pos=ESPFS_POS,
	.fw2Pos=0,
	.fwSize=ESPFS_SIZE,
};
#define INCLUDE_FLASH_FNS
#endif
#ifdef OTA_FLASH_SIZE_K
CgiUploadFlashDef uploadParams={
	.type=CGIFLASH_TYPE_FW,
	.fw1Pos=0x1000,
	.fw2Pos=((OTA_FLASH_SIZE_K*1024)/2)+0x1000,
	.fwSize=((OTA_FLASH_SIZE_K*1024)/2)-0x1000,
};
#define INCLUDE_FLASH_FNS
#endif

/*
This is the main url->function dispatching data struct.
In short, it's a struct with various URLs plus their handlers. The handlers can
be 'standard' CGI functions you wrote, or 'special' CGIs requiring an argument.
They can also be auth-functions. An asterisk will match any url starting with
everything before the asterisks; "*" matches everything. The list will be
handled top-down, so make sure to put more specific rules above the more
general ones. Authorization things (like authBasic) act as a 'barrier' and
should be placed above the URLs they protect.
*/
HttpdBuiltInUrl builtInUrls[]={
	{"*", cgiRedirectApClientToHostname, "esp8266.nonet"},
	{"/", cgiRedirect, "/index.tpl"},
	{"/flash.bin", cgiReadFlash, NULL},
	{"/led.tpl", cgiEspFsTemplate, tplLed},
	{"/alarm.tpl", cgiEspFsTemplate, tplAlarm},
	{"/index.tpl", cgiEspFsTemplate, tplCounter},
	{"/led.cgi", cgiLed, NULL},
	{"/alarm.cgi", cgiAlarm, NULL},
	{"/discardalarm.cgi", discardcgiAlarm, NULL},
	{"/flash/download", cgiReadFlash, NULL},
#ifdef INCLUDE_FLASH_FNS
	{"/flash/next", cgiGetFirmwareNext, &uploadParams},
	{"/flash/upload", cgiUploadFirmware, &uploadParams},
#endif
	{"/flash/reboot", cgiRebootFirmware, NULL},

	//Routines to make the /wifi URL and everything beneath it work.

//Enable the line below to protect the WiFi configuration with an username/password combo.
//	{"/wifi/*", authBasic, myPassFn},

	{"/wifi", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/", cgiRedirect, "/wifi/wifi.tpl"},
	{"/wifi/wifiscan.cgi", cgiWiFiScan, NULL},
	{"/wifi/wifi.tpl", cgiEspFsTemplate, tplWlan},
	{"/wifi/connect.cgi", cgiWiFiConnect, NULL},
	{"/wifi/connstatus.cgi", cgiWiFiConnStatus, NULL},
	{"/wifi/setmode.cgi", cgiWiFiSetMode, NULL},

	{"/websocket/ws.cgi", cgiWebsocket, myWebsocketConnect},

	{"*", cgiEspFsHook, NULL}, //Catch-all cgi function for the filesystem
	{NULL, NULL, NULL}
};


#ifdef SHOW_HEAP_USE
static ETSTimer prHeapTimer;

static void ICACHE_FLASH_ATTR prHeapTimerCb(void *arg) {
	os_printf("Heap: %ld\n", (unsigned long)system_get_free_heap_size());
}
#endif


void ICACHE_FLASH_ATTR applyTZ(tm_struct* time, uint8_t hours) 
{
	bool dst = false;

	// apply base timezone offset
	time->tm_hour += hours; // e.g. central europe

	// call mktime to fix up (needed if applying offset has rolled the date back/forward a day)
	// also sets yday and fixes wday (if it was wrong from the rtc)
	mktime(time);

	// work out if we should apply dst, modify according to your local rules
    if (time->tm_mon < 2 || time->tm_mon > 9) {
		// these months are completely out of DST
	} else if (time->tm_mon > 2 && time->tm_mon < 9) {
		// these months are completely in DST
		dst = true;
	} else {
		// else we must be in one of the change months
		// work out when the last sunday was (could be today)
		int previousSunday = time->tm_mday - time->tm_wday;
		if (time->tm_mon == 2) { // march
			// was last sunday (which could be today) the last sunday in march
			if (previousSunday >= 25) {
				// are we actually on the last sunday today
				if (time->tm_wday == 0) {
					// if so are we at/past 2am gmt
					int s = (time->tm_hour * 3600) + (time->tm_min * 60) + time->tm_sec;
					if (s >= 7200) dst = true;
				} else {
					dst = true;
				}
			}
		} else if (time->tm_mon == 9) {
			// was last sunday (which could be today) the last sunday in october
			if (previousSunday >= 25) {
				// we have reached/passed it, so is it today?
				if (time->tm_wday == 0) {
					// change day, so are we before 1am gmt (2am localtime)
					int s = (time->tm_hour * 3600) + (time->tm_min * 60) + time->tm_sec;
					if (s < 3600) dst = true;
				}
			} else {
				// not reached the last sunday yet
				dst = true;
			}
		}
	}

	if (dst) {
		// add the dst hour
		time->tm_hour += 1;
		// mktime will fix up the time/date if adding an hour has taken us to the next day
		mktime(time);
		// don't rely on isdst returned by mktime, it doesn't know about timezones and tends to reset this to 0
		time->tm_isdst = 1;
	} else {
		time->tm_isdst = 0;
	}

}

char* ICACHE_FLASH_ATTR main_get_time(void)
{
	return string_current_time;
}

char* ICACHE_FLASH_ATTR main_get_datetime(void)
{
	return string_current_datetime;
}

void ICACHE_FLASH_ATTR store_alarm_settings(bool* days, uint32_t* time, uint32_t* color, uint32_t* prep)
{
	os_memcpy(stored_alarmdays, days, sizeof(stored_alarmdays)); 
	os_memcpy(stored_alarmtime, time, sizeof(stored_alarmtime)); 
	os_memcpy(stored_alarmcolor, color, sizeof(stored_alarmcolor)); 
	os_memcpy(stored_prep, prep, sizeof(stored_prep)); 
}

bool* ICACHE_FLASH_ATTR get_alarmdays(void)
{
	return stored_alarmdays;
}

uint32_t* ICACHE_FLASH_ATTR get_alarmtime(void)
{
	return stored_alarmtime;
}

uint32_t* ICACHE_FLASH_ATTR get_alarmcolor(void)
{
	return stored_alarmcolor;
}

uint32_t* ICACHE_FLASH_ATTR get_alarmprep(void)
{
	return stored_prep;
}

static uint32_t anim_sm;
static uint32_t red_dc;
static uint32_t blue_dc;
static uint32_t green_dc;
static uint32_t anim_counter_blink;
static uint32_t anim_blinks;

void ICACHE_FLASH_ATTR alarm_anim_tick(void)
{
	if(anim_sm == 0)
	{
		pwm_set_duty(red_dc * stored_alarmcolor[0] / 100, LED_R_CHANNEL);
		pwm_set_duty(blue_dc * stored_alarmcolor[2] / 100, LED_B_CHANNEL);
		pwm_set_duty(green_dc * stored_alarmcolor[1] / 100, LED_G_CHANNEL);
		red_dc += (red_dc / ALARM_ANIM_FADE_DIV) + ALARM_ANIM_FADE_INC;
		blue_dc += (blue_dc / ALARM_ANIM_FADE_DIV) + ALARM_ANIM_FADE_INC;
		green_dc += (green_dc / ALARM_ANIM_FADE_DIV) + ALARM_ANIM_FADE_INC;
		if((blue_dc > MAX_DUTY_C) || (red_dc > MAX_DUTY_C) || (green_dc > MAX_DUTY_C))
		{
			anim_sm++;
		}			
	}
	else if(anim_sm == 1)
	{
		if(anim_counter_blink++ == 1000/ALARM_ANIM_MS)
		{
			anim_counter_blink = 0;
			anim_blinks++;
			if(pwm_get_duty(LED_B_CHANNEL) > 0)
			{
				pwm_set_duty(0, LED_R_CHANNEL);
				pwm_set_duty(0, LED_G_CHANNEL);
				pwm_set_duty(0, LED_B_CHANNEL);
			}
			else
			{
				pwm_set_duty(MAX_DUTY_C, LED_R_CHANNEL);
				pwm_set_duty(MAX_DUTY_C, LED_G_CHANNEL);
				pwm_set_duty(MAX_DUTY_C, LED_B_CHANNEL);
			}
			if(anim_blinks == 40)
			{
				anim_sm++;
			}
		}
	}

	// Check if the user pressed the button or animation is over
	if(gpio16_input_get() == 0 || anim_sm == 2)
	{
		pwm_set_duty(0, LED_R_CHANNEL);
		pwm_set_duty(0, LED_G_CHANNEL);
		pwm_set_duty(0, LED_B_CHANNEL);
		anim_sm = 0;
		red_dc = 0;
		blue_dc = 0;
		green_dc = 0;
		anim_blinks = 0;
		alarm_ack = true;
		alarm_in_progress = false;
		os_timer_disarm(&timer_alarmanim);
	}

	pwm_start();
}

int32_t ICACHE_FLASH_ATTR get_nbmins_before_alarm(void)
{
	bool alarm_discard_copy = alarm_discard;
	tm_struct alarm_tm;

	if(time_fetched)
	{
		os_memcpy(&alarm_tm, current_time_dt, sizeof(alarm_tm));
		alarm_tm.tm_hour = stored_alarmtime[0];
		alarm_tm.tm_min = stored_alarmtime[1];

		// Is the alarm today ?
		if(stored_alarmdays[current_time_dt->tm_wday] && ((current_time_dt->tm_hour < stored_alarmtime[0]) || (current_time_dt->tm_hour == stored_alarmtime[0] && current_time_dt->tm_min < stored_alarmtime[1])))
		{	
			if(alarm_discard_copy)
			{
				for(uint8_t i = 1; i < 8; i++)
				{
					if(stored_alarmdays[(current_time_dt->tm_wday+i)%7])
					{
						alarm_tm.tm_mday += i;
						mktime(&alarm_tm);
						return (int32_t)difftime(mktime(&alarm_tm), mktime(current_time_dt))/60;
					}
				}

				return -1;
			}
			else
			{	
				return (int32_t)difftime(mktime(&alarm_tm), mktime(current_time_dt))/60;
			}
		}
		else
		{
			for(uint8_t i = 1; i < 8; i++)
			{
				if(stored_alarmdays[(current_time_dt->tm_wday+i)%7])
				{
					alarm_tm.tm_mday += i;
					mktime(&alarm_tm);
					return (int32_t)difftime(mktime(&alarm_tm), mktime(current_time_dt))/60;
				}
			}

			return -1;
		}
	}
	else
	{
		return -1;
	}
}

void ICACHE_FLASH_ATTR discard_next_alarm(void)
{
	alarm_discard = true;
	pwm_set_duty(0, LED_R_CHANNEL);
	pwm_set_duty(0, LED_G_CHANNEL);
	pwm_set_duty(0, LED_B_CHANNEL);
	pwm_start();
}

void ICACHE_FLASH_ATTR second_tick(void)
{
	current_timestamp = sntp_get_current_timestamp();
	if(current_timestamp != 0)
	{
		// Prepare code
		if(get_nbmins_before_alarm() <= 30 && get_nbmins_before_alarm() > 0 && !alarm_in_progress)
		{
			pwm_set_duty(stored_prep[2], LED_R_CHANNEL);
			pwm_set_duty(0, LED_G_CHANNEL);
			pwm_set_duty(0, LED_B_CHANNEL);
			pwm_start();
		}
		else if(get_nbmins_before_alarm() <= 60 && get_nbmins_before_alarm() > 0 && !alarm_in_progress)
		{
			pwm_set_duty(0, LED_R_CHANNEL);
			pwm_set_duty(stored_prep[1], LED_G_CHANNEL);
			pwm_set_duty(0, LED_B_CHANNEL);
			pwm_start();
		}
		else if(get_nbmins_before_alarm() <= 120 && get_nbmins_before_alarm() > 0 && !alarm_in_progress)
		{
			pwm_set_duty(0, LED_R_CHANNEL);
			pwm_set_duty(0, LED_G_CHANNEL);
			pwm_set_duty(stored_prep[0], LED_B_CHANNEL);
			pwm_start();
		}		

		if(time_fetched_light == true)
		{
			pwm_set_duty(0, LED_G_CHANNEL);
			time_fetched_light = false;
			pwm_start();
		}
		if(time_fetched == false)
		{
			time_fetched = true;
			time_fetched_light = true;
			pwm_set_duty(MAX_DUTY_C, LED_G_CHANNEL);
			pwm_start();
		}

		current_time_dt = gmtime(&current_timestamp);
		applyTZ(current_time_dt, 1);
		//os_printf("%02d:%02d:%02d\r\n", dt->tm_hour, dt->tm_min, dt->tm_sec);
		ets_sprintf(string_current_time, "%02d:%02d:%02d\r\n", current_time_dt->tm_hour, current_time_dt->tm_min, current_time_dt->tm_sec);
		ets_sprintf(string_current_datetime, "%02d:%02d:%02d %02d/%02d/%02d\r\n", current_time_dt->tm_hour, current_time_dt->tm_min, current_time_dt->tm_sec, current_time_dt->tm_mday, current_time_dt->tm_mon + 1, current_time_dt->tm_year + 1900);

		// Alarm code
		if(stored_alarmdays[current_time_dt->tm_wday] && current_time_dt->tm_hour == stored_alarmtime[0] && current_time_dt->tm_min == stored_alarmtime[1])
		{
			if(!alarm_in_progress && !alarm_ack)
			{
				if(alarm_discard)
				{
					alarm_discard = false;
					alarm_ack = true;
				}
				else
				{
					alarm_in_progress = true;					
					os_timer_disarm(&timer_alarmanim);
					os_timer_setfn(&timer_alarmanim, (os_timer_func_t*)alarm_anim_tick, NULL);
					os_timer_arm(&timer_alarmanim, ALARM_ANIM_MS, 1);
				}
			}
		}
		else
		{
			alarm_ack = false;
		}
	}
}

//Main routine. Initialize stdout, the I/O, filesystem and the webserver and we're done.
void ICACHE_FLASH_ATTR user_init(void) {
	stdoutInit();
	captdnsInit();

	/*
	struct station_config stationConf; 		// Station conf struct
	wifi_set_opmode(0x1); 				// Set station mode
	os_memcpy(&stationConf.ssid, ssid, 32); 	// Set settings
	os_memcpy(&stationConf.password, password, 64); // Set settings
	wifi_station_set_config(&stationConf); 		// Set wifi conf
*/
	// 0x40200000 is the base address for spi flash memory mapping, ESPFS_POS is the position
	// where image is written in flash that is defined in Makefile.
#ifdef ESPFS_POS
	espFsInit((void*)(0x40200000 + ESPFS_POS));
#else
	espFsInit((void*)(webpages_espfs_start));
#endif
	httpdInit(builtInUrls, 80);
#ifdef SHOW_HEAP_USE
	os_timer_disarm(&prHeapTimer);
	os_timer_setfn(&prHeapTimer, prHeapTimerCb, NULL);
	os_timer_arm(&prHeapTimer, 3000, 1);
#endif
	os_timer_disarm(&websockTimer);
	os_timer_setfn(&websockTimer, websockTimerCb, NULL);
	os_timer_arm(&websockTimer, 1000, 1);
	os_printf("\nReady\n");
	
	// Ntp client
	ip_addr_t *addr = (ip_addr_t *)os_zalloc(sizeof(ip_addr_t));
	sntp_setservername(0, "us.pool.ntp.org"); 	// set server 0 by domain name
	sntp_setservername(1, "ntp.sjtu.edu.cn"); 	// set server 1 by domain name
	sntp_setservername(2, "europe.pool.ntp.org"); 	// set server 1 by domain name
	sntp_set_timezone(0);				// UTC time
	sntp_init();					// Init sntp
	os_free(addr);					// free alloc

	// Increment seconds timer
	os_timer_disarm(&timer_second);
	os_timer_setfn(&timer_second, (os_timer_func_t*)second_tick, NULL);
	os_timer_arm(&timer_second, 1000, 1);

	// Gpio16
	gpio16_input_conf();

	// Pwm Init
	uint32 io_info[][3] = {{PWM_0_OUT_IO_MUX,PWM_0_OUT_IO_FUNC,PWM_0_OUT_IO_NUM},{PWM_1_OUT_IO_MUX,PWM_1_OUT_IO_FUNC,PWM_1_OUT_IO_NUM},{PWM_2_OUT_IO_MUX,PWM_2_OUT_IO_FUNC,PWM_2_OUT_IO_NUM}};
	uint32 pwm_duty_init[PWM_CHANNEL] = {0,0,0};
	pwm_init(PWM_PER, pwm_duty_init, PWM_CHANNEL, io_info);
	set_pwm_debug_en(0);
	pwm_start();
}

void ICACHE_FLASH_ATTR user_rf_pre_init() {
	//Not needed, but some SDK versions want this defined.
}
