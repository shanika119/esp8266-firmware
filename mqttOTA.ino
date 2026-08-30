#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// ---------- WiFi ----------
const char* ssid = "abc";
const char* password = "12345678";

// ---------- MQTT ----------
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* otaTopic = "yourname/esp8266/ota";

WiFiClient espClient;
PubSubClient client(espClient);

// ---------- OTA ----------
const char* firmwareURL = "https://raw.githubusercontent.com/yourusername/yourrepo/main/firmware.bin";

// ---------- LED ----------
#define LED_PIN LED_BUILTIN
unsigned long lastBlink = 0;
bool ledState = false;

void setup_wifi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("WiFi Connected");
}

void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)message[i];
  }

  Serial.print("Message on [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  if (String(topic) == otaTopic && msg == "update") {
    startOTA();
  }
}

void startOTA() {
  Serial.println("Starting OTA update...");

  WiFiClientSecure secureClient;
  secureClient.setInsecure(); // skip cert validation for GitHub raw link

  ESPhttpUpdate.rebootOnUpdate(true);

  t_httpUpdate_return result = ESPhttpUpdate.update(secureClient, firmwareURL);

  switch (result) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("OTA Failed (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("No update available");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("Update successful, rebooting...");
      break;
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = "ESP8266Client-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(otaTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 2 seconds");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // OFF

  setup_wifi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Non-blocking 5-second blink
  unsigned long now = millis();
  if (now - lastBlink >= 5000) {
    lastBlink = now;
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? LOW : HIGH); // active LOW
  }
}