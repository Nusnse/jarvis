#include <Arduino.h>
#include <Wire.h>

#include "display.h"
#include "pin_config.h"
#include "ui.h"
#include "voice.h"
#include "wifi_setup.h"
#include "worker.h"

// Phase 1 of Jarvis on the Waveshare ESP32-S3-Touch-AMOLED-1.75C.
// The watch draws a face, sends short prompts (and optional microphone
// audio) to the laptop brain, and shows the reply.

static const uint32_t kBootHoldMs = 2000;
static uint32_t boot_down_at = 0;

static void pollSerial() {
    if (!Serial.available()) {
        return;
    }
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.startsWith("say ")) {
        String text = line.substring(4);
        text.trim();
        if (text.length() > 0) {
            Serial.println(text);
            workerRequestChat(text.c_str());
        }
    } else if (line.length() > 0) {
        Serial.println("Type: say hello");
    }
}

static void pollBootButton() {
    if (!bootButtonHeld()) {
        boot_down_at = 0;
        return;
    }
    if (boot_down_at == 0) {
        boot_down_at = millis();
        return;
    }
    if (millis() - boot_down_at < kBootHoldMs) {
        return;
    }
    boot_down_at = 0;
    Serial.println("BOOT held. Opening Wi-Fi setup.");
    wifiConnect(true);
    ESP.restart();
}

void setup() {
    Serial.begin(115200);
    delay(400);
    Serial.println();
    Serial.println("Jarvis watch, phase 1");
    Serial.println("Board: ESP32-S3-Touch-AMOLED-1.75C");

    Wire.begin(IIC_SDA, IIC_SCL);
    if (!displayBegin()) {
        Serial.println("Stopping: the display did not start.");
        return;
    }

    wifiConnect(false);
    voiceBegin();
    uiBegin();
    workerStart();
    Serial.println("Ready. Hold BOOT for 2 seconds to change Wi-Fi.");
    Serial.println("Serial command: say hello");
}

void loop() {
    pollBootButton();
    pollSerial();
    uiLoop();
    delay(5);
}
