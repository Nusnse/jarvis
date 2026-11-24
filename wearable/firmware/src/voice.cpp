#include "voice.h"

#include <ESP_I2S.h>
#include <Wire.h>

#include "es7210.h"
#include "pin_config.h"

// The codec registers come from Waveshare's 06_ES7210 example.
// That example opens the I2S bus with the old driver/i2s.h API. Arduino-ESP32
// 3.3.12 no longer puts that header on the include path (it lives under
// driver/deprecated). The pins and the 16 kHz / 16-bit setup are the same.
// The bus itself is opened with ESP_I2S.h, which is what Waveshare's
// 07_ES8311 example uses on this board. The speaker data pin is passed in
// so a later text-to-speech phase can write to the ES8311 on the same clock.

static bool g_ready = false;
static I2SClass i2s;

static void write32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)(value & 0xff);
    p[1] = (uint8_t)((value >> 8) & 0xff);
    p[2] = (uint8_t)((value >> 16) & 0xff);
    p[3] = (uint8_t)((value >> 24) & 0xff);
}

static bool i2cPresent(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

bool voiceBegin() {
    // Same codec settings as examples/arduino/examples/06_ES7210/08_ES7210.ino.
    if (!i2cPresent(ES7210_ADDR)) {
        Serial.println("ES7210 microphone chip was not found on I2C. Talk will stay off.");
        g_ready = false;
        return false;
    }

    audio_hal_codec_config_t cfg = {};
    cfg.adc_input = AUDIO_HAL_ADC_INPUT_ALL;
    cfg.codec_mode = AUDIO_HAL_CODEC_MODE_ENCODE;
    cfg.i2s_iface.mode = AUDIO_HAL_MODE_SLAVE;
    cfg.i2s_iface.fmt = AUDIO_HAL_I2S_NORMAL;
    cfg.i2s_iface.samples = AUDIO_HAL_16K_SAMPLES;
    cfg.i2s_iface.bits = AUDIO_HAL_BIT_LENGTH_16BITS;

    esp_err_t err = es7210_adc_init(&Wire, &cfg);
    err |= es7210_adc_config_i2s(cfg.codec_mode, &cfg.i2s_iface);
    err |= es7210_adc_set_gain((es7210_input_mics_t)(ES7210_INPUT_MIC1 | ES7210_INPUT_MIC2), GAIN_0DB);
    err |= es7210_adc_set_gain((es7210_input_mics_t)(ES7210_INPUT_MIC3 | ES7210_INPUT_MIC4), GAIN_37_5DB);
    err |= es7210_adc_ctrl_state(cfg.codec_mode, AUDIO_HAL_CTRL_START);
    if (err != ESP_OK) {
        Serial.printf("ES7210 setup returned %d\n", (int)err);
        g_ready = false;
        return false;
    }

    // TODO(phase-later): speech replies will write PCM to PIN_ES8311_DOUT
    // and raise PA (GPIO 46) to turn the speaker amplifier on. Nothing
    // plays audio in this version.
    i2s.setPins(PIN_ES7210_BCLK, PIN_ES7210_LRCK, PIN_ES8311_DOUT, PIN_ES7210_DIN, PIN_ES7210_MCLK);
    if (!i2s.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT)) {
        Serial.println("I2S microphone bus did not start.");
        g_ready = false;
        return false;
    }
    g_ready = true;
    Serial.println("Microphone path is ready.");
    return true;
}

bool voiceReady() { return g_ready; }

size_t voiceRecordWav(uint8_t *wav_out, size_t wav_cap, volatile bool *stop_flag, uint32_t max_ms) {
    if (!g_ready || wav_out == nullptr || wav_cap < 64) {
        return 0;
    }
    const size_t header = 44;
    size_t pcm_cap = wav_cap - header;
    // Keep the recording inside the buffer even if the caller asks for longer.
    size_t max_bytes = (size_t)16000 * 2 * max_ms / 1000;
    if (max_bytes > pcm_cap) {
        max_bytes = pcm_cap;
    }
    // Even number of bytes: samples are 16-bit.
    max_bytes &= ~((size_t)1);

    uint8_t *pcm = wav_out + header;
    size_t filled = 0;
    uint32_t started = millis();

    // Throw away the first moment. The codec clicks when a capture starts.
    uint8_t discard[256];
    i2s.readBytes((char *)discard, sizeof(discard));

    while (filled < max_bytes && (millis() - started) < max_ms) {
        if (stop_flag != nullptr && *stop_flag) {
            break;
        }
        size_t want = max_bytes - filled;
        if (want > 1024) {
            want = 1024;
        }
        size_t n = i2s.readBytes((char *)(pcm + filled), want);
        if (n == 0) {
            break;
        }
        filled += n;
    }
    filled &= ~((size_t)1);
    if (filled < 1600) {
        // Less than about 50 ms of audio. Not worth sending.
        return 0;
    }

    memcpy(wav_out, "RIFF", 4);
    write32(wav_out + 4, (uint32_t)(filled + 36));
    memcpy(wav_out + 8, "WAVE", 4);
    memcpy(wav_out + 12, "fmt ", 4);
    write32(wav_out + 16, 16);
    wav_out[20] = 1;
    wav_out[21] = 0;
    wav_out[22] = 1;
    wav_out[23] = 0;  // mono
    write32(wav_out + 24, 16000);
    write32(wav_out + 28, 16000 * 2);
    wav_out[32] = 2;
    wav_out[33] = 0;
    wav_out[34] = 16;
    wav_out[35] = 0;
    memcpy(wav_out + 36, "data", 4);
    write32(wav_out + 40, (uint32_t)filled);
    return header + filled;
}
