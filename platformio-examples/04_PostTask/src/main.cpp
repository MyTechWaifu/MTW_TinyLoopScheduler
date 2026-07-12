// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — Post Task

  Demonstrates:
    - scheduler initialization with tinyls::init();
    - a one-shot task queued with tinyls::post();
    - deferred task execution from tinyls::tick();
    - the difference between posting a callback and calling it directly;
    - automatic removal of a POSTED task after execution.

  Hardware:
    - any supported AVR Arduino board;
    - USB connection to a computer for Serial Monitor.

  Serial Monitor:
    - baud rate: 9600.

  Behavior:
    - setup() queues postedTask() with tinyls::post();
    - post() does not execute the callback immediately;
    - setup() continues and prints its next message;
    - postedTask() is executed later from the POSTED phase of tinyls::tick();
    - the callback runs only once;
    - after execution, the task is automatically removed from the POSTED queue;
    - pressing the board reset button restarts the sketch and queues the
      one-shot task again.

  Expected output:

    MTW TinyLoopScheduler started
    Queueing POSTED task
    setup() continues after post()
    POSTED task executed from tick()

  The output demonstrates that postedTask() is not called directly by post().
  It runs only after loop() starts calling tinyls::tick().

  Important:
    - POSTED tasks are one-shot tasks;
    - POSTED callbacks execute synchronously inside tinyls::tick();
    - callbacks should complete quickly;
    - tick() should be called continuously from loop();
    - a POSTED callback must not continuously post itself because the scheduler
      completely drains the POSTED queue before leaving the POSTED phase;
    - persistent repeated execution should use tinyls::poll() or
      tinyls::every() instead.

  Power saving:
    - MTW TinyLoopScheduler automatically uses AVR SLEEP_MODE_IDLE when
      the current scheduler pass has no immediate ISR, PERIODIC, or DELAYED
      work to execute;
    - POSTED tasks execute after the processor wakes from SLEEP_MODE_IDLE;
    - timers and interrupt-capable peripherals continue operating while
      the processor is in SLEEP_MODE_IDLE;
    - SLEEP_MODE_IDLE is a lightweight idle state, not a deep-sleep mode.

*/

#include <Arduino.h>
#include <MTW_TinyLoopScheduler.h>

/*
  POSTED callback.

  This function is not called directly from setup().
  It is queued with tinyls::post() and later executed by tinyls::tick().
*/
static void postedTask()
{
    Serial.println(F("POSTED task executed from tick()"));
}

void setup()
{
    /*
      Initialize serial communication.

      Open Serial Monitor and select 9600 baud.
    */
    Serial.begin(9600);

    /*
      Initialize the scheduler before registering or posting any tasks.
    */
    tinyls::init();

    Serial.println(F("MTW TinyLoopScheduler started"));
    Serial.println(F("Queueing POSTED task"));

    /*
      Queue postedTask() for one-shot execution in the POSTED phase.

      post() only places the callback into the POSTED queue.
      It does not execute postedTask() immediately.
    */
    tinyls::post(postedTask);

    /*
      This message is printed before postedTask() executes.

      postedTask() will run later, after loop() calls tinyls::tick().
    */
    Serial.println(F("setup() continues after post()"));
}

void loop()
{
    /*
      Execute one complete scheduler pass.

      During the POSTED phase, tick() removes postedTask() from the queue
      and executes it once.

      tick() should be called continuously. Do not place delay() or other
      long blocking operations in loop().
    */
    tinyls::tick();
}