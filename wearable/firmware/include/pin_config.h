#pragma once

// Pin numbers for the Waveshare ESP32-S3-Touch-AMOLED-1.75C.
// Copied from the official board_config library. Do not change these
// numbers to "try something"; they are wired on the circuit board.
// Source (main branch, read while this project was written):
// https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75C/blob/main/examples/arduino/libraries/board_config/pin_config.h
//
// The same pins are what the official Arduino demos use:
//   01_HelloWorld  — display
//   05_LVGL_Widgets — display + CST9217 touch
//   06_ES7210 — microphones
//   07_ES8311 — speaker codec

#define XPOWERS_CHIP_AXP2101

#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 38
#define LCD_RESET 1
#define LCD_CS 12
#define LCD_WIDTH 466
#define LCD_HEIGHT 466

// Touch controller (CST9217) and the shared I2C bus.
// AXP2101 (power) and QMI8658 (motion) also sit on this bus.
// Their addresses are 0x34 and 0x6B. Touch is 0x5A. ES7210 is 0x40.
#define IIC_SDA 15
#define IIC_SCL 14
#define TP_INT 11
#define TP_RST 2

#define PIN_ES7210_BCLK 9
#define PIN_ES7210_LRCK 45
#define PIN_ES7210_DIN 10
#define PIN_ES7210_MCLK 16
#define PIN_ES8311_DOUT 8

#define PA 46

// The BOOT button is the ESP32-S3 strap pin (GPIO0), active low.
// Waveshare's product page names this button BOOT. It is not part of
// pin_config.h because it is the chip's own boot strap, not a display pin.
// Hold it only AFTER Jarvis is already running. Holding it while the
// cable is plugged in puts the chip in download mode instead.
#define BOOT_BUTTON_PIN 0

// Column offset used by every official CO5300 demo for this panel.
#define LCD_COL_OFFSET 6
