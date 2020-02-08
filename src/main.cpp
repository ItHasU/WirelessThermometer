#include <Arduino.h>
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

#include "../../config.h"
// const char *ssid = "...";
// const char *password = "...";

const int LED = 5;
const int SENSOR = 2;

// Create AsyncWebServer object on port 80
AsyncWebServer *server = NULL;

void setup_webserver();

void setup()
{
  Serial.begin(9600);
  //-- Initialisation des entrées / sorties --
  pinMode(LED, OUTPUT);
  pinMode(SENSOR, INPUT_PULLUP);

  //-- Connexion au WIFI --
  Serial.println("Connecting to wifi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    // On essaye de se connecter
    WiFi.begin(ssid, password);
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

void setup_webserver()
{
  server = new AsyncWebServer(80);
  // Initialize SPIFFS
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Route for root / web page
  server->serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");

  server->on("/scan.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"chip_id\":\"";

    char boardId[13]; // 6*2 bytes for chipID + 1 for \0
    uint64_t chipid = ESP.getEfuseMac();
    sprintf(boardId, "%04X%08X",
            (uint16_t)(chipid >> 32), //print High 2 bytes
            (uint32_t)chipid);        //print Low 4bytes.
    json += boardId;

    json += "\", \"networks\": [";

    int n = WiFi.scanComplete();
    if (n == -2)
    {
      WiFi.scanNetworks(true);
    }
    else if (n)
    {
      for (int i = 0; i < n; ++i)
      {
        if (i)
        {
          json += ",";
        }
        json += "{";
        json += ",\"ssid\":\"" + WiFi.SSID(i) + "\"";
        json += ",\"channel\":" + String(WiFi.channel(i));
        json += ",\"secure\":" + String(WiFi.encryptionType(i));
        json += "}";
      }
      WiFi.scanDelete();
      if (WiFi.scanComplete() == -2)
      {
        WiFi.scanNetworks(true);
      }
    }
    json += "]}";
    request->send(200, "application/json", json);
    json = String();
  });

  // Start server
  server->begin();
}
