#include <Arduino.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <Preferences.h>

//-----------------------------------------------------------------------------
//-- Constants ----------------------------------------------------------------
//-----------------------------------------------------------------------------

const char *PREFERENCES_NAME = "settings";

const char *P_NETWORK_SSID = "net_ssid";
const char *P_NETWORK_PASSWORD = "net_pass";

const char *P_MQTT_SERVER = "mqtt_server";
const char *P_MQTT_PORT = "mqtt_port";
const char *P_MQTT_USERNAME = "mqtt_user";
const char *P_MQTT_PASSWORD = "mqtt_pass";

const char *P_BOARD_UID = "board_uid";

const int LED = 5;
const int SENSOR = 2;

//-----------------------------------------------------------------------------
//-- Globals ------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//-- HTTP Server --------------------------------------------------------------
//-----------------------------------------------------------------------------

AsyncWebServer *server = NULL;
void setup_webserver();

//-----------------------------------------------------------------------------
//-- Main ---------------------------------------------------------------------
//-----------------------------------------------------------------------------

int reboot_timeout_s = 60;

void setup()
{
  Serial.begin(9600);
  //-- Initialisation des entrées / sorties --
  pinMode(LED, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);

  // //-- Write data --
  // {
  //   #include "../../config.h"
  //   Preferences preferences;
  //   preferences.begin(PREFERENCES_NAME, false);
  //   preferences.putString(P_NETWORK_SSID, ssid);
  //   preferences.putString(P_NETWORK_PASSWORD, password);
  //   preferences.end();
  //   return;
  // }

  //-- Read preferences --
#include "../../config.h"
  String p_ssid(ssid);
  String p_password(password);

  // Preferences preferences;

  // preferences.begin("settings", true);
  // String p_ssid = preferences.getString(P_NETWORK_SSID);
  // String p_password = preferences.getString(P_NETWORK_PASSWORD);
  // preferences.end();

  //-- Connexion au WIFI --
  Serial.printf("Connecting to %s:%s\n", p_ssid.c_str(), p_password.c_str());
  while (WiFi.status() != WL_CONNECTED)
  {
    // On essaye de se connecter
    WiFi.begin(p_ssid.c_str(), p_password.c_str());
    // On attend un peu pour voir si on est connecté
    for (int i = 0; i < 20; i++)
    { // 20 * 100ms
      Serial.print(".");
      digitalWrite(LED, !digitalRead(LED));
      delay(100);
    }
    Serial.print("\n");
  }

  Serial.println("Connected");
  Serial.println(WiFi.localIP());

  setup_webserver();
}

void loop()
{
  digitalWrite(LED, LOW);
  delay(500); // ms
  digitalWrite(LED, HIGH);
  delay(500); // ms
  reboot_timeout_s--;
  if (reboot_timeout_s == 0)
  {
    Serial.println("System reached timeout, restarting");
  }
}

//-----------------------------------------------------------------------------
//-- HTTP Server (implementation) ---------------------------------------------
//-----------------------------------------------------------------------------

void setup_webserver()
{
  server = new AsyncWebServer(80);
  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
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
    json += "\"" + String(P_MQTT_PORT) + "\": \"" + preferences.getInt(P_MQTT_PORT, 1883) + "\",";
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
          preferences.putInt(p->name().c_str(), atoi(p->value().c_str()));
        }
        else
        {
          preferences.putString(p->name().c_str(), p->value());
        }
        Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
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

    ESP.restart();
  });

  server->onNotFound([](AsyncWebServerRequest *request) {
    Serial.println(request->url().c_str());
    request->send(404, "text/plain", "Not found");
  });

  // Route for root / web page
  server->serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");

  // Start server
  server->begin();
}
