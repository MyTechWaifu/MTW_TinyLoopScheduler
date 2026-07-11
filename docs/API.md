# MTW TinyLoopScheduler API

This document describes the public API of MTW TinyLoopScheduler version `0.1.3`.

The library is a header-only cooperative scheduler for AVR targets using an Arduino-compatible core.

Include it with:

```cpp
#include <MTW_TinyLoopScheduler.h>
```

All public scheduler functions are defined in:

```cpp
namespace tinyls
```

## Callback type

All scheduler callbacks must use this exact signature:

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
- must return quickly;
- must not call `tinyls::tick()`.

Use global, static, or application-owned state when callbacks need persistent data.

## `tinyls::init()`

```cpp
tinyls::init();
```

Initializes the scheduler state.

Call it exactly once from `setup()` after hardware initialization and before registering initial jobs.

Example:

```cpp
void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    tinyls::init();

    tinyls::every(blinkLed, 1000u);
}
```

Do not call `init()` from:

- `loop()`;
- `tinyls::tick()`;
- a scheduler callback;
- a PARTED step;
- an ISR.

Calling `init()` resets the logical contents of all enabled scheduler queues.

## `tinyls::tick()`

```cpp
tinyls::tick();
```

Executes one complete cooperative scheduler pass.

Call it continuously and only from `loop()`:

```cpp
void loop()
{
    tinyls::tick();
}
```

`tick()` is not reentrant.

Do not call it from:

- another `tick()` call;
- a PERIODIC callback;
- a DELAYED callback;
- a POSTED callback;
- a POLL callback;
- a PARTED step;
- a hardware ISR.

The scheduler pass order is:

1. update `tinyls::now`;
2. update the dynamic IDLE gate;
3. process deferred ISR callbacks;
4. process PERIODIC callbacks;
5. process DELAYED callbacks;
6. enter dynamic IDLE when the pass is eligible;
7. process POSTED callbacks;
8. process POLL callbacks.

## `tinyls::now`

```cpp
tinyls::now
```

Contains the cached `millis()` snapshot captured near the beginning of the current scheduler pass.

Example:

```cpp
static void printSchedulerTime()
{
    Serial.println(tinyls::now);
}
```

Application code may read `tinyls::now` but must not modify it.

All callbacks dispatched during one scheduler pass observe the same cached value unless application code incorrectly writes to it.

A 32-bit read is not atomic on an 8-bit AVR. Do not rely on `tinyls::now` from hardware ISR context.

# Task classes

MTW TinyLoopScheduler provides five ordinary task classes:

- PERIODIC;
- DELAYED;
- POSTED;
- POLL;
- deferred ISR requests.

PARTED is an optional step-execution layer over POLL.

## PERIODIC

### Register a repeating callback

```cpp
tinyls::every(fn, period_ms);
```

Example:

```cpp
static void updateDisplay()
{
    // Short periodic work.
}

void setup()
{
    tinyls::init();
    tinyls::every(updateDisplay, 500u);
}
```

Behavior:

- the callback repeats with the selected period;
- the initial deadline is based on cached `tinyls::now`;
- the same function is not registered twice in PERIODIC;
- the same function may still be used in another task class;
- each PERIODIC callback executes at most once per `tick()`;
- missed periods do not generate catch-up bursts;
- a zero stored period makes the callback due on every scheduler pass.

PERIODIC callbacks should remain short and non-blocking.

### Remove a PERIODIC callback

```cpp
tinyls::removeEvery(fn);
```

Example:

```cpp
tinyls::removeEvery(updateDisplay);
```

Removal marks the record for later compaction.

The occupied queue slot is not immediately reusable until the next PERIODIC maintenance pass.

## DELAYED

### Register a one-shot delayed callback

```cpp
tinyls::after(fn, delay_ms);
```

Example:

```cpp
static void disableRelay()
{
    digitalWrite(RELAY_PIN, LOW);
}

void setup()
{
    tinyls::init();
    tinyls::after(disableRelay, 2000u);
}
```

Behavior:

- the callback executes once after the selected delay;
- the deadline is based on cached `tinyls::now`;
- the same function is not registered twice in DELAYED;
- the same function may still be used in another task class;
- after execution, the record is removed during later queue maintenance.

