#include <Arduino.h>

class weatherData
{
public:
    weatherData(String weatherApiUrl);
    int getHumidity() const;
    int getWeatherCode() const;
    int getWorseWeatherCode() const;
    bool getSuccess() const;
    void setExternalWeatherData();
private:
    int humidity;
    int weatherCode;
    int worseWeatherCode;
    bool success;
    String weatherApiUrl;
    void setWeatherData(int humidity, int weatherCode, int worseWeatherCode, bool success);
    void setSuccess(bool success);
    void testOpenMeteoConnection();
};