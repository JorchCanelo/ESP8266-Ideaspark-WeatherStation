#include "weatherIcon.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

void drawWeatherIcon(int code, int x, int y) {
    int size = 16;
    if (code == 0) {
        display.drawBitmap(x, y, icon_sunny, size, size, WHITE);
    } else if (code >= 1 && code <= 3) {
        display.drawBitmap(x, y, icon_cloudy, size, size, WHITE);
    } else if (code >= 51) {
        display.drawBitmap(x, y, icon_rainy, size, size, WHITE);
    } else {
        display.drawBitmap(x, y, icon_cloudy, size, size, WHITE);
    }
}