A zero stored delay becomes due in the next DELAYED phase that can observe the record.

### Remove a DELAYED callback

```cpp
tinyls::removeAfter(fn);
```

Example:

```cpp
tinyls::removeAfter(disableRelay);
```

Removal marks the record for later compaction.

The occupied queue slot is not immediately reusable until the next DELAYED maintenance pass.

## POSTED

### Queue a one-shot callback

```cpp
tinyls::post(fn);
```

Example:

```cpp
static void processCommand()
{
    // Deferred main-context work.
}

static void commandReceived()
{
    tinyls::post(processCommand);
}
```

Behavior:

- POSTED callbacks execute once;
- callbacks are processed in LIFO order;
- the same function is not queued twice at the same time;
- the callback is removed before it is invoked;
- callbacks posted during the POSTED phase are also processed in that phase;
- the POSTED phase drains completely before `tick()` continues.

Do not create:

- self-reposting callbacks;
- cyclic repost chains.

Such patterns can prevent the POSTED phase and `tick()` from terminating.

Use POLL for persistent per-pass execution.

## POLL

### Register a persistent callback

```cpp
tinyls::poll(fn);
```

Example:

```cpp
static void readControls()
{
    // Short non-blocking polling work.
}

void setup()
{
    tinyls::init();
    tinyls::poll(readControls);
}
```

Behavior:

- the callback executes during each POLL phase;
- the callback remains registered until removed;
- the same function is not registered twice in POLL;
- the POLL phase uses a callback-count snapshot;
- callbacks added during the POLL phase normally start on the next scheduler pass;
- an ordinary POLL callback may remove itself.

POLL callbacks must remain short and non-blocking.

### Remove a POLL callback

```cpp
tinyls::removePoll(fn);
```

Example:

```cpp
tinyls::removePoll(readControls);
```

POLL storage is compacted immediately.

Do not call `removePoll()` directly from inside a PARTED step.

## Deferred ISR requests

### Defer a callback from a hardware ISR

```cpp
tinyls::postISR(fn);
```

`postISR()` records a callback request in hardware interrupt context.

The callback itself executes later from `tinyls::tick()` in normal main context.

Example:

