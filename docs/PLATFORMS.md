# MTW TinyLoopScheduler 0.2.0 platforms: AVR and Generic

The public ordinary-task API and the existing examples are preserved.

| Build target | Backend | `TLS_USE_IDLE` default | `TLS_ISR` default |
| --- | --- | --- | --- |
| AVR, including megaAVR and AVR Dx | AVR | `TLS_ENABLE` | `TLS_ENABLE` |
| Other Arduino-compatible targets | Generic | `TLS_DISABLE` | `TLS_DISABLE` |

The backend is selected automatically: targets defining `__AVR__` use AVR;
all other targets use Generic. Generic provides no TLS hardware sleep,
critical-section implementation, or `postISR()`. Arduino Due (SAM3X8E),
Uno R4 Minima (RA4M1), RP2040, RP2350, and STM32 currently share Generic.
It requires `Arduino.h` and a working `millis()` source. `TLS_PROFILE`
selects queue capacities separately and is not chosen from the platform.

## Configuration

Define settings before the first inclusion of `MTW_TinyLoopScheduler.h`.

```cpp
#define TLS_USE_IDLE TLS_DISABLE
#define TLS_ISR TLS_DISABLE
#include <MTW_TinyLoopScheduler.h>
```

On AVR, both definitions are optional: the defaults preserve IDLE sleep and
deferred ISR requests. Either feature can be disabled independently.

On Generic, both features are disabled by default. Explicitly enabling either
unsupported feature produces a compile-time error. No hardware stubs are used.

`TLS_IDLE` retains its original purpose: selecting the IDLE load threshold
(`TLS_AGGRESSIVE`, `TLS_NORMAL`, or `TLS_LAZY`). `TLS_SLEEP_GUARD` retains its
policy of reaffirming the sleep mode. Neither setting enables sleep when
`TLS_USE_IDLE` is disabled. Hardware IDLE descriptions in the original source
comments apply only when `TLS_USE_IDLE` is enabled. The original default of
enabled ISR support applies to AVR.

The original explanation of interrupt blocking through `SREG.I` applies to
classic AVR. AVR Dx / AVRxt use CPUINT execution-level state instead.
`postISR()` requires non-nested access to its request buffer; concurrent access
from normal-priority handlers and LVL1/NMI is not supported.

## Compile-time removal

With `TLS_USE_IDLE == TLS_DISABLE`, the scheduler does not allocate or update
`idle_prev_ms`, calculate the IDLE load interval, maintain IDLE skip bits,
configure sleep, or enter hardware sleep. `tinyls::now` still updates as before,
and ordinary queue maintenance and execution are preserved. POSTED and POLL
continue without waiting for an IDLE wake-up, so POLL may run more frequently.

With `TLS_ISR == TLS_DISABLE`, the `postISR()` API, request buffer, counter,
and execution phase are absent. Ordinary `post()` is controlled separately by
`TLS_POSTED`. `TLS_ISR` does not disable the microcontroller's hardware
interrupts. On AVR, initialization retains its critical section even when
`TLS_ISR` is disabled.

## Execution context

Generic requires a single main execution context for `init()`, `tick()`, and
the other TLS APIs. Calls from hardware ISRs, concurrent threads, or another
core are not supported. `tick()` remains non-reentrant. These restrictions
apply to calls into TLS; they do not disable the Arduino core's hardware
interrupts or internal mechanisms.

## Multi-file projects

Exactly one translation unit owns the implementation and scheduler state by
including the public header without `TLS_DECL_ONLY`. Arduino normally combines
the sketch's `.ino` tabs into one translation unit. Every additional real
`.cpp` translation unit that includes TLS must define `TLS_DECL_ONLY` first.
Enable PARTED in the implementation owner and in each `.cpp` that defines a
PARTED callback. Use a shared configuration header or build flags to keep
settings consistent; local definitions do not propagate between translation
units.

The existing 18 Arduino examples and 18 PlatformIO projects are unchanged.
Examples using `postISR()` require a backend with ISR support; Generic does
not currently provide that API.

Install the library through Arduino Library Manager or PlatformIO.
The public include is `#include <MTW_TinyLoopScheduler.h>`.
See the README for installation instructions and the first example.

## Backend extension

Platform headers declare capabilities and paths to hardware fragments.
Those paths are relative to the shared `mtw_ls/` directory from which the
fragments are included. Generic defines no paths for unsupported hardware
features. An enhanced AVR Dx backend, Standby sleep, and automatic sleep-depth
selection are not implemented in this version.
