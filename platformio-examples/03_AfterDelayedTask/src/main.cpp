// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — After Delayed Task

  Demonstrates:
    - scheduler initialization with tinyls::init();
    - a one-shot delayed task registered with tinyls::after();
    - delayed callback execution without delay();
    - reading the scheduler time from tinyls::now;
    - automatic removal of a DELAYED task after execution.

  Hardware:
    - any supported AVR Arduino board;
    - USB connection to a computer for Serial Monitor.

  Serial Monitor:
    - baud rate: 9600.

  Behavior:
    - delayedTask() is registered during setup();
    - the scheduler waits 3000 milliseconds;
    - delayedTask() is then executed once;
    - the callback prints the scheduler time at which it runs;
    - the task is automatically removed after execution;
    - delayedTask() is not called again during the current program run;
    - pressing the board reset button restarts the sketch;
    - after reset, setup() registers the one-shot task again, so it executes
      once more after another 3000 milliseconds.

  Expected output:

    MTW TinyLoopScheduler started
    Delayed task scheduled after 3000 ms
    Delayed task executed at 3000 ms

  The exact displayed time can be slightly greater than 3000 milliseconds
  if loop() or another callback blocks scheduler execution.

  Important:
    - TinyLoopScheduler is a cooperative scheduler;
    - callbacks execute synchronously inside tinyls::tick();
    - callbacks should complete quickly;
    - tick() should be called continuously from loop();
    - long blocking operations delay all scheduler tasks.

  Power saving:
    - MTW TinyLoopScheduler automatically uses AVR SLEEP_MODE_IDLE when
      the current scheduler pass has no immediate ISR, PERIODIC, or DELAYED
      work to execute;
    - the CPU sleeps until the next enabled interrupt while timers and
      interrupt-capable peripherals continue operating;
    - SLEEP_MODE_IDLE is a lightweight idle state, not a deep-sleep mode.

*/

#include <Arduino.h>
#include <MTW_TinyLoopScheduler.h>

/*
  DELAYED callback.

  The scheduler executes this function once, after the configured delay
  has elapsed.

  tinyls::now contains the cached millis() value captured at the beginning
  of the current scheduler pass.
*/
static void delayedTask()
{
    /*
      Print the scheduler timestamp at which the delayed task executes.
    */
    Serial.print(F("Delayed task executed at "));
    Serial.print(tinyls::now);
    Serial.println(F(" ms"));
}

void setup()
{
    /*
      Initialize serial communication.

      Open Serial Monitor and select 9600 baud.
    */
    Serial.begin(9600);

    /*
      Initialize the scheduler before registering any tasks.
    */
    tinyls::init();

    /*
      Print the initial status messages.
    */
    Serial.println(F("MTW TinyLoopScheduler started"));
    Serial.println(F("Delayed task scheduled after 3000 ms"));

    /*
      Register delayedTask() as a one-shot DELAYED task.

      The callback becomes due after 3000 milliseconds.
      After execution, the scheduler automatically removes the task.

      Resetting the board restarts the sketch, so setup() runs again
      and registers this one-shot task again.
    */
    tinyls::after(delayedTask, 3000u);
}

void loop()
{
    /*
      Execute one complete scheduler pass.

      tick() checks whether the DELAYED task has reached its deadline.
      It should be called continuously without delay() or other long
      blocking operations.
    */
    tinyls::tick();
}