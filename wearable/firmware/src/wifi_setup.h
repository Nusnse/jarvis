#pragma once

#include <Arduino.h>

// Joins a saved network, or opens the Jarvis-Setup captive portal.
// force_portal forgets the saved network and waits for a new one.
// The laptop address typed into the portal is stored on the watch.
bool wifiConnect(bool force_portal);

// The address of the laptop brain, with no trailing slash.
// Empty until someone types it into the setup page.
const char *brainBaseUrl();

// True while the BOOT button is held. Used to re-open Wi-Fi setup.
bool bootButtonHeld();
