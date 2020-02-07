#include <Arduino.h>
#include "WiFi.h"

#include "../../config.h"
// const char *ssid = "...";
// const char *password = "...";

const int LED = 5;
const int SENSOR = 2;

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
