//https://www.bakke.online/index.php/2017/06/24/esp8266-wifi-power-reduction-avoiding-network-scan/

#include <ESP8266WiFi.h>
#include "log.h"

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
  log("Connecting to " + String(WLAN_SSID), "Connecting to {SSID}", "SSID", WLAN_SSID);
  // Try to read WiFi settings from RTC memory
  bool rtcValid = false;
  if( ESP.rtcUserMemoryRead( 0, (uint32_t*)&rtcData, sizeof( rtcData ) ) ) {
    rtcValid = true;
  }

  if( rtcValid ) {
    log("The RTC data was good, making a quick connection");
    WiFi.begin( WLAN_SSID, WLAN_PASSWD, rtcData.channel, rtcData.bssid, true );
  }
  else {
    log("The RTC data was not valid, so make a regular connection");
    WiFi.begin( WLAN_SSID, WLAN_PASSWD );
  }

  int retries = 0;
  int wifiStatus = WiFi.status();
  while( wifiStatus != WL_CONNECTED ) {
    Serial.print(".");
    retries++;
    if( retries == 100 ) {
      log("Quick connect didn't work, resetting wifi and trying regular connection");
      WiFi.disconnect();
      delay( 10 );
      WiFi.forceSleepBegin();
      delay( 10 );
      WiFi.forceSleepWake();
      delay( 10 );
      WiFi.begin( WLAN_SSID, WLAN_PASSWD );
    }
    if( retries == 600 ) {
      log("Giving up after 30 seconds and going back to sleep");
      WiFi.disconnect( true );
      delay( 1 );
      WiFi.mode( WIFI_OFF );
      ESP.deepSleep( SLEEPTIME, WAKE_RF_DISABLED );
      return; // Not expecting this to be called, the previous call will never return.
    }
    delay( 50 );
    wifiStatus = WiFi.status();
  }

  log("Successfully connected to wifi!");
  String ipAddress = WiFi.localIP().toString();
  log("IP address is " + ipAddress, "IP Address is {IPAddress}", "IPAddress", ipAddress);

  // Write current connection info back to RTC
  rtcData.channel = WiFi.channel();
  memcpy( rtcData.bssid, WiFi.BSSID(), 6 ); // Copy 6 bytes of BSSID (AP's MAC address)
  ESP.rtcUserMemoryWrite( 0, (uint32_t*)&rtcData, sizeof( rtcData ) );
}