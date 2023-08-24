/**
   httpUpdate.ino
   Created on: 27.11.2015
   Sourced from: https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266httpUpdate/examples/httpUpdate/httpUpdate.ino
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include "log.h"

ESP8266WiFiMulti WiFiMulti;

void update_started() {
  Serial.println("CALLBACK:  HTTP update process started");
  log("OTA Update: HTTP update process started");
}

void update_finished() {
  Serial.println("CALLBACK:  HTTP update process finished");
  log("OTA Update: HTTP update process finished");
  flushLogs();
}

void update_progress(int cur, int total) {
  Serial.printf("CALLBACK:  HTTP update process at %d of %d bytes...\n", cur, total);
  //log("OTA Update: HTTP update process at " + String(cur) + "of " + String(total) + " bytes...", "HTTP update process {CurrentBytes} of {TotalBytes}", "CurrentBytes", String(cur), "TotalBytes", String(total));
}

void update_error(int err) {
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
  //log("OTA Update: HTTP update fatal error code " + err, "HTTP update process failed with {ErrorCode}", "ErrorCode", String(err));
}

void CheckForUpdate() {

  log("OTA Update: Checking for update", true);

  // The line below is optional. It can be used to blink the LED on the board during flashing
  // The LED will be on during download of one buffer of data from the network. The LED will
  // be off during writing that buffer to flash
  // On a good connection the LED should flash regularly. On a bad connection the LED will be
  // on much longer than it will be off. Other pins than LED_BUILTIN may be used. The second
  // value is used to put the LED on. If the LED is on with HIGH, that value should be passed
  ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);

  // Add optional callback notifiers
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);
  ESPhttpUpdate.closeConnectionsOnUpdate(false);

  WiFiClientSecure wifiClient;
  wifiClient.setInsecure(); //todo: fix so it does proper validation
  ESPhttpUpdate.setAuthorization(OTA_USERNAME, OTA_PASSWORD);
  t_httpUpdate_return ret = ESPhttpUpdate.update(wifiClient, OTA_ENDPOINT "/update", VERSION_NUMBER);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      // log("OTA Update: HTTP update failed with error code " + String(ESPhttpUpdate.getLastError()) + ": " + ESPhttpUpdate.getLastErrorString(), "HTTP update process failed with {ErrorCode}: {ErrorDescription}", "ErrorCode", String(ESPhttpUpdate.getLastError()), "ErrorDescription", ESPhttpUpdate.getLastErrorString());
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      log("OTA Update: No updates available");
      break;

    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      log("OTA Update: Update successful");
      break;

    default:
      Serial.println("HTTP_UPDATE_UNKNOWN");
      log("OTA Update: Unknown response");
      break;

  }

  log("OTA Update: Finished");

  flushLogs();
}
