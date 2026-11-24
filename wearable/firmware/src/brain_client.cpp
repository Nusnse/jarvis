#include "brain_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "wifi_setup.h"

static WifiBrainLink g_link;
static char g_device_id[24] = "jarvis-watch";
static char g_session_id[28] = "boot";

WifiBrainLink &wifiBrain() { return g_link; }

const char *deviceId() { return g_device_id; }

const char *sessionId() { return g_session_id; }

static void ensureIds() {
    static bool ready = false;
    if (ready) {
        return;
    }
    uint64_t mac = ESP.getEfuseMac();
    snprintf(g_device_id, sizeof(g_device_id), "jarvis-%04x", (unsigned)(mac & 0xffff));
    snprintf(g_session_id, sizeof(g_session_id), "watch-%08lx", (unsigned long)esp_random());
    ready = true;
}

static void copyField(const char *src, char *dest, size_t dest_len) {
    if (dest == nullptr || dest_len == 0) {
        return;
    }
    if (src == nullptr) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, dest_len - 1);
    dest[dest_len - 1] = '\0';
}

static bool postJson(const char *path, const char *body, char *reply, size_t reply_len, char *transcript,
                     size_t transcript_len) {
    ensureIds();
    const char *base = brainBaseUrl();
    if (base == nullptr || base[0] == '\0') {
        copyField("The watch does not know the laptop address yet. Hold BOOT for 2 seconds and type it in.",
                  reply, reply_len);
        return false;
    }
    if (WiFi.status() != WL_CONNECTED) {
        copyField("Wi-Fi is down. Hold BOOT for 2 seconds to open setup again.", reply, reply_len);
        return false;
    }

    String url = String(base) + path;
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(25000);
    if (!http.begin(client, url)) {
        copyField("Could not open a connection to the laptop.", reply, reply_len);
        return false;
    }
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(body);
    String payload = http.getString();
    http.end();

    if (code <= 0) {
        copyField("I can't reach the laptop. Is Jarvis Brain running, and is the address right?", reply, reply_len);
        Serial.printf("HTTP %s failed: %d\n", path, code);
        return false;
    }

    JsonDocument doc;
    if (deserializeJson(doc, payload)) {
        copyField("The laptop answered, but not in a way I understand.", reply, reply_len);
        Serial.println(payload);
        return false;
    }
    copyField(doc["reply"] | "", reply, reply_len);
    if (transcript != nullptr) {
        copyField(doc["transcript"] | "", transcript, transcript_len);
    }
    if (reply[0] == '\0') {
        copyField("The laptop sent an empty reply.", reply, reply_len);
        return false;
    }
    return doc["ok"] | (code >= 200 && code < 300);
}

bool WifiBrainLink::postJson(const char *path, const char *body, char *reply, size_t reply_len, char *transcript,
                             size_t transcript_len) {
    return ::postJson(path, body, reply, reply_len, transcript, transcript_len);
}

bool WifiBrainLink::chat(const char *text, char *reply, size_t reply_len) {
    ensureIds();
    JsonDocument doc;
    doc["text"] = text;
    doc["session_id"] = g_session_id;
    doc["device_id"] = g_device_id;
    String body;
    serializeJson(doc, body);
    return postJson("/v1/chat", body.c_str(), reply, reply_len, nullptr, 0);
}

bool WifiBrainLink::voice(const uint8_t *wav, size_t wav_len, char *transcript, size_t transcript_len, char *reply,
                          size_t reply_len) {
    ensureIds();
    const char *base = brainBaseUrl();
    if (base == nullptr || base[0] == '\0') {
        copyField("Set the laptop address first. Hold BOOT for 2 seconds.", reply, reply_len);
        return false;
    }
    if (WiFi.status() != WL_CONNECTED) {
        copyField("Wi-Fi is down, so I could not send the recording.", reply, reply_len);
        return false;
    }

    String url = String(base) + "/v1/voice";
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(30000);
    if (!http.begin(client, url)) {
        copyField("Could not open a connection to the laptop.", reply, reply_len);
        return false;
    }
    http.addHeader("Content-Type", "audio/wav");
    http.addHeader("X-Device-Id", g_device_id);
    http.addHeader("X-Session-Id", g_session_id);
    int code = http.POST(const_cast<uint8_t *>(wav), wav_len);
    String payload = http.getString();
    http.end();
    if (code <= 0) {
        copyField("The recording did not reach the laptop.", reply, reply_len);
        return false;
    }
    JsonDocument doc;
    if (deserializeJson(doc, payload)) {
        copyField("The laptop could not read that recording.", reply, reply_len);
        return false;
    }
    copyField(doc["transcript"] | "", transcript, transcript_len);
    copyField(doc["reply"] | "", reply, reply_len);
    return reply[0] != '\0';
}

bool WifiBrainLink::health() {
    const char *base = brainBaseUrl();
    if (base == nullptr || base[0] == '\0' || WiFi.status() != WL_CONNECTED) {
        return false;
    }
    String url = String(base) + "/health";
    WiFiClient client;
    HTTPClient http;
    http.setTimeout(4000);
    if (!http.begin(client, url)) {
        return false;
    }
    int code = http.GET();
    http.end();
    return code == 200;
}
