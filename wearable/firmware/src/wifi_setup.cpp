#include "wifi_setup.h"

#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "display.h"
#include "pin_config.h"

static const char *kApName = "Jarvis-Setup";
static const char *kPrefsNamespace = "jarvis";
static const char *kPrefsKey = "brain_url";

static char brain_url[96] = "";
static WiFiManagerParameter brain_param("brain", "Laptop address (http://ip:8787)", brain_url, sizeof(brain_url));

static void loadBrainUrl() {
    Preferences prefs;
    prefs.begin(kPrefsNamespace, true);
    String saved = prefs.getString(kPrefsKey, "");
    prefs.end();
    saved.trim();
    strncpy(brain_url, saved.c_str(), sizeof(brain_url) - 1);
    brain_url[sizeof(brain_url) - 1] = '\0';
}

static bool urlLooksUsable(const char *value) {
    if (value == nullptr) {
        return false;
    }
    return strstr(value, "://") != nullptr && strlen(value) > 12;
}

static void saveBrainUrl() {
    const char *typed = brain_param.getValue();
    if (!urlLooksUsable(typed)) {
        return;
    }
    strncpy(brain_url, typed, sizeof(brain_url) - 1);
    brain_url[sizeof(brain_url) - 1] = '\0';
    // Drop a trailing slash so paths join cleanly later.
    size_t len = strlen(brain_url);
    while (len > 0 && brain_url[len - 1] == '/') {
        brain_url[--len] = '\0';
    }
    Preferences prefs;
    prefs.begin(kPrefsNamespace, false);
    prefs.putString(kPrefsKey, brain_url);
    prefs.end();
    Serial.print("Saved laptop address: ");
    Serial.println(brain_url);
}

const char *brainBaseUrl() { return brain_url; }

bool bootButtonHeld() {
    return digitalRead(BOOT_BUTTON_PIN) == LOW;
}

bool wifiConnect(bool force_portal) {
    loadBrainUrl();
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);

    WiFi.mode(WIFI_STA);
    WiFiManager portal;
    portal.setDebugOutput(true);
    portal.setTitle("Jarvis");
    portal.setConnectTimeout(20);
    portal.setConfigPortalTimeout(180);
    portal.addParameter(&brain_param);
    portal.setSaveParamsCallback(saveBrainUrl);

    bool joined = false;
    if (force_portal) {
        displaySplash("JARVIS", "Join Wi-Fi Jarvis-Setup", "Then open 192.168.4.1");
        Serial.println("Opening Wi-Fi setup. Join Jarvis-Setup and visit http://192.168.4.1");
        portal.resetSettings();
        joined = portal.startConfigPortal(kApName);
    } else {
        displaySplash("JARVIS", "Connecting to Wi-Fi", "Or join Jarvis-Setup");
        Serial.println("Connecting to Wi-Fi. If that fails, join Jarvis-Setup.");
        joined = portal.autoConnect(kApName);
    }

    saveBrainUrl();
    if (joined) {
        Serial.print("Wi-Fi connected. IP ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("Wi-Fi is not connected.");
    }
    return joined;
}
