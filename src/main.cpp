/*
#include "WiFi.h"
#include "PubSubClient.h" //pio lib install "knolleary/PubSubClient"

#define SSID          "IB3"
#define PWD           "ingenieursbeleving3"

#define MQTT_SERVER   "192.168.0.190"
#define MQTT_PORT     1883

#define LED_PIN       2

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void callback(char *topic, byte *message, unsigned int length);

void setup_wifi()
{
  delay(10);
  Serial.println("Connecting to WiFi..");

  WiFi.begin(SSID, PWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup()
{

  Serial.begin(115200);

  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  pinMode(LED_PIN, OUTPUT);
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  if (String(topic) == "esp32/output")
  {
    Serial.print("Changing output to ");
    if (messageTemp == "on")
    {
      Serial.println("on");
      digitalWrite(LED_PIN, HIGH);
    }
    else if (messageTemp == "off")
    {
      Serial.println("off");
      digitalWrite(LED_PIN, LOW);
    }
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // creat unique client ID
    // in Mosquitto broker enable anom. access
    if (client.connect("ESP8266Client"))
    {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000)
  {
    lastMsg = now;
  }
}
*/

/**************************************************************************/
/*
  ESP32: PN532 (I2C) NFC reader -> MQTT publisher

  Wiring:
    PN532 SDA  -> ESP32 GPIO21 (default SDA)
    PN532 SCL  -> ESP32 GPIO22 (default SCL)
    PN532 IRQ  -> ESP32 GPIO4
    PN532 RSTO -> ESP32 GPIO5
    PN532 3.3V -> 3.3V
    PN532 GND  -> GND

  Every detected card UID is published to MQTT_TOPIC_TAG as an uppercase
  hex string (e.g. "049C496A995B80").
  The esp32/output topic still controls the LED ("on" / "off").
*/
/**************************************************************************/
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_PN532.h>

// ---------- WiFi / MQTT settings ----------
#define SSID            "IB3"
#define PWD             "ingenieursbeleving3"

#define MQTT_SERVER     "192.168.0.190"
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "ESP32NfcClient"
#define MQTT_TOPIC_TAG  "esp32/nfc"
#define MQTT_TOPIC_LED  "esp32/output"

#define LED_PIN         2

// ---------- PN532 pins ----------
#define PN532_IRQ       4
#define PN532_RESET     5

// Time (ms) during which the same card is not reported again
const unsigned long SAME_CARD_COOLDOWN = 1500;
// Small pause after every successful read
const unsigned long DELAY_BETWEEN_CARDS = 500;

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
WiFiClient espClient;
PubSubClient client(espClient);

String lastUid = "";
unsigned long lastUidTime = 0;

void callback(char *topic, byte *message, unsigned int length);

void setup_wifi()
{
  delay(10);
  Serial.println("Connecting to WiFi..");

  WiFi.begin(SSID, PWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_nfc()
{
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata)
  {
    Serial.println("Didn't find PN532 board");
    while (1)
      delay(1000); // halt
  }

  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (String(topic) == MQTT_TOPIC_LED)
  {
    Serial.print("Changing output to ");
    if (messageTemp == "on")
    {
      Serial.println("on");
      digitalWrite(LED_PIN, HIGH);
    }
    else if (messageTemp == "off")
    {
      Serial.println("off");
      digitalWrite(LED_PIN, LOW);
    }
  }
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(MQTT_CLIENT_ID))
    {
      Serial.println("connected");
      client.subscribe(MQTT_TOPIC_LED);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Convert a UID to an uppercase hex string without separators
String uidToString(const uint8_t *uid, uint8_t uidLength)
{
  String s;
  for (uint8_t i = 0; i < uidLength; i++)
  {
    if (uid[i] < 0x10)
      s += '0';
    s += String(uid[i], HEX);
  }
  s.toUpperCase();
  return s;
}

void checkNfc()
{
  uint8_t uid[7] = {0, 0, 0, 0, 0, 0, 0};
  uint8_t uidLength = 0;

  // Blocks for at most 100 ms, so MQTT keeps being serviced in loop().
  // Returns false when no card is present (timeout).
  if (!nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100))
    return;

  String uidStr = uidToString(uid, uidLength);
  unsigned long now = millis();

  // Ignore the same card being held against the reader
  if (uidStr == lastUid && (now - lastUidTime) < SAME_CARD_COOLDOWN)
  {
    lastUidTime = now; // keep extending while the card stays present
    return;
  }

  lastUid = uidStr;
  lastUidTime = now;

  Serial.print("Card ID HEX Value: ");
  Serial.println(uidStr);

  if (uidLength == 4)
  {
    uint32_t cardid = ((uint32_t)uid[0] << 24) | ((uint32_t)uid[1] << 16) |
                      ((uint32_t)uid[2] << 8) | uid[3];
    Serial.print("Card ID NUMERIC Value: ");
    Serial.println(cardid);
  }

  if (client.publish(MQTT_TOPIC_TAG, uidStr.c_str()))
    Serial.println("Published to MQTT");
  else
    Serial.println("MQTT publish failed");

  delay(DELAY_BETWEEN_CARDS);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Begin NFC532 -> MQTT software.");

  pinMode(LED_PIN, OUTPUT);

  setup_nfc();
  setup_wifi();

  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    setup_wifi();
  }

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  checkNfc();
}