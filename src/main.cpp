/******************************************************************************
 * Mulysa NFC reader
 *
 * NFC door access reader for Mulysa project
 *
 * GitHub Repository
 * https://github.com/TampereHacklab/mulysa_nfc_reader
  ****************************************************************************/
#include <Arduino.h>

// for saving settings in spiffs
#include "FS.h"
#include "SPIFFS.h"
#include "ArduinoJson.h"

// for talking with the nfc reader
#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"

// wifimanager
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>

// for talking with the server
#include <HTTPClient.h>

// Let's Encrypt Authority X3
// valid untill 3/17/21, 6:40:46 PM GMT+2
const char *root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEkjCCA3qgAwIBAgIQCgFBQgAAAVOFc2oLheynCDANBgkqhkiG9w0BAQsFADA/\n"
    "MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n"
    "DkRTVCBSb290IENBIFgzMB4XDTE2MDMxNzE2NDA0NloXDTIxMDMxNzE2NDA0Nlow\n"
    "SjELMAkGA1UEBhMCVVMxFjAUBgNVBAoTDUxldCdzIEVuY3J5cHQxIzAhBgNVBAMT\n"
    "GkxldCdzIEVuY3J5cHQgQXV0aG9yaXR5IFgzMIIBIjANBgkqhkiG9w0BAQEFAAOC\n"
    "AQ8AMIIBCgKCAQEAnNMM8FrlLke3cl03g7NoYzDq1zUmGSXhvb418XCSL7e4S0EF\n"
    "q6meNQhY7LEqxGiHC6PjdeTm86dicbp5gWAf15Gan/PQeGdxyGkOlZHP/uaZ6WA8\n"
    "SMx+yk13EiSdRxta67nsHjcAHJyse6cF6s5K671B5TaYucv9bTyWaN8jKkKQDIZ0\n"
    "Z8h/pZq4UmEUEz9l6YKHy9v6Dlb2honzhT+Xhq+w3Brvaw2VFn3EK6BlspkENnWA\n"
    "a6xK8xuQSXgvopZPKiAlKQTGdMDQMc2PMTiVFrqoM7hD8bEfwzB/onkxEz0tNvjj\n"
    "/PIzark5McWvxI0NHWQWM6r6hCm21AvA2H3DkwIDAQABo4IBfTCCAXkwEgYDVR0T\n"
    "AQH/BAgwBgEB/wIBADAOBgNVHQ8BAf8EBAMCAYYwfwYIKwYBBQUHAQEEczBxMDIG\n"
    "CCsGAQUFBzABhiZodHRwOi8vaXNyZy50cnVzdGlkLm9jc3AuaWRlbnRydXN0LmNv\n"
    "bTA7BggrBgEFBQcwAoYvaHR0cDovL2FwcHMuaWRlbnRydXN0LmNvbS9yb290cy9k\n"
    "c3Ryb290Y2F4My5wN2MwHwYDVR0jBBgwFoAUxKexpHsscfrb4UuQdf/EFWCFiRAw\n"
    "VAYDVR0gBE0wSzAIBgZngQwBAgEwPwYLKwYBBAGC3xMBAQEwMDAuBggrBgEFBQcC\n"
    "ARYiaHR0cDovL2Nwcy5yb290LXgxLmxldHNlbmNyeXB0Lm9yZzA8BgNVHR8ENTAz\n"
    "MDGgL6AthitodHRwOi8vY3JsLmlkZW50cnVzdC5jb20vRFNUUk9PVENBWDNDUkwu\n"
    "Y3JsMB0GA1UdDgQWBBSoSmpjBH3duubRObemRWXv86jsoTANBgkqhkiG9w0BAQsF\n"
    "AAOCAQEA3TPXEfNjWDjdGBX7CVW+dla5cEilaUcne8IkCJLxWh9KEik3JHRRHGJo\n"
    "uM2VcGfl96S8TihRzZvoroed6ti6WqEBmtzw3Wodatg+VyOeph4EYpr/1wXKtx8/\n"
    "wApIvJSwtmVi4MFU5aMqrSDE6ea73Mj2tcMyo5jMd6jmeWUHK8so/joWUoHOUgwu\n"
    "X4Po1QYz+3dszkDqMp4fklxBwXRsW10KXzPMTZ+sOPAveyxindmjkW8lGy+QsRlG\n"
    "PfZ+G6Z6h7mjem0Y+iWlkYcV4PIWL1iwBi8saCbGS5jN2p8M+X+Q7UNKEkROb3N6\n"
    "KOqkqm57TH2H3eDJAkSnh6/DNFu0Qg==\n"
    "-----END CERTIFICATE-----\n";

