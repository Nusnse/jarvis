#pragma once

#include <Arduino.h>

void workerStart();
bool workerBusy();

// Ask the laptop a question. Ignored if a job is already running.
void workerRequestChat(const char *text);

// Record from the microphone and send the audio to the laptop.
void workerRequestListen();

// Ends a recording early. The audio already captured is still sent.
void workerStopListen();

// Copies the latest status line. Returns true when it changed.
bool workerTakeStatus(char *dest, size_t dest_len);

// Copies a finished reply and clears it. Returns false if nothing is waiting.
bool workerTakeResult(bool *ok, char *transcript, size_t transcript_len, char *reply, size_t reply_len);
