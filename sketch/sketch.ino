#include <HCSR04.h>
#include <ESP8266HTTPClient.h>
#include "config.h"
#include "log.h"
#include "wifi.h"
#include "httpUpdate.h"
#include <ArduinoHA.h>


HCSR04 hc(13, 12);  // Initialize Pin D7, D6

WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);
HASensorNumber waterTankSensor("WaterTankLevel");
unsigned long lastWaterLevelCheckTime = 0;
unsigned long lastUpdateCheckTime = 0;

void sendDataToIoAdafruitCom(float percentageFull) {
  if (percentageFull < 0) {
    log("Value is less than 0. Not going to publish the data to io.adafruit.com.");
    return;
  }
  else if (percentageFull > 100) {
    log("Value is greater than 100. Not going to publish the data to io.adafruit.com.");
    return;
  }

  Serial.println("Sending data to io.adafruit.com");

  HTTPClient http;
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure(); //todo: fix so it does proper validation

  http.begin(wifiClient, "https://io.adafruit.com/api/v2/" IO_USERNAME "/feeds/" IO_FEEDNAME "/data");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("X-AIO-Key", IO_KEY);
  String postDataPrefix = "value=";
  String postData = postDataPrefix + String(percentageFull);
  Serial.print("Sending data: ");
  Serial.println(postData);
  int httpCode = http.POST(postData);
  Serial.println("io.adafruit.com returned http code " + String(httpCode));
  String payload = http.getString();
  Serial.println("io.adafruit.com returned payload: " + payload); 
  http.end(); //Close connection
}

void sendDataToHomeAssistant(float percentageFull) {
  if (percentageFull < 0) {
    log("Value is less than 0. Not going to publish the data to Home Assistant.");
    return;
  }
  else if (percentageFull > 100) {
    log("Value is greater than 100. Not going to publish the data to Home Assistant.");
    return;
  }
  log("Setting Home Assistant water tank sensor value to " + String(percentageFull), "Setting Home Assistant water tank sensor value to {SensorValue}%", "SensorValue", String(percentageFull));

  waterTankSensor.setValue(percentageFull);
}

void sendData(float percentageFull) {
  sendDataToIoAdafruitCom(percentageFull);
  sendDataToHomeAssistant(percentageFull);
}

float average (float * array, int len) {
  float sum = 0;
  int nonZeroValues = 0;
  for (int i = 0 ; i < len ; i++)
  {
    if (array[i] > 0) {
      nonZeroValues++;
      sum += array [i];
    }
  }
  if (nonZeroValues == 0)
    return -1;
  return  ((float) sum) / nonZeroValues;
}

void onMqttConnected() {
  log("MQTT is connected");
}

void calculateWaterLevel() {
  float readings[READINGS_TO_TAKE];

  for (int i = 0; i < READINGS_TO_TAKE; i++) {
    readings[i] = hc.dist();
    char buffer[60];
    sprintf(buffer, "Water level (reading %d) is %f cm from the sensor", i, readings[i]);
    log(buffer, "Water level (reading {ReadingNumber}) is {ReadingValue} cm from the sensor", "ReadingNumber", String(i + 1), "ReadingValue", String(readings[i]));
    delay(DELAY_BETWEEN_READINGS_IN_MS);
  }
  float averageReading = average(readings, READINGS_TO_TAKE);
  if (averageReading == -1) {
     log("Average reading was unable to be calculated");
  }
  log("Average reading is " + String(averageReading), "Average reading is {AverageValue} cm from the sensor", "AverageValue", String(averageReading));

  float distanceFromBottomOfTank = TANK_SENSOR_HEIGHT_IN_CM - averageReading;
  float percentageFull = (distanceFromBottomOfTank / TANK_OVERFLOW_HEIGHT_IN_CM) * 100;

  log("Tank is " + String(percentageFull) + String("% full"), "Tank is {PercentFull}% full", "PercentFull", String(percentageFull));

  sendData(percentageFull);

  log("Finished checking water level.");
}

