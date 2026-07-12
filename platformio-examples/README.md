# MTW TinyLoopScheduler — PlatformIO Examples

Ready-to-build PlatformIO examples for
[MTW TinyLoopScheduler](https://github.com/MyTechWaifu/MTW_TinyLoopScheduler).

Each directory inside `platformio-examples/` is an independent PlatformIO
project.

## Project Structure

Every example uses the following structure:

```text
ExampleName/
├── platformio.ini
└── src/
    └── main.cpp
```

The examples are currently configured for:

- Arduino Uno;
- Arduino Nano ATmega328;
- Arduino Mega 2560;
- Arduino Leonardo.

## Requirements

Use one of the following:

- Visual Studio Code with the PlatformIO IDE extension;
- PlatformIO Core CLI.

Current target configuration:

```ini
[env]
platform = atmelavr
framework = arduino

lib_deps =
    techiewaifu/mtw-tinyloopscheduler

[env:uno]
board = uno

[env:nanoatmega328]
board = nanoatmega328

[env:megaatmega2560]
board = megaatmega2560

[env:leonardo]
board = leonardo
```

The library is installed automatically from the PlatformIO Registry.

No library version is pinned, so PlatformIO resolves the dependency using the
latest available published version.

## Open an Example

In Visual Studio Code with PlatformIO IDE:

1. Open PlatformIO Home.
2. Select **Open Project**.
3. Select one individual example directory.

For example:

```text
platformio-examples/02_EveryPeriodicTask
```

Open the example directory containing `platformio.ini`, not the entire
`platformio-examples/` directory.

## Build

Running the following command from an example directory builds the project for
all configured boards:

```bash
pio run
```

For example:

```bash
cd platformio-examples/02_EveryPeriodicTask
pio run
```

From the repository root:

```bash
pio run -d platformio-examples/02_EveryPeriodicTask
```

To build only one board environment:

```bash
pio run -e uno
pio run -e nanoatmega328
pio run -e megaatmega2560
pio run -e leonardo
```

From the repository root:

```bash
pio run -d platformio-examples/02_EveryPeriodicTask -e uno
```

## Upload

When uploading, always select the environment that matches the physically
connected board.

### Arduino Uno

```bash
pio run -e uno -t upload
```

### Arduino Nano ATmega328

```bash
pio run -e nanoatmega328 -t upload
```

### Arduino Mega 2560

```bash
pio run -e megaatmega2560 -t upload
```

### Arduino Leonardo

```bash
pio run -e leonardo -t upload
```

To upload from the repository root:

```bash
pio run -d platformio-examples/02_EveryPeriodicTask -e uno -t upload
```

Do not use the following command when multiple environments are configured:

```bash
pio run -t upload
```

Without `-e`, PlatformIO processes every configured environment and attempts
to upload each firmware image sequentially.

## Serial Monitor

Examples that use Serial communication operate at `9600` baud.

Open Serial Monitor with:

```bash
pio device monitor -b 9600
```

Expected output, where applicable, is documented in the corresponding
`src/main.cpp` file.

## Examples

| Directory | Demonstrates |
|:----------|:-------------|
| [`01_EmptyTemplate`](./01_EmptyTemplate/) | Minimal scheduler project template |
| [`02_EveryPeriodicTask`](./02_EveryPeriodicTask/) | Repeating task with `tinyls::every()` |
| [`03_AfterDelayedTask`](./03_AfterDelayedTask/) | One-shot delayed task with `tinyls::after()` |
| [`04_PostTask`](./04_PostTask/) | Deferred one-shot task with `tinyls::post()` |
| [`05_PollTask`](./05_PollTask/) | Persistent task with `tinyls::poll()` |
| [`06_ISRPostTask`](./06_ISRPostTask/) | Deferred ISR work with `tinyls::postISR()` |
| [`07_ConfigProfile`](./07_ConfigProfile/) | Predefined compile-time queue profiles |
| [`08_ConfigManualQueues`](./08_ConfigManualQueues/) | Manual scheduler queue capacities |
| [`09_ConfigPeriod`](./09_ConfigPeriod/) | 16-bit and 32-bit period storage |
| [`10_ConfigMinimalKernel`](./10_ConfigMinimalKernel/) | Minimal PERIODIC-only kernel configuration |
| [`11_BlinkWithVariable`](./11_BlinkWithVariable/) | Non-blocking LED blink with a state variable |
| [`12_BlinkWithoutVariable`](./12_BlinkWithoutVariable/) | Non-blocking LED blink without a state variable |
| [`13_ButtonPoll`](./13_ButtonPoll/) | Button debounce through a POLL task |
| [`14_ButtonPeriodic`](./14_ButtonPeriodic/) | Periodic button sampling and debounce |
| [`15_ButtonISRWithPoll`](./15_ButtonISRWithPoll/) | ISR deferral with temporary POLL debounce |
| [`16_ButtonISRWithDelayedCheck`](./16_ButtonISRWithDelayedCheck/) | ISR deferral with delayed debounce check |
| [`17_AnalogReadSerial`](./17_AnalogReadSerial/) | Periodic ADC sampling and Serial output |
| [`18_PartedTask`](./18_PartedTask/) | Multi-step PARTED task execution |

## Arduino IDE Examples

Equivalent Arduino IDE sketches are available in:

```text
examples/
```

Arduino IDE examples use `.ino` files:

```text
examples/02_EveryPeriodicTask/02_EveryPeriodicTask.ino
```

PlatformIO examples use ordinary C++ source files:

```text
platformio-examples/02_EveryPeriodicTask/src/main.cpp
```

PlatformIO examples therefore explicitly include:

```cpp
#include <Arduino.h>
```

followed by:

```cpp
#include <MTW_TinyLoopScheduler.h>
```

For compile-time configuration examples, all `TLS_*` macros must be defined
before including the scheduler header:

```cpp
#include <Arduino.h>

#define TLS_PROFILE TLS_TINY

#include <MTW_TinyLoopScheduler.h>
```

## Basic Usage

Initialize the scheduler in `setup()`:

```cpp
void setup()
{
    tinyls::init();

    tinyls::every(myTask, 1000u);
}
```

Execute the scheduler continuously from `loop()`:

```cpp
void loop()
{
    tinyls::tick();
}
```

Scheduler callbacks should:

- accept no arguments;
- return no value;
- complete quickly;
- avoid `delay()`;
- avoid long blocking operations.

## License

These examples are distributed under the Apache License 2.0.

See the repository `LICENSE` and `NOTICE` files for details.
