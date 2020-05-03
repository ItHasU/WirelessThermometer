#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "constants.h"
#include "setup.h"
#include "sensor.h"

//-----------------------------------------------------------------------------
//-- Constants ----------------------------------------------------------------
//-----------------------------------------------------------------------------

const int LED = 5;
const int SENSOR = 19;

#define uS_TO_S_FACTOR 1000000 /* Conversion factor for u seconds to seconds */
#define TIME_TO_SLEEP 600      /* Time ESP32 will go to sleep (in seconds) */
#define TIME_FOR_SETUP 300     /* Time ESP32 will wait for setup (in seconds) */

//-----------------------------------------------------------------------------
//-- Globals ------------------------------------------------------------------
//-----------------------------------------------------------------------------

RTC_DATA_ATTR int successCount = 0;
RTC_DATA_ATTR int errorCount = 0;
RTC_DATA_ATTR int setupCount = 0;

#ifdef __cplusplus
extern "C"
{
#endif
  /** ESP temperature sensor */
  uint8_t temprature_sens_read();

#ifdef __cplusplus
}
#endif

//-----------------------------------------------------------------------------
//-- Temperature --------------------------------------------------------------
//-----------------------------------------------------------------------------

OneWire oneWire(SENSOR);
DallasTemperature sensors(&oneWire);

static float get_temperature()
{
  //-- Lecture de la température --------------------------------------------
  sensors.begin();
  sensors.setResolution(12);
  sensors.setWaitForConversion(true);

  LOG("Requesting temperature\n");
  sensors.requestTemperatures();

  delay(800);

  LOG("Reading temperature: ");
  return sensors.getTempCByIndex(0);
}

String generate_payload(const char * board_uid)
{
  String payload = "{";
  payload += "\"uid\":\"";
  payload += board_uid;
  payload += "\",\"temperature\":";
  payload += get_temperature();
  payload += ",\"success\":";
  payload += successCount;
  payload += ",\"error\":";
  payload += errorCount;
  payload += "}";

  return payload;
}

//-----------------------------------------------------------------------------
//-- Main ---------------------------------------------------------------------
//-----------------------------------------------------------------------------

void blink(int count, int delay_ms)
{
  while (count--)
  {
    digitalWrite(LED, !digitalRead(LED));
    delay(1000);
  }
}

void setup()
{
#if DEBUG
  Serial.begin(9600);
#endif

  //-- Initialisation des entrées / sorties --
  pinMode(LED, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);
  digitalWrite(LED, HIGH); // make sure LED is off

  //-- Check that board is configured -----------------------------------------
  if (setupCount == 0)
  {
    // Reset button was pressed, setup count was reset to 0
    LOG("Starting setup ...\n");
    // We never tried to run the config yet ...
    // setup AP and HTTP server
    setupCount++; // Don't try again until board was reset
    setup_config();
    // Wait for 5 minutes
    blink(TIME_FOR_SETUP, 1000);
    LOG("Setup timed out.\n");
  }
  else
  {
    //-- Try to send temperature ------------------------------------------------
    int error = try_post(&generate_payload);
    if (error == 0)
    {
      successCount++;
    }
    else
    {
#if DEBUG
      Serial.printf("Posting failed with code: %d\n", error);
#endif
      // Some blinks to say there was an error
      blink(10 /*times*/, 100 /*ms*/);
      errorCount++;
    }
  }
}

void loop()
{
  LOG("Time to sleep\n");
  // Make sure LED is OFF
  digitalWrite(LED, HIGH);
  // We are done, go to deep sleep
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}
