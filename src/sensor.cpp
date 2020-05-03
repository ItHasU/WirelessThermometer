#include "sensor.h"
#include "constants.h"

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <mdns.h>
#include <Preferences.h>

//-----------------------------------------------------------------------------
//-- Network tools ------------------------------------------------------------
//-----------------------------------------------------------------------------

static bool query_mdns_host(const char *host_name, char *buffer)
{
    struct ip4_addr addr;
    addr.addr = 0;

    //initialize mDNS service
    esp_err_t err = mdns_init();
    if (err)
    {
        ERROR("mDNS init failed: ");
        ERROR(esp_err_to_name(err));
        ERROR("\n");
        return false;
    }

    err = mdns_query_a(host_name, 2000, &addr);
    if (err)
    {
        ERROR("mDNS query failed: ");
        ERROR(esp_err_to_name(err));
        ERROR("\n");
        return false;
    }

    sprintf(buffer, IPSTR, IP2STR(&addr));
    return true;
}

static bool wifi_connect(const char *ssid, const char *pass)
{
    WiFi.mode(WIFI_STA);
    LOG("Trying to connect to ");
    LOG(ssid);
    LOG("\n");

    WiFi.begin(ssid, pass);

    //-- Attente active du wifi --
    int wifiCount = 100;
    while (WiFi.status() != WL_CONNECTED && wifiCount--)
    {
        LOG(".");
        delay(100);
    }
    LOG("\n");
    if (WiFi.status() == WL_CONNECTED)
    {
        LOG("Connected to wifi, with ip: ");
        LOG(WiFi.localIP());
        LOG("\n");

        return true;
    }
    else
    {
        // Sécurité pour la batterie, si ça ne marche pas on se met en veille
        ERROR("Failed to connect wifi ");
        ERROR(ssid);
        ERROR(", error: ");
        ERROR(WiFi.status());
        ERROR("\n");
        return false;
    }
}

int try_post(String (*payload_generator)())
{
    //-- Read preferences -----------------------------------------------------
    Preferences preferences;

    preferences.begin("settings", true);
    String NETWORK_SSID = preferences.getString(P_NETWORK_SSID);
    String NETWORK_PASSWORD = preferences.getString(P_NETWORK_PASSWORD);

    String MQTT_SERVER = preferences.getString(P_MQTT_SERVER);
    int MQTT_PORT = preferences.getInt(P_MQTT_PORT, MQTT_PORT_DEFAULT);
    String MQTT_USERNAME = preferences.getString(P_MQTT_USERNAME);
    String MQTT_PASSWORD = preferences.getString(P_MQTT_PASSWORD);

    String BOARD_UID = preferences.getString(P_BOARD_UID);
    preferences.end();

    //-- Wifi -----------------------------------------------------------------
    if (!wifi_connect(NETWORK_SSID.c_str(), NETWORK_PASSWORD.c_str()))
    {
        return ERROR_WIFI;
    }

    //-- Resolving ------------------------------------------------------------
    char ip[IP4ADDR_STRLEN_MAX];
    bool isMDNS = MQTT_SERVER.endsWith(".local");
    if (isMDNS)
    {
        if (!query_mdns_host(MQTT_SERVER.c_str(), ip))
        {
            ERROR("Failed to resolve hostname");
            return ERROR_MDNS;
        }

        LOG("Resolved ip: ");
        LOG(ip);
        LOG("\n");
    }

    //-- MQTT -----------------------------------------------------------------
    LOG("Connecting to MQTT\n");
    char boardId[13]; // 6*2 bytes for chipID + 1 for \0

    uint64_t chipid = ESP.getEfuseMac();
    sprintf(boardId, "%04X%08X",
            (uint16_t)(chipid >> 32), //print High 2 bytes
            (uint32_t)chipid);        //print Low 4bytes.

    LOG("Board id: ");
    LOG(boardId);
    LOG("\n");

    WiFiClient wifiClient;
    PubSubClient client(wifiClient);
    client.setServer(isMDNS ? ip : MQTT_SERVER.c_str(), MQTT_PORT);
    if (!client.connect(boardId, MQTT_USERNAME.c_str(), MQTT_PASSWORD.c_str()))
    {
        // Sécurité pour la batterie, si ça ne marche pas on se met en veille
        ERROR("Unable to connect MQTT\n");
        return ERROR_MQTT;
    }
    LOG("Connected to MQTT\n");

    //-- Préparation du message -----------------------------------------------
    String payload = payload_generator();

    String topic = "n/";
    topic += BOARD_UID;

    // Send payload
    if (client.publish(topic.c_str(), payload.c_str(), false))
    {
        LOG("Published\n");
    }
    else
    {
        ERROR("MQTT publish failed\n");
        return ERROR_POST;
    }
    client.loop();

    client.disconnect();
    wifiClient.flush();

    //-- Wait a little, for all to be sent --
    delay(100);

    //-- Success --------------------------------------------------------------
    // TODO: Si tout a bien bien marché on attend un max de temps, sinon on
    // réduit le temps
    return ERROR_NONE;
}
