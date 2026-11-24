#pragma once

#include "brain_link.h"

// Wi-Fi implementation of BrainLink. Posts JSON to the laptop.
class WifiBrainLink : public BrainLink {
  public:
    bool chat(const char *text, char *reply, size_t reply_len) override;
    bool voice(const uint8_t *wav, size_t wav_len, char *transcript, size_t transcript_len, char *reply,
               size_t reply_len) override;
    bool health() override;

  private:
    bool postJson(const char *path, const char *body, char *reply, size_t reply_len, char *transcript,
                  size_t transcript_len);
};

WifiBrainLink &wifiBrain();
const char *deviceId();
const char *sessionId();
