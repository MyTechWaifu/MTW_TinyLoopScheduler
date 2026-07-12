// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — Config Minimal Kernel

  Demonstrates:
    - selection of the smallest predefined TLS_TINY profile;
    - compile-time removal of unused scheduler subsystems;
    - a minimal scheduler configuration with PERIODIC support only;
    - LED blinking without delay();
    - reduction of Flash and SRAM usage by removing unused functionality.

  Hardware:
    - Arduino Uno;
    - built-in LED connected to LED_BUILTIN.

  Configuration:
    - profile: TLS_TINY;
    - PERIODIC: enabled;
    - DELAYED: disabled;
    - POSTED: disabled;
    - POLL: disabled;
    - ISR deferred requests: disabled;
    - PARTED: disabled.

  Behavior:
    - blinkLed() is registered as the only scheduler task;
    - the scheduler executes blinkLed() every 1000 milliseconds;
    - each execution changes the built-in LED state;
    - the LED stays ON for 1 second;
    - the LED stays OFF for 1 second;
    - one complete blink cycle takes 2 seconds;
    - no Serial code is used, keeping the compiled project smaller.

  Important:
    - all compile-time configuration macros must be defined before including
      MTW_TinyLoopScheduler.h;
    - disabled subsystems contribute no public API declarations;
    - disabled subsystems contribute no queue storage or queue counters;
    - disabled subsystems contribute no corresponding tick() phases;
    - only tinyls::every() and tinyls::removeEvery() are available from the
      task-management API in this configuration;
    - after(), post(), poll(), postISR(), and PARTED must not be used;
    - changing the configuration requires rebuilding the project.

  Compare build sizes:
    1. build this project and note Flash and SRAM usage in the PlatformIO
       build output;
    2. build an example using the default configuration;
    3. compare the results;
    4. enable only the scheduler subsystems required by the application.

  Power saving:
    - MTW TinyLoopScheduler may enter AVR SLEEP_MODE_IDLE while waiting
      for the next PERIODIC deadline;
    - Arduino timer interrupts continue operating and update millis();
    - timer interrupts wake the processor so the scheduler can check whether
      the PERIODIC task is due;
    - SLEEP_MODE_IDLE is a lightweight idle state, not a deep-sleep mode.

*/

#include <Arduino.h>

/*
  Select the smallest predefined scheduler profile.

  TLS_TINY recommends up to one active or pending task in each ordinary
  scheduler queue. This project uses only one PERIODIC task.
*/
#define TLS_PROFILE TLS_TINY

/*
  Keep only the PERIODIC subsystem.

  TLS_PERIODIC is enabled explicitly to make the minimal configuration
  visible in the example.
*/
#define TLS_PERIODIC TLS_ENABLE

/*
  Remove every unused scheduler subsystem from the build.
*/
#define TLS_DELAYED TLS_DISABLE
#define TLS_POSTED  TLS_DISABLE
#define TLS_POLL    TLS_DISABLE
#define TLS_ISR     TLS_DISABLE
#define TLS_PARTED  TLS_DISABLE

/*
  All MTW TinyLoopScheduler compile-time configuration macros must appear
  before this include.
*/
#include <MTW_TinyLoopScheduler.h>

/*
  Current state of the built-in LED.

  The LED starts switched off.
*/
static uint8_t ledState = LOW;

/*
  PERIODIC callback.

  The scheduler executes this function every 1000 milliseconds.
  Each execution changes the stored LED state and writes it to the pin.
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
}

void setup()
{
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
      Register the only task used by this configuration.

      blinkLed() executes every 1000 milliseconds.
    */
    tinyls::every(blinkLed, 1000u);
}

void loop()
{
    /*
      Execute one complete scheduler pass.

      In this configuration, tick() contains only:
        - the scheduler core;
        - the PERIODIC phase;
        - the AVR IDLE backend.

      tick() should be called continuously. Do not place delay() or other
      long blocking operations in loop().
    */
    tinyls::tick();
}

