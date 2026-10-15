#include "weatherData.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

weatherData::weatherData(String weatherApiUrl)
{
    this->weatherApiUrl = weatherApiUrl;
    this->humidity = 0;
    this->weatherCode = 0;
    this->worseWeatherCode = 0;
    this->success = false;
}

int weatherData::getHumidity() const
{
    return this->humidity;
}

int weatherData::getWeatherCode() const
{
    return this->weatherCode;
}

int weatherData::getWorseWeatherCode() const
{
    return this->worseWeatherCode;
}

bool weatherData::getSuccess() const
{
    return this->success;
}

void weatherData::setWeatherData(int humidity, int weatherCode, int worseWeatherCode, bool success)
{
    this->humidity = humidity;
    this->weatherCode = weatherCode;
    this->worseWeatherCode = worseWeatherCode;
    this->success = success;
}

void weatherData::setSuccess(bool success)
{
    this->success = success;
}

void weatherData::setExternalWeatherData()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("API Error: Wi-Fi not connected.");
        setSuccess(false);
        return;
    }

    WiFiClientSecure client;
    client.setInsecure();
    client.setBufferSizes(1024, 1024);

    HTTPClient http;
    http.setReuse(false);
    http.setTimeout(15000);
    Serial.println("Contacting Open-Meteo API...");

    if (!http.begin(client, this->weatherApiUrl))
    {
        Serial.println("HTTP API Request failed to begin.");
        setSuccess(false);
        http.end();
        return;
    }

    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK)
    {
        Serial.printf("HTTP request failed: %s\n",
                      http.errorToString(httpCode).c_str());
        setSuccess(false);
        // testOpenMeteoConnection();
        http.end();
        return;
    }

    String payload = http.getString();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
        Serial.print("JSON Parsing failed: ");
        setSuccess(false);
        Serial.println(error.c_str());
        http.end();
        return;
    }

    int fetchedHumidity = doc["current"]["relative_humidity_2m"];
    int fetchedWeatherCode = doc["current"]["weather_code"];
    int fetchedWorseWeatherCode = doc["daily"]["weather_code"][0];
    bool fetchedSuccess = true;
    setWeatherData(fetchedHumidity, fetchedWeatherCode, fetchedWorseWeatherCode, fetchedSuccess);
    
    // testOpenMeteoConnection();
    Serial.print("API Fetch Successful. Humidity: ");
    Serial.print(this->humidity);
    Serial.println("%");
    Serial.print("Weather Code: ");
    Serial.println(this->weatherCode);
    Serial.print("Worse Weather Code: ");
    Serial.println(this->worseWeatherCode);
    http.end();
    return;
}

void weatherData::testOpenMeteoConnection()
{
    const char *host = "api.open-meteo.com";

    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Free stack: %u bytes\n", ESP.getFreeContStack());

    // Test TCP without TLS.
    WiFiClient tcp;

    if (tcp.connect(host, 443))
    {
        Serial.println("TCP connection successful.");
        tcp.stop();
    }
    else
    {
        Serial.println("TCP connection failed.");
    }

    // Test TLS.
    WiFiClientSecure secure;
    secure.setInsecure();
    secure.setTimeout(15000);

    if (secure.connect(host, 443))
    {
        Serial.println("TLS connection successful.");
        secure.stop();
    }
    else
    {
        char errorBuffer[160] = {0};

        int errorCode =
            secure.getLastSSLError(errorBuffer, sizeof(errorBuffer));

        Serial.printf("TLS connection failed. Error: %d\n", errorCode);
        Serial.printf("TLS details: %s\n", errorBuffer);
    }
}