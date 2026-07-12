// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — PARTED Restart Task

  Demonstrates:
    - enabling PARTED support;
    - splitting one task into several execution steps;
    - sequential transition with TLS_NEXT();
    - repeating a step with TLS_NEXT(N);
    - jumping directly to another numbered step;
    - restarting the task with TLS_RESTART();
    - persistent state shared between PARTED steps.

  Hardware:
    - any supported AVR Arduino board;
    - built-in LED connected to LED_BUILTIN.

  Task sequence:

    Step 0:
      - resets the accumulator to zero;
      - switches the built-in LED on;
      - TLS_NEXT() selects step 1.

    Step 1:
      - increments the accumulator;
      - while the value is below 1023, TLS_NEXT(1) repeats step 1;
      - when the value reaches 1023, the LED is switched off;
      - TLS_NEXT(3) selects step 3.

    Step 3:
      - continues incrementing the accumulator;
      - while the value is below 2047, TLS_NEXT(3) repeats step 3;
      - when the value reaches 2047, TLS_RESTART() selects step 0.

  Behavior:
    - the LED is switched on while the accumulator grows from 0 to 1023;
    - the LED is switched off while the accumulator grows from 1023 to 2047;
    - after reaching 2047, the task restarts from step 0;
    - the sequence repeats continuously;
    - each PARTED invocation executes only one selected step;
    - delay() is not used.

  Important:
    - the accumulator counts PARTED executions, not milliseconds;
    - the visible LED timing depends on scheduler pass frequency,
      enabled interrupts and application load;
    - PARTED executes through the POLL phase;
    - automatic local variables do not persist between steps;
    - persistent data must use static or global variables;
    - TLS_NEXT(), TLS_NEXT(N), and TLS_RESTART() immediately return
      from the current callback invocation.

*/

#include <Arduino.h>

/*
  PARTED is disabled by default.

  It must be enabled before including the library header.
  All other library settings use their default values.
*/
#define TLS_PARTED TLS_ENABLE

#include <MTW_TinyLoopScheduler.h>

/*
  Accumulator shared between all PARTED steps.

  It must persist because the PARTED function returns after every step.
*/
static uint16_t accumulator = 0u;

/*
  Multi-step PARTED task.

  Execution route:

      step 0
         ↓
      step 1 repeated until accumulator reaches 1023
         ↓
      step 3 repeated until accumulator reaches 2047
         ↓
      restart at step 0
*/
static void partedTask()
{
    TLS_PART_START(partedTask) {

        /*
          Step 0: initialize a new cycle.
        */
        TLS_STEP(0) {
            /*
              Reset the shared accumulator.
            */
            accumulator = 0u;

            /*
              Switch the built-in LED on.
            */
            digitalWrite(LED_BUILTIN, HIGH);

            /*
              Select the next sequential step:

                  step 0 -> step 1

              Step 1 executes during a later POLL pass.
            */
            TLS_NEXT();
        }

        /*
          Step 1: increment the accumulator while the LED is on.
        */
        TLS_STEP(1) {
            /*
              Perform one small unit of work.
            */
            ++accumulator;

            /*
              Keep step 1 selected while the first threshold
              has not been reached.

              TLS_NEXT(1) selects step 1 again and immediately returns.
            */
            if (accumulator < 1023u) {
                TLS_NEXT(1);
            }

            /*
              The accumulator has reached 1023.

              Switch the LED off.
            */
            digitalWrite(LED_BUILTIN, LOW);

            /*
              Jump directly to step 3.

              PARTED step numbers do not need to be consecutive.
            */
            TLS_NEXT(3);
        }

        /*
          Step 3: continue incrementing while the LED is off.
        */
        TLS_STEP(3) {
            /*
              Perform one more small unit of work.
            */
            ++accumulator;

            /*
              Keep step 3 selected while the second threshold
              has not been reached.
            */
            if (accumulator < 2047u) {
                TLS_NEXT(3);
            }

            /*
              The accumulator has reached 2047.

              Keep the task registered in POLL and select step 0.
              The next cycle begins during a later POLL pass.
            */
            TLS_RESTART();
        }

    } TLS_PART_END();
}

void setup()
{
    /*
      Configure the built-in LED pin as an output.
    */
    pinMode(LED_BUILTIN, OUTPUT);

    /*
      Start with the LED switched off.

      PARTED step 0 switches it on.
    */
    digitalWrite(LED_BUILTIN, LOW);

    /*
      Initialize the scheduler before registering tasks.
    */
    tinyls::init();

    /*
      Register the PARTED function in the POLL queue.

      The first POLL invocation initializes the PARTED state
      and executes step 0.
    */
    tinyls::poll(partedTask);
}

void loop()
{
    /*
      Execute one complete scheduler pass.

      During the POLL phase, the stored PARTED state selects
      one TLS_STEP block for execution.

      tick() should be called continuously. Do not place delay()
      or other long blocking operations in loop().
    */
    tinyls::tick();
}
