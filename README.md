# MTW TinyLoopScheduler
[![DOI](https://zenodo.org/badge/1275970080.svg)](https://doi.org/10.5281/zenodo.20784799)

A compact cooperative scheduler for Arduino-compatible targets, with native AVR support and a portable Generic backend.

MTW TinyLoopScheduler replaces scattered `millis()` checks with named callbacks dispatched from one explicit place:

```cpp
tinyls::tick();
```

The library is header-only, uses static compile-time storage, does not allocate memory from the heap, and allows unused scheduler subsystems to be removed at compile time.

## Status

Current version: **0.2.1**.

The platform backend is selected automatically from the target selected in
Arduino IDE or PlatformIO. No platform selector is required in a sketch.

| Build target | Automatically selected backend | `TLS_USE_IDLE` default | `TLS_ISR` default |
| --- | --- | --- | --- |
| AVR, including classic AVR, megaAVR 0-series, and AVR Dx | AVR | `TLS_ENABLE` | `TLS_ENABLE` |
| Other Arduino-compatible targets | Generic | `TLS_DISABLE` | `TLS_DISABLE` |

The current selector uses `__AVR__`: AVR targets use the AVR backend; all other
targets use Generic. STM32, RP2040/RP2350, and Renesas targets currently share
Generic; dedicated hardware backends have not been added yet.

Generic provides PERIODIC, DELAYED, POSTED, POLL, and optional PARTED using
`Arduino.h` and `millis()`. It provides no TLS sleep backend, critical-section
backend, or `postISR()`. Explicitly enabling unsupported IDLE or ISR support
produces a compile-time error.

`TLS_PROFILE` is separate: it controls queue capacities and is not selected
automatically from the platform. See [platform details](docs/PLATFORMS.md).

## Hardware and platform support

### Verified microcontrollers and boards

MTW TinyLoopScheduler supports AVR and Arduino-compatible 32-bit targets. The table below lists the verified microcontrollers and representative boards.

| MCU family | Hardware-tested MCU | Example boards and devices |
| -------------------------------- | ------------------------------ | --------------------------------------------- |
| tinyAVR | **ATtiny85, ATtiny88** | Digispark, Trinket, GEMMA, MH-Tiny ATtiny88 |
| classic megaAVR | **ATmega328P** | Uno, Nano, Pro Mini |
| classic megaAVR | **ATmega328PB** | Nano 328PB |
| classic megaAVR | **ATmega32U4** | Leonardo, Micro, Pro Micro |
| classic megaAVR | **ATmega2560** | Mega 2560, Mega ADK, Mega 1280 |
| megaAVR 0-series | **ATmega4808, ATmega4809** | ATmega4808/4809 boards, Nano Every, Uno WiFi Rev2 |
| AVR Dx series | **AVR64DD32, AVR32DB32** | AVR Dx boards |
| AVR-compatible / LGT8F family | **LGT8F328P** | Nano LGT8F328P |
| Microchip / Atmel SAM | **SAM3X8E** | Arduino Due |
| Renesas RA | **RA4M1** | Arduino Uno R4 Minima |
| Raspberry Pi RP | **RP2040** | Raspberry Pi Pico |
| Raspberry Pi RP | **RP2350** | Raspberry Pi Pico 2 |


### Verified AVR clock-frequency compatibility

The following MCU and clock-frequency combinations have been verified on physical hardware.

| Clock frequency | Hardware-tested microcontrollers                                                                                        |
| --------------- | ----------------------------------------------------------------------------------------------------------------------- |
| 16 MHz          | `ATmega328P`, `ATmega328PB`, `ATmega32U4`, `ATmega2560`, `ATmega4808`, `ATmega4809`, `ATmega88PA`, `ATmega8A`, `ATtiny85`, `ATtiny88`, `AVR64DD32`, `AVR32DB32`|
| 20 MHz          | `ATmega328P`, `ATmega88PA`, `ATmega4808`, `ATmega4809`, `AVR64DD32`, `AVR32DB32`|
| 24 MHz          | `AVR64DD32`, `AVR32DB32`|
| 32 MHz          | `LGT8F328P`|

Only the combinations listed above have been explicitly tested. Other clock frequencies may work when supported by the selected microcontroller, clock source, fuse configuration, and Arduino core, but they are not currently included in the verified hardware matrix.

### Verified AVR Arduino core compatibility

The following Arduino core and MCU combinations have been successfully compiled and tested on physical hardware.

| Arduino core / board package | Hardware-tested microcontrollers         |
| ---------------------------- | ---------------------------------------- |
| Arduino AVR Boards           | `ATmega328P`, `ATmega32U4`, `ATmega2560` |
| ATTinyCore                   | `ATtiny85`, `ATtiny88`                   |
| MiniCore                     | `ATmega328P`, `ATmega88PA`, `ATmega8A`   |
| MegaCoreX                     | `ATmega4808`, `ATmega4809`   |
| DxCore                     | `AVR64DD32`, `AVR32DB32`   |
| Thinary Nano Every           | `ATmega4808`                             |
| LGT8Fx                       | `LGT8F328P`                                  |

This table describes verified MTW TinyLoopScheduler compatibility, not the complete list of microcontrollers supported by each Arduino core.



The microcontrollers listed in the Hardware-tested MCU column are confirmed working on physical hardware.

Other microcontrollers and boards from the listed families are expected to work, provided that they are supported by a compatible Arduino core and have sufficient Flash and SRAM.

### Generic compatibility

Arduino Due, Arduino Uno R4 Minima, RP2040, and RP2350 use the Generic backend.
It requires a working Arduino-compatible `millis()` source and a single main
execution context for calls into TLS. Calls from hardware ISRs, another core,
or concurrent threads are not supported.

Generic provides PERIODIC, DELAYED, POSTED, POLL, and optional PARTED.
TLS IDLE and `postISR()` are disabled by default and cannot be enabled until
a hardware backend is implemented. `TLS_ISR` does not disable the Arduino
core's hardware interrupts.

STM32 targets also use Generic. Compilation and linking have been verified
for STM32F401RE / Nucleo F401RE.

PlatformIO package metadata:

| Field | Value |
|---|---|
| Framework | Arduino |
| PlatformIO platforms (`library.json`) | `*` |
| Arduino architectures (`library.properties`) | `*` |

Actual board availability depends on the installed Arduino core, PlatformIO
platform package, and board definition.

#### Ready-to-build PlatformIO examples

Ready-to-build PlatformIO projects are available in:

[`platformio-examples/`](./platformio-examples/)

Each example is an independent PlatformIO project containing:

```text
platformio.ini
src/main.cpp
```

## Main features

- cooperative callback scheduling;
- static storage only;
- no heap allocation;
- no dynamic containers;
- header-only implementation;
- automatic AVR / Generic platform selection;
- repeating PERIODIC callbacks;
- one-shot DELAYED callbacks;
- one-shot POSTED callbacks;
- persistent POLL callbacks;
- optional deferred callback requests from AVR hardware ISRs;
- optional step-based PARTED execution;
- predefined and manual queue-capacity profiles;
- compile-time removal of unused subsystems;
- selectable 16-bit or 32-bit period storage;
- cached `millis()` value exposed as `tinyls::now`;
- independently switchable AVR IDLE and deferred-ISR support;
- automatic AVR `SLEEP_MODE_IDLE` entry when IDLE is enabled and its load gate permits it;
- optional sleep-mode guard that reaffirms `SLEEP_MODE_IDLE` before sleeping;
- rollover-safe deadline comparison within the documented interval limits.

MTW TinyLoopScheduler is not:

- a preemptive RTOS;
- a thread scheduler;
- a dynamic task manager;
- a hard real-time kernel.

## Installation

MTW TinyLoopScheduler can be installed through Arduino Library Manager or PlatformIO.

### Arduino IDE

#### Arduino Library Manager

1. Open Arduino IDE.
2. Open **Sketch → Include Library → Manage Libraries...**
3. Search for **MTW TinyLoopScheduler**.
4. Select the library.
5. Click **Install**.

Include the library with:

```cpp
#include <MTW_TinyLoopScheduler.h>
```

#### Minimal example

```cpp
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
```

This example registers one PERIODIC callback. The callback changes the built-in LED state once every 1000 milliseconds.

### PlatformIO

MTW TinyLoopScheduler can be installed in PlatformIO through the PlatformIO Registry or directly from GitHub.

PlatformIO Registry page:

https://registry.platformio.org/libraries/techiewaifu/mtw-tinyloopscheduler

#### Install from PlatformIO Registry

Add the library to `platformio.ini`:

```ini
[env:uno]
platform = atmelavr
board = uno
framework = arduino

lib_deps =
    techiewaifu/mtw-tinyloopscheduler
```

#### Install from GitHub

Use the GitHub repository directly:

```ini
[env:uno]
platform = atmelavr
board = uno
framework = arduino

lib_deps =
    https://github.com/MyTechWaifu/MTW_TinyLoopScheduler.git
```

#### Minimal example
Minimal `src/main.cpp`:

```cpp
#include <Arduino.h>
#include <MTW_TinyLoopScheduler.h>

static void turnLedOff()
{
    digitalWrite(LED_BUILTIN, LOW);
}

static void turnLedOn()
{
    digitalWrite(LED_BUILTIN, HIGH);
    tinyls::after(turnLedOff, 1000u);
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    tinyls::init();
    tinyls::every(turnLedOn, 2000u);
    turnLedOn();
}

void loop()
{
    tinyls::tick();
}
```

Build the project:

```bash
pio run
```

Upload to the board:

```bash
pio run -t upload
```

To pin a fixed release version, use a Git tag:

```ini
lib_deps =
    https://github.com/MyTechWaifu/MTW_TinyLoopScheduler.git#0.2.1
```

The library uses the Arduino framework. The selected board automatically
determines whether the AVR or Generic backend is used. Actual board availability
depends on the installed Arduino core, PlatformIO platform package, and board definition.

## Callback type

All scheduler callbacks use the exact signature:

```cpp
void callback(void);
```

The public callback type is:

```cpp
tinyls::Fn
```

Callbacks:

- accept no arguments;
- return no value;
- execute synchronously in main context;
- must return quickly.

Use global, static, or application-owned state when callbacks need persistent data or communication.

## Runtime model

Execution is cooperative:

- there are no threads;
- there is no preemption;
- only one callback executes at a time;
- callbacks run inside `tinyls::tick()`;
- the Arduino `loop()` remains the application entry point; TLS does not replace it;
- callback duration directly affects scheduler latency;
- `tick()` is not reentrant;
- `tick()` must not be called from a callback, PARTED step, ISR, or nested context.

Long work should be divided into short PARTED steps or another explicit application state machine.

## Scheduler pass order

One `tinyls::tick()` pass performs the following phases:

1. capture one common `millis()` snapshot;
2. update the dynamic IDLE load gate, if `TLS_USE_IDLE` is enabled;
3. drain deferred ISR callbacks, if `TLS_ISR` is enabled;
4. compact or reorder PERIODIC storage when required;
5. execute due PERIODIC callbacks;
6. compact or reorder DELAYED storage when required;
7. execute due DELAYED callbacks;
8. enter AVR `SLEEP_MODE_IDLE` if `TLS_USE_IDLE` is enabled and the IDLE gate permits it;
9. fully drain POSTED callbacks;
10. execute a bounded POLL pass.

Disabled phases are omitted at compile time. On Generic, execution continues
from DELAYED directly to POSTED, then POLL. Without IDLE, POSTED and POLL do
not wait for a wake-up interrupt, so POLL may run more frequently.

All scheduler-dispatched callbacks in one pass see the same cached timestamp in `tinyls::now`.

## Public API

### Initialization

```cpp
void tinyls::init();
```

Initializes cached time and logically empties all enabled queues. When
`TLS_USE_IDLE` is enabled, it also selects the platform sleep mode
(currently AVR `SLEEP_MODE_IDLE`). With IDLE disabled, no sleep setup is performed.

Call it exactly once from `setup()`, after hardware initialization and before registering initial jobs.

### Scheduler pass

```cpp
void tinyls::tick();
```

Executes one complete cooperative scheduler pass.

### Cached time

```cpp
extern uint32_t tinyls::now;
```

`tinyls::now` contains the cached `millis()` snapshot captured near the beginning of the current scheduler pass.

Application code may read it but must not modify it.

A 32-bit read is not atomic on an 8-bit AVR, so `tinyls::now` must not be relied on from an ISR.

## PERIODIC callbacks

Register a repeating callback:

```cpp
tinyls::every(fn, period_ms);
```

Remove it:

```cpp
tinyls::removeEvery(fn);
```

Example:

```cpp
void updateDisplay()
{
    // Short periodic work.
}

void setup()
{
    tinyls::init();
    tinyls::every(updateDisplay, 500);
}
```

Properties:

- the same function pointer cannot appear twice in PERIODIC;
- the same function may still be used in another queue class;
- the first deadline is based on cached `tinyls::now`;
- each PERIODIC record runs at most once per `tick()`;
- missed periods do not produce catch-up bursts;
- a zero stored period makes the callback due on every scheduler pass;
- removal creates a tombstone;
- a tombstone does not free physical capacity until the next PERIODIC maintenance pass.

## DELAYED callbacks

Register a one-shot callback:

```cpp
tinyls::after(fn, delay_ms);
```

Remove it before execution:

```cpp
tinyls::removeAfter(fn);
```

Example:

```cpp
void turnRelayOff()
{
    // One-shot action.
}

void setup()
{
    tinyls::init();
    tinyls::after(turnRelayOff, 2000);
}
```

Properties:

- the same function pointer cannot appear twice in DELAYED;
- the callback is one-shot;
- the deadline is based on cached `tinyls::now`;
- expiration or removal creates a tombstone;
- a tombstone does not free physical capacity until the next DELAYED maintenance pass.

## POSTED callbacks

Queue a one-shot callback for the POSTED phase:

```cpp
tinyls::post(fn);
```

Example:

```cpp
void processCommand()
{
    // Deferred main-context work.
}

void onApplicationEvent()
{
    tinyls::post(processCommand);
}
```

Properties:

- POSTED uses LIFO order;
- duplicate function pointers already waiting in POSTED are rejected;
- a callback is removed before invocation;
- the POSTED phase is fully drained;
- callbacks posted while POSTED is running are also consumed during that phase.

Do not create self-reposting or cyclic repost chains. They can prevent the POSTED phase, and therefore `tick()`, from terminating.

Use POLL for persistent per-pass execution.

## POLL callbacks

Register a persistent callback:

```cpp
tinyls::poll(fn);
```

Remove it:

```cpp
tinyls::removePoll(fn);
```

Example:

```cpp
void checkInput()
{
    // Short non-blocking check.
}

void setup()
{
    tinyls::init();
    tinyls::poll(checkInput);
}
```

Properties:

- duplicate function pointers inside POLL are rejected;
- the callback remains active until removed;
- the POLL phase takes a callback-count snapshot at entry;
- callbacks appended during POLL do not increase the current pass budget;
- an ordinary POLL callback may remove itself;
- POLL storage is compacted immediately after removal.

Do not call `removePoll()` directly from inside a PARTED step.

## Deferred ISR callbacks

Available only when `TLS_ISR == TLS_ENABLE` and the selected backend
supports it (currently AVR). Generic has no `postISR()` declaration or buffer.

Record a deferred request from a normal AVR hardware ISR:

```cpp
tinyls::postISR(fn);
```

The requested callback does not run inside the ISR. It runs later from `tinyls::tick()` in main context.

Complete example for Arduino Uno/Nano with an ATmega328P. Digital pin 2 is used because it supports an external interrupt on these boards:

```cpp
#include <MTW_TinyLoopScheduler.h>

const uint8_t INTERRUPT_PIN = 2;
static uint8_t ledState = LOW;

void processInterruptEvent()
{
    // This callback runs later from tinyls::tick() in main context.
    ledState = (ledState == LOW) ? HIGH : LOW;
    digitalWrite(LED_BUILTIN, ledState);
}

void onExternalInterrupt()
{
    // This function runs in hardware interrupt context.
    tinyls::postISR(processInterruptEvent);
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, ledState);

    pinMode(INTERRUPT_PIN, INPUT_PULLUP);

    tinyls::init();

    attachInterrupt(
        digitalPinToInterrupt(INTERRUPT_PIN),
        onExternalInterrupt,
        FALLING
    );
}

void loop()
{
    tinyls::tick();
}
```

Connect a normally open push button between digital pin 2 and GND. This example demonstrates ISR deferral only; a mechanical button can generate multiple interrupt requests because of contact bounce.

### How the example works

1. `attachInterrupt()` invokes `onExternalInterrupt()` in hardware interrupt context.
2. The ISR calls only `tinyls::postISR(processInterruptEvent)`.
3. `postISR()` stores the callback request and returns without executing it.
4. A later `tinyls::tick()` removes the request from the ISR buffer.
5. `processInterruptEvent()` then runs in normal main context.

### `postISR()` rules and behavior

- call `postISR()` only from a normal, non-nested AVR hardware ISR;
- do not allow another ISR to interrupt a `postISR()` call and access the same TLS request buffer;
- nested interrupts are not supported;
- `ISR_NOBLOCK` handlers are not supported;
- manually re-enabling interrupts inside the ISR is not supported;
- duplicate callback requests are allowed;
- pending ISR requests are processed in LIFO order;
- `postISR()` performs no `millis()` call;
- `postISR()` performs no sorting or duplicate scan;
- `postISR()` does not insert the callback into an ordinary scheduler queue;
- the deferred callback runs later in main context and may use enabled main-context scheduler APIs.

On classic AVR, normal ISR entry clears the global interrupt-enable bit.
AVR Dx / AVRxt instead use CPUINT execution-level state to prevent another
normal-priority handler from entering; `SREG.I` is not automatically cleared.
The required property is non-nested access to the request buffer. Do not use
`postISR()` concurrently from normal and high-priority handlers (LVL1), or from NMI.
This platform distinction does not change the deferred-callback API.
See the [Microchip AVR instruction-set manual, RETI notes](https://ww1.microchip.com/downloads/en/DeviceDoc/AVR-InstructionSet-Manual-DS40002198.pdf)
and [AVR DB interrupt priority](https://onlinedocs.microchip.com/oxy/GUID-8FC17F92-806A-426E-8A7C-60D2F975DE7F-en-US-7/GUID-B51E7F4A-0EFE-4F1F-8EDF-AEEE5C273022.html).

Do not call `postISR()` from normal main-context code. Use `every()`, `after()`, `post()`, or `poll()` instead.


## Compile-time subsystem selection

PERIODIC, DELAYED, POSTED, and POLL are enabled by default on both backends.
`TLS_ISR` defaults to enabled on AVR and disabled on Generic. PARTED remains
optional on both backends. `TLS_USE_IDLE` is independent of task-class selection.

| Switch | AVR default | Generic default |
| --- | --- | --- |
| `TLS_PERIODIC`, `TLS_DELAYED`, `TLS_POSTED`, `TLS_POLL` | `TLS_ENABLE` | `TLS_ENABLE` |
| `TLS_ISR` | `TLS_ENABLE` | `TLS_DISABLE` |
| `TLS_USE_IDLE` | `TLS_ENABLE` | `TLS_DISABLE` |
| `TLS_PARTED` | `TLS_DISABLE` | `TLS_DISABLE` |

`TLS_ISR` controls only TLS deferred requests; disabling it does not disable
hardware interrupts or ordinary `post()`. `TLS_POSTED` controls `post()` independently.

PARTED is disabled by default:

```cpp
TLS_PARTED
```

Available selector values:

```cpp
TLS_ENABLE
TLS_DISABLE
```

### Disable all scheduler classes

Use the following configuration to compile only the common scheduler core,
without task classes or IDLE:

```cpp
#define TLS_PERIODIC TLS_DISABLE
#define TLS_DELAYED  TLS_DISABLE
#define TLS_POSTED   TLS_DISABLE
#define TLS_POLL     TLS_DISABLE
#define TLS_ISR      TLS_DISABLE
#define TLS_PARTED   TLS_DISABLE
#define TLS_USE_IDLE TLS_DISABLE

#include <MTW_TinyLoopScheduler.h>
```

A disabled ordinary subsystem contributes no:

- public API declarations for that subsystem;
- runtime implementation for that subsystem;
- queue array;
- queue counter;
- initialization code for that subsystem;
- corresponding `tick()` phase.

### Enable one scheduler class

Disable the other task classes and enable only the required subsystem.
The examples below leave IDLE at its platform default; define
`TLS_USE_IDLE TLS_DISABLE` separately if sleep is not wanted.

PERIODIC only:

```cpp
#define TLS_PERIODIC TLS_ENABLE
#define TLS_DELAYED  TLS_DISABLE
#define TLS_POSTED   TLS_DISABLE
#define TLS_POLL     TLS_DISABLE
#define TLS_ISR      TLS_DISABLE
#define TLS_PARTED   TLS_DISABLE

#include <MTW_TinyLoopScheduler.h>
```

For another ordinary class, keep the other classes disabled and enable only one of:

```cpp
#define TLS_DELAYED TLS_ENABLE
#define TLS_POSTED  TLS_ENABLE
#define TLS_POLL    TLS_ENABLE
#define TLS_ISR     TLS_ENABLE
```

Enabling `TLS_ISR` is supported only on AVR. Explicitly enabling it on
Generic produces a compile-time error.

PARTED is not an independent queue class. It requires POLL:

```cpp
#define TLS_PERIODIC TLS_DISABLE
#define TLS_DELAYED  TLS_DISABLE
#define TLS_POSTED   TLS_DISABLE
#define TLS_POLL     TLS_ENABLE
#define TLS_ISR      TLS_DISABLE
#define TLS_PARTED   TLS_ENABLE

#include <MTW_TinyLoopScheduler.h>
```

### Approximate footprint by enabled class

The following reference AVR builds used `TLS_USE_IDLE = TLS_ENABLE`
(the AVR default). These figures do not describe Generic targets. They used:

- Arduino Uno / ATmega328P at 16 MHz;
- `TLS_MANUAL`;
- queue limits set to `2`;
- only `tinyls::init()` in `setup()`;
- only `tinyls::tick()` in `loop()`;
- all unlisted scheduler classes disabled.

The figures are approximate complete build totals for the minimal benchmark sketch. They are not isolated object-file sizes and must not be added together.

| Configuration                    | Approx. SRAM | Approx. Flash | Added over core |
| -------------------------------- | -----------: | ------------: | --------------: |
| Empty Arduino sketch             |        ~10 B |        ~445 B |               — |
| Scheduler core, all classes off  |        ~15 B |        ~585 B |        baseline |
| PERIODIC only                    |        ~40 B |       ~1210 B |  ~25 B / ~625 B |
| DELAYED only                     |        ~35 B |       ~1070 B |  ~20 B / ~485 B |
| POSTED only                      |        ~15 B |        ~615 B |   ~0 B /  ~30 B |
| POLL only                        |        ~15 B |        ~635 B |   ~0 B /  ~50 B |
| ISR only                         |        ~20 B |        ~660 B |   ~5 B /  ~75 B |
| POLL + PARTED                    |        ~15 B |        ~685 B |   ~0 B / ~100 B |
| All ordinary classes, PARTED off |        ~65 B |       ~1790 B |  ~50 B / ~1205 B |

In the `Added over core` column, the first value is additional SRAM and the second value is additional Flash relative to the all-disabled scheduler core.

`POLL + PARTED` adds approximately `~50 B` of Flash over the POLL-only benchmark in this minimal configuration.

Actual size depends on:

- AVR core version;
- compiler and toolchain version;
- link-time optimization;
- selected queue profile;
- `TLS_PERIOD`;
- application code;
- which API functions are actually referenced.

AVR LTO may remove enabled storage or code that is provably unused. Use the Arduino compiler build report for the exact size of a real sketch.

## Queue profiles

Select a profile before including the header:

```cpp
#define TLS_PROFILE TLS_MINI
#include <MTW_TinyLoopScheduler.h>
```

Available profiles:

```cpp
TLS_TINY
TLS_MINI
TLS_MID
TLS_DEF
TLS_MAX
TLS_MANUAL
```

Default:

```cpp
TLS_PROFILE TLS_DEF
```

### Recommended queue capacities

The predefined profiles are intended for the following simultaneous active or pending workload:

| Profile    | PERIODIC | DELAYED | POLL | POSTED | ISR requests |
| ---------- | -------: | ------: | ---: | -----: | -----------: |
| `TLS_TINY` |        1 |       1 |    1 |      1 |            1 |
| `TLS_MINI` |        2 |       2 |    2 |      2 |            2 |
| `TLS_MID`  |        6 |       4 |    4 |      4 |            3 |
| `TLS_DEF`  |       12 |       8 |    8 |      8 |            6 |
| `TLS_MAX`  |       24 |      16 |   16 |     16 |           12 |

The table describes recommended workload, not physical array sizes. In this
version, each predefined physical queue has one additional usable slot above
the listed recommendation. That slot is not reserved or protected; it gives
headroom for bursts and PERIODIC/DELAYED tombstones.

Choose the smallest profile that safely covers the maximum expected number of simultaneous active or pending callbacks in every enabled queue.


## Manual profile

`TLS_MANUAL` lets the application define queue limits directly.

Example:

```cpp
#define TLS_PROFILE TLS_MANUAL

#define TLS_MAX_PERIODIC      5
#define TLS_MAX_DELAYED       4
#define TLS_MAX_POLL          4
#define TLS_MAX_POSTED        3
#define TLS_MAX_ISR_REQUESTS  3

#include <MTW_TinyLoopScheduler.h>
```

Manual limits are physical capacities; no extra slot is added. Each limit is
clamped to `2..255`, with a compiler warning when clamping occurs. Undefined
manual limits use the `TLS_DEF` physical capacities: PERIODIC `13`, DELAYED `9`,
POLL `9`, POSTED `9`, and ISR requests `7`.

When using `TLS_MANUAL`, select values that cover the maximum expected simultaneous workload for each enabled queue.

## Period storage width

Select the internal period width before including the header:

```cpp
#define TLS_PERIOD 16
#include <MTW_TinyLoopScheduler.h>
```

Available values:

```cpp
TLS_PERIOD=16
TLS_PERIOD=32
```

Default:

```cpp
TLS_PERIOD=32
```

### Supported ranges

| Mode            | Supported effective interval |
| --------------- | ---------------------------: |
| `TLS_PERIOD=16` |               `0..65,535 ms` |
| `TLS_PERIOD=32` |        `0..2,147,483,647 ms` |

Public `every()` and `after()` parameters remain `uint32_t` in both modes.

The implementation does not clamp or reject values outside the selected storage width.

With `TLS_PERIOD=16`, conversion uses normal `uint16_t` modulo narrowing:

```text
65,536 ms → 0 ms
65,537 ms → 1 ms
```

Application code must enforce the documented range.

The 32-bit maximum is constrained by signed-delta deadline comparison. Larger values are accepted by the API but have unsupported deadline semantics.

For longer waits, use a shorter supported scheduler interval plus an application counter.

## IDLE enable / disable

`TLS_USE_IDLE` controls the complete TLS sleep subsystem. It defaults to
`TLS_ENABLE` on AVR and `TLS_DISABLE` on Generic.

To disable AVR sleep while keeping the ordinary task queues:

```cpp
#define TLS_USE_IDLE TLS_DISABLE
#include <MTW_TinyLoopScheduler.h>
```

`TLS_ISR` can be disabled independently. Generic needs neither definition:
both features are already disabled by default. Explicitly enabling either
unsupported feature on Generic produces a compile-time error.

With IDLE disabled, there is no sleep setup, hardware sleep instruction,
`idle_prev_ms` storage/update, load-gate interval calculation, or IDLE skip-bit
maintenance. The common `tinyls::now` snapshot and ordinary queue maintenance
remain. POSTED and POLL execute without waiting for a wake-up interrupt.

When AVR IDLE is enabled, call `tick()` with interrupts enabled and keep a
working wake-up source, normally the Arduino core's millis timer. TLS does not
configure a separate wake-up timer or replace the core's timekeeping.

## Dynamic IDLE selection

Select the IDLE load threshold. `TLS_IDLE` selects only the threshold; it does not enable or disable IDLE:

```cpp
#define TLS_IDLE TLS_AGGRESSIVE
#include <MTW_TinyLoopScheduler.h>
```

Available modes:

| Selector         | Threshold |
| ---------------- | --------: |
| `TLS_AGGRESSIVE` |      3 ms |
| `TLS_NORMAL`     |      8 ms |
| `TLS_LAZY`       |     16 ms |

Default:

```cpp
TLS_IDLE TLS_NORMAL
```

The following load-gate behavior applies only when `TLS_USE_IDLE` is enabled.
At the beginning of `tick()`, the scheduler measures the interval since the previous `tick()` entry using the low 16 bits of `millis()`.

IDLE is skipped for the current pass when:

- the measured interval is strictly greater than the selected threshold;
- at least one deferred ISR callback executed;
- at least one PERIODIC callback was due;
- at least one DELAYED callback was due.

POSTED and POLL intentionally do not suppress IDLE. When none of the IDLE skip conditions is present, the MCU enters `SLEEP_MODE_IDLE`; POSTED and POLL execute after a normal interrupt wakes it.

The 16-bit load-gate interval wraps every 65.536 seconds. This affects only the IDLE heuristic, not 32-bit scheduler deadlines.

## Sleep mode guard

`TLS_SLEEP_GUARD` controls whether the scheduler reaffirms
`SLEEP_MODE_IDLE` immediately before every actual sleep entry. It has no
effect when `TLS_USE_IDLE` is disabled and does not enable sleep itself.

The default configuration is:

```cpp
#define TLS_SLEEP_GUARD TLS_ENABLE
```

## PARTED execution

PARTED divides a long cooperative operation into numbered execution steps.

Enable it before including the header:

```cpp
#define TLS_PARTED TLS_ENABLE
#include <MTW_TinyLoopScheduler.h>
```

PARTED requires POLL.

### Example: DELAYED activation

The PARTED callback is registered through the DELAYED class. The DELAYED callback does not execute a user step directly. When its deadline is reached, the PARTED function registers itself in POLL, and the individual steps then execute from later POLL phases.

```cpp
#define TLS_PARTED TLS_ENABLE
#include <MTW_TinyLoopScheduler.h>

static void readSmallPart()
{
    // Read one small part of the input.
}

static void processSmallPart()
{
    // Process one small part.
}

static void saveResult()
{
    // Save the completed result.
}

static void loadData()
{
    TLS_PART_START(loadData) {

        TLS_STEP(0) {
            readSmallPart();
            TLS_NEXT();
        }

        TLS_STEP(1) {
            processSmallPart();
            TLS_NEXT(5);
        }

        TLS_STEP(5) {
            saveResult();
            TLS_FINISH();
        }

    } TLS_PART_END();
}

void setup()
{
    tinyls::init();

    /*
      Activate the PARTED callback through the DELAYED class after
      1000 milliseconds.
    */
    tinyls::after(loadData, 1000u);
}

void loop()
{
    tinyls::tick();
}
```

Available macros:

```cpp
TLS_PART_START(fn)
TLS_STEP(N)
TLS_NEXT()
TLS_NEXT(N)
TLS_RESTART()
TLS_FINISH()
TLS_PART_END()
```

### Step states

```text
0..247   user-defined steps
248..254 reserved
255      internal start/uninitialized state
```

### Recommended activation paths

Register or trigger a PARTED callback through an enabled scheduler class.
The `postISR()` activation path requires AVR support and `TLS_ISR` enabled:

```cpp
tinyls::poll(partedFn);
tinyls::every(partedFn, period_ms);
tinyls::after(partedFn, delay_ms);
tinyls::post(partedFn);
tinyls::postISR(partedFn);
```

Behavior by source:

- `poll(partedFn)` registers the function directly in POLL;
- PERIODIC, DELAYED, POSTED, and deferred ISR callbacks invoke the PARTED function in main context;
- `TLS_PART_START()` then registers that function in POLL and returns;
- user `TLS_STEP` blocks execute only from the POLL phase.

Repeated PERIODIC, DELAYED, POSTED, or deferred-ISR triggers while the same PARTED function is already active do not restart its current step sequence.

PARTED activation requires an available POLL slot. If POLL is full, the activation attempt is lost.

### Step execution

Each POLL invocation executes at most one matching `TLS_STEP`.

Transition macros immediately return from the callback:

- `TLS_NEXT()` selects the next numeric step;
- `TLS_NEXT(N)` selects step `N`;
- `TLS_RESTART()` selects step `0` while keeping the callback active;
- `TLS_FINISH()` resets PARTED state and removes the callback from POLL.

If a matched step reaches its end without a transition macro, the same step repeats on the next POLL pass.

If the stored step has no matching `TLS_STEP`, `TLS_PART_END()` resets the state, removes the callback from POLL, and returns.

Automatic local variables do not persist between PARTED steps. Use static, global, or application-owned storage.

Do not:

- call a PARTED function directly from another POLL callback;
- call `removePoll()` directly from inside a PARTED step;
- assume activation succeeds when POLL is full.

## Approximate memory usage

Reference measurements in the source were produced for Arduino Uno / ATmega328P at 16 MHz with a minimal sketch containing only `tinyls::init()` and `tinyls::tick()`.

Actual results depend on the AVR core, compiler, toolchain, LTO, enabled subsystems, sketch structure, and library revision.

### All ordinary subsystems enabled, PARTED disabled

| Profile    | `TLS_PERIOD=32` SRAM | Flash   | `TLS_PERIOD=16` SRAM | Flash   |
| ---------- | -------------------: | ------: | -------------------: | ------: |
| `TLS_TINY` |                ~65 B | ~1790 B |                ~60 B | ~1800 B |
| `TLS_MINI` |                ~85 B | ~1790 B |                ~75 B | ~1800 B |
| `TLS_MID`  |               ~145 B | ~1790 B |               ~130 B | ~1800 B |
| `TLS_DEF`  |               ~245 B | ~1790 B |               ~215 B | ~1800 B |
| `TLS_MAX`  |               ~445 B | ~1790 B |               ~395 B | ~1800 B |

### All ordinary subsystems enabled, PARTED enabled

| Profile    | `TLS_PERIOD=32` SRAM | Flash   | `TLS_PERIOD=16` SRAM | Flash   |
| ---------- | -------------------: | ------: | -------------------: | ------: |
| `TLS_TINY` |                ~65 B | ~1840 B |                ~60 B | ~1870 B |
| `TLS_MINI` |                ~85 B | ~1840 B |                ~80 B | ~1870 B |
| `TLS_MID`  |               ~145 B | ~1840 B |               ~130 B | ~1870 B |
| `TLS_DEF`  |               ~245 B | ~1840 B |               ~220 B | ~1870 B |
| `TLS_MAX`  |               ~445 B | ~1840 B |               ~395 B | ~1870 B |

Use the Arduino compiler build report for the exact result of a real sketch.

## Approximate CPU cost

Reference measurements were produced on Arduino Uno / ATmega328P at 16 MHz using Timer1 with prescaler 1.

Approximate scheduler costs:

| Operation                                       | Approximate cost   |
| ----------------------------------------------- | -----------------: |
| Core-only empty `tick()`                        |         ~40 cycles |
| All ordinary subsystems enabled, empty `tick()` |        ~105 cycles |
| Minimal direct callback                         |         ~20 cycles |
| Active POLL callback                            |     ~50 cycles/job |
| Queued POSTED callback                          |     ~30 cycles/job |
| Deferred ISR request                            | ~45 cycles/request |
| Active PARTED step                              | ~70–75 cycles/step |

A reference mixed pass executing one PERIODIC, DELAYED, POSTED, POLL, deferred ISR, and PARTED callback required approximately:

```text
535 cycles ≈ 33 µs at 16 MHz
```

These figures describe scheduler overhead with minimal callbacks. Application callback execution time must be added separately.

## Source structure

The public include remains `<MTW_TinyLoopScheduler.h>`. The internal split keeps
the existing procedural implementation and phase order; `.inc` fragments are
included inside their original namespaces or functions, not compiled separately.

| Path under `src/` | Purpose |
| --- | --- |
| `MTW_TinyLoopScheduler.h` | Public entry point and implementation assembly |
| `mtw_ls/MTW_LS_Features.h` | Version, feature switches, and validation |
| `mtw_ls/MTW_LS_Config.h` | Queue profiles, capacities, and period width |
| `mtw_ls/MTW_LS_Public.h` | Public declarations |
| `mtw_ls/MTW_LS_Parted.h` | Optional PARTED macros |
| `mtw_ls/MTW_LS_Storage.inc` | Static scheduler state |
| `mtw_ls/MTW_LS_Init.inc`, `mtw_ls/MTW_LS_API.inc` | Initialization and queue API implementations |
| `mtw_ls/MTW_LS_Tick*.inc` | Timestamp, ISR, PERIODIC, DELAYED, IDLE, POSTED, and POLL phases |
| `mtw_ls/MTW_LS_Platform.h`, `mtw_ls/MTW_LS_Critical.inc` | Backend selection and critical-section inclusion |
| `mtw_ls/platforms/MTW_LS_Platform_AVR*` | AVR capabilities, critical sections, sleep setup, and sleep entry |
| `mtw_ls/platforms/MTW_LS_Platform_Generic.h` | Portable capabilities with IDLE/ISR disabled |

## Multi-file sketches

Exactly one translation unit must own the implementation and scheduler state.

A normal Arduino sketch composed only of `.ino` tabs is normally combined into one generated C++ translation unit and requires no special handling.

The implementation-owning translation unit includes the library normally:

```cpp
#include <MTW_TinyLoopScheduler.h>
```

Every additional real `.cpp` translation unit must use:

```cpp
#define TLS_DECL_ONLY
#include <MTW_TinyLoopScheduler.h>
```

All translation units must use matching values for:

```cpp
TLS_PERIODIC
TLS_DELAYED
TLS_POSTED
TLS_POLL
TLS_ISR
TLS_PARTED
TLS_USE_IDLE
TLS_SLEEP_GUARD
```

Use a shared configuration header or build flags for consistent settings.
A `#define` in one `.cpp` does not configure other translation units.
Keep the same `TLS_PERIOD` where implementation/configuration types are used.
`TLS_DECL_ONLY` is the deliberate exception: define it only in declaration-only files.

Every translation unit that defines a PARTED callback must enable PARTED, and the implementation-owning translation unit must also enable it.

## Important restrictions

- Call `tinyls::init()` exactly once from `setup()`.
- Register initial jobs after `tinyls::init()`.
- Call `tinyls::tick()` frequently and only from `loop()`.
- Do not call `tick()` recursively.
- Do not call `tick()` from an ISR.
- Do not call `tick()` from a scheduler callback.
- Do not call `tick()` from a PARTED step.
- Keep callbacks short and non-blocking.
- Avoid long `delay()` calls inside callbacks.
- Generic: access TLS from one main execution context; no ISR, concurrent thread, or second-core calls.
- Use only `postISR()` from supported AVR ISR context.
- Do not call normal queue APIs directly from a hardware ISR.
- Do not create endless POSTED repost cycles.
- Do not call `removePoll()` directly from a PARTED step.
- Reserve enough POLL capacity for all simultaneously active PARTED callbacks.
- Do not write to `tinyls::now`.

## Documentation

- [API](docs/API.md)
- [Configuration](docs/CONFIGURATION.md)
- [Platforms](docs/PLATFORMS.md)
- [Changelog](CHANGELOG.md)

## Examples

The library package contains 18 Arduino examples.

After installation, open them through:

```text
File → Examples → MTW TinyLoopScheduler
```

Examples that use `postISR()` require the AVR backend.

## License

Copyright 2026 MyTechWaifu.

Licensed under the Apache License, Version 2.0.

See:

- `LICENSE`
- `NOTICE`

SPDX identifier:

```text
Apache-2.0
```
