// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — Button ISR With Poll

  Demonstrates:
    - detecting a button edge with a hardware interrupt;
    - deferring interrupt work with tinyls::postISR();
    - temporarily monitoring the input with tinyls::poll();
    - removing a POLL callback with tinyls::removePoll();
    - non-blocking button debounce.

  Hardware:
    - supported AVR Arduino board;
    - button connected between pin 2 and GND;
    - built-in LED connected to LED_BUILTIN.

  Behavior:
    - pressing the button toggles the built-in LED;
    - the ISR only records a deferred callback;
    - debounce is performed later from the POLL phase.

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

#include <Arduino.h>
#include <MTW_TinyLoopScheduler.h>

static const uint8_t BUTTON_PIN = 2u;
static const uint32_t DEBOUNCE_MS = 30u;

/*
  Accessed from both ISR and main context.
  uint8_t reads and writes are atomic on 8-bit AVR.
*/
static volatile uint8_t buttonRequestPending = 0u;

static uint8_t ledState = LOW;
static uint8_t lastButtonSample = HIGH;
static uint32_t sampleChangedAt = 0u;

static void toggleLed()
{
    ledState = (ledState == LOW) ? HIGH : LOW;
    digitalWrite(LED_BUILTIN, ledState);
}

static void pollButton()
{
    uint8_t currentSample = digitalRead(BUTTON_PIN);

    /*
      The input changed again, so restart the stable-state timer.
    */
    if (currentSample != lastButtonSample) {
        lastButtonSample = currentSample;
        sampleChangedAt = tinyls::now;
        return;
    }

    /*
      Wait until the current state remains unchanged long enough.
    */
    if ((uint32_t)(tinyls::now - sampleChangedAt) < DEBOUNCE_MS) {
        return;
    }

    /*
      A stable LOW confirms a press.
      A stable HIGH means noise or a very short press.
    */
    if (currentSample == LOW) {
        toggleLed();
    }

    /*
      Debounce is complete. Stop the temporary POLL job.
    */
    tinyls::removePoll(pollButton);

    /*
      Allow the next interrupt request.
    */
    buttonRequestPending = 0u;
}

static void startButtonPolling()
{
    lastButtonSample = digitalRead(BUTTON_PIN);
    sampleChangedAt = tinyls::now;

    /*
      If POLL registration fails, release the request gate.
    */
    if (tinyls::poll(pollButton) == 0u) {
        buttonRequestPending = 0u;
    }
}

static void onButtonInterrupt()
{
    /*
      Ignore bounce-generated falling edges while one press is being handled.
    */
    if (buttonRequestPending != 0u) {
        return;
    }

    buttonRequestPending = 1u;

    /*
      Only record deferred work from ISR context.
    */
    if (tinyls::postISR(startButtonPolling) == 0u) {
        buttonRequestPending = 0u;
    }
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, ledState);

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    tinyls::init();

    attachInterrupt(
        digitalPinToInterrupt(BUTTON_PIN),
        onButtonInterrupt,
        FALLING
    );
}

void loop()
{
    tinyls::tick();
}
