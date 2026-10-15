#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESPSupabase.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "weatherData.h"
#include "weatherIcon.h"

// Track execution times (in milliseconds)
unsigned long lastDisplayTime = 0;
unsigned long lastDatabaseTime = 0;

const unsigned long DISPLAY_INTERVAL = 10000;    // 10 seconds
const unsigned long DATABASE_INTERVAL = 3600000; // 1 hour 3600000

bool firstDatabasePostDone = false;

#define ONE_WIRE_BUS 2 // D4 Pin FOR SENSOR
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// IdeaSpark OLED Configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SCREEN_SDA 12                                             // D6 Pin FOR OLED
#define SCREEN_SCL 14                                             // D5 Pin FOR OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // -1 means no reset pin

// Supabase and WiFi Configuration
Supabase db;
String supabase_url = "";
String anon_key = "";
String tableTemperature = "";
String tableHumidity = "";
String ssid = "";
String password = "";
bool upsert = false;

weatherData meteoApiData("");

float rawLow = -0.5;
float refLow = 0;
float rawHigh = 96.9;
float refHigh = 99.9;

float rawRange = rawHigh - rawLow;
float refRange = refHigh - refLow;
float additionalCorrection = 2.4;

float tempCorrection(float rawTemp);
void printTemperature(float tempC);
void displayTemperature(float tempC);
void printHumidity(int humidity);
void displayHumidity(int humidity);
void getAndSendDataToSupabase(float tempC, String tableTemperature, String tableHumidity, bool upsert);
bool looseWireDetected(float tempC);
bool humidityOutOfRange(int humidity);

void setup()
{
  Serial.begin(115200);
  sensors.begin();

  Wire.begin(SCREEN_SDA, SCREEN_SCL); // Initialize I2C for OLED

  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1); // Short pause to let the radio shut down cleanly

  // Initialize the built-in OLED (Address 0x3C is standard for SSD1306)
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("OLED allocation failed"));
    for (;;)
      ;
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
}

void loop()
{
  unsigned long currentMillis = millis();

  // TASK 1: Update the Screen (Every 10 Seconds)
  if (currentMillis - lastDisplayTime >= DISPLAY_INTERVAL || lastDisplayTime == 0)
  {
    lastDisplayTime = currentMillis;

    sensors.requestTemperatures();
    float rawTemp = sensors.getTempCByIndex(0);
    float tempC = tempCorrection(rawTemp);

    printTemperature(tempC);
    printHumidity(meteoApiData.getHumidity());

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("ARDILLA'S SENSOR");
    displayTemperature(tempC);
    displayHumidity(meteoApiData.getHumidity());
    drawWeatherIcon(meteoApiData.getWeatherCode(), 100, 32);
    drawWeatherIcon(meteoApiData.getWorseWeatherCode(), 100, 0);
    display.display();
    Serial.println("Screen updated.");
  }

  // TASK 2: Send to Supabase (First Boot + Every Hour)
  if (!firstDatabasePostDone)
  {
    delay(5000);
  }
  if (!firstDatabasePostDone || (currentMillis - lastDatabaseTime >= DATABASE_INTERVAL))
  {
    lastDatabaseTime = currentMillis;

    sensors.requestTemperatures();
    float originalTemp = sensors.getTempCByIndex(0);
    float currentTemp = tempCorrection(originalTemp);
    getAndSendDataToSupabase(currentTemp, tableTemperature, tableHumidity, upsert);
  }
}

void printTemperature(float tempC)
{
  if (looseWireDetected(tempC))
  {
    Serial.println("Error: Sensor not connected or wires are loose.");
  }
  else
  {
    Serial.print("Temperature: ");
    Serial.print(tempC, 1);
    Serial.println(" °C");
  }
}

void printHumidity(int humidity)
{
  if (humidityOutOfRange(humidity))
  {
    Serial.println("Error: Invalid humidity value.");
  }
  else
  {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  }
}

void displayTemperature(float tempC)
{
  display.setTextSize(2);
  display.setCursor(0, 25);

  if (looseWireDetected(tempC))
  {
    display.print("ERR: WIRE");
  }
  else
  {
    display.print(tempC, 1);
    display.print(" C");
  }
}

void displayHumidity(int humidity)
{
  display.setTextSize(1);
  display.setCursor(0, 50);
  if (humidityOutOfRange(humidity))
  {
    display.print("ERR: HUMIDITY");
  }
  else
  {

    display.print(humidity);
    display.print(" %");
  }
}

void getAndSendDataToSupabase(float tempC, String tableTemperature, String tableHumidity, bool upsert)
{
  if (looseWireDetected(tempC))
  {
    Serial.println("Error: Sensor not connected or wires are loose. Not sending to Supabase.");
    return;
  }

  Serial.println("Waking up Wi-Fi...");
  WiFi.forceSleepWake();
  delay(1);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int timeoutCounter = 0;
  while (WiFi.status() != WL_CONNECTED && timeoutCounter < 20)
  {
    delay(500);
    Serial.print(".");
    timeoutCounter++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    firstDatabasePostDone = true;
    Serial.println("Wi-Fi connected! Getting data from open-meteo...");
    meteoApiData.setExternalWeatherData();
    Serial.println("Sending data to Supabase...");
    String JSONTemperature = "{\"temperature\": " + String(tempC, 1) + "}";
    String JSONHumidity = "{\"humidity_percent\": " + String(meteoApiData.getHumidity()) + "}";
    db.begin(supabase_url, anon_key);
    int codeTemperature = db.insert(tableTemperature, JSONTemperature, upsert);
    int codeHumidity = db.insert(tableHumidity, JSONHumidity, upsert);

    Serial.println("Temperature to Supabase Code: " + String(codeTemperature));
    Serial.println("Humidity to Supabase Code: " + String(codeHumidity));
    db.urlQuery_reset();
  }

  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
  Serial.println("Wi-Fi powered down. Back to low-power mode.");
  Serial.println("--------------------------------------------------");
  return;
}

bool looseWireDetected(float tempC)
{
  return tempC == -127.00;
}

float tempCorrection(float rawTemp)
{
  return (((rawTemp - rawLow) * refRange) / rawRange) + refLow + additionalCorrection;
}

bool humidityOutOfRange(int humidity)
{
  return humidity <= 0 || humidity > 100;
}