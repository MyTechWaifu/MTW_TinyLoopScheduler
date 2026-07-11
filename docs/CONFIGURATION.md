# MTW TinyLoopScheduler Configuration

This document describes the compile-time configuration of MTW TinyLoopScheduler version `0.1.3`.

All configuration macros must be defined before the first inclusion of:

```cpp
#include <MTW_TinyLoopScheduler.h>
```

Example:

```cpp
#define TLS_PROFILE TLS_MINI
#define TLS_PERIOD  16

#include <MTW_TinyLoopScheduler.h>
```

## Default configuration

If no configuration macros are defined, the library uses:

```cpp
#define TLS_PERIODIC TLS_ENABLE
#define TLS_DELAYED  TLS_ENABLE
#define TLS_POSTED   TLS_ENABLE
#define TLS_POLL     TLS_ENABLE
#define TLS_ISR      TLS_ENABLE
#define TLS_PARTED   TLS_DISABLE

#define TLS_PROFILE  TLS_DEF
#define TLS_PERIOD   32
#define TLS_IDLE     TLS_NORMAL
#define TLS_SLEEP_GUARD TLS_ENABLE
```

## Compile-time task-class control

The scheduler task classes are controlled independently:

```cpp
TLS_PERIODIC
TLS_DELAYED
TLS_POSTED
TLS_POLL
TLS_ISR
TLS_PARTED
```

Available values:

```cpp
TLS_ENABLE
TLS_DISABLE
```

### Default state

Enabled by default:

```cpp
TLS_PERIODIC
TLS_DELAYED
TLS_POSTED
TLS_POLL
TLS_ISR
```

Disabled by default:

```cpp
TLS_PARTED
```

### Full kernel trimming

Disabling a task class removes it at compile time.

For an ordinary disabled class, the compiled scheduler contains no:

- public API declarations for that class;
- implementation code for that class;
- static queue storage;
- queue counter;
- initialization logic;
- corresponding `tick()` phase.

This is compile-time kernel trimming, not runtime deactivation.

### Disable all task classes

```cpp
#define TLS_PERIODIC TLS_DISABLE
#define TLS_DELAYED  TLS_DISABLE
#define TLS_POSTED   TLS_DISABLE
#define TLS_POLL     TLS_DISABLE
#define TLS_ISR      TLS_DISABLE
#define TLS_PARTED   TLS_DISABLE

#include <MTW_TinyLoopScheduler.h>
```

This configuration retains only the common scheduler core.

### PERIODIC only

```cpp
#define TLS_PERIODIC TLS_ENABLE
#define TLS_DELAYED  TLS_DISABLE
#define TLS_POSTED   TLS_DISABLE
#define TLS_POLL     TLS_DISABLE
#define TLS_ISR      TLS_DISABLE
#define TLS_PARTED   TLS_DISABLE

#include <MTW_TinyLoopScheduler.h>
```

### DELAYED only

```cpp
#define TLS_PERIODIC TLS_DISABLE
#define TLS_DELAYED  TLS_ENABLE
#define TLS_POSTED   TLS_DISABLE
#define TLS_POLL     TLS_DISABLE
#define TLS_ISR      TLS_DISABLE
#define TLS_PARTED   TLS_DISABLE

#include <MTW_TinyLoopScheduler.h>
```

### POSTED only

```cpp
#define TLS_PERIODIC TLS_DISABLE
#define TLS_DELAYED  TLS_DISABLE
#define TLS_POSTED   TLS_ENABLE
#define TLS_POLL     TLS_DISABLE
#define TLS_ISR      TLS_DISABLE
#define TLS_PARTED   TLS_DISABLE

#include <MTW_TinyLoopScheduler.h>
```

### POLL only

```cpp
#define TLS_PERIODIC TLS_DISABLE
#define TLS_DELAYED  TLS_DISABLE
#define TLS_POSTED   TLS_DISABLE
#define TLS_POLL     TLS_ENABLE
#define TLS_ISR      TLS_DISABLE
#define TLS_PARTED   TLS_DISABLE

#include <MTW_TinyLoopScheduler.h>
```

### ISR only

