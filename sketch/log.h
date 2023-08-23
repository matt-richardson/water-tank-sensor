#ifndef log_h
#define log_h

#include <ezTime.h>
#include <config.h>
#include <ESP8266HTTPClient.h>

const int MAX_LOGS = 50;
String logs[MAX_LOGS];
unsigned long logTime[MAX_LOGS];
int numLogs = 0;

void log(String message, String messageTemplate = "", String value1Name = "", String value1 = "", String value2Name = "", String value2 = "")
{
  Serial.println(message);
  logTime[numLogs] = millis();

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
  postData = postData + ",\"LogNumber\": \"" + String(numLogs) + "\"";
  if (value1Name != "")
    postData = postData + ",\"" + value1Name + "\": \"" + value1 + "\"";
  if (value2Name != "")
    postData = postData + ",\"" + value2Name + "\": \"" + value2 + "\"";
  logs[numLogs] = postData;
  numLogs++;
}

void flushLogs()
{
  log("Waiting for NTP sync");
  waitForSync();
  unsigned long millisecondsSinceBoot = millis();
  log("NTP sync complete. Time is " + dateTime(ISO8601) + ". It has been " + String(millisecondsSinceBoot) + " milliseconds since boot", 
      "NTP sync complete. Time is {Time}. It has been {MillisecondsSinceBoot} milliseconds since boot", 
      "Time", dateTime(ISO8601), 
      "MillisecondsSinceBoot", String(millisecondsSinceBoot));

  HTTPClient http;
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure(); //todo: fix so it does proper validation
  http.begin(wifiClient, SEQ_URL "/api/events/raw");
  http.addHeader("Content-Type", "application/vnd.serilog.clef");
  http.addHeader("X-Seq-ApiKey", SEQ_API_KEY);

  String postData = "";
  unsigned long timeNow = now();
  for (int i=0; i < numLogs; i++) {
    time_t messageTime = timeNow - (millisecondsSinceBoot / 1000) + (logTime[i] / 1000);
    postData = postData + "{" + "\"@t\":\"" + dateTime(messageTime, ISO8601) + "\"" + logs[i] + "}\n";
  }

  Serial.print("Sending data: ");
  Serial.println(postData);

  int httpCode = http.POST(postData);
  Serial.println("Seq return http code " + String(httpCode));
  String payload = http.getString();
  Serial.println("Seq returned: " + payload); 
  http.end();
}

#endif
