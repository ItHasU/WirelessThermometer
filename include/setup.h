#pragma once
#ifndef _SETUP_H
#define _SETUP_H

/**
 * @return true if config is available
 * (based on presence of WIFI SSID only)
 */
bool has_config();

/**
 * Start Wifi AP and HTTP server
 */
void setup_config();

#endif
