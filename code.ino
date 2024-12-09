#include <TrueRMS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>

// WiFi credentials
const char* ssid = "your-ssid";
const char* password = "your-password";

// InfluxDB v2 parameters
const char* influxDBServer = "http://influxdb-ip:8086";
const char* influxDBToken = "influxdb-api-key";
const char* influxDBOrg = "your-influxdb-organisation";
const char* influxDBBucket = "your-influxdb-bucket";
const char* influxDBMeasurement = "power_meter";

#define BUFFER_SIZE 300  // Adjust the buffer size as needed
char influxDBUrl[BUFFER_SIZE];

#define loopPeriod 5000  // Adjust to 5000 milliseconds (5 seconds)
#define adcPin A0
#define rmsWindow 40
#define triggerOut 4
#define threshold 0.075
#define sensorGain 1
#define ampsPerVolt 30
#define adcMaxVoltage 3.3
#define adcResolution (adcMaxVoltage / 1024)
#define voltRange 3.3

unsigned long nextLoop;
Rms readRms;

ESP8266WebServer server(80);

void sendToInfluxDB(float currentRms, float power) {
  HTTPClient http;
  
  // Construct the InfluxDB URL with organization and bucket
  snprintf(influxDBUrl, BUFFER_SIZE, "%s/api/v2/write?org=%s&bucket=%s", influxDBServer, influxDBOrg, influxDBBucket);
  
  WiFiClient client;  // Create a WiFiClient object

  http.begin(client, influxDBUrl); // Use WiFiClient object with HTTPClient::begin

  // Construct the InfluxDB line protocol data
  char data[BUFFER_SIZE];
  snprintf(data, BUFFER_SIZE, "%s,device=esp8266 current=%.2f,power=%.2f", influxDBMeasurement, currentRms, power);

  http.addHeader("Authorization", String("Token ") + influxDBToken);
  http.addHeader("Content-Type", "text/plain; charset=utf-8");

  int httpCode = http.POST(data);

  if (httpCode == 204) {
    Serial.println("Data sent to InfluxDB successfully");
  } else {
    Serial.print("Failed to send data to InfluxDB. HTTP code: ");
    Serial.println(httpCode);
  }

  http.end();
}

void setup() {
  Serial.begin(9600);
  pinMode(triggerOut, OUTPUT);
  digitalWrite(triggerOut, 0);

  // Configure for automatic base-line restoration and continuous scan mode
  readRms.begin(voltRange, rmsWindow, ADC_10BIT, BLR_ON, CNT_SCAN);
  readRms.start();
  nextLoop = micros() + loopPeriod;

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Print the ESP8266 IP address
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Define web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.begin();
}

void loop() {
  server.handleClient(); // Handle web server requests

  static int lapsedIntervals = 0;
  int adcVal = analogRead(adcPin);
  readRms.update(adcVal);

  lapsedIntervals++;
  if (lapsedIntervals >= 1000) {
    readRms.publish();
    float rmsVal = readRms.rmsVal;
    float currentRms = (rmsVal / sensorGain) * ampsPerVolt;
    float power = currentRms * 240.0; // Assuming a constant voltage of 240V

    // Send data to InfluxDB
    sendToInfluxDB(currentRms, power);

    if (currentRms >= threshold) {
      digitalWrite(triggerOut, 1);
    } else {
      digitalWrite(triggerOut, 0);
    }

    Serial.print("ADC Reading: ");
    Serial.println(adcVal);
    Serial.print("RMS Voltage: ");
    Serial.print(rmsVal / sensorGain * 1000, 2);  // Show raw rms voltage (mV) on ADC
    Serial.print("mV ");
    Serial.print("Actual Current: ");
    Serial.print(currentRms * 1000, 2);  // Show actual current (mA) with 2 decimal places
    Serial.print("mA ");
    Serial.print("Power: ");
    Serial.print(power, 2);  // Show power in watts with 2 decimal places
    Serial.println("W");

    lapsedIntervals = 0;
  }

  while (nextLoop > micros());
  nextLoop += loopPeriod;
}

void handleRoot() {
  float rmsVal = readRms.rmsVal;
  float currentRms = (rmsVal / sensorGain) * ampsPerVolt;
  float power = currentRms * 240.0;

  String html = "<html><body>";
  html += "<h1>Power Meter</h1>";
  html += "<p>RMS Current: " + String(currentRms, 2) + " A</p>";
  html += "<p>Power: " + String(power, 2) + " W</p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}
