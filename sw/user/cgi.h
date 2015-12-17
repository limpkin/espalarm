#ifndef CGI_H
#define CGI_H

#include "httpd.h"

int cgiLed(HttpdConnData *connData);
int cgiAlarm(HttpdConnData *connData);
int tplLed(HttpdConnData *connData, char *token, void **arg);
int tplAlarm(HttpdConnData *connData, char *token, void **arg);
int tplCounter(HttpdConnData *connData, char *token, void **arg);

#endif
