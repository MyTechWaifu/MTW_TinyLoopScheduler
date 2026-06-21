// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — Config Profile

  Demonstrates:
    - selection of a predefined scheduler profile;
    - compile-time configuration before including the library header;
    - simultaneous use of PERIODIC and POLL tasks;
    - LED blinking and Serial output without delay().

  Hardware:
    - any supported AVR Arduino board;
    - built-in LED connected to LED_BUILTIN;
    - USB connection to a computer for Serial Monitor.

  Serial Monitor:
    - baud rate: 9600.

  Available predefined profiles and recommended working load:

    Profile    PERIODIC  DELAYED  POLL  POSTED  ISR requests

    TLS_TINY       1        1      1       1         1
    TLS_MINI       2        2      2       2         2
    TLS_MID        6        4      4       4         3
    TLS_DEF       12        8      8       8         6
    TLS_MAX       24       16     16      16        12

  Recommended working load means the suggested maximum number of
  simultaneously active or pending jobs in each scheduler queue.

  Predefined profiles provide one additional physical queue slot as
  practical headroom.

  Try different profiles:
    1. leave only one TLS_PROFILE line enabled below;
    2. compile and upload the sketch again;
    3. compare SRAM and Flash usage in the Arduino IDE build report;
    4. choose the smallest profile that supports the required number of
       simultaneous jobs in every queue used by the application.

  This example uses:
    - one PERIODIC task;
    - one POLL task.

  Therefore, TLS_TINY is sufficient because the tasks occupy two different
  scheduler queues.

  Behavior:
    - blinkLed() runs as one PERIODIC task every 1000 milliseconds;
    - blinkLed() changes the built-in LED state;
    - reportLedState() runs as one persistent POLL task;
    - reportLedState() prints a message only after the LED state changes;
    - the LED stays ON for 1 second;
    - the LED stays OFF for 1 second;
    - one complete blink cycle takes 2 seconds.

  Expected output:

    MTW TinyLoopScheduler started
    LED ON
    LED OFF
    LED ON
    LED OFF

  Important:
    - TLS_PROFILE must be defined before including
      MTW_TinyLoopScheduler.h;
    - enable only one TLS_PROFILE line;
    - changing the profile requires recompiling the sketch;
    - if TLS_PROFILE is not defined, the library uses TLS_DEF;
    - profiles configure the capacities of separate scheduler queues;
    - one PERIODIC task and one POLL task occupy different queues;
    - profiles do not enable or disable scheduler subsystems;
    - manual queue configuration and subsystem removal are demonstrated
      in the following examples;
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

/*
  Select one predefined scheduler profile.

  Leave only one line enabled. Then compile the sketch again and compare
  memory usage in the Arduino IDE build report.

  TLS_TINY is enabled because this example uses only:
    - one PERIODIC task;
    - one POLL task.
*/
#define TLS_PROFILE TLS_TINY
// #define TLS_PROFILE TLS_MINI
// #define TLS_PROFILE TLS_MID
// #define TLS_PROFILE TLS_DEF
// #define TLS_PROFILE TLS_MAX

/*
  All compile-time configuration macros must be defined before the library
  header is included.
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

  The callback:
    1. changes the stored LED state;
    2. writes the new state to LED_BUILTIN;
    3. notifies the POLL task that the state has changed.
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
      Notify reportLedState() that a new LED state is available.
    */
    ledStateChanged = 1u;
}

/*
  POLL task.

  The scheduler calls this callback during every POLL phase.

  The callback immediately returns when the LED state has not changed.
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

      blinkLed() runs every 1000 milliseconds and changes the LED state.
    */
    tinyls::every(blinkLed, 1000u);

    /*
      Register one POLL task.

      reportLedState() checks whether the LED state changed and prints
      the new state to Serial Monitor.
    */
    tinyls::poll(reportLedState);

    /*
      Print the startup message once.
    */
    Serial.println(F("MTW TinyLoopScheduler started"));
}

void loop()
{
    /*
      Execute one complete scheduler pass.

      PERIODIC tasks execute before the POLL phase, so reportLedState()
      can observe and print the state changed by blinkLed() during the
      same scheduler pass.

      tick() should be called continuously. Do not place delay() or other
      long blocking operations in loop().
    */
    tinyls::tick();
}