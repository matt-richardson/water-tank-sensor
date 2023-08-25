#ifndef log_h
#define log_h
;
#include <ezTime.h>
#include <config.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoQueue.h>

const int MAX_LOGS = 50;

struct LogEntry {
    unsigned long time;
    String message;
};

void flushLogs();
void log(String message, String messageTemplate, String value1Name, String value1, String value2Name, String value2, bool immediate = false);
void log(String message, String messageTemplate, String value1Name, String value1, bool immediate = false);
void log(String message, bool immediate = false);

ArduinoQueue<LogEntry> logs(50);
bool ntpSyncComplete = false;
unsigned long millisecondsSinceBoot = millis();

String formatLogEntry(LogEntry logEntry) {
  unsigned long timeNow = now();
  time_t messageTime = timeNow - (millisecondsSinceBoot / 1000) + (logEntry.time / 1000);
  String formattedLogEntry = "";
  formattedLogEntry = formattedLogEntry + "{" + "\"@t\":\"" + dateTime(messageTime, ISO8601) + "\"" + logEntry.message + "}\n";
  return formattedLogEntry;
}

int postDataToSeq(String postData) {
  HTTPClient http;
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure(); //todo: fix so it does proper validation
  http.begin(wifiClient, SEQ_URL "/api/events/raw");
  http.addHeader("Content-Type", "application/vnd.serilog.clef");
  http.addHeader("X-Seq-ApiKey", SEQ_API_KEY);

  int httpCode = http.POST(postData);
  Serial.println("Seq returned http code " + String(httpCode));
  String payload = http.getString();
  Serial.println("Seq returned payload: " + payload); 
  http.end();
  return httpCode;
}

void log(String message, String messageTemplate, String value1Name, String value1, String value2Name, String value2, bool immediate /*= false*/) {
  LogEntry logEntry;

  logEntry.time = millis();

  Serial.println(message);

  //these are not valid json strings - they need modification before publishing
  //we dont always have a valid time (ie, before we have connected to wifi, and we haven't done an ntp sync)
  //so we cant add a valid time to it yet - we do that when we publish the message. 
  String postData = "";
  if (messageTemplate != "")
    postData = postData + ",\"@mt\": \"" + messageTemplate + "\"";
  else 
    postData = postData + ",\"@mt\": \"" + message + "\"";
  postData = postData + ",\"Application\": \"WaterTankSensor\"";
  postData = postData + ",\"Environment\": \"Production\"";
  postData = postData + ",\"Version\": \"" + VERSION_NUMBER + "\"";
  if (value1Name != "")
    postData = postData + ",\"" + value1Name + "\": \"" + value1 + "\"";
  if (value2Name != "")
    postData = postData + ",\"" + value2Name + "\": \"" + value2 + "\"";
  logEntry.message = postData;
  logs.enqueue(logEntry);
  if (immediate) {
    flushLogs();
  }
}

void log(String message, String messageTemplate, String value1Name, String value1, bool immediate /*= false*/) {
  log(message, messageTemplate, value1Name, value1, /*value2Name:*/ "", /*value2:*/ "", immediate);
}

void log(String message, bool immediate /*= false*/) {
  log(message, /*messageTemplate:*/ "", /*value1Name:*/ "", /*value1:*/ "", immediate);
}

void syncTime() {
  if (!ntpSyncComplete) {
    log("Waiting for NTP sync before publishing logs");
    waitForSync(); //todo: this can block forever if there's no ip address
    log("NTP sync complete. Time is " + dateTime(ISO8601) + ". It has been " + String(millisecondsSinceBoot) + " milliseconds since boot", 
        "NTP sync complete. Time is {Time}. It has been {MillisecondsSinceBoot} milliseconds since boot", 
        "Time", dateTime(ISO8601), 
        "MillisecondsSinceBoot", String(millisecondsSinceBoot));
    ntpSyncComplete = true;
  }
}

void flushLogs() {
  syncTime();

  String postData = "";
  while(!logs.isEmpty()) {
    LogEntry logEntry = logs.dequeue(); 
    String formattedLogEntry = formatLogEntry(logEntry);
    postData = postData + formattedLogEntry;
  }
  if (postData != "") {
    Serial.printf("Sending data to %s\n", SEQ_URL "/api/events/raw");
    Serial.println(postData);

    bool retry;
    int attemptCount = 0;
    do {
      retry = false;
      int returnCode = postDataToSeq(postData);
      if (returnCode < 0 && attemptCount++ < 5) {
        Serial.printf("Sending data to seq failed with %d. Retrying.\n", returnCode);
        retry = true;
      }
    } while (retry);
  } else {
    Serial.println("Nothing to send to seq...");
  }
}

#endif
