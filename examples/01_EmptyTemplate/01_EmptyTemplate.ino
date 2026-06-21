// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — Empty Template

  Minimal template for a new project using MTW TinyLoopScheduler.

  Basic workflow:
    1. initialize the hardware;
    2. initialize the scheduler with tinyls::init();
    3. register initial jobs;
    4. call tinyls::tick() continuously from loop().

  Scheduler callbacks must:
    - have the signature void functionName(void);
    - accept no arguments;
    - return no value;
    - return quickly;
    - avoid delay() and other long blocking operations.

  Supported targets:
    - AVR microcontrollers using an Arduino-compatible core.

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
  Add scheduler callback functions here.

  Callback example:

  static void myTask()
  {
      // Short non-blocking operation.
  }
*/

void setup()
{
    /*
      Initialize pins, peripherals, Serial, sensors, displays,
      communication interfaces, and other hardware here.
    */

    /*
      Initialize the scheduler after hardware initialization
      and before registering the initial jobs.
    */
    tinyls::init();

    /*
      Register initial scheduler jobs here.

      Examples:

      tinyls::every(myTask, 1000u);
      tinyls::after(myTask, 500u);
      tinyls::post(myTask);
      tinyls::poll(myTask);
    */
}

void loop()
{
    /*
      Execute one scheduler pass.

      Call tick() continuously and only from loop().
      Do not call delay() or run long blocking operations here.
    */
    tinyls::tick();
}
