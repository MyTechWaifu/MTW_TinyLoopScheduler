// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — Button Poll

  Demonstrates:
    - reading a button without a hardware interrupt;
    - registering a persistent callback with tinyls::poll();
    - non-blocking button debounce;
    - using tinyls::now instead of an additional millis() call.

  Hardware:
    - supported AVR Arduino board;
    - button connected between pin 2 and GND;
    - built-in LED connected to LED_BUILTIN.

  Behavior:
    - pressing the button toggles the built-in LED;
    - the button is checked during every POLL phase;
    - the input must remain stable for 30 milliseconds.

  Power saving:
    - MTW TinyLoopScheduler automatically uses AVR SLEEP_MODE_IDLE when
      the current scheduler pass has no immediate ISR, PERIODIC, or DELAYED
      work to execute;
    - the CPU sleeps until the next enabled interrupt while timers and
      interrupt-capable peripherals continue operating;
    - after wake-up, the scheduler continues with POSTED and POLL jobs;
    - this reduces active CPU time and power consumption compared with a
      conventional sketch that continuously spins through an empty loop();
    - actual power savings depend on the board, enabled peripherals,
      interrupt frequency, scheduled workload, and blocking application code;
    - SLEEP_MODE_IDLE is a lightweight idle state, not a deep-sleep mode.

*/

#include <MTW_TinyLoopScheduler.h>

static const uint8_t BUTTON_PIN = 2u;
static const uint32_t DEBOUNCE_MS = 30u;

static uint8_t ledState = LOW;
static uint8_t lastSample = HIGH;
static uint8_t stableState = HIGH;
static uint32_t sampleChangedAt = 0u;

static void toggleLed()
{
    ledState = (ledState == LOW) ? HIGH : LOW;
    digitalWrite(LED_BUILTIN, ledState);
}

static void checkButton()
{
    uint8_t currentSample = digitalRead(BUTTON_PIN);

    /*
      Restart the stable-state timer whenever the raw input changes.
    */
    if (currentSample != lastSample) {
        lastSample = currentSample;
        sampleChangedAt = tinyls::now;
        return;
    }

    /*
      Wait until the current raw state remains unchanged long enough.
    */
    if ((uint32_t)(tinyls::now - sampleChangedAt) < DEBOUNCE_MS) {
        return;
    }

    /*
      Ignore a state that was already accepted.
    */
    if (currentSample == stableState) {
        return;
    }

    stableState = currentSample;

    /*
      INPUT_PULLUP:
        LOW  = pressed;
        HIGH = released.
    */
    if (stableState == LOW) {
        toggleLed();
    }
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, ledState);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    tinyls::init();

    /*
      Initialize debounce state from the real input level.
    */
    lastSample = digitalRead(BUTTON_PIN);
    stableState = lastSample;
    sampleChangedAt = tinyls::now;

    tinyls::poll(checkButton);
}

void loop()
{
    tinyls::tick();
}