void setup() {
  Serial.begin(115200);

  ConnectWifi();

  Serial.println();

  log("WLAN_SSID: " WLAN_SSID, "Config entry {ConfigName} = {ConfigValue}", "ConfigName", "WLAN_SSID", "ConfigValue", WLAN_SSID);
  log("SEQ_URL: " SEQ_URL, "Config entry {ConfigName} = {ConfigValue}", "ConfigName", "SEQ_URL", "ConfigValue", SEQ_URL);
  log("IO_USERNAME: " IO_USERNAME, "Config entry {ConfigName} = {ConfigValue}", "ConfigName", "IO_USERNAME", "ConfigValue", IO_USERNAME);
  log("IO_FEEDNAME: " IO_FEEDNAME, "Config entry {ConfigName} = {ConfigValue}", "ConfigName", "IO_FEEDNAME", "ConfigValue", IO_FEEDNAME);
  log("OTA_ENDPOINT: " OTA_ENDPOINT, "Config entry {ConfigName} = {ConfigValue}", "ConfigName", "OTA_ENDPOINT", "ConfigValue", OTA_ENDPOINT);
  log("OTA_USERNAME: " OTA_USERNAME, "Config entry {ConfigName} = {ConfigValue}", "ConfigName", "OTA_USERNAME", "ConfigValue", OTA_USERNAME);
  log("VERSION_NUMBER: " VERSION_NUMBER, "Config entry {ConfigName} = {ConfigValue}", "ConfigName", "VERSION_NUMBER", "ConfigValue", VERSION_NUMBER);
  log("MQTT_BROKER_ADDR: " MQTT_BROKER_ADDR, "Config entry {ConfigName} = {ConfigValue}", "ConfigName", "MQTT_BROKER_ADDR", "ConfigValue", MQTT_BROKER_ADDR);
  log("MQTT_BROKER_PORT: " + String(MQTT_BROKER_PORT), "Config entry {ConfigName} = {ConfigValue}", "ConfigName", "MQTT_BROKER_PORT", "ConfigValue", String(MQTT_BROKER_PORT));
  log("MQTT_BROKER_USER: " MQTT_BROKER_USER, "Config entry {ConfigName} = {ConfigValue}", "ConfigName", "MQTT_BROKER_USER", "ConfigValue", MQTT_BROKER_USER);

  Serial.println();

  // configure HomeAssistant device
  byte mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  device.setUniqueId(mac, sizeof(mac));
  device.setName("Water Tank");
  device.setSoftwareVersion(VERSION_NUMBER);
  device.enableSharedAvailability();
  device.enableLastWill();

  // configure HomeAssistant sensor
  waterTankSensor.setIcon("mdi:water");
  waterTankSensor.setName("Water Level");
  waterTankSensor.setUnitOfMeasurement("%");

  mqtt.onConnected(onMqttConnected);

  log("Calling mqtt.begin()", true);
  mqtt.begin(MQTT_BROKER_ADDR, MQTT_BROKER_PORT, MQTT_BROKER_USER, MQTT_BROKER_PASS);
  log("Called mqtt.begin()", true);
    
  // it doesn't connect until the first `loop()` call
  log("Calling mqtt.loop()", true);
  mqtt.loop();
  log("Called mqtt.loop()", true);
}

void loop()
{
  log("loop()", true);
  mqtt.loop();

  if (mqtt.isConnected()) {
    log("mqtt is connected. yay.", true);
    calculateWaterLevel();
    CheckForUpdate();
    flushLogs();

    delay(1000);

    flushLogs();
    delay(1000);

    log("my work here is done. sleeping.", true);
    ESP.deepSleep( SLEEPTIME_IN_MINUTES * 60 * 1000000, WAKE_RF_DISABLED );
  } else {
    log("mqtt is not connected. boo.", true);
  }

  delay(2000);
}
