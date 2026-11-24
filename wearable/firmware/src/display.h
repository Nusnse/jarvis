#pragma once

#include <Arduino.h>

// Starts the AMOLED using the same bus and panel settings as Waveshare's
// 01_HelloWorld demo. Call this once, before LVGL.
bool displayBegin();

// Simple full-screen text used before LVGL is running (boot and Wi-Fi setup).
// Three short lines, drawn in the middle of the round panel.
void displaySplash(const char *line1, const char *line2, const char *line3);

// The GFX object LVGL uses to push pixels. Defined in display.cpp.
#include <Arduino_GFX_Library.h>
extern Arduino_CO5300 *gfx;
