
#include <ezTime.h>

//todo: batch logging and send at end
void log(String message, String messageTemplate = "", String value1Name = "", String value1 = "", String value2Name = "", String value2 = "")
{
  Serial.println(message);

  if (messageTemplate != "") {
      HTTPClient http;
      WiFiClientSecure wifiClient;
      wifiClient.setInsecure(); //todo: fix so it does proper validation
      http.begin(wifiClient, SEQ_URL "/api/events/raw");
      http.addHeader("Content-Type", "application/vnd.serilog.clef");
      http.addHeader("X-Seq-ApiKey", SEQ_API_KEY);
      String postData = String("{") +
                        "\"@t\":\"" + dateTime(ISO8601) + "\"," +
                        "\"@mt\": \"" + messageTemplate + "\"," +
                        "\"Application\": \"WaterTankSensor\"," +
                        "\"Environment\": \"Production\"," +
                        "\"Version\": \"" + VERSION_NUMBER + "\"";
      if (value1Name != "") {
          postData = postData + ",\"" + value1Name + "\": \"" + value1 + "\"";
      }
      if (value1Name != "") {
          postData = postData + ",\"" + value2Name + "\": \"" + value2 + "\"";
      }
      postData = postData + "}";
      Serial.print("Sending data: ");
      Serial.println(postData);
      auto httpCode = http.POST(postData);
      Serial.println(httpCode); //Print HTTP return code
      String payload = http.getString();
      Serial.println(payload); //Print request response payload
      http.end(); //Close connection
  }
}