#include <HCSR04.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "config.h"
#include "httpUpdate.h"

HCSR04 hc(13, 12); // Initialize Pin D7, D6

//--------------------------------------------------
//https://www.bakke.online/index.php/2017/06/24/esp8266-wifi-power-reduction-avoiding-network-scan/
//--------------------------------------------------
// The ESP8266 RTC memory is arranged into blocks of 4 bytes. The access methods read and write 4 bytes at a time,
// so the RTC data structure should be padded to a 4-byte multiple.
struct {
  uint32_t crc32;   // 4 bytes
  uint8_t channel;  // 1 byte,   5 in total
  uint8_t bssid[6]; // 6 bytes, 11 in total
  uint8_t padding;  // 1 byte,  12 in total
} rtcData;

void ConnectWifi()
{
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  // Try to read WiFi settings from RTC memory
  bool rtcValid = false;
  if( ESP.rtcUserMemoryRead( 0, (uint32_t*)&rtcData, sizeof( rtcData ) ) ) {
    rtcValid = true;
  }

  if( rtcValid ) {
    Serial.println("The RTC data was good, making a quick connection");
    WiFi.begin( WLAN_SSID, WLAN_PASSWD, rtcData.channel, rtcData.bssid, true );
  }
  else {
    Serial.println("The RTC data was not valid, so make a regular connection");
    WiFi.begin( WLAN_SSID, WLAN_PASSWD );
  }

  int retries = 0;
  int wifiStatus = WiFi.status();
  while( wifiStatus != WL_CONNECTED ) {
    Serial.print(".");
    retries++;
    if( retries == 100 ) {
      Serial.println("Quick connect didn't work, resetting wifi and trying regular connection");
      WiFi.disconnect();
      delay( 10 );
      WiFi.forceSleepBegin();
      delay( 10 );
      WiFi.forceSleepWake();
      delay( 10 );
      WiFi.begin( WLAN_SSID, WLAN_PASSWD );
    }
    if( retries == 600 ) {
      Serial.println("Giving up after 30 seconds and going back to sleep");
      WiFi.disconnect( true );
      delay( 1 );
      WiFi.mode( WIFI_OFF );
      ESP.deepSleep( SLEEPTIME, WAKE_RF_DISABLED );
      return; // Not expecting this to be called, the previous call will never return.
    }
    delay( 50 );
    wifiStatus = WiFi.status();
  }

  Serial.println("success!");
  Serial.print("IP Address is: ");
  Serial.println(WiFi.localIP());

  // Write current connection info back to RTC
  rtcData.channel = WiFi.channel();
  memcpy( rtcData.bssid, WiFi.BSSID(), 6 ); // Copy 6 bytes of BSSID (AP's MAC address)
  ESP.rtcUserMemoryWrite( 0, (uint32_t*)&rtcData, sizeof( rtcData ) );

}
//--------------------------------------------------

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
  String postData = postDataPrefix + averageReading;
  Serial.print("Sending data: ");
  Serial.println(postData);
  auto httpCode = http.POST(postData);
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
      sum += array [i] ;
    }
  }
  return  ((float) sum) / nonZeroValues;
}

void setup()
{
  delay(1000);
  Serial.begin(115200);

  Serial.println();

  float readings[10];

  for (int i = 0; i < 10; i++) {
    readings[i] = hc.dist();
    Serial.println(readings[i]); // Print in centimeters the value from the sensor
    delay(250);
  }
  Serial.print("Average reading is ");
  float averageReading = average(readings, 10);
  Serial.println(averageReading);

  // tank sensor sits at 180cm
  // tank overflow is at 150cm

  float distanceFromBottomOfTank = 180 - averageReading;
  float percentageFull = (distanceFromBottomOfTank / 150) * 100;

  ConnectWifi();
  
  SendData(percentageFull);

  CheckForUpdate();

  Serial.println("Going into deep sleep mode for 30 seconds");
  ESP.deepSleep( SLEEPTIME, WAKE_RF_DISABLED );
}

void loop()
{
}
