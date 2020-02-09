#include <Arduino.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <Preferences.h>

//-----------------------------------------------------------------------------
//-- Constants ----------------------------------------------------------------
//-----------------------------------------------------------------------------

const char *PREFERENCES_NAME = "settings";

const char *P_HAS_CONFIG = "has_config";

const char *P_NETWORK_SSID = "net_ssid";
const char *P_NETWORK_PASSWORD = "net_pass";

const int LED = 5;
const int SENSOR = 2;

//-----------------------------------------------------------------------------
//-- Preferences --------------------------------------------------------------
//-----------------------------------------------------------------------------

Preferences preferences;

//-----------------------------------------------------------------------------
//-- HTTP Server --------------------------------------------------------------
//-----------------------------------------------------------------------------

AsyncWebServer *server = NULL;
void setup_webserver();

//-----------------------------------------------------------------------------
//-- Main ---------------------------------------------------------------------
//-----------------------------------------------------------------------------

void setup()
{
  Serial.begin(9600);
  //-- Initialisation des entrées / sorties --
  pinMode(LED, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);

  // //-- Write data --
  // preferences.begin("settings", false);
  // preferences.putString("ssid", ssid);
  // preferences.putString("password", password);
  // preferences.end();
  // return;

  //-- Read preferences --
  preferences.begin("settings", true);
  String p_ssid = preferences.getString(P_NETWORK_SSID, "Sherlock");
  String p_password = preferences.getString(P_NETWORK_PASSWORD, "elementairemoncherwatson");
  preferences.end();

  //-- Connexion au WIFI --
  Serial.println("Connecting to wifi...");
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
  digitalWrite(LED, !digitalRead(LED));
  delay(500); // ms

  if (WiFi.status() != WL_CONNECTED)
  {
    // On redémarre la carte pour quelle se reconnecte
    ESP.restart();
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
    json += ", \"settings\": {";
    preferences.begin(PREFERENCES_NAME, true);
    json += "\"" + String(P_NETWORK_SSID) + "\": \"" + preferences.getString(P_NETWORK_SSID) + "\",";
    json += "\"" + String(P_NETWORK_PASSWORD) + "\": \"" + preferences.getString(P_NETWORK_PASSWORD) + "\",";

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
    preferences.begin(PREFERENCES_NAME, false);
    for (int i = 0; i < params; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      if (p->isPost())
      {
        preferences.putString(p->name().c_str(), p->value());
        Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
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

  // Route for root / web page
  server->serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");

  // Start server
  server->begin();
}
