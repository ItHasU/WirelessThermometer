#pragma once
#ifndef _CONSTANTS_H
#define _CONSTANTS_H

#include <WiFi.h>

extern const char *PREFERENCES_NAME;

extern const char *P_NETWORK_SSID ;
extern const char *P_NETWORK_PASSWORD;

extern const char *P_MQTT_SERVER;
extern const char *P_MQTT_PORT;
extern const char *P_MQTT_USERNAME;
extern const char *P_MQTT_PASSWORD;

extern const char *P_BOARD_UID;
extern const char *AP_SSID;
extern const IPAddress AP_IP;

#define DEBUG 1

#if DEBUG
#define LOG(S) Serial.print(S);
#define ERROR(S) Serial.print(S);
#else
#define LOG(S)
#define ERROR(S) Serial.print(S);
#endif

#endif
