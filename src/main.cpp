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

// display
#include <U8g2lib.h>

// Let's Encrypt ISRG Root X1
// valid untill 6/4/35, 2:04:38 PM GMT+3
const char *root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "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\n"
    "-----END CERTIFICATE-----\n";

// endpoint for asking if the card is ok
#define ENDPOINT_LENGTH (100)
char endpoint[ENDPOINT_LENGTH];

WiFiManager wifiManager;
bool saveconfig = false;
#define RESET_SETTINGS_PIN (21)
#define RELAY_PIN (22)

// card reader
#define SPI_SS_PIN (5)
PN532_SPI pn532spi(SPI, SPI_SS_PIN);
PN532 nfc(pn532spi);

// our chipid
String chipId = String((uint32_t)ESP.getEfuseMac(), HEX);

// display (nokia 5110 PCD8544)
U8G2_PCD8544_84X48_1_4W_SW_SPI u8g2(U8G2_R2, /* clock=*/13, /* data=*/15, /* cs=*/16, /* dc=*/2, /* reset=*/4);

#define trehacklablogow 29
#define trehacklablogoh 48
static unsigned char trehacklablogo[] = {
    0x80, 0xff, 0xff, 0x1f, 0x80, 0xff, 0xff, 0x1f, 0x80, 0xff, 0xff, 0x1f,
    0x80, 0xff, 0xff, 0x1f, 0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e,
    0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e,
    0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e, 0xff, 0xff, 0x03, 0x1e,
    0xff, 0xff, 0x03, 0x1e, 0xff, 0xff, 0x03, 0x1e, 0xff, 0xff, 0x03, 0x1e,
    0x00, 0x00, 0x00, 0x1e, 0xfc, 0xff, 0x00, 0x1e, 0xfc, 0xff, 0x00, 0x1e,
    0xfc, 0xff, 0x00, 0x1e, 0xfc, 0xff, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e,
    0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e,
    0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e,
    0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e,
    0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e,
    0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e,
    0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e,
    0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e,
    0x80, 0x07, 0x00, 0x1e, 0x80, 0x07, 0x00, 0x1e, 0x80, 0xff, 0xff, 0x1f,
    0x80, 0xff, 0xff, 0x1f, 0x80, 0xff, 0xff, 0x1f, 0x80, 0xff, 0xff, 0x1f};

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

void drawIdleScreen()
{
  // idle screen
  u8g2.firstPage();
  do
  {
    u8g2.drawXBM(5, 0, trehacklablogow, trehacklablogoh, trehacklablogo);
    u8g2.setFont(u8g2_font_tenfatguys_tf);
    u8g2.drawStr(40, 20, "TRE");
    u8g2.drawStr(40, 40, "NFC");
  } while (u8g2.nextPage());
}

void drawStartupScreen()
{
  // idle screen
  u8g2.firstPage();
  do
  {
    u8g2.drawXBM(5, 0, trehacklablogow, trehacklablogoh, trehacklablogo);
    u8g2.setFont(u8g2_font_tenfatguys_tf);
    u8g2.drawStr(40, 20, "Please");
    u8g2.drawStr(40, 40, "Wait");
  } while (u8g2.nextPage());
}

void drawErrorScreen(String errmsg)
{
  u8g2.firstPage();
  do
  {
    u8g2.drawXBM(5, 0, trehacklablogow, trehacklablogoh, trehacklablogo);
    u8g2.setFont(u8g2_font_tenfatguys_tf);
    u8g2.drawStr(40, 20, "Error");
    u8g2.setCursor(40, 40);
    u8g2.print(errmsg);
  } while (u8g2.nextPage());
}

void drawCardRead(String cardid)
{
  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_tenfatguys_tf);
    u8g2.drawStr(0, 20, "Card:");
    u8g2.setFont(u8g2_font_t0_11_tf);
    u8g2.setCursor(0, 40);
    u8g2.print(cardid);
  } while (u8g2.nextPage());
}

void drawWelcome(String name)
{
  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_tenfatguys_tf);
    u8g2.drawStr(10, 20, "Welcome");
    u8g2.setCursor(10, 40);
    u8g2.print(name);
  } while (u8g2.nextPage());
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting up");

  u8g2.begin();
  drawStartupScreen();

  pinMode(RESET_SETTINGS_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata)
  {
    Serial.print("Didn't find PN53x board check connections, halting");
    drawErrorScreen("noreader");
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
    drawErrorScreen("SPIFFS");
    while (1)
    {
      delay(10);
    }
  }
  if (!SPIFFS.exists("/config.json"))
  {
    Serial.println("Generating default config.json in SPIFFS");
    writeConfig("{\"endpoint\": \"https://mulysa.example.com/api/v1/access/nfc/\"}");
  }

  // start wifi
  // callback for saving
  wifiManager.setAPCallback(configModeCallback);
  // wifi parameter settings
  WiFiManagerParameter endpointparameter("endpoint", "Endpoint: https://mulysa.example.com/api/v1/access/nfc/", endpoint, ENDPOINT_LENGTH);
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
  // show our idle screen
  drawIdleScreen();

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
    String cardid = "";
    for (uint8_t i = 0; i < cardUIDLength; i++)
    {
      cardid.concat(String(cardUID[i], HEX));
    }

    String payload = "{\"";
    payload.concat("deviceid\":\"");
    payload.concat(chipId);
    payload.concat("\",\"");
    payload.concat("payload\":\"");
    payload.concat(cardid);
    payload.concat("\"}");

    drawCardRead(cardid);

    Serial.print("request data: ");
    Serial.println(payload);

    int httpCode = http.POST(payload);
    Serial.print("Got response: ");
    Serial.println(httpCode);
    if (httpCode == 200)
    {
      Serial.println("access granted");
      drawWelcome("TempName");
      digitalWrite(RELAY_PIN, HIGH);
      // TODO: make this without delay :)
      delay(30*1000);
      digitalWrite(RELAY_PIN, LOW);
    }
    else
    {
      drawIdleScreen();
      Serial.println("access denied");
    }
    http.end();
  }
  else
  {
    // PN532 probably timed out waiting for a card
    // Serial.println("Timed out waiting for a card");
  }

  if (digitalRead(RESET_SETTINGS_PIN) == LOW)
  {
    resetSettings();
  }
}