```cpp
#define TLS_PERIODIC TLS_DISABLE
#define TLS_DELAYED  TLS_DISABLE
#define TLS_POSTED   TLS_DISABLE
#define TLS_POLL     TLS_DISABLE
#define TLS_ISR      TLS_ENABLE
#define TLS_PARTED   TLS_DISABLE

#include <MTW_TinyLoopScheduler.h>
```

### POLL with PARTED

PARTED requires POLL:

```cpp
#define TLS_PERIODIC TLS_DISABLE
#define TLS_DELAYED  TLS_DISABLE
#define TLS_POSTED   TLS_DISABLE
#define TLS_POLL     TLS_ENABLE
#define TLS_ISR      TLS_DISABLE
#define TLS_PARTED   TLS_ENABLE

#include <MTW_TinyLoopScheduler.h>
```

The following configuration is invalid:

```cpp
#define TLS_POLL   TLS_DISABLE
#define TLS_PARTED TLS_ENABLE
```

It produces a compile-time error.

## Queue profiles

Select a predefined profile with:

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

### Recommended workload

The predefined profiles are documented by their recommended simultaneous active or pending workload:

| Profile    | PERIODIC | DELAYED | POLL | POSTED | ISR requests |
| ---------- | -------: | ------: | ---: | -----: | -----------: |
| `TLS_TINY` |        1 |       1 |    1 |      1 |            1 |
| `TLS_MINI` |        2 |       2 |    2 |      2 |            2 |
| `TLS_MID`  |        6 |       4 |    4 |      4 |            3 |
| `TLS_DEF`  |       12 |       8 |    8 |      8 |            6 |
| `TLS_MAX`  |       24 |      16 |   16 |     16 |           12 |

Choose the smallest profile that safely covers the maximum expected workload in every enabled task class.

The implementation may keep internal headroom beyond the documented recommended workload. That internal detail is not part of the public profile contract.

## Manual queue configuration

Use `TLS_MANUAL` when the predefined profiles do not match the application.

```cpp
#define TLS_PROFILE TLS_MANUAL

#define TLS_MAX_PERIODIC      5
#define TLS_MAX_DELAYED       4
#define TLS_MAX_POLL          4
#define TLS_MAX_POSTED        3
#define TLS_MAX_ISR_REQUESTS  3

#include <MTW_TinyLoopScheduler.h>
```

Available manual limits:

```cpp
TLS_MAX_PERIODIC
TLS_MAX_DELAYED
TLS_MAX_POLL
TLS_MAX_POSTED
TLS_MAX_ISR_REQUESTS
```

Each value defines the corresponding queue limit directly.

Undefined manual limits fall back to the default `TLS_DEF` configuration values.

### Supported limit range

Every queue limit is validated independently and clamped to:

```text
2..255
```

Values below `2` are clamped to `2`.

Values above `255` are clamped to `255`.

The compiler emits a warning when clamping occurs.

Limits belonging to disabled task classes are still preprocessed and validated, but disabled classes allocate no queue storage.

## Period storage width

Select the stored period width with:

```cpp
#define TLS_PERIOD 16
#include <MTW_TinyLoopScheduler.h>
```

Available values:

```cpp
TLS_PERIOD 16
TLS_PERIOD 32
```

Default:

```cpp
TLS_PERIOD 32
```

### `TLS_PERIOD=16`

Use this mode only when every PERIODIC interval and DELAYED timeout is within:

```text
0..65,535 ms
```

The public `every()` and `after()` parameters remain `uint32_t`, but the implementation narrows the value through `uint16_t`.

Normal modulo narrowing applies:

```text
65,536 ms -> 0 ms
65,537 ms -> 1 ms
```

No runtime range validation or saturation is performed.

### `TLS_PERIOD=32`

Supported interval range:

```text
0..2,147,483,647 ms
```

This is approximately:

```text
24.855 days
```

Larger values are accepted by the API but are outside the supported deadline semantics because deadline checks use signed 32-bit deltas.

For longer waits, use a shorter supported scheduler interval plus an application counter.

### Invalid `TLS_PERIOD`

Any value other than `16` or `32` is replaced with `32`, and the compiler emits a warning.

## Automatic dynamic IDLE

Dynamic IDLE is automatic and is not enabled or disabled with a feature switch.

The selectable setting controls only the load threshold:

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