```cpp
#include <MTW_TinyLoopScheduler.h>

const uint8_t INTERRUPT_PIN = 2;
static uint8_t ledState = LOW;

static void processInterruptEvent()
{
    ledState = (ledState == LOW) ? HIGH : LOW;
    digitalWrite(LED_BUILTIN, ledState);
}

static void onExternalInterrupt()
{
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

Rules:

- call `postISR()` only from a normal AVR hardware ISR;
- global interrupts must remain disabled during that ISR;
- nested interrupts are not supported;
- `ISR_NOBLOCK` handlers are not supported;
- manually re-enabling interrupts inside the ISR is not supported;
- duplicate callback requests are allowed;
- pending ISR requests are processed in LIFO order;
- `postISR()` performs no `millis()` call;
- `postISR()` performs no sorting;
- `postISR()` performs no duplicate scan;
- `postISR()` does not insert the callback into an ordinary task queue;
- the deferred callback may use enabled main-context scheduler APIs.

Do not call `postISR()` from normal main-context code.

Use `every()`, `after()`, `post()`, or `poll()` from main context.

# PARTED API

PARTED divides a long cooperative operation into short numbered steps.

PARTED:

- is disabled by default;
- requires POLL;
- has no independent queue;
- stores its step state alongside POLL slots;
- executes user steps only from the POLL phase.

Enable it before including the header:

```cpp
#define TLS_PARTED TLS_ENABLE
#include <MTW_TinyLoopScheduler.h>
```

## Recommended activation

Activate PARTED through a scheduler task class:

```cpp
tinyls::poll(partedFn);
tinyls::every(partedFn, period_ms);
tinyls::after(partedFn, delay_ms);
tinyls::post(partedFn);
tinyls::postISR(partedFn);
```

For PERIODIC, DELAYED, POSTED, or deferred ISR activation:

1. the selected task class invokes the PARTED function in main context;
2. `TLS_PART_START()` registers the function in POLL;
3. the function returns without executing a user step;
4. later POLL phases execute the individual steps.

Repeated triggers while the same PARTED function is active do not restart its current sequence.

PARTED activation requires an available POLL slot.

## PARTED structure

```cpp
static void processData()
{
    TLS_PART_START(processData) {

        TLS_STEP(0) {
            readChunk();
            TLS_NEXT();
        }

        TLS_STEP(1) {
            processChunk();
            TLS_NEXT(5);
        }

        TLS_STEP(5) {
            saveResult();
            TLS_FINISH();
        }

    } TLS_PART_END();
}
```

## `TLS_PART_START(fn)`

```cpp
TLS_PART_START(fn)
```

Begins PARTED activation or step dispatch.

The argument must refer to the same callback that contains the PARTED block.

## `TLS_STEP(N)`

```cpp
TLS_STEP(N)
```

Declares one numbered PARTED step.

Supported user step range:

```text
0..247
```

Reserved values:

```text
248..254
```

Internal start state:

```text
255
```

Step numbers must be compile-time constants.

Source order does not determine execution order. The stored step value selects the matching block.

## `TLS_NEXT()`

```cpp
TLS_NEXT();
```

Selects the next numeric step and immediately returns from the callback.

The next step executes during a later POLL pass.

## `TLS_NEXT(N)`

```cpp
TLS_NEXT(5);
```

Selects step `N` and immediately returns from the callback.

A constant step value must remain in the supported range `0..247`.

## `TLS_RESTART()`

```cpp
TLS_RESTART();
```

Keeps the callback registered in POLL, selects step `0`, and immediately returns.

Step `0` executes during a later POLL pass.

## `TLS_FINISH()`

```cpp
TLS_FINISH();
```

Resets PARTED state, removes the current callback from POLL, and immediately returns.

Use this macro for explicit PARTED completion.

## `TLS_PART_END()`

```cpp
TLS_PART_END();
```

Closes the PARTED state-selection block.

If the current stored step has no matching `TLS_STEP`, PARTED resets its state and removes the callback from POLL.

If a matched step reaches the end without a transition macro, the same step remains selected and repeats during the next POLL pass.

## PARTED restrictions

- one POLL invocation executes at most one matching PARTED step;
- automatic local variables do not persist between steps;
- use static, global, or application-owned storage for persistent data;
- do not call a PARTED function from inside another POLL callback;
- do not call `removePoll()` directly from inside a PARTED step;
- use `TLS_FINISH()` for immediate self-removal;
- reserve enough POLL capacity for all simultaneously active PARTED callbacks.

# Compile-time availability

Public functions are available only when their corresponding task class is enabled:

| Switch         | Public API                                      |
| -------------- | ----------------------------------------------- |
| `TLS_PERIODIC` | `every()`, `removeEvery()`                      |
| `TLS_DELAYED`  | `after()`, `removeAfter()`                      |
| `TLS_POSTED`   | `post()`                                        |
| `TLS_POLL`     | `poll()`, `removePoll()`                        |
| `TLS_ISR`      | `postISR()`                                     |
| `TLS_PARTED`   | `TLS_PART_START`, `TLS_STEP`, transition macros |

The ordinary task classes are enabled by default.

PARTED is disabled by default.

When an ordinary task class is disabled, its public declarations, implementation code, queue storage, queue counter, initialization logic, and scheduler phase are removed from the compiled scheduler.

# Multi-file builds

Exactly one translation unit must own the implementation and scheduler state.

## Implementation owner

A normal Arduino sketch containing only `.ino` tabs normally acts as the implementation owner:

```cpp
#include <MTW_TinyLoopScheduler.h>
```

## Additional `.cpp` files

Every additional real `.cpp` translation unit must define:

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
TLS_SLEEP_GUARD
```

Every translation unit that defines a PARTED callback must enable PARTED.

The implementation-owning translation unit must also enable PARTED so that the shared PARTED state is allocated.

# General restrictions

- AVR targets only;
- call `init()` once before registering initial jobs;
- call `tick()` continuously from `loop()`;
- do not call `tick()` from nested contexts;
- keep callbacks short;
- avoid long blocking operations;
- do not use `delay()` inside scheduler callbacks for long waits;
- use only `postISR()` from supported ISR context;
- use main-context APIs from main context;
- do not modify `tinyls::now`;
- do not create endless POSTED repost chains;
- do not directly remove POLL jobs from inside PARTED steps.
