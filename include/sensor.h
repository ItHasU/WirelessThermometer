#pragma once
#ifndef _SENSOR_H
#define _SENSOR_H

#include <Arduino.h>

#define ERROR_NONE 0
#define ERROR_WIFI 1
#define ERROR_MDNS 2
#define ERROR_MQTT 3
#define ERROR_POST 4

/**
 * Full process:
 * - Connect to WIFI
 * - Connect to MQTT
 * - Fetch temperature
 * - Post temperature
 */
int try_post(String (*payload_generator)(const char * board_uid));

#endif
