// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — Blink Without Variable

  Demonstrates:
    - scheduler initialization with tinyls::init();
    - a repeating task registered with tinyls::every();
    - a one-shot delayed task registered with tinyls::after();
    - LED blinking without delay();
    - LED blinking without a user state variable.

  Hardware:
    - any supported AVR Arduino board;
    - built-in LED connected to LED_BUILTIN.

  Behavior:
    - turnLedOn() runs every 2000 milliseconds;
    - each turnLedOn() schedules turnLedOff() after 1000 milliseconds;
    - LED stays ON for 1 second;
    - LED stays OFF for 1 second;
    - one complete blink cycle takes 2 seconds.

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

/*
  One-shot DELAYED callback.

  This function is scheduled by turnLedOn() and executes once after
  1000 milliseconds.
*/
static void turnLedOff()
{
    digitalWrite(LED_BUILTIN, LOW);
}

/*
  PERIODIC callback.

  The function switches the LED on and schedules a separate one-shot
  callback that switches it off after 1000 milliseconds.
*/
static void turnLedOn()
{
    digitalWrite(LED_BUILTIN, HIGH);

    /*
      Switch the LED off once after 1000 milliseconds.
    */
    tinyls::after(turnLedOff, 1000u);
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    /*
      Initialize the scheduler before registering jobs.
    */
    tinyls::init();

    /*
      Switch the LED on every 2000 milliseconds.
    */
    tinyls::every(turnLedOn, 2000u);

    /*
      Start the first blink immediately.

      turnLedOn() also schedules the first turnLedOff() callback.
    */
    turnLedOn();
}

void loop()
{
    /*
      Execute scheduler jobs.

      tick() should be called continuously. Do not place delay() or other
      long blocking operations in loop().
    */
    tinyls::tick();
}