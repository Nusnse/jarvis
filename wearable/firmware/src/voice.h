#pragma once

#include <Arduino.h>

// Push-to-talk microphone path.
// Uses the ES7210 driver and the I2S pin numbers from Waveshare's
// 06_ES7210 example. The I2S bus is opened with the ESP_I2S API from
// their 07_ES8311 example, because the older driver/i2s.h include is
// not available on current Arduino-ESP32. Recording is mono 16 kHz, 16-bit.
// The bytes are wrapped in a WAV file and posted to the laptop, which
// turns speech into text.

bool voiceBegin();
bool voiceReady();

// Fills wav_out with a WAV file. Returns the number of bytes, or 0.
// stop_flag is checked between reads so a second tap can end early.
// max_ms caps the recording (the buffer is sized for 4 seconds).
size_t voiceRecordWav(uint8_t *wav_out, size_t wav_cap, volatile bool *stop_flag, uint32_t max_ms);
