#include "input_task.h"
#include "app_state.h"
#include <Arduino.h>

#define BUTTON_PIN 5

#define LONG_PRESS_MS 800
#define CLICK_GAP_MS 500
#define DEBOUNCE_MS 30

void inputTask(void *param) {
    (void)param;

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    bool lastReading = HIGH;
    bool stableState = HIGH;
    unsigned long lastDebounceTime = 0;

    unsigned long pressStart = 0;
    unsigned long lastRelease = 0;

    int clickCount = 0;
    bool longPressSent = false;

    for (;;) {
        bool reading = digitalRead(BUTTON_PIN);
        unsigned long now = millis();

        if (reading != lastReading) {
            lastDebounceTime = now;
        }

        if ((now - lastDebounceTime) > DEBOUNCE_MS) {
            if (reading != stableState) {
                stableState = reading;

                if (stableState == LOW) {
                    pressStart = now;
                    longPressSent = false;
                }

                if (stableState == HIGH) {
                    if (!longPressSent) {
                        clickCount++;
                        lastRelease = now;
                    }
                }
            }
        }

        // HOLD = select / start
        if (stableState == LOW && !longPressSent) {
            if (now - pressStart >= LONG_PRESS_MS) {
                longPressSent = true;
                clickCount = 0;

                GestureEvent evt = { LONG_PRESS, now };
                xQueueSend(gestureQueue, &evt, 0);
            }
        }

        // CLICK actions
        if (clickCount > 0 && now - lastRelease > CLICK_GAP_MS) {

            if (clickCount == 1) {
                GestureEvent evt = { SINGLE_TAP, now };   // next / scroll
                xQueueSend(gestureQueue, &evt, 0);
            }

            else if (clickCount >= 2) {
                GestureEvent evt = { DOUBLE_TAP, now };   // back / cancel
                xQueueSend(gestureQueue, &evt, 0);
            }

            clickCount = 0;
        }

        lastReading = reading;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}