#include "worker.h"

#include <WiFi.h>

#include "brain_client.h"
#include "voice.h"

namespace {

enum class Job : uint8_t { None, Chat, Listen };

SemaphoreHandle_t lock = nullptr;
Job pending = Job::None;
volatile bool stop_listen = false;
volatile bool busy = false;
bool result_ready = false;
bool result_ok = false;
char prompt[180] = "";
char transcript[180] = "";
char reply[480] = "";
char status[72] = "Starting";
char seen_status[72] = "";

uint8_t *wav_buf = nullptr;
size_t wav_cap = 0;

void setStatus(const char *text) {
    xSemaphoreTake(lock, portMAX_DELAY);
    strncpy(status, text, sizeof(status) - 1);
    status[sizeof(status) - 1] = '\0';
    xSemaphoreGive(lock);
}

void publish(bool ok, const char *heard, const char *answer) {
    xSemaphoreTake(lock, portMAX_DELAY);
    result_ok = ok;
    strncpy(transcript, heard ? heard : "", sizeof(transcript) - 1);
    transcript[sizeof(transcript) - 1] = '\0';
    strncpy(reply, answer ? answer : "", sizeof(reply) - 1);
    reply[sizeof(reply) - 1] = '\0';
    result_ready = true;
    busy = false;
    pending = Job::None;
    xSemaphoreGive(lock);
}

void checkBrain() {
    if (WiFi.status() != WL_CONNECTED) {
        setStatus("No Wi-Fi");
        return;
    }
    bool up = wifiBrain().health();
    char line[72];
    String ssid = WiFi.SSID();
    if (ssid.length() > 16) {
        ssid.remove(16);
    }
    snprintf(line, sizeof(line), "%s · brain %s", ssid.c_str(), up ? "online" : "offline");
    setStatus(line);
}

void task(void *) {
    // Give Wi-Fi a moment after setup() returns.
    vTaskDelay(pdMS_TO_TICKS(400));
    checkBrain();

    for (;;) {
        Job job = Job::None;
        char local_prompt[180];
        xSemaphoreTake(lock, portMAX_DELAY);
        job = pending;
        if (job != Job::None) {
            pending = Job::None;
            busy = true;
            strncpy(local_prompt, prompt, sizeof(local_prompt) - 1);
            local_prompt[sizeof(local_prompt) - 1] = '\0';
        }
        xSemaphoreGive(lock);

        if (job == Job::Chat) {
            setStatus("Asking Jarvis...");
            char answer[480];
            bool ok = wifiBrain().chat(local_prompt, answer, sizeof(answer));
            publish(ok, "", answer);
            checkBrain();
        } else if (job == Job::Listen) {
            // TODO(phase-2): if Wi-Fi is down here, hand the WAV to BleBrainLink
            // instead of failing, then switch back once Wi-Fi returns.
            if (!voiceReady() || wav_buf == nullptr) {
                publish(false, "", "The microphone is not ready yet. Tap a prompt instead.");
                setStatus("Mic offline");
            } else {
                setStatus("Listening...");
                size_t bytes = voiceRecordWav(wav_buf, wav_cap, &stop_listen, 4000);
                stop_listen = false;
                if (bytes == 0) {
                    publish(false, "", "I didn't catch that. Tap Talk and speak for a couple of seconds.");
                } else {
                    setStatus("Sending audio...");
                    char heard[180];
                    char answer[480];
                    bool ok = wifiBrain().voice(wav_buf, bytes, heard, sizeof(heard), answer, sizeof(answer));
                    publish(ok, heard, answer);
                }
                checkBrain();
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(40));
        }
    }
}

}  // namespace

void workerStart() {
    lock = xSemaphoreCreateMutex();
    wav_cap = 130 * 1024;
    wav_buf = (uint8_t *)heap_caps_malloc(wav_cap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (wav_buf == nullptr) {
        wav_cap = 48 * 1024;
        wav_buf = (uint8_t *)malloc(wav_cap);
    }
    if (wav_buf == nullptr) {
        Serial.println("No buffer for recordings. Talk is disabled.");
        wav_cap = 0;
    }
    xTaskCreatePinnedToCore(task, "jarvis-net", 16384, nullptr, 1, nullptr, 0);
}

bool workerBusy() { return busy; }

void workerRequestChat(const char *text) {
    if (text == nullptr || text[0] == '\0' || busy) {
        return;
    }
    xSemaphoreTake(lock, portMAX_DELAY);
    if (!busy && pending == Job::None) {
        strncpy(prompt, text, sizeof(prompt) - 1);
        prompt[sizeof(prompt) - 1] = '\0';
        pending = Job::Chat;
        busy = true;
    }
    xSemaphoreGive(lock);
}

void workerRequestListen() {
    if (busy) {
        return;
    }
    xSemaphoreTake(lock, portMAX_DELAY);
    if (!busy && pending == Job::None) {
        stop_listen = false;
        pending = Job::Listen;
        busy = true;
    }
    xSemaphoreGive(lock);
}

void workerStopListen() { stop_listen = true; }

bool workerTakeStatus(char *dest, size_t dest_len) {
    if (dest == nullptr || dest_len == 0 || lock == nullptr) {
        return false;
    }
    bool changed = false;
    xSemaphoreTake(lock, portMAX_DELAY);
    if (strcmp(seen_status, status) != 0) {
        strncpy(seen_status, status, sizeof(seen_status) - 1);
        seen_status[sizeof(seen_status) - 1] = '\0';
        changed = true;
    }
    strncpy(dest, status, dest_len - 1);
    dest[dest_len - 1] = '\0';
    xSemaphoreGive(lock);
    return changed;
}

bool workerTakeResult(bool *ok, char *transcript_out, size_t transcript_len, char *reply_out, size_t reply_len) {
    if (lock == nullptr) {
        return false;
    }
    bool ready = false;
    xSemaphoreTake(lock, portMAX_DELAY);
    if (result_ready) {
        ready = true;
        result_ready = false;
        if (ok) {
            *ok = result_ok;
        }
        if (transcript_out && transcript_len) {
            strncpy(transcript_out, transcript, transcript_len - 1);
            transcript_out[transcript_len - 1] = '\0';
        }
        if (reply_out && reply_len) {
            strncpy(reply_out, reply, reply_len - 1);
            reply_out[reply_len - 1] = '\0';
        }
    }
    xSemaphoreGive(lock);
    return ready;
}
