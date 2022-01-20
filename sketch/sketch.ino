#include <HCSR04.h>
#include <ESP8266HTTPClient.h>
#include "config.h"
#include "httpUpdate.h"
#include "log.h"
#include "wifi.h"

HCSR04 hc(13, 12); // Initialize Pin D7, D6

void SendData(float averageReading)
{
  Serial.println("Sending data to io.adafruit.com");

  HTTPClient http;
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure(); //todo: fix so it does proper validation

  http.begin(wifiClient, "https://io.adafruit.com/api/v2/" IO_USERNAME "/feeds/" IO_FEEDNAME "/data");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("X-AIO-Key", IO_KEY);
  String postDataPrefix = "value=";
  String postData = postDataPrefix + String(averageReading);
  Serial.print("Sending data: ");
  Serial.println(postData);
  int httpCode = http.POST(postData);
  Serial.println(httpCode); //Print HTTP return code
  String payload = http.getString();
  Serial.println(payload); //Print request response payload
  http.end(); //Close connection
}

float average (float * array, int len)
{
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

void setup()
{
  delay(1000);
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

  Serial.println();

  float readings[10];

  for (int i = 0; i < 10; i++) {
    readings[i] = hc.dist();
    char buffer[60];
    sprintf(buffer, "Water level (reading %d) is %f cm from the sensor", i, readings[i]);
    log(buffer, "Water level (reading {ReadingNumber}) is {ReadingValue} cm from the sensor", "ReadingNumber", String(i), "ReadingValue", String(readings[i]));
    delay(250);
  }
  float averageReading = average(readings, 10);
  log("Average reading is " + String(averageReading), "Average reading is {AverageValue} cm from the sensor", "AverageValue", String(averageReading));

  float distanceFromBottomOfTank = TANK_SENSOR_HEIGHT_IN_CM - averageReading;
  float percentageFull = (distanceFromBottomOfTank / TANK_OVERFLOW_HEIGHT_IN_CM) * 100;

  log("Tank is " + String(percentageFull) + String("% full"), "Tank is {PercentFull}% full", "PercentFull", String(percentageFull));

  if (percentageFull < 0)
    log("Value is less than 0. Not going to publish the data.");
  else
    SendData(percentageFull);
  flushLogs();

  CheckForUpdate();
  log("Going into deep sleep mode for " + String(SLEEPTIME / 1000000) + String(" seconds"), "Going into deep sleep mode for {SleepTime} seconds", "SleepTime", String(SLEEPTIME / 1000000));
  ESP.deepSleep( SLEEPTIME, WAKE_RF_DISABLED );
}

void loop()
{
}
