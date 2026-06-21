// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — Poll Task

  Demonstrates:
    - scheduler initialization with tinyls::init();
    - a persistent task registered with tinyls::poll();
    - execution of a POLL task during scheduler passes;
    - a short condition check inside a POLL callback;
    - continuous operation without delay().

  Hardware:
    - any supported AVR Arduino board;
    - USB connection to a computer for Serial Monitor.

  Serial Monitor:
    - baud rate: 9600.

  Behavior:
    - pollTask() is registered once during setup();
    - the scheduler calls pollTask() during each POLL phase;
    - every call increments the accumulator by one;
    - when the accumulator reaches 1023, the callback prints a message;
    - after printing, the accumulator is reset to zero;
    - accumulation then starts again;
    - the POLL task remains registered until it is explicitly removed or
      the board is reset.

  Expected output:

    MTW TinyLoopScheduler started
    Accumulator reached 1023
    Accumulator reached 1023
    Accumulator reached 1023

  Important:
    - the accumulator counts POLL callback executions, not milliseconds;
    - the time between messages depends on how frequently tinyls::tick()
      executes and on the current scheduler workload;
    - POLL callbacks should contain only short, non-blocking operations;
    - printing on every POLL invocation should be avoided because continuous
      Serial output can block the cooperative scheduler.

  Power saving:
    - MTW TinyLoopScheduler may enter AVR SLEEP_MODE_IDLE before the POLL
      phase when no immediate ISR, PERIODIC, or DELAYED work is pending;
    - after an interrupt wakes the processor, the scheduler continues with
      the POSTED and POLL phases;
    - SLEEP_MODE_IDLE is a lightweight idle state, not a deep-sleep mode.

*/

#include <MTW_TinyLoopScheduler.h>

/*
  POLL execution accumulator.

  uint16_t is sufficient because the value only needs to reach 1023.
*/
static uint16_t accumulator = 0u;

/*
  POLL callback.

  The scheduler calls this function during every POLL phase.

  Each invocation performs only:
    1. one increment;
    2. one condition check;
    3. an occasional Serial message.
*/
static void pollTask()
{
    /*
      Count the current POLL execution.
    */
    ++accumulator;

    /*
      Return immediately until the accumulator reaches 1023.
    */
    if (accumulator < 1023u) {
        return;
    }

    /*
      The required number of POLL executions has been reached.
    */
    Serial.println(F("Accumulator reached 1023"));

    /*
      Reset the accumulator and start counting again.
    */
    accumulator = 0u;
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
      Register pollTask() as a persistent POLL task.

      Unlike after() and post(), poll() registers a task that remains active
      and is called repeatedly during scheduler operation.
    */
    tinyls::poll(pollTask);

    /*
      Print the startup message once.
    */
    Serial.println(F("MTW TinyLoopScheduler started"));
}

void loop()
{
    /*
      Execute one complete scheduler pass.

      During the POLL phase, tick() calls pollTask().

      tick() should be called continuously. Do not place delay() or other
      long blocking operations in loop().
    */
    tinyls::tick();
}