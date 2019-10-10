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

// endpoint for asking if the card is ok
#define ENDPOINT_LENGTH (100)
char endpoint[ENDPOINT_LENGTH];

WiFiManager wifiManager;
bool saveconfig = false;

#define SPI_SS_PIN (5)

PN532_SPI pn532spi(SPI, SPI_SS_PIN);
PN532 nfc(pn532spi);

// callback for saving the config
void configModeCallback(WiFiManager *wifiManager)
{
  saveconfig = true;
}

void writeConfig(String json)
{
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile)
  {
    Serial.println("could not write the default config file, halting");
    while (1)
    {
      delay(10);
    }
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
    Serial.println("could not read the config file, halting");
    while (1)
    {
      delay(10);
    }
  }
  size_t size = configFile.size();
  std::unique_ptr<char[]> buf(new char[size]);

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, configFile.readString());
  if (error)
  {
    Serial.println("Failed to deserialize config file, halting");
    while (1)
    {
      delay(10);
    }
  }

  strcpy(endpoint, doc["endpoint"]);
  // read our utc offset from the doc
  configFile.close();
}

void setup()
{
  Serial.begin(115200);

  Serial.println("Starting up");

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata)
  {
    Serial.print("Didn't find PN53x board, halting");
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
  SPIFFS.format();

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
    writeConfig("{\"endpoint\": \"https://mulysa/access/\"}");
  }

  // read the config from SPIFFS
  readConfig();

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
    String newconfig = "{\"endpoint\": ";
    newconfig.concat(new_endpoint);
    newconfig.concat("}");
    Serial.println("Trying to save new config: ");
    Serial.println(newconfig);
    writeConfig(newconfig);
  }
}

void loop(void)
{
  boolean success;
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
  uint8_t uidLength;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);

  if (success)
  {
    Serial.println("Found a card!");
    Serial.print("UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++)
    {
      Serial.print(" 0x");
      Serial.print(uid[i], HEX);
    }
    Serial.println("");

    // wait until the card is taken away
    while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength))
    {
    }
  }
  else
  {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
  }
}
