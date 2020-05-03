#include "setup.h"

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Preferences.h>

#include "constants.h"

AsyncWebServer *server = NULL;

bool has_config()
{
  Preferences preferences;

  preferences.begin("settings", true);
  String p_ssid = preferences.getString(P_NETWORK_SSID);
  preferences.end();

  return p_ssid.length();
}

static RTC_DATA_ATTR bool is_reset = true;

int smart_reset(uint32_t timeout_ms)
{
  int count = 0;
  //-- Check Reset was pressed ------------------------------------------------
  if (is_reset)
  {
    // Simple trick to know if it was a reset, since it will only
    // be set to true if reset was called. It is maintained on deep sleep.
    Preferences preferences;
    preferences.begin("smart_reset", false);
    count = preferences.getInt("count", 0);
    is_reset = false;
    //-- Count clicks -----------------------------------------------------------
    count++;
    preferences.putInt("count", count);
    preferences.end();

    //-- Time out ---------------------------------------------------------------
    digitalWrite(LED_BUILTIN, LOW);
    delay(timeout_ms);
    digitalWrite(LED_BUILTIN, HIGH);

    //-- Clear for next time ----------------------------------------------------
    preferences.begin("smart_reset", false);
    preferences.putInt("count", 0);
    preferences.end();
  }
  return count;
}

void setup_config()
{
  WiFi.disconnect();   //added to start with the wifi off, avoid crashing
  WiFi.mode(WIFI_OFF); //added to start with the wifi off, avoid crashing
  WiFi.mode(WIFI_AP);
  //WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID);

  server = new AsyncWebServer(80);
  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
#if DEBUG
    Serial.println("An Error has occurred while mounting SPIFFS");
#endif
    return;
  }

  //-- GET settings.json ------------------------------------------------------
  // Keep this request first so demo files are not sent
  server->on("/settings.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    //-- Chip id --------------------------------------------------------------
    json += "\"chip_id\":\"";
    char boardId[13]; // 6*2 bytes for chipID + 1 for \0
    uint64_t chipid = ESP.getEfuseMac();
    sprintf(boardId, "%04X%08X",
            (uint16_t)(chipid >> 32), //print High 2 bytes
            (uint32_t)chipid);        //print Low 4bytes.
    json += boardId;
    json += "\"";

    //-- Get preferences ------------------------------------------------------
    Preferences preferences;
    json += ", \"settings\": {";
    preferences.begin(PREFERENCES_NAME, true);
    json += "\"" + String(P_NETWORK_SSID) + "\": \"" + preferences.getString(P_NETWORK_SSID) + "\",";
    json += "\"" + String(P_NETWORK_PASSWORD) + "\": \"" + preferences.getString(P_NETWORK_PASSWORD) + "\",";

    json += "\"" + String(P_MQTT_SERVER) + "\": \"" + preferences.getString(P_MQTT_SERVER) + "\",";
    int port = preferences.getInt(P_MQTT_PORT, MQTT_PORT_DEFAULT);
    if (port != MQTT_PORT_DEFAULT)
    {
      json += "\"" + String(P_MQTT_PORT) + "\": \"" + +"\",";
    }
    json += "\"" + String(P_MQTT_USERNAME) + "\": \"" + preferences.getString(P_MQTT_USERNAME) + "\",";
    json += "\"" + String(P_MQTT_PASSWORD) + "\": \"" + preferences.getString(P_MQTT_PASSWORD) + "\",";

    json += "\"" + String(P_BOARD_UID) + "\": \"" + preferences.getString(P_BOARD_UID) + "\",";

    json += "\"end\":0";
    json += "}";
    // End of object
    json += "}";

    request->send(200, "application/json", json);
    json = String();
  });

  //-- Save preferences -------------------------------------------------------
  server->on("/settings.json", HTTP_POST, [](AsyncWebServerRequest *request) {
    //List all parameters
    int params = request->params();

    Preferences preferences;
    preferences.begin(PREFERENCES_NAME, false);
    for (int i = 0; i < params; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      if (p->isPost())
      {
        if (strcmp(p->name().c_str(), P_MQTT_PORT) == 0)
        {
          int port = atoi(p->value().c_str());
          if (port)
          {
            preferences.putInt(p->name().c_str(), port);
          }
          else
          {
            preferences.remove(p->name().c_str());
          }
        }
        else
        {
          preferences.putString(p->name().c_str(), p->value());
        }
#if DEBUG
        Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
#endif
      }
    }
    preferences.end();

    request->send(200);
  });

  //-- Clear preferences ------------------------------------------------------
  server->on("/settings.json", HTTP_DELETE, [](AsyncWebServerRequest *request) {
    Preferences preferences;
    preferences.begin(PREFERENCES_NAME);
    preferences.clear();
    preferences.end();

    request->send(200);
  });

  //-- Scan networks ----------------------------------------------------------
  server->on("/scan.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";

    json += "\", \"networks\": [";

    int n = WiFi.scanNetworks(false);
    for (int i = 0; i < n; i++)
    {
      if (i)
      {
        json += ",";
      }
      json += "{";
      json += "\"ssid\":\"" + WiFi.SSID(i) + "\"";
      json += ",\"channel\":" + String(WiFi.channel(i));
      json += ",\"secure\":" + String(WiFi.encryptionType(i));
      json += "}";
    }
    WiFi.scanDelete();
    json += "]}";
    request->send(200, "application/json", json);
    json = String();
  });

  //-- Reboot -----------------------------------------------------------------
  server->on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200);

    // We cannot use this, else it will get back to setup
    // ESP.restart();
    // We make a small deep sleep
    esp_sleep_enable_timer_wakeup(1000);
    esp_deep_sleep_start();
  });

  //-- Reboot -----------------------------------------------------------------
  server->on("/factory", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200);

    Preferences preferences;
    preferences.begin(PREFERENCES_NAME, false);
    preferences.clear();
    preferences.end();

    // We will restart setup process
    ESP.restart();
  });

  server->onNotFound([](AsyncWebServerRequest *request) {
#if DEBUG
    Serial.println(request->url().c_str());
#endif
    request->send(404, "text/plain", "Not found");
  });

  // Route for root / web page
  server->serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");

  // Start server
  server->begin();
}