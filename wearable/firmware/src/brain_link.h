#pragma once

#include <stddef.h>
#include <stdint.h>

// How the watch asks the laptop a question.
// Phase 1 is Wi-Fi. A later phase can add a Bluetooth link that is used
// only while Wi-Fi is down, then switch back to Wi-Fi on its own.
// The screen talks to this interface and does not care which link is live.

class BrainLink {
  public:
    virtual ~BrainLink() {}
    virtual bool chat(const char *text, char *reply, size_t reply_len) = 0;
    virtual bool voice(const uint8_t *wav, size_t wav_len, char *transcript, size_t transcript_len,
                       char *reply, size_t reply_len) = 0;
    virtual bool health() = 0;
};

// TODO(phase-2): class BleBrainLink : public BrainLink
// Talk to the laptop over Bluetooth LE when Wi-Fi cannot reach it.
// When Wi-Fi starts working again, the worker should drop back to WifiBrainLink
// without asking anyone to flip a switch.
