# MTW TinyLoopScheduler
[![DOI](https://zenodo.org/badge/1275970080.svg)](https://doi.org/10.5281/zenodo.20784799)

A compact cooperative scheduler for AVR targets using an Arduino-compatible core.

MTW TinyLoopScheduler replaces scattered `millis()` checks with named callbacks dispatched from one explicit place:

```cpp
tinyls::tick();
```

The library is header-only, uses static compile-time storage, does not allocate memory from the heap, and allows unused scheduler subsystems to be removed at compile time.

## Status

Current version:

```text
0.1.4
```

Current target support:

```text
AVR only
```

Non-AVR targets are rejected at compile time.

## Hardware support

MTW TinyLoopScheduler is intended to work across the 8-bit AVR ecosystem. The table below lists the AVR families covered by hardware testing and the representative microcontrollers used for verification.

| AVR series / family              | Hardware-tested MCU    | Example boards and devices                   |
| -------------------------------- | ---------------------- | -------------------------------------------- |
| tinyAVR / ATtiny family          | **ATtiny85, ATtiny88** | Digispark, Trinket, GEMMA, MH-Tiny ATtiny88  |
| classic megaAVR / ATmega family  | **ATmega328P**         | Uno, Nano, Pro Mini                          |
| classic megaAVR / ATmega family  | **ATmega32U4**         | Leonardo, Micro, Pro Micro                   |
| classic megaAVR / ATmega family  | **ATmega2560**         | Mega 2560, Mega ADK, Mega 1280               |
| megaAVR 0-series / ATmega family | **ATmega4808**         | ATmega4808 boards, Nano Every, Uno WiFi Rev2 |

The microcontrollers listed in the Hardware-tested MCU column are confirmed working on physical hardware.

Other microcontrollers and boards from the same AVR families are expected to work, provided that they are supported by a compatible Arduino core and have sufficient Flash and SRAM.

PlatformIO package metadata:

| Field | Value |
|---|---|
| PlatformIO platforms | `atmelavr`, `atmelmegaavr` |
| Arduino architectures | `avr`, `megaavr` |

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
- repeating PERIODIC callbacks;
- one-shot DELAYED callbacks;
- one-shot POSTED callbacks;
- persistent POLL callbacks;
- deferred callback requests from AVR hardware ISRs;
- optional step-based PARTED execution;
- predefined and manual queue-capacity profiles;
- compile-time removal of unused subsystems;
- selectable 16-bit or 32-bit period storage;
- cached `millis()` value exposed as `tinyls::now`;
- automatic AVR `SLEEP_MODE_IDLE` entry when the IDLE gate permits it;
- optional sleep-mode guard that reaffirms `SLEEP_MODE_IDLE` before sleeping;
- rollover-safe deadline comparison within the documented interval limits.

MTW TinyLoopScheduler is not:

- a preemptive RTOS;
- a thread scheduler;
- a dynamic task manager;
- a hard real-time kernel.

## Installation

MTW TinyLoopScheduler can be installed through Arduino IDE or PlatformIO.

### Arduino IDE

#### Arduino Library Manager

After publication in Arduino Library Manager:

1. Open Arduino IDE.
2. Open **Sketch → Include Library → Manage Libraries...**
3. Search for **MTW TinyLoopScheduler**.
4. Select the library.
5. Click **Install**.

#### Manual ZIP installation

1. Download the repository as a ZIP archive.
2. Open Arduino IDE.
3. Open **Sketch → Include Library → Add .ZIP Library...**
4. Select the downloaded archive.

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
    https://github.com/MyTechWaifu/MTW_TinyLoopScheduler.git#v0.1.3
```

The library declares PlatformIO compatibility with:

| PlatformIO platform | Arduino architecture | Target class |
|---|---|---|
| `atmelavr` | `avr` | classic 8-bit AVR, ATtiny, ATmega |
| `atmelmegaavr` | `megaavr` | megaAVR 0-series, ATmega4808, ATmega4809 |

Actual board availability depends on the installed Arduino core, PlatformIO platform package, and board definition.



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
- callback duration directly affects scheduler latency;
- `tick()` is not reentrant;
- `tick()` must not be called from a callback, PARTED step, ISR, or nested context.

Long work should be divided into short PARTED steps or another explicit application state machine.

## Scheduler pass order

One `tinyls::tick()` pass performs the following phases:

1. capture one common `millis()` snapshot;
2. update the dynamic IDLE load gate;
3. drain deferred ISR callbacks;
4. compact or reorder PERIODIC storage when required;
5. execute due PERIODIC callbacks;
6. compact or reorder DELAYED storage when required;
7. execute due DELAYED callbacks;
8. enter AVR `SLEEP_MODE_IDLE` when the IDLE gate permits it;
9. fully drain POSTED callbacks;
10. execute a bounded POLL pass.

All scheduler-dispatched callbacks in one pass see the same cached timestamp in `tinyls::now`.

## Public API

### Initialization

```cpp
void tinyls::init();
```

Initializes cached time, selects AVR `SLEEP_MODE_IDLE`, and logically empties
all enabled queues.

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

- call `postISR()` only from a normal AVR hardware ISR;
- global interrupts must remain disabled for the duration of that ISR;
- nested interrupts are not supported;
- `ISR_NOBLOCK` handlers are not supported;
- manually re-enabling interrupts inside the ISR is not supported;
- duplicate callback requests are allowed;
- pending ISR requests are processed in LIFO order;
- `postISR()` performs no `millis()` call;
- `postISR()` performs no sorting or duplicate scan;
- `postISR()` does not insert the callback into an ordinary scheduler queue;
- the deferred callback runs later in main context and may use enabled main-context scheduler APIs.

Do not call `postISR()` from normal main-context code. Use `every()`, `after()`, `post()`, or `poll()` instead.


## Compile-time subsystem selection

The following ordinary scheduler subsystems are enabled by default:

```cpp
TLS_PERIODIC
TLS_DELAYED
TLS_POSTED
TLS_POLL
TLS_ISR
```

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

Use the following configuration to compile only the scheduler core:

```cpp
#define TLS_PERIODIC TLS_DISABLE
#define TLS_DELAYED  TLS_DISABLE
#define TLS_POSTED   TLS_DISABLE
#define TLS_POLL     TLS_DISABLE
#define TLS_ISR      TLS_DISABLE
#define TLS_PARTED   TLS_DISABLE

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

Start from the all-disabled configuration and change only the required subsystem to `TLS_ENABLE`.

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

The following reference builds used:

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

Undefined manual limits default to the `TLS_DEF` configuration.

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
`SLEEP_MODE_IDLE` immediately before every actual sleep entry.

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

Register or trigger a PARTED callback through one of the scheduler classes:

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
```

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
- Use only `postISR()` from supported ISR context.
- Do not call normal queue APIs directly from a hardware ISR.
- Do not create endless POSTED repost cycles.
- Do not call `removePoll()` directly from a PARTED step.
- Reserve enough POLL capacity for all simultaneously active PARTED callbacks.
- Do not write to `tinyls::now`.

## Examples

The library package contains 18 Arduino examples.

After installation, open them through:

```text
File → Examples → MTW TinyLoopScheduler
```

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
