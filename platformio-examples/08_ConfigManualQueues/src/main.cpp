// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — Config Manual Queues

  Demonstrates:
    - selection of the TLS_MANUAL profile;
    - manual configuration of scheduler queue capacities;
    - compile-time configuration before including the library header;
    - simultaneous use of PERIODIC, DELAYED, POSTED, and POLL tasks;
    - LED blinking and Serial output without delay().

  Hardware:
    - any supported AVR Arduino board;
    - built-in LED connected to LED_BUILTIN;
    - USB connection to a computer for Serial Monitor.

  Serial Monitor:
    - baud rate: 9600.

  Manual queue configuration used by this example:

    Queue           Physical capacity   Used by this project

    PERIODIC                2                    1
    DELAYED                 2                    1
    POLL                    2                    1
    POSTED                  2                    1
    ISR requests            2                    0

  In the TLS_MANUAL profile, TLS_MAX_* values specify direct physical
  capacities. The library does not add an additional headroom slot.

  This example assigns two physical slots to every queue:
    - one slot can be used by the example;
    - one slot remains available as headroom.

  The ISR request queue is configured explicitly but is not used by this
  project. Hardware interrupt handling was demonstrated in the previous
  ISR Post Task example.

  Try different queue capacities:
    1. change one or more TLS_MAX_* values below;
    2. build the project again;
    3. compare SRAM usage in the PlatformIO build output;
    4. select capacities according to the maximum number of simultaneous
       active or pending jobs expected in each queue.

  Behavior:
    - postedTask() is queued once during setup();
    - postedTask() executes once from the POSTED phase;
    - blinkLed() executes every 1000 milliseconds as a PERIODIC task;
    - blinkLed() changes the built-in LED state;
    - reportLedState() runs as a persistent POLL task;
    - reportLedState() prints only after the LED state changes;
    - delayedTask() executes once after 3000 milliseconds;
    - no task uses delay() or a manual millis() comparison.

  Example output:

    MTW TinyLoopScheduler started
    POSTED task executed
    LED ON
    LED OFF
    DELAYED task executed at 3000 ms
    LED ON
    LED OFF

  The exact displayed time can be slightly greater than 3000 milliseconds
  if another callback or application code delays scheduler execution.

  Important:
    - TLS_PROFILE and all TLS_MAX_* values must be defined before including
      MTW_TinyLoopScheduler.h;
    - TLS_MANUAL capacities are physical queue capacities;
    - no extra slot is automatically added in TLS_MANUAL mode;
    - undefined TLS_MAX_* values use the TLS_DEF physical capacities;
    - this example defines every capacity explicitly to avoid hidden defaults;
    - supported queue capacities are clamped to the range 2 through 255;
    - every scheduler queue has an independent capacity;
    - one PERIODIC task does not consume a slot in POLL, DELAYED, or POSTED;
    - callbacks should complete quickly;
    - do not place delay() or other long blocking operations in loop().

  Power saving:
    - MTW TinyLoopScheduler automatically uses AVR SLEEP_MODE_IDLE when
      the current scheduler pass has no immediate ISR, PERIODIC, or DELAYED
      work to execute;
    - timers and interrupt-capable peripherals continue operating while
      the processor is in SLEEP_MODE_IDLE;
    - after wake-up, the scheduler continues with POSTED and POLL tasks;
    - SLEEP_MODE_IDLE is a lightweight idle state, not a deep-sleep mode.

*/

#include <Arduino.h>

/*
  Select manual queue configuration.

  TLS_MANUAL tells the library to use the TLS_MAX_* values defined below
  instead of one of the predefined profiles.
*/
#define TLS_PROFILE TLS_MANUAL

/*
  Set direct physical capacities for all ordinary scheduler queues.

  Each queue has two physical slots. This project uses no more than one
  simultaneous job in each queue, leaving one free slot as headroom.
*/
#define TLS_MAX_PERIODIC      2
#define TLS_MAX_DELAYED       2
#define TLS_MAX_POLL          2
#define TLS_MAX_POSTED        2
#define TLS_MAX_ISR_REQUESTS  2

/*
  All MTW TinyLoopScheduler compile-time configuration macros must be
  defined before the library header is included.
*/
#include <MTW_TinyLoopScheduler.h>

/*
  Current state of the built-in LED.

  The LED starts switched off.
*/
static uint8_t ledState = LOW;

/*
  LED state-change flag.

  blinkLed() sets this flag after changing the LED state.
  reportLedState() clears it after printing the new state.

  Both callbacks execute in the main program context, so this variable
  does not need to be declared volatile.
*/
static uint8_t ledStateChanged = 0u;

/*
  PERIODIC task.

  The scheduler executes this callback every 1000 milliseconds.
  It changes the LED state and notifies the POLL task.
*/
static void blinkLed()
{
    /*
      Toggle the stored LED state.
    */
    ledState = (ledState == LOW) ? HIGH : LOW;

    /*
      Apply the new state to the built-in LED.
    */
    digitalWrite(LED_BUILTIN, ledState);

    /*
      Notify reportLedState() that the LED state has changed.
    */
    ledStateChanged = 1u;
}

/*
  POLL task.

  The scheduler calls this callback during every POLL phase.

  The callback immediately returns while the LED state remains unchanged.
  Serial output therefore occurs only once after each LED transition.
*/
static void reportLedState()
{
    /*
      Nothing has changed since the previous report.
    */
    if (ledStateChanged == 0u) {
        return;
    }

    /*
      Print the current LED state.
    */
    if (ledState == HIGH) {
        Serial.println(F("LED ON"));
    } else {
        Serial.println(F("LED OFF"));
    }

    /*
      Mark the current LED state as reported.
    */
    ledStateChanged = 0u;
}

/*
  DELAYED task.

  The scheduler executes this callback once after 3000 milliseconds.
  The task is automatically removed after execution.
*/
static void delayedTask()
{
    Serial.print(F("DELAYED task executed at "));
    Serial.print(tinyls::now);
    Serial.println(F(" ms"));
}

/*
  POSTED task.

  This callback is queued during setup() but does not execute there.
  It runs later from the POSTED phase of tinyls::tick().
*/
static void postedTask()
{
    Serial.println(F("POSTED task executed"));
}

void setup()
{
    /*
      Initialize serial communication.

      Open Serial Monitor and select 9600 baud.
    */
    Serial.begin(9600);

    /*
      Configure the built-in LED pin as an output.
    */
    pinMode(LED_BUILTIN, OUTPUT);

    /*
      Start with the LED switched off.
    */
    digitalWrite(LED_BUILTIN, ledState);

    /*
      Initialize the scheduler before registering any tasks.
    */
    tinyls::init();

    /*
      Register one PERIODIC task.

      blinkLed() changes the LED state every 1000 milliseconds.
    */
    tinyls::every(blinkLed, 1000u);

    /*
      Register one DELAYED task.

      delayedTask() executes once after 3000 milliseconds.
    */
    tinyls::after(delayedTask, 3000u);

    /*
      Register one persistent POLL task.

      reportLedState() checks whether the LED state has changed.
    */
    tinyls::poll(reportLedState);

    /*
      Queue one one-shot POSTED task.

      postedTask() executes later from tinyls::tick(), not directly here.
    */
    tinyls::post(postedTask);

    /*
      Print the startup message once.
    */
    Serial.println(F("MTW TinyLoopScheduler started"));
}

void loop()
{
    /*
      Execute one complete scheduler pass.

      The scheduler processes enabled task classes in their defined phases.
      tick() should be called continuously.

      Do not place delay() or other long blocking operations in loop().
    */
    tinyls::tick();
}