The current pass does not enter IDLE when:

- the tick-entry interval is strictly greater than the selected threshold;
- at least one deferred ISR callback executed;
- at least one PERIODIC callback was due;
- at least one DELAYED callback was due.

POSTED and POLL intentionally do not suppress IDLE.

When the current pass is eligible, the scheduler enters AVR `SLEEP_MODE_IDLE`. After a normal interrupt wakes the MCU, execution continues with POSTED and POLL.

The 16-bit IDLE interval wraps modulo:

```text
65,536 ms
```

This affects only the dynamic IDLE heuristic. Scheduler deadlines remain 32-bit.

### Invalid `TLS_IDLE`

An invalid selector is replaced with:

```cpp
TLS_NORMAL
```

The compiler emits a warning.

## Sleep mode guard

`TLS_SLEEP_GUARD` controls whether TinyLoopScheduler reaffirms
`SLEEP_MODE_IDLE` immediately before every actual sleep entry.

Default:

```cpp
#define TLS_SLEEP_GUARD TLS_ENABLE
```

## PARTED configuration

Enable PARTED with:

```cpp
#define TLS_PARTED TLS_ENABLE
#include <MTW_TinyLoopScheduler.h>
```

PARTED:

- requires POLL;
- has no independent queue;
- uses POLL slots;
- adds one step-state byte for each configured POLL slot;
- adds shared active-slot state;
- is completely absent from runtime storage and execution logic when disabled.

Supported user step range:

```text
0..247
```

Reserved states:

```text
248..254
```

Internal start state:

```text
255
```

## Invalid task-class selector values

Each task-class selector must be either:

```cpp
TLS_ENABLE
TLS_DISABLE
```

An invalid value is replaced with the class default:

- PERIODIC -> `TLS_ENABLE`;
- DELAYED -> `TLS_ENABLE`;
- POSTED -> `TLS_ENABLE`;
- POLL -> `TLS_ENABLE`;
- ISR -> `TLS_ENABLE`;
- PARTED -> `TLS_DISABLE`.

The compiler emits a warning.

## Invalid profile value

If `TLS_PROFILE` is not one of the supported profile constants, it is replaced with:

```cpp
TLS_DEF
```

The compiler emits a warning.

## Multi-file configuration

Exactly one translation unit must own the implementation and scheduler state.

### Implementation owner

The implementation-owning translation unit includes the header normally:

```cpp
#define TLS_PROFILE     TLS_MINI
#define TLS_PERIODIC    TLS_ENABLE
#define TLS_DELAYED     TLS_DISABLE
#define TLS_SLEEP_GUARD TLS_ENABLE

#include <MTW_TinyLoopScheduler.h>
```

### Additional `.cpp` files

Every additional real `.cpp` translation unit must define `TLS_DECL_ONLY`:

```cpp
#define TLS_PROFILE     TLS_MINI
#define TLS_PERIODIC    TLS_ENABLE
#define TLS_DELAYED     TLS_DISABLE
#define TLS_SLEEP_GUARD TLS_ENABLE
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
TLS_SLEEP_GUARD
```

Every translation unit that defines a PARTED callback must enable PARTED.

The implementation owner must also enable PARTED so the shared PARTED state is allocated.

## Configuration order

Use this order at the beginning of the implementation-owning sketch:

```cpp
/*
  1. Select task classes.
*/
#define TLS_PERIODIC TLS_ENABLE
#define TLS_DELAYED  TLS_ENABLE
#define TLS_POSTED   TLS_DISABLE
#define TLS_POLL     TLS_ENABLE
#define TLS_ISR      TLS_ENABLE
#define TLS_PARTED   TLS_ENABLE

/*
  2. Select profile or manual limits.
*/
#define TLS_PROFILE TLS_MINI

/*
  3. Select period width.
*/
#define TLS_PERIOD 16

/*
  4. Select the automatic dynamic IDLE threshold.
*/
#define TLS_IDLE TLS_NORMAL

/*
  5. Select the sleep-mode guard policy.
*/
#define TLS_SLEEP_GUARD TLS_ENABLE

/*
  6. Include the library.
*/
#include <MTW_TinyLoopScheduler.h>
```

Do not define configuration macros after including the library header.
