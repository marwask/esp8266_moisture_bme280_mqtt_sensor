#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "config.h"

#define SLEEPTIME     120 * 1000000

Adafruit_BME280 bme;
unsigned long delayTime;

IPAddress staticIP(192, 168, 0, 18);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

long lastReconnectAttempt = 0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

boolean reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(mqttClientName, mqttUser, mqttPass)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(50);
    }
  }
  return mqttClient.connected();
}

void sendValues(float moisture) {
  mqttClient.publish(mqttTemperatureTopic, String(bme.readTemperature()).c_str());
  mqttClient.publish(mqttHumidityTopic, String(bme.readHumidity()).c_str());
  mqttClient.publish(mqttPressureTopic, String(bme.readPressure() / 100.0F).c_str());
  mqttClient.publish(mqttMoistureTopic, String(moisture).c_str());
}

void printValues(float moisture) {
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  Serial.print("Moisture = ");
  Serial.print(moisture);
  Serial.println(" %");
  Serial.println(analogRead(A0));

  Serial.println();
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  unsigned status;
  status = bme.begin(0x76);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    while (1) delay(10);
  }

  delayTime = 30000;
  Serial.println();

  printValues(0);

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(staticIP, gateway, subnet);
  while (WiFi.status() != WL_CONNECTED) {
    //    while(WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(100);
    Serial.print(WiFi.status());
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  mqttClient.setServer(mqttServer, 10883);
  if (!mqttClient.connected()) {
    reconnect();
  }

  //    ESP.deepSleep(SLEEPTIME, WAKE_RF_DISABLED);
}

void loop() {
  if (!mqttClient.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 60000 || lastReconnectAttempt == 0) {
      lastReconnectAttempt = now;
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  }
  mqttClient.loop();

  float moisture = map(analogRead(A0), 500, 1024, 100, 0); // 480
  printValues(moisture);
  sendValues(moisture);

  delay(delayTime);
}

