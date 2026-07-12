// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — Every Periodic Task

  Demonstrates:
    - scheduler initialization with tinyls::init();
    - a repeating task registered with tinyls::every();
    - periodic callback execution without delay();
    - reading the scheduler time from tinyls::now;
    - continuous scheduler operation through tinyls::tick().

  Hardware:
    - any supported AVR Arduino board;
    - USB connection to a computer for Serial Monitor.

  Serial Monitor:
    - baud rate: 9600.

  Behavior:
    - periodicTask() is registered as a PERIODIC task;
    - the scheduler calls periodicTask() every 1000 milliseconds;
    - each call increases the execution counter;
    - the callback prints its execution number and scheduler time;
    - delay() and manual millis() comparisons are not required.

  Expected output:

    MTW TinyLoopScheduler started
    Periodic task #1 at 1000 ms
    Periodic task #2 at 2000 ms
    Periodic task #3 at 3000 ms

  Important:
    - TinyLoopScheduler is a cooperative scheduler;
    - callbacks execute inside tinyls::tick();
    - callbacks should complete quickly;
    - tick() should be called continuously from loop();
    - long blocking operations delay all other scheduler tasks.

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
  Number of completed PERIODIC task executions.

  The variable is increased every time periodicTask() is called.
*/
static uint32_t runCount = 0u;

/*
  PERIODIC callback.

  The scheduler calls this function every 1000 milliseconds.

  tinyls::now contains the cached millis() value captured at the beginning
  of the current scheduler pass. Reading it avoids an additional millis()
  call inside the callback.
*/
static void periodicTask()
{
    /*
      Count the current task execution.
    */
    ++runCount;

    /*
      Print the execution number.
    */
    Serial.print(F("Periodic task #"));
    Serial.print(runCount);

    /*
      Print the scheduler timestamp for the current pass.
    */
    Serial.print(F(" at "));
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
      Register periodicTask() as a repeating PERIODIC task.

      The first execution is scheduled after 1000 milliseconds.
      Further executions are scheduled every 1000 milliseconds.
    */
    tinyls::every(periodicTask, 1000u);

    /*
      Confirm that setup is complete.
    */
    Serial.println(F("MTW TinyLoopScheduler started"));
}

void loop()
{
    /*
      Execute one complete scheduler pass.

      tick() should be called continuously. Do not place delay() or other
      long blocking operations in loop().
    */
    tinyls::tick();
}