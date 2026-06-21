// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — Config Period

  Demonstrates:
    - selection of 16-bit or 32-bit period storage;
    - compile-time configuration before including the library header;
    - use of TLS_PERIOD with PERIODIC and DELAYED tasks;
    - LED blinking and delayed execution without delay();
    - comparison of SRAM usage between both configurations.

  Hardware:
    - any supported AVR Arduino board;
    - built-in LED connected to LED_BUILTIN;
    - USB connection to a computer for Serial Monitor.

  Serial Monitor:
    - baud rate: 9600.

  Available period configurations:

    TLS_PERIOD 16
      - PERIODIC periods are stored as uint16_t;
      - valid period and delay values are 0 through 65,535 milliseconds;
      - approximately 65.535 seconds is the maximum supported interval;
      - each PERIODIC record uses 2 fewer SRAM bytes than in 32-bit mode;
      - values greater than 65,535 are not rejected or clamped;
      - excessive values are narrowed modulo 65,536.

    TLS_PERIOD 32
      - PERIODIC periods are stored as uint32_t;
      - supported intervals can be longer than 65.535 seconds;
      - the maximum supported interval is 2,147,483,647 milliseconds;
      - this is approximately 24.855 days;
      - TLS_PERIOD 32 is used by default when TLS_PERIOD is not defined.

  DELAYED tasks:
    - their absolute deadlines remain 32-bit in both configurations;
    - however, after() uses the selected TLS_PERIOD width when accepting
      the requested delay;
    - therefore, TLS_PERIOD 16 also limits after() delays to 65,535 ms.

  Try both configurations:
    1. leave only one TLS_PERIOD line enabled below;
    2. compile the sketch;
    3. note the SRAM usage in the Arduino IDE build report;
    4. switch to the other value;
    5. compile the sketch again and compare the result.

  Behavior:
    - blinkLed() executes every 1000 milliseconds;
    - each execution changes the built-in LED state;
    - the new LED state is printed to Serial Monitor;
    - delayedTask() executes once after 5000 milliseconds;
    - both intervals fit safely into either period configuration;
    - runtime behavior is therefore the same with TLS_PERIOD 16 and 32.

  Expected output:

    MTW TinyLoopScheduler started
    LED ON
    LED OFF
    LED ON
    LED OFF
    DELAYED task executed at 5000 ms
    LED ON
    LED OFF

  The exact delayed execution time can be slightly greater than 5000 ms
  if another callback or blocking application code delays the scheduler.

  Important:
    - TLS_PERIOD must be defined before including
      MTW_TinyLoopScheduler.h;
    - enable only one TLS_PERIOD line;
    - changing TLS_PERIOD requires recompiling the sketch;
    - use TLS_PERIOD 16 only when every PERIODIC interval and DELAYED timeout
      is guaranteed to remain within 0 through 65,535 milliseconds;
    - use TLS_PERIOD 32 when longer intervals are required;
    - callbacks should complete quickly;
    - do not place delay() or other long blocking operations in loop().

  Power saving:
    - MTW TinyLoopScheduler automatically uses AVR SLEEP_MODE_IDLE when
      the current scheduler pass has no immediate ISR, PERIODIC, or DELAYED
      work to execute;
    - timers and interrupt-capable peripherals continue operating while
      the processor is in SLEEP_MODE_IDLE;
    - SLEEP_MODE_IDLE is a lightweight idle state, not a deep-sleep mode.

*/

/*
  Select one period-storage width.

  Leave only one line enabled.

  TLS_PERIOD 16 is enabled in this example because both configured
  intervals are shorter than 65,536 milliseconds.
*/
#define TLS_PERIOD 16
// #define TLS_PERIOD 32

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
  PERIODIC task.

  The scheduler executes this callback every 1000 milliseconds.
  Each execution changes the LED state and prints the new state.
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
      Print the current LED state.
    */
    if (ledState == HIGH) {
        Serial.println(F("LED ON"));
    } else {
        Serial.println(F("LED OFF"));
    }
}

/*
  DELAYED task.

  The scheduler executes this callback once after 5000 milliseconds.

  tinyls::now contains the cached millis() value captured at the beginning
  of the current scheduler pass.
*/
static void delayedTask()
{
    Serial.print(F("DELAYED task executed at "));
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
      Register a repeating PERIODIC task.

      1000 milliseconds fits safely into both 16-bit and 32-bit
      period configurations.
    */
    tinyls::every(blinkLed, 1000u);

    /*
      Register a one-shot DELAYED task.

      5000 milliseconds also fits safely into both configurations.
    */
    tinyls::after(delayedTask, 5000u);

    /*
      Print the startup message once.
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