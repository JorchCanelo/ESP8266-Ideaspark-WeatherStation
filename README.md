# ESP8266-Ideaspark-WeatherStation
A C++ project using platform.io to program a ESP8266 Ideaspark microcontroller with integrated OLED screen to display the current temperature using a ds18b20 sensor and the open-meteo API to get humidity, current weather code and the reported worse weather code for the day. The program also sends the Temperature and Humidity to a Supabase DB.

## Setting up
You need to configure the supabase settings and your network settings in main.cpp for the project to work correctly and also use platform.io on Visual Studio Code to import the project. Also, check the COM settings on platformio.ini and change it to the COM you will be using to flash your ESP8266. D4 PIN is used for the sensor's data and D5 and D6 for the OLED screen.