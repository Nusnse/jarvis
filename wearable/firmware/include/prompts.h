#pragma once

// Quick phrases shown as buttons on the watch.
// Change the words inside the quotes. Keep each label short (it has to
// fit on a round screen) and each message a single sentence.
// The label is what Kyla taps. The message is what is sent to the laptop.

struct JarvisPrompt {
    const char *label;
    const char *message;
};

static const JarvisPrompt kPrompts[] = {
    {"Morning", "Good morning, Jarvis. How should I start the day?"},
    {"Joke", "Tell me a short joke."},
    {"Focus", "Give me one sentence to help me focus."},
    {"Question", "Ask me one useful question about my day."},
};

static const int kPromptCount = sizeof(kPrompts) / sizeof(kPrompts[0]);
