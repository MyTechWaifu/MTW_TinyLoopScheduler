// SPDX-License-Identifier: Apache-2.0
/*
  MTW TinyLoopScheduler — Analog Read Serial

  Demonstrates:
    - reading an analog input with analogRead();
    - sending the raw ADC value to Serial Monitor;
    - registering a repeating task with tinyls::every();
    - periodic sampling without delay() or manual millis() checks.

  Hardware:
    - supported AVR Arduino board;
    - analog signal connected to A0;
    - for a potentiometer:
        one outer pin to 5V,
        the other outer pin to GND,
        the center pin to A0.

  Behavior:
    - the analog input is sampled every 250 milliseconds;
    - the raw ADC value is printed to Serial Monitor at 9600 baud;
    - on a typical 10-bit AVR ADC, the displayed range is 0 to 1023.

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

#include <Arduino.h>
#include <MTW_TinyLoopScheduler.h>

static const uint8_t ANALOG_PIN = A0;
static const uint32_t SAMPLE_PERIOD_MS = 250u;

/*
  PERIODIC callback.

  Reads the raw ADC value and sends it to Serial Monitor.
*/
static void readAndPrintAnalog()
{
    int sensorValue = analogRead(ANALOG_PIN);
    Serial.println(sensorValue);
}

void setup()
{
    /*
      Start Serial Monitor communication.
    */
    Serial.begin(9600);

    /*
      Initialize the scheduler after hardware initialization
      and before registering jobs.
    */
    tinyls::init();

    /*
      Read and print the analog input every 250 milliseconds.
    */
    tinyls::every(readAndPrintAnalog, SAMPLE_PERIOD_MS);
}

void loop()
{
    /*
      Execute one scheduler pass.

      No delay() and no manual millis() checks are required.
    */
    tinyls::tick();
}
