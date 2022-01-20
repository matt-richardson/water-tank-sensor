#ifndef log_h
#define log_h

#include <ezTime.h>
#include <config.h>
#include <ESP8266HTTPClient.h>

const unsigned MAX_LOGS = 50;
String logs[MAX_LOGS];
unsigned long logTime[MAX_LOGS];
unsigned int numLogs = 0;

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
  log("NTP sync complete. Time is " + dateTime(ISO8601), "NTP sync complete. Time is {Time}.", "Time", dateTime(ISO8601));
  unsigned long millisecondsSinceBoot = millis();
  log("It has been " + String(millisecondsSinceBoot) + " milliseconds since boot", "It has been {MillisecondsSinceBoot} milliseconds since boot", "MillisecondsSinceBoot", String(millisecondsSinceBoot));

  HTTPClient http;
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure(); //todo: fix so it does proper validation
  http.begin(wifiClient, SEQ_URL "/api/events/raw");
  http.addHeader("Content-Type", "application/vnd.serilog.clef");
  http.addHeader("X-Seq-ApiKey", SEQ_API_KEY);

  String postData = "";
  for (int i=0; i < numLogs; i++) {
    Serial.println("TIME_NOW is " + String(TIME_NOW) + ". logTime[" + String(i) + "] is " + String(logTime[i]));
    time_t messageTime = TIME_NOW - millisecondsSinceBoot + logTime[i];
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
