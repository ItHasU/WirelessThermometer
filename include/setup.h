#pragma once
#ifndef _SETUP_H
#define _SETUP_H

#include <stdint.h>

/**
 * @return true if config is available
 * (based on presence of WIFI SSID only)
 */
bool has_config();

/**
 * @return The number of times, the reset button has been pressed.
 */
int smart_reset(uint32_t timeout_ms = 2000);

/**
 * Start Wifi AP and HTTP server
 */
void setup_config();

#endif
