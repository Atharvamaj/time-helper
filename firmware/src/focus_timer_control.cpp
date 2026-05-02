#include "focus_timer_control.h"
#include "display_helpers.h"
#include <Arduino.h>

#define FOCUS_BUTTON_PIN 5
#define CLICK_GAP_MS 400

static bool menuOpen = false;
static bool running = false;

static int options[] = {10, 15, 30};
static int selected = 0;

static unsigned long timerStartMs = 0;
static unsigned long timerDurationMs = 0;

static bool lastButton = HIGH;
static unsigned long lastReleaseMs = 0;
static int clickCount = 0;

void focusTimerButtonInit() {
    pinMode(FOCUS_BUTTON_PIN, INPUT_PULLUP);
}

bool focusTimerRunning() {
    return running;
}

static unsigned long remainingSeconds() {
    if (!running) return 0;

    unsigned long elapsed = millis() - timerStartMs;
    if (elapsed >= timerDurationMs) return 0;

    return (timerDurationMs - elapsed) / 1000;
}

static void drawMenu() {
    char line[32];
    snprintf(line, sizeof(line), "%d minutes", options[selected]);

    showText("[ Focus Timer ]", "", line, "1 next | 2 start | 3 back");
}

void focusTimerDrawRunning() {
    unsigned long sec = remainingSeconds();

    char line[32];
    snprintf(line, sizeof(line), "%02lu:%02lu", sec / 60, sec % 60);

    showText("[ Focus Mode ]", "", line, "Triple click cancels");
}

static void singleClick() {
    if (running) return;

    if (!menuOpen) {
        menuOpen = true;
        selected = 0;
    } else {
        selected++;
        if (selected >= 3) selected = 0;
    }

    drawMenu();
}

static void doubleClick() {
    if (!menuOpen) return;

    timerDurationMs = options[selected] * 60UL * 1000UL;
    timerStartMs = millis();

    running = true;
    menuOpen = false;

    focusTimerDrawRunning();
}

static void tripleClick() {
    running = false;
    menuOpen = false;
}

void focusTimerButtonUpdate() {
    bool button = digitalRead(FOCUS_BUTTON_PIN);
    unsigned long now = millis();

    if (lastButton == LOW && button == HIGH) {
        clickCount++;
        lastReleaseMs = now;
    }

    if (clickCount > 0 && now - lastReleaseMs > CLICK_GAP_MS) {
        if (clickCount == 1) singleClick();
        else if (clickCount == 2) doubleClick();
        else if (clickCount >= 3) tripleClick();

        clickCount = 0;
    }

    if (running && remainingSeconds() == 0) {
        running = false;
        showText("[ Timer Done ]", "", "Time is up!", "Tap for menu");
    }

    lastButton = button;
}