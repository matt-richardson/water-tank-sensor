#ifndef log_h
#define log_h

#include <ezTime.h>

const unsigned MAX_LOGS = 50;
String logs[MAX_LOGS];
unsigned int numLogs = 0;

void log(String message, String messageTemplate = "", String value1Name = "", String value1 = "", String value2Name = "", String value2 = "")
{
  Serial.println(message);

  String postData = "{";
  postData = postData + "\"@t\":\"" + dateTime(ISO8601) + "\",";
  if (messageTemplate != "")
    postData = postData + "\"@mt\": \"" + messageTemplate + "\",";
  else 
    postData = postData + "\"@m\": \"" + message + "\",";
  postData = postData + "\"Application\": \"WaterTankSensor\",";
  postData = postData + "\"Environment\": \"Production\",";
  postData = postData + "\"Version\": \"" + VERSION_NUMBER + "\"";
  postData = postData + "\"LogNumber\": \"" + String(numLogs) + "\"";
  if (value1Name != "")
    postData = postData + ",\"" + value1Name + "\": \"" + value1 + "\"";
  if (value1Name != "")
    postData = postData + ",\"" + value2Name + "\": \"" + value2 + "\"";
  postData = postData + "}";
  logs[numLogs] = postData;
  numLogs++;
}

void flushLogs()
{
  HTTPClient http;
  WiFiClientSecure wifiClient;
  wifiClient.setInsecure(); //todo: fix so it does proper validation
  http.begin(wifiClient, SEQ_URL "/api/events/raw");
  http.addHeader("Content-Type", "application/vnd.serilog.clef");
  http.addHeader("X-Seq-ApiKey", SEQ_API_KEY);

  String postData = "";
  for (int i=0; i < numLogs; i++)
    postData = postData + logs[i] + "\n";

  Serial.print("Sending data: ");
  Serial.println(postData);

  int httpCode = http.POST(postData);
  Serial.println(httpCode); //Print HTTP return code
  String payload = http.getString();
  Serial.println(payload); //Print request response payload
  http.end(); //Close connection
}

#endif
