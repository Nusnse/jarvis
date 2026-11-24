#include "display.h"

#include "pin_config.h"

Arduino_CO5300 *gfx = nullptr;

static Arduino_DataBus *bus = nullptr;

bool displayBegin() {
    // Same constructor arguments as examples/arduino/examples/01_HelloWorld.
    bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
    gfx = new Arduino_CO5300(bus, LCD_RESET, 0 /* rotation */, LCD_WIDTH, LCD_HEIGHT,
                             LCD_COL_OFFSET, 0, 0, 0);
    if (!gfx->begin()) {
        Serial.println("Display failed to start.");
        return false;
    }
    gfx->fillScreen(RGB565_BLACK);
    // 0 is off, 255 is full. The panel is an OLED, so this is pixel brightness.
    gfx->setBrightness(160);
    return true;
}

void displaySplash(const char *line1, const char *line2, const char *line3) {
    if (gfx == nullptr) {
        return;
    }
    gfx->fillScreen(RGB565_BLACK);
    gfx->setTextColor(RGB565_CYAN);
    gfx->setTextSize(3);
    gfx->setCursor(150, 170);
    gfx->println(line1);

    gfx->setTextColor(RGB565_WHITE);
    gfx->setTextSize(2);
    gfx->setCursor(48, 230);
    gfx->println(line2);
    gfx->setCursor(48, 270);
    gfx->println(line3);
}
