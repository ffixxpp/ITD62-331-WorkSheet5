#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

const char* SSID = "pinsudaa";
const char* PASSWORD = "123456zx";

const char* SERVER_URL = "http://172.20.10.5:3000/server";

const int DHT_PIN = D4;
DHT sensor(DHT_PIN, DHT11);

const long GMT_OFFSET_SEC = 25200; 
const int NTP_REFRESH_INTERVAL_MS = 60000; 
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", GMT_OFFSET_SEC, NTP_REFRESH_INTERVAL_MS);

WiFiClient networkClient;
HTTPClient webClient;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  sensor.begin();
  timeClient.begin();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi เชื่อมต่อสำเร็จ");
}

void loop() {
  static unsigned long lastUpdateTimestamp = 0;
  const unsigned long UPDATE_DELAY_MS = 10000; 

  if ((millis() - lastUpdateTimestamp) > UPDATE_DELAY_MS) {
    timeClient.update();

    float humidity = sensor.readHumidity();
    float temperature = sensor.readTemperature();

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("ไม่สามารถอ่านข้อมูลจากเซ็นเซอร์ DHT");
    } else {
    
      Serial.printf("ความชื้น: %.2f%%\n", humidity);
      Serial.printf("อุณหภูมิ: %.2f°C\n", temperature);

      time_t now = timeClient.getEpochTime();
      struct tm* timeDetails = gmtime(&now);
      char formattedTime[25];
      strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", timeDetails);

      StaticJsonDocument<300> jsonDoc;
      JsonObject sensorData = jsonDoc.createNestedObject(formattedTime);
      sensorData["humidity"] = humidity;
      sensorData["temperature"] = temperature;

      String jsonData;
      serializeJson(jsonDoc, jsonData);
      webClient.begin(networkClient, SERVER_URL);
      webClient.addHeader("Content-Type", "application/json");
      int httpResponseCode = webClient.PATCH(jsonData);

      if (httpResponseCode > 0) {
        String response = webClient.getString();
        Serial.printf("รหัสการตอบกลับ HTTP: %d\n", httpResponseCode);
        Serial.println("Payload ที่ได้รับ:");
        Serial.println(response);
      } else {
        Serial.printf("ข้อผิดพลาดในการส่ง PATCH: %d\n", httpResponseCode);
      }

      webClient.end();
    }

    lastUpdateTimestamp = millis();
  }
}
