// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — Blink With Variable

  Demonstrates:
    - scheduler initialization with tinyls::init();
    - a repeating task registered with tinyls::every();
    - LED blinking without delay();
    - explicit LED state stored in a variable.

  Hardware:
    - any supported AVR Arduino board;
    - built-in LED connected to LED_BUILTIN.

  Behavior:
    - LED state changes every 1000 milliseconds;
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
  Current LED state.

  The variable is initialized to HIGH because the LED is switched on
  immediately in setup().
*/
static uint8_t ledState = HIGH;

/*
  PERIODIC callback.

  This function changes the stored LED state and writes the new state
  to the output pin.
*/
static void blinkLed()
{
    ledState = (ledState == HIGH) ? LOW : HIGH;
    digitalWrite(LED_BUILTIN, ledState);
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    /*
      Start with the LED switched on.
    */
    digitalWrite(LED_BUILTIN, ledState);

    /*
      Initialize the scheduler before registering jobs.
    */
    tinyls::init();

    /*
      Change the LED state every 1000 milliseconds.
    */
    tinyls::every(blinkLed, 1000u);
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