// endpoint for asking if the card is ok
#define ENDPOINT_LENGTH (100)
char endpoint[ENDPOINT_LENGTH];

WiFiManager wifiManager;
bool saveconfig = false;
//#define NEWSETTINGS_PIN (12)

// card reader
#define SPI_SS_PIN (5)
PN532_SPI pn532spi(SPI, SPI_SS_PIN);
PN532 nfc(pn532spi);

// our chipid
String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);

// callback for saving the config
void configModeCallback(WiFiManager *wifiManager)
{
  saveconfig = true;
}

void resetSettings()
{
  Serial.println("Forgetting settings and restarting to get portal up");
  wifiManager.resetSettings();
  Serial.println("Restarting");
  delay(1000);
  ESP.restart();
}

void writeConfig(String json)
{
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile)
  {
    Serial.println("could not write the default config file, resetting");
    resetSettings();
  }
  configFile.println(json);
  Serial.println("Wrote default config");
  configFile.close();
}

void readConfig()
{
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile)
  {
    Serial.println("could not read the config file, resetting");
    resetSettings();
  }
  size_t size = configFile.size();
  std::unique_ptr<char[]> buf(new char[size]);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, configFile.readString());
  if (error)
  {
    Serial.println("Failed to deserialize config file, resetting");
    resetSettings();
  }

  strcpy(endpoint, doc["endpoint"]);
  configFile.close();
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting up");

  //  pinMode(NEWSETTINGS_PIN, INPUT_PULLUP);

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata)
  {
    Serial.print("Didn't find PN53x board check connections, halting");
    while (1)
    {
      delay(10);
    }
  }

  // Got ok data, print it out!
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0xFF);

  // configure board to read RFID tags
  nfc.SAMConfig();

  // format spiffs
  // SPIFFS.format();

  if (!SPIFFS.begin(true))
  {
    Serial.println("Failed to start or format SPIFFS, freezing");
    while (1)
    {
      delay(10);
    }
  }
  if (!SPIFFS.exists("/config.json"))
  {
    Serial.println("Generating default config.json in SPIFFS");
    writeConfig("{\"endpoint\": \"https://mulysa/access/\"}");
  }

  // start wifi
  // callback for saving
  wifiManager.setAPCallback(configModeCallback);
  // wifi parameter settings
  WiFiManagerParameter endpointparameter("endpoint", "Endpoint: https://foo/bar/", endpoint, ENDPOINT_LENGTH);
  wifiManager.addParameter(&endpointparameter);
  // connect or start the settings ap
  wifiManager.autoConnect();

  if (saveconfig)
  {
    Serial.println("New settings available, trying to save them");
    char new_endpoint[ENDPOINT_LENGTH];
    strcpy(new_endpoint, endpointparameter.getValue());
    String newconfig = "{\"endpoint\": \"";
    newconfig.concat(new_endpoint);
    newconfig.concat("\"}");
    Serial.print("Trying to save new config: ");
    Serial.println(newconfig);
    writeConfig(newconfig);
  }

  // read the config from SPIFFS
  readConfig();
}

void loop(void)
{
  boolean cardRead;
  uint8_t cardUID[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned cardUID
  uint8_t cardUIDLength;                     // Length of the cardUID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'cardUID' will be populated with the cardUID, and cardUIDLength will indicate
  // if the cardUID is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  cardRead = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &cardUID[0], &cardUIDLength);

  if (cardRead)
  {
    Serial.print("Card with cardUID value: ");
    Serial.print("0x");
    for (uint8_t i = 0; i < cardUIDLength; i++)
    {
      Serial.print(cardUID[i], HEX);
    }
    Serial.println("");

    // Wait until the card is taken away
    while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &cardUID[0], &cardUIDLength))
    {
    }

    Serial.print("using endpoint: ");
    Serial.println(endpoint);

    HTTPClient http;
    http.begin(endpoint, root_ca);
    http.addHeader("Content-Type", "application/json");

    // TODO: there must be a better way to build the json string
    String payload = "{\"";
    payload.concat("deviceid\":\"");
    payload.concat(chipId);
    payload.concat("\",\"");
    payload.concat("cardid\":\"");
    for (uint8_t i = 0; i < cardUIDLength; i++)
    {
      payload.concat(String(cardUID[i], HEX));
    }
    payload.concat("\"}");

    Serial.print("request data: ");
    Serial.println(payload);

    int httpCode = http.POST(payload);
    Serial.print("Got response: ");
    Serial.println(httpCode);
    http.end();
  }
  else
  {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
  }

  //  if (digitalRead(NEWSETTINGS_PIN) == LOW)
  //  {
  //    resetSettings();
  //  }
}
