// SPDX-License-Identifier: Apache-2.0
/*
 MTW TinyLoopScheduler

 Copyright 2026 MyTechWaifu

 Licensed under the Apache License, Version 2.0.

 Purpose:
   MTW TinyLoopScheduler is a small cooperative scheduler for AVR targets
   using an Arduino-compatible core. It replaces scattered millis() checks
   with named callbacks dispatched from one explicit place: tinyls::tick().

 Public API:
   - tinyls::every(fn, period_ms):
       register a repeating callback;
   - tinyls::after(fn, delay_ms):
       register a one-shot delayed callback;
   - tinyls::post(fn):
       queue a one-shot callback for the POSTED phase;
   - tinyls::poll(fn):
       register a persistent callback for the POLL phase;
   - tinyls::postISR(fn):
       defer a callback from a normal hardware ISR to main context;
   - tinyls::removeEvery(fn), tinyls::removeAfter(fn), and
     tinyls::removePoll(fn):
       remove the corresponding registered callback.

 Compile-time features:
   - TLS_PERIODIC controls every() and removeEvery();
   - TLS_DELAYED controls after() and removeAfter();
   - TLS_POSTED controls post();
   - TLS_POLL controls poll() and removePoll();
   - TLS_ISR controls postISR();
   - TLS_PARTED controls the PARTED macro API and its per-POLL-slot step
     state; PARTED requires TLS_POLL.
   - TLS_SLEEP_GUARD controls whether SLEEP_MODE_IDLE is reaffirmed before
     every actual sleep entry.

 PERIODIC, DELAYED, POSTED, POLL, and ISR are enabled by default. Disabling
 one of these ordinary subsystems removes its public declarations, runtime
 implementation, queue storage, counters, initialization code, and tick()
 phase. PARTED is disabled by default. Disabled PARTED keeps only diagnostic
 macro stubs so accidental PARTED source code fails at compile time; it adds
 no PARTED runtime storage or execution logic.

 Scheduler pass order:
   1. capture one common millis() snapshot and update the IDLE load gate;
   2. drain deferred ISR callbacks when TLS_ISR is enabled;
   3. compact/reorder PERIODIC storage as required and execute due callbacks;
   4. compact/reorder DELAYED storage as required and execute due callbacks;
   5. enter AVR SLEEP_MODE_IDLE when the IDLE gate permits it;
   6. fully drain one-shot POSTED callbacks when enabled;
   7. execute a bounded POLL pass when enabled.

 Runtime model:
   - cooperative execution only;
   - no preemption and no threads;
   - callbacks dispatched by the scheduler execute synchronously inside
     tinyls::tick() in main context;
   - tick() should be called frequently from loop();
   - callbacks must return quickly;
   - long work should be divided into short PARTED steps;
   - hardware ISRs may call only postISR(), when ISR support is enabled;
   - postISR() records a deferred request; the callback itself runs later
     from tick() in main context.

 API contexts:
   - postISR() is restricted to a normal AVR hardware ISR in which global
     interrupts remain disabled;
   - enabled non-ISR queue APIs are main-context APIs;
   - a callback deferred through postISR() already runs in main context and
     may call any enabled main-context scheduler API;
   - tick() is not reentrant and must not be called from a callback, PARTED
     step, ISR, or other nested context.

 Return values:
   - queue-management functions return uint8_t;
   - 1 means that the requested operation was performed;
   - 0 means that it was rejected, was not found, or could not be performed;
   - applications may ignore the returned value when failure is acceptable.

 Design goals:
   - static storage only;
   - no heap allocation;
   - no dynamic containers;
   - low SRAM usage;
   - predictable memory layout;
   - predictable execution path;
   - simple low-level C/C++ style code;
   - AVR support through Arduino-compatible cores.

 Multi-file builds:
   - exactly one translation unit must include this header without
     TLS_DECL_ONLY; that translation unit owns the implementation and all
     scheduler state;
   - Arduino normally combines all .ino tabs of one sketch into one generated
     C++ translation unit, which is the intended implementation owner;
   - the compile-time switches must be defined before the first inclusion of
     this header in that generated sketch translation unit;
   - every additional real .cpp translation unit that includes this header
     must define TLS_DECL_ONLY before the include;
   - all translation units must use matching TLS_PERIODIC, TLS_DELAYED,
     TLS_POSTED, TLS_POLL, TLS_ISR, TLS_PARTED, and TLS_SLEEP_GUARD values so
     declarations and available symbols remain consistent;
   - every translation unit that defines a PARTED function must enable
     TLS_PARTED, and the implementation-owning translation unit must also
     enable it so the shared PARTED state is allocated.

 Non-goals:
   - not a preemptive RTOS;
   - not a thread scheduler;
   - not a dynamic task manager;
   - not a hard real-time kernel.
*/
#ifndef MTW_TINY_LOOP_SCHEDULER_DECLARATIONS_H
#define MTW_TINY_LOOP_SCHEDULER_DECLARATIONS_H

#include <Arduino.h>

#if !defined(__AVR__)
#error "MTW TinyLoopScheduler currently supports AVR targets only"
#endif

#include <avr/sleep.h>

/*
 --------------------------------------------------------------------------
 Version
 --------------------------------------------------------------------------
*/

#define TLS_VERSION_MAJOR 0
#define TLS_VERSION_MINOR 1
#define TLS_VERSION_PATCH 3
#define TLS_VERSION_STRING "0.1.3"

/*
 --------------------------------------------------------------------------
 Compile-time subsystem selection
 --------------------------------------------------------------------------
*/

#define TLS_ENABLE  1
#define TLS_DISABLE 0

/*
 PERIODIC, DELAYED, POSTED, POLL, and ISR are ordinary queue subsystems and
 are enabled by default. Define a switch before the first inclusion of this
 header to remove the corresponding subsystem completely:

   #define TLS_PERIODIC TLS_DISABLE
   #define TLS_DELAYED  TLS_DISABLE
   #define TLS_POSTED   TLS_DISABLE
   #define TLS_POLL     TLS_DISABLE
   #define TLS_ISR      TLS_DISABLE

 A disabled ordinary subsystem contributes no public API declaration, API
 implementation, queue storage, queue counter, initialization code, or
 tick() phase.

 PARTED is a separate optional macro layer over POLL. It is disabled by
 default, has no independent queue, and is configured below with TLS_PARTED.

 Queue profiles and TLS_MANUAL still define numeric capacities for all
 ordinary queues. These capacity macros consume no SRAM by themselves; a
 capacity belonging to a disabled subsystem is simply not used for storage.
*/

#ifndef TLS_PERIODIC
#define TLS_PERIODIC TLS_ENABLE
#endif

#ifndef TLS_DELAYED
#define TLS_DELAYED TLS_ENABLE
#endif

#ifndef TLS_POSTED
#define TLS_POSTED TLS_ENABLE
#endif

#ifndef TLS_POLL
#define TLS_POLL TLS_ENABLE
#endif

#ifndef TLS_ISR
#define TLS_ISR TLS_ENABLE
#endif

#ifndef TLS_PARTED
#define TLS_PARTED TLS_DISABLE
#endif

#ifndef TLS_SLEEP_GUARD
#define TLS_SLEEP_GUARD TLS_ENABLE
#endif

#if TLS_PERIODIC != TLS_ENABLE && TLS_PERIODIC != TLS_DISABLE
#warning "MTW TinyLoopScheduler: invalid TLS_PERIODIC value; clamped to TLS_ENABLE"
#undef TLS_PERIODIC
#define TLS_PERIODIC TLS_ENABLE
#endif

#if TLS_DELAYED != TLS_ENABLE && TLS_DELAYED != TLS_DISABLE
#warning "MTW TinyLoopScheduler: invalid TLS_DELAYED value; clamped to TLS_ENABLE"
#undef TLS_DELAYED
#define TLS_DELAYED TLS_ENABLE
#endif

#if TLS_POSTED != TLS_ENABLE && TLS_POSTED != TLS_DISABLE
#warning "MTW TinyLoopScheduler: invalid TLS_POSTED value; clamped to TLS_ENABLE"
#undef TLS_POSTED
#define TLS_POSTED TLS_ENABLE
#endif

#if TLS_POLL != TLS_ENABLE && TLS_POLL != TLS_DISABLE
#warning "MTW TinyLoopScheduler: invalid TLS_POLL value; clamped to TLS_ENABLE"
#undef TLS_POLL
#define TLS_POLL TLS_ENABLE
#endif

#if TLS_ISR != TLS_ENABLE && TLS_ISR != TLS_DISABLE
#warning "MTW TinyLoopScheduler: invalid TLS_ISR value; clamped to TLS_ENABLE"
#undef TLS_ISR
#define TLS_ISR TLS_ENABLE
#endif

#if TLS_PARTED != TLS_ENABLE && TLS_PARTED != TLS_DISABLE
#warning "MTW TinyLoopScheduler: invalid TLS_PARTED value; clamped to TLS_DISABLE"
#undef TLS_PARTED
#define TLS_PARTED TLS_DISABLE
#endif

#if TLS_SLEEP_GUARD != TLS_ENABLE && TLS_SLEEP_GUARD != TLS_DISABLE
#warning "MTW TinyLoopScheduler: invalid TLS_SLEEP_GUARD value; clamped to TLS_ENABLE"
#undef TLS_SLEEP_GUARD
#define TLS_SLEEP_GUARD TLS_ENABLE
#endif

/*
 PARTED executes only through the POLL phase. When enabled, it adds one step
 state byte for every physical POLL slot plus one shared active-slot byte. It
 has no independent job queue and therefore cannot be enabled without POLL.
*/
#if TLS_PARTED == TLS_ENABLE && TLS_POLL == TLS_DISABLE
#error "MTW TinyLoopScheduler: TLS_PARTED requires TLS_POLL support"
#endif

#if TLS_PARTED == TLS_ENABLE
#pragma message("MTW TinyLoopScheduler: PARTED support enabled")
#endif

/*
 PARTED is disabled by default. In that configuration no poll_part[] array or
 shared poll_slot byte is allocated.

 Enable it before including this header in projects that define PARTED
 callbacks:

   #define TLS_PARTED TLS_ENABLE
   #include <MTW_TinyLoopScheduler.h>

 In a multi-file build, the implementation-owning translation unit and every
 TLS_DECL_ONLY translation unit that defines a PARTED callback must enable
 TLS_PARTED. All translation units must also use matching ordinary subsystem
 switches; otherwise declarations may not match the symbols emitted by the
 implementation owner.
*/

/*
 PARTED state values:

   0..247   user-defined execution steps;
   248..254 reserved for future PARTED control states;
   255      start/uninitialized state.

 Reserved values must not be selected by TLS_STEP() or by a constant
 TLS_NEXT(N).
*/
#define TLS_PART_MAX_STEP    247u
#define TLS_PART_STATE_START 255u

/*
 --------------------------------------------------------------------------
 Public declarations
 --------------------------------------------------------------------------
*/

namespace tinyls {

/**
 @brief Scheduler callback type.

 Every callback passed to an enabled scheduler API must have the exact
 signature:

     void functionName(void);

 A scheduled callback accepts no arguments and returns no value. Use global
 or static application state when a callback must read or modify data.
*/
typedef void (*Fn)(void);

/**
 @brief Cached millis() snapshot used by the current scheduler pass.

 init() initializes this value, and tick() updates it once near the beginning
 of every pass before any scheduler-dispatched callback runs. All callbacks
 dispatched during that pass therefore see the same cached timestamp unless
 application code incorrectly modifies it. Reading tinyls::now avoids an
 additional millis() call.

 This variable is writable only for implementation reasons and is intended
 for application reads. Do not modify it. A 32-bit read is not atomic on an
 8-bit AVR and therefore must not be relied on from an ISR.
*/
extern uint32_t now;

/**
 @brief Initialize cached time, sleep mode, and all enabled queues.

 Call init() exactly once from setup(), after hardware initialization and
 before registering initial jobs. SLEEP_MODE_IDLE is selected in every
 TLS_SLEEP_GUARD configuration. Queue counts are reset; old array contents need
 not be erased because they become unreachable. Do not call init() from tick(),
 a callback, a PARTED step, or an ISR.
*/
void init(void);

/**
 @brief Execute one complete cooperative scheduler pass.

 Call tick() frequently and only from loop(). The function is not reentrant;
 do not call it from a scheduler callback, PARTED step, hardware ISR, or any
 other nested context. Callbacks dispatched by tick() execute synchronously in
 the caller's main context and must return quickly.

 When the IDLE gate permits sleep, tick() enters SLEEP_MODE_IDLE before the
 POSTED and POLL phases. Execution resumes after an interrupt wakes the MCU,
 then POSTED and POLL continue before tick() returns.
*/
void tick(void);

#if TLS_PERIODIC == TLS_ENABLE

/**
 @brief Register a repeating callback with a millisecond period.

 @param fn Callback with the exact signature void fn(void).
 @param period_ms Requested period in milliseconds.
 @return 1 if registered; 0 if fn is null, fn is already present in PERIODIC,
         or the PERIODIC queue is full.

 Identity is the function pointer within the PERIODIC queue. The same function
 may still be used independently in another queue class. The first deadline is
 calculated from cached tinyls::now rather than from a new millis() call, so
 init() must precede registration. A call made between tick() passes is
 anchored to the most recent cached scheduler timestamp.

 With TLS_PERIOD=16, period_ms is narrowed to uint16_t without clamping. The
 scheduler invokes each due PERIODIC record at most once per tick(); it does
 not issue catch-up bursts for missed periods. A zero stored period makes the
 callback due on every scheduler pass and suppresses IDLE through PERIODIC.
 When POLL is enabled, poll() is the clearer API for ordinary per-pass work.
*/
uint8_t every(Fn fn, uint32_t period_ms);

#endif // TLS_PERIODIC


#if TLS_DELAYED == TLS_ENABLE

/**
 @brief Register a one-shot callback after a millisecond delay.

 @param fn Callback with the exact signature void fn(void).
 @param delay_ms Requested delay in milliseconds.
 @return 1 if registered; 0 if fn is null, fn is already present in DELAYED,
         or the DELAYED queue is full.

 Identity is the function pointer within the DELAYED queue. The same function
 may still be used independently in another queue class. The deadline is
 calculated from cached tinyls::now rather than from a new millis() call, so
 init() must precede registration. A call made between tick() passes is
 anchored to the most recent cached scheduler timestamp.

 With TLS_PERIOD=16, delay_ms is narrowed to uint16_t without clamping. A zero
 stored delay becomes due at the next DELAYED phase that can see the record.
*/
uint8_t after(Fn fn, uint32_t delay_ms);

#endif // TLS_DELAYED


#if TLS_POSTED == TLS_ENABLE

/**
 @brief Queue a one-shot callback for the POSTED phase of tick().

 @param fn Callback with the exact signature void fn(void).
 @return 1 if queued; 0 if fn is null, the same fn is already waiting in
         POSTED, or the POSTED buffer is full.

 POSTED uses LIFO order. A callback is removed from the buffer before it is
 invoked, so it may technically post itself again; do not do this. The POSTED
 phase is fully drained, including callbacks posted by callbacks that are
 currently running. Self-reposting or a repost cycle therefore prevents the
 phase, and potentially tick(), from terminating.

 When POLL is enabled, poll() is the intended API for persistent per-pass work.
*/
uint8_t post(Fn fn);

#endif // TLS_POSTED


#if TLS_POLL == TLS_ENABLE

/**
 @brief Register a persistent callback for POLL execution.

 @param fn Callback with the exact signature void fn(void).
 @return 1 if registered; 0 if fn is null, fn is already present in POLL, or
         the POLL queue is full.

 The callback remains active until removePoll() succeeds. The POLL phase takes
 a callback-count snapshot, so callbacks appended during that phase do not
 increase the current pass budget and normally begin on the next tick. A POLL
 callback may remove itself; compaction is immediate.
*/
uint8_t poll(Fn fn);

#endif // TLS_POLL


#if TLS_ISR == TLS_ENABLE

/**
 @brief Defer a callback from a hardware ISR to the main scheduler loop.

 @param fn Callback with the exact signature void fn(void).
 @return 1 if appended; 0 if fn is null or the ISR request buffer is full.

 postISR() is an ISR-only API. Call it only from a normal AVR hardware ISR in
 which global interrupts remain disabled for the handler duration. Main-context
 code must use an enabled main-context API instead. Nested interrupts,
 ISR_NOBLOCK handlers, and manually re-enabling interrupts inside an ISR are
 not supported.

 The ISR path performs no duplicate scan, sorting, millis() call, critical
 section, or normal queue insertion. It stores only a function pointer and
 publishes the new count. tick() later pops requests in LIFO order and invokes
 them in main context. Duplicate requests are allowed. Requests arriving while
 a non-empty ISR phase is being drained may also be consumed by that phase.
*/
uint8_t postISR(Fn fn);

#endif // TLS_ISR


#if TLS_PERIODIC == TLS_ENABLE

/**
 @brief Mark a callback registered with every() for removal.

 @param fn Previously registered callback.
 @return 1 if found and marked; 0 if fn is null or not present in PERIODIC.

 Removal creates a tombstone. The physical slot and periodic_count entry remain
 occupied until the next PERIODIC maintenance pass, which may occur later in
 the same tick() when removal happens before PERIODIC, or in a later tick().
 Until compaction, a tombstone in a full queue does not provide reusable space.
*/
uint8_t removeEvery(Fn fn);

#endif // TLS_PERIODIC


#if TLS_DELAYED == TLS_ENABLE

/**
 @brief Mark a callback waiting in DELAYED for removal.

 @param fn Previously registered callback.
 @return 1 if found and marked; 0 if fn is null or not present in DELAYED.

 Removal creates a tombstone. The physical slot and delayed_count entry remain
 occupied until the next DELAYED maintenance pass, which may occur later in the
 same tick() when removal happens before DELAYED, or in a later tick(). Until
 compaction, a tombstone in a full queue does not provide reusable space.
*/
uint8_t removeAfter(Fn fn);

#endif // TLS_DELAYED


#if TLS_POLL == TLS_ENABLE

/**
 @brief Remove a callback registered with poll().

 @param fn Previously registered callback.
 @return 1 if found and removed; 0 if fn is null or not present in POLL.

 POLL storage is compacted immediately and does not use tombstones. Self-removal
 from an ordinary POLL callback is supported. Direct removePoll() calls from a
 PARTED step are restricted as documented in the PARTED section.
*/
uint8_t removePoll(Fn fn);

#endif // TLS_POLL

/*
 Internal symbols shared with the PARTED macro implementation.

 These names are implementation details, not supported user-facing API. They
 require external linkage because PARTED macros may be expanded in a
 TLS_DECL_ONLY .cpp translation unit while their storage is defined by the
 single implementation-owning translation unit.
*/
namespace kernel {

#if TLS_PARTED == TLS_ENABLE

enum {
    POLL_SLOT_NONE = 255u
};

extern uint8_t poll_slot;
extern uint8_t poll_part[];

#endif

} // namespace kernel

} // namespace tinyls

/*
 --------------------------------------------------------------------------
 PARTED macro API
 --------------------------------------------------------------------------
*/

/*
 PARTED turns one void callback into a small cooperative state machine whose
 user code is divided into numbered TLS_STEP blocks. The callback function
 itself must have the normal scheduler signature void fn(void).

 Activation paths:
   - tinyls::poll(fn) may register the PARTED callback directly; its first
     POLL invocation initializes the state and executes step 0;
   - a direct application call, PERIODIC callback, DELAYED callback, POSTED
     callback, or callback deferred through postISR() invokes the PARTED
     function outside POLL; that invocation only attempts tinyls::poll(fn)
     and returns without executing a user step.

 User steps execute only when the POLL phase invokes the function while its
 POLL slot is active. If activation occurs before POLL in the current tick(),
 step 0 may execute later in that same tick(). A callback added after the POLL
 phase begins is constrained by the POLL phase snapshot and normally starts on
 the next tick.

 Repeated outside-POLL triggers while the same function is already registered
 in POLL are ignored by poll() as duplicates. They do not restart the callback
 or modify its current step. This permits a PERIODIC source to keep triggering
 while one PARTED run remains active; a later trigger can start a new run only
 after the previous run has left POLL.

 Activation requires a free POLL slot. TLS_PART_START() has no return status:
 if an outside-POLL activation occurs while POLL is full, the attempt is lost.
 A later PERIODIC trigger may retry. A failed direct call is not replayed unless
 application code calls the function again, and a failed DELAYED, POSTED, or
 deferred-ISR activation is not automatically replayed. Reserve POLL capacity
 for every PARTED callback that may be active at the same time.

 Each POLL invocation executes at most one matching TLS_STEP block. TLS_NEXT(),
 TLS_NEXT(N), TLS_RESTART(), and TLS_FINISH() all update state as applicable and
 immediately return from the callback, so the selected next step cannot run
 until a later POLL pass. Automatic local variables are recreated on every
 invocation and do not persist between steps; persistent PARTED data must use
 static/global storage or another explicit application-owned state container.

 Step numbers 0..247 are available to application code. TLS_STEP(N) requires a
 compile-time constant. Values 248..254 are reserved, and 255 is the internal
 start/uninitialized state. Source order does not define execution order; the
 stored step number selects the matching case.

 If a matched TLS_STEP block reaches its end without TLS_NEXT(), TLS_NEXT(N),
 TLS_RESTART(), or TLS_FINISH(), its step number remains selected and the same
 block repeats on the next POLL pass. If the current stored step has no matching
 TLS_STEP block, TLS_PART_END() resets PARTED state, removes the callback from
 POLL, and returns. This unmatched-step path provides natural completion after
 advancing beyond the last implemented step. TLS_FINISH() performs immediate
 explicit completion. TLS_RESTART() keeps the callback in POLL and selects
 step 0 for the next pass.

 Do not call a PARTED function from inside another POLL callback. poll_slot
 identifies the currently executing outer POLL slot; a nested PARTED call would
 incorrectly use and modify that outer slot's PARTED state.

 Do not call removePoll() directly from inside a PARTED step, whether removing
 the current function or another POLL callback. Immediate POLL compaction can
 move slots while TLS_PART_START() still holds the original slot index. Finish
 the current PARTED callback with TLS_FINISH() or unmatched-step completion.
 Defer unrelated POLL changes until after the PARTED function returns, for
 example through a POSTED callback or an application flag.

 Example:

   void loadBigData() {
       TLS_PART_START(loadBigData) {

           TLS_STEP(0) {
               readSmallPart();
               TLS_NEXT();
           }

           TLS_STEP(1) {
               processSmallPart();
               TLS_NEXT(5);
           }

           TLS_STEP(5) {
               finishSmallPart();
               TLS_FINISH();
           }

       } TLS_PART_END();
   }
*/
#define TLS_PRIVATE_NEXT_SELECT(_0, _1, NAME, ...) NAME

#if TLS_PARTED == TLS_ENABLE

/**
 @brief Begin PARTED activation or dispatch the current stored step.

 @param fn The same void callback that contains this macro.

 Outside POLL, this macro attempts poll(fn) and returns without executing a
 step; the poll() result is intentionally not exposed. Inside POLL, it binds
 the current POLL slot, converts the internal start state to step 0, and opens
 the step-selection switch.
*/
#define TLS_PART_START(fn)                                                       \
    do {                                                                         \
        tinyls::Fn tls_part_fn_local = (tinyls::Fn)(fn);                         \
        uint8_t tls_part_index_local = 0u;                                       \
                                                                                 \
        if (tls_part_fn_local == 0) {                                            \
            return;                                                              \
        }                                                                        \
                                                                                 \
        if (tinyls::kernel::poll_slot ==                                         \
            tinyls::kernel::POLL_SLOT_NONE) {                                    \
            tinyls::poll(tls_part_fn_local);                                     \
            return;                                                              \
        }                                                                        \
                                                                                 \
        tls_part_index_local = tinyls::kernel::poll_slot;                        \
                                                                                 \
        if (tinyls::kernel::poll_part[tls_part_index_local] ==                   \
            TLS_PART_STATE_START) {                                              \
            tinyls::kernel::poll_part[tls_part_index_local] = 0u;                \
        }                                                                        \
                                                                                 \
        uint8_t tls_part_matched_local = 0u;                                     \
        switch (tinyls::kernel::poll_part[tls_part_index_local])

/**
 @brief Declare one numbered PARTED execution step.

 @param N Compile-time constant in the user range 0..247.

 Duplicate step numbers are rejected by the generated switch. The stored step
 number, not source order, selects which block executes.
*/
#define TLS_STEP(N)                                                              \
        break;                                                                   \
        static_assert((long)(N) >= 0L &&                                         \
                      (unsigned long)(N) <=                                      \
                          (unsigned long)TLS_PART_MAX_STEP,                       \
                      "TLS_STEP must be in range 0..247");                      \
        case (uint8_t)(N):                                                       \
            tls_part_matched_local = 1u;

/**
 @brief Close the PARTED switch and return from the callback.

 If no TLS_STEP block matched the stored state, the macro resets PARTED state
 and removes the callback from POLL. If a step matched but reached the end
 without a transition macro, the callback remains registered with the same
 step selected for the next POLL pass.
*/
#define TLS_PART_END()                                                           \
        if (!tls_part_matched_local) {                                           \
            tinyls::kernel::poll_part[tls_part_index_local] =                    \
                TLS_PART_STATE_START;                                            \
            tinyls::removePoll(tls_part_fn_local);                               \
        }                                                                        \
        return;                                                                  \
    } while (0)

#define TLS_PRIVATE_NEXT_0()                                                     \
    do {                                                                         \
        if (tinyls::kernel::poll_part[tls_part_index_local] <                    \
            TLS_PART_MAX_STEP) {                                                 \
            ++tinyls::kernel::poll_part[tls_part_index_local];                   \
        } else {                                                                 \
            tinyls::kernel::poll_part[tls_part_index_local] =                    \
                TLS_PART_STATE_START;                                            \
            tinyls::removePoll(tls_part_fn_local);                               \
        }                                                                        \
        return;                                                                  \
    } while (0)

#define TLS_PRIVATE_VALIDATE_NEXT(N)                                             \
    static_assert(!__builtin_constant_p(N) ||                                    \
                      (((long)(N) >= 0L) &&                                      \
                       ((unsigned long)(N) <=                                    \
                        (unsigned long)TLS_PART_MAX_STEP)),                       \
                  "TLS_NEXT constant must be in range 0..247")

#define TLS_PRIVATE_NEXT_1(N)                                                    \
    do {                                                                         \
        TLS_PRIVATE_VALIDATE_NEXT(N);                                            \
        tinyls::kernel::poll_part[tls_part_index_local] = (uint8_t)(N);          \
        return;                                                                  \
    } while (0)

/*
 TLS_NEXT() uses the GNU comma-swallowing extension for an empty __VA_ARGS__
 list. The supported AVR GCC toolchain provides this extension.
*/
/**
 @brief Select the next PARTED state and return from the callback.

 TLS_NEXT() increments the current step. When the current state is 247, the
 callback is completed and removed instead of entering a reserved value.
 TLS_NEXT(N) selects N directly. A constant N is checked at compile time. A
 runtime N is cast to uint8_t without a runtime range check; application code
 must keep it in 0..247. The selected step executes on a later POLL pass.
*/
#define TLS_NEXT(...)                                                            \
    TLS_PRIVATE_NEXT_SELECT(_, ##__VA_ARGS__,                                    \
                            TLS_PRIVATE_NEXT_1,                                  \
                            TLS_PRIVATE_NEXT_0)(__VA_ARGS__)

/**
 @brief Keep the callback in POLL, select step 0, and return.

 Step 0 executes on the callback's next POLL invocation, not in the current
 invocation.
*/
#define TLS_RESTART()                                                            \
    do {                                                                         \
        tinyls::kernel::poll_part[tls_part_index_local] = 0u;                    \
        return;                                                                  \
    } while (0)

/**
 @brief Reset PARTED state, remove this callback from POLL, and return.

 This is the supported immediate self-removal path from inside a PARTED step.
*/
#define TLS_FINISH()                                                             \
    do {                                                                         \
        tinyls::kernel::poll_part[tls_part_index_local] =                        \
            TLS_PART_STATE_START;                                                \
        tinyls::removePoll(tls_part_fn_local);                                   \
        return;                                                                  \
    } while (0)

#else

/*
 Disabled PARTED stubs preserve source-level macro names so the header remains
 self-contained. Any function definition that expands TLS_PART_START() while
 TLS_PARTED is disabled fails at compile time. No PARTED runtime storage or
 executable PARTED path is emitted.
*/
#define TLS_PART_START(fn)                                                       \
    do {                                                                         \
        (void)(fn);                                                              \
        static_assert(TLS_PARTED == TLS_ENABLE,                                  \
                      "TLS_PART_START requires #define TLS_PARTED "             \
                      "TLS_ENABLE before including MTW TinyLoopScheduler");     \
        switch (0)

#define TLS_STEP(N)                                                              \
        break;                                                                   \
        case (uint8_t)(N):

#define TLS_PART_END()                                                           \
        return;                                                                  \
    } while (0)

#define TLS_PRIVATE_NEXT_0()                                                     \
    do {                                                                         \
        return;                                                                  \
    } while (0)

#define TLS_PRIVATE_NEXT_1(N)                                                    \
    do {                                                                         \
        (void)(N);                                                               \
        return;                                                                  \
    } while (0)

#define TLS_NEXT(...)                                                            \
    TLS_PRIVATE_NEXT_SELECT(_, ##__VA_ARGS__,                                    \
                            TLS_PRIVATE_NEXT_1,                                  \
                            TLS_PRIVATE_NEXT_0)(__VA_ARGS__)

#define TLS_RESTART()                                                            \
    do {                                                                         \
        return;                                                                  \
    } while (0)

#define TLS_FINISH()                                                             \
    do {                                                                         \
        return;                                                                  \
    } while (0)

#endif

#endif // MTW_TINY_LOOP_SCHEDULER_DECLARATIONS_H

#if !defined(TLS_DECL_ONLY) && \
    !defined(MTW_TINY_LOOP_SCHEDULER_IMPLEMENTATION_H)
#define MTW_TINY_LOOP_SCHEDULER_IMPLEMENTATION_H

#ifndef MTW_TINY_LOOP_SCHEDULER_CONFIG_H
#define MTW_TINY_LOOP_SCHEDULER_CONFIG_H

/*
 --------------------------------------------------------------------------
 Scheduler profile selection
 --------------------------------------------------------------------------
*/

/*
 Select one storage profile before including this header in the implementation
 owner:

   #define TLS_PROFILE TLS_TINY
   #define TLS_PROFILE TLS_MINI
   #define TLS_PROFILE TLS_MID
   #define TLS_PROFILE TLS_DEF
   #define TLS_PROFILE TLS_MAX
   #define TLS_PROFILE TLS_MANUAL

 If TLS_PROFILE is not defined, TLS_DEF is used.

 Profiles define physical queue capacities. The ordinary subsystem switches
 determine which of those queues are actually compiled. For example, TLS_MINI
 with TLS_DELAYED disabled retains MINI capacities for enabled queues while
 allocating no DELAYED queue.

 The following table is a recommended working load, not the physical capacity.
 Every predefined profile sets the corresponding physical capacity to one
 entry above the listed value. That extra entry is ordinary usable capacity;
 it is not reserved or protected. The recommendation leaves practical
 headroom for bursts and for PERIODIC/DELAYED tombstones that remain counted
 until their next maintenance pass.

 Recommended working load:

   Profile      PERIODIC  DELAYED  POLL  POSTED  ISR_REQUESTS
   TLS_TINY         1        1       1      1          1
   TLS_MINI         2        2       2      2          2
   TLS_MID          6        4       4      4          3
   TLS_DEF         12        8       8      8          6
   TLS_MAX         24       16      16     16         12

 Application code may occupy every physical slot. For more predictable
 behavior, size each queue at least one entry above the maximum simultaneous
 active or pending workload.

 TLS_MANUAL values are direct physical capacities; no additional headroom is
 added. Undefined manual values default to the TLS_DEF physical capacities.
 Capacity macros for disabled ordinary subsystems are still preprocessed and
 range-checked but do not allocate queue storage.
*/

#define TLS_TINY    1
#define TLS_MINI    2
#define TLS_MID     3
#define TLS_DEF     4
#define TLS_MAX     5
#define TLS_MANUAL  6

#ifndef TLS_PROFILE
#define TLS_PROFILE TLS_DEF
#endif

#if TLS_PROFILE != TLS_TINY && \
    TLS_PROFILE != TLS_MINI && \
    TLS_PROFILE != TLS_MID && \
    TLS_PROFILE != TLS_DEF && \
    TLS_PROFILE != TLS_MAX && \
    TLS_PROFILE != TLS_MANUAL
#warning "MTW TinyLoopScheduler: invalid TLS_PROFILE value; clamped to TLS_DEF"
#undef TLS_PROFILE
#define TLS_PROFILE TLS_DEF
#endif

/*
 --------------------------------------------------------------------------
 Profile-defined queue sizes
 --------------------------------------------------------------------------
*/

/*
 Fixed-capacity storage exists for:

   PERIODIC / DELAYED / POLL / POSTED / ISR_REQUESTS

 Each enabled queue uses a statically allocated array and uint8_t count. If a
 queue or buffer is full, the new request returns 0 and no existing entry is
 replaced or evicted. A disabled ordinary subsystem allocates no queue array or
 count. PARTED reuses POLL and, when enabled, adds only per-slot step state.
*/

#if TLS_PROFILE == TLS_MANUAL

/*
 Manual profile.

 Values define physical capacities directly and are later clamped to 2..255.
 Keep each enabled queue at least one entry above the expected maximum number
 of simultaneous active or pending jobs. Define only values that differ from
 the defaults below. Values for disabled ordinary subsystems do not allocate
 storage, although their macros are still validated by the preprocessor.
*/

#ifndef TLS_MAX_PERIODIC
#define TLS_MAX_PERIODIC      13
#endif

#ifndef TLS_MAX_DELAYED
#define TLS_MAX_DELAYED       9
#endif

#ifndef TLS_MAX_POLL
#define TLS_MAX_POLL          9
#endif

#ifndef TLS_MAX_POSTED
#define TLS_MAX_POSTED        9
#endif

#ifndef TLS_MAX_ISR_REQUESTS
#define TLS_MAX_ISR_REQUESTS  7
#endif

#elif TLS_PROFILE == TLS_TINY

#undef TLS_MAX_PERIODIC
#undef TLS_MAX_DELAYED
#undef TLS_MAX_POLL
#undef TLS_MAX_POSTED
#undef TLS_MAX_ISR_REQUESTS

#define TLS_MAX_PERIODIC      2
#define TLS_MAX_DELAYED       2
#define TLS_MAX_POLL          2
#define TLS_MAX_POSTED        2
#define TLS_MAX_ISR_REQUESTS  2

#elif TLS_PROFILE == TLS_MINI

#undef TLS_MAX_PERIODIC
#undef TLS_MAX_DELAYED
#undef TLS_MAX_POLL
#undef TLS_MAX_POSTED
#undef TLS_MAX_ISR_REQUESTS

#define TLS_MAX_PERIODIC      3
#define TLS_MAX_DELAYED       3
#define TLS_MAX_POLL          3
#define TLS_MAX_POSTED        3
#define TLS_MAX_ISR_REQUESTS  3

#elif TLS_PROFILE == TLS_MID

#undef TLS_MAX_PERIODIC
#undef TLS_MAX_DELAYED
#undef TLS_MAX_POLL
#undef TLS_MAX_POSTED
#undef TLS_MAX_ISR_REQUESTS

#define TLS_MAX_PERIODIC      7
#define TLS_MAX_DELAYED       5
#define TLS_MAX_POLL          5
#define TLS_MAX_POSTED        5
#define TLS_MAX_ISR_REQUESTS  4

#elif TLS_PROFILE == TLS_MAX

#undef TLS_MAX_PERIODIC
#undef TLS_MAX_DELAYED
#undef TLS_MAX_POLL
#undef TLS_MAX_POSTED
#undef TLS_MAX_ISR_REQUESTS

#define TLS_MAX_PERIODIC      25
#define TLS_MAX_DELAYED       17
#define TLS_MAX_POLL          17
#define TLS_MAX_POSTED        17
#define TLS_MAX_ISR_REQUESTS  13

#else

/*
 TLS_DEF physical capacities. The documented recommended working load is one
 entry lower in each queue.
*/

#undef TLS_MAX_PERIODIC
#undef TLS_MAX_DELAYED
#undef TLS_MAX_POLL
#undef TLS_MAX_POSTED
#undef TLS_MAX_ISR_REQUESTS

#define TLS_MAX_PERIODIC      13
#define TLS_MAX_DELAYED       9
#define TLS_MAX_POLL          9
#define TLS_MAX_POSTED        9
#define TLS_MAX_ISR_REQUESTS  7

#endif

/*
 --------------------------------------------------------------------------
 Reference memory usage and build sizes
 --------------------------------------------------------------------------
*/

/*
 MTW TinyLoopScheduler uses static compile-time storage only. TLS_PROFILE
 selects physical capacities for ordinary queues. TLS_PERIODIC, TLS_DELAYED,
 TLS_POSTED, TLS_POLL, and TLS_ISR select which ordinary subsystems are
 compiled. TLS_PARTED optionally adds a per-POLL-slot state array and shared
 active-slot state. A disabled ordinary subsystem contributes no queue array,
 queue counter, public API implementation, initialization code, or tick()
 phase.

 Reference measurements below were produced for Arduino Uno / ATmega328P at
 16 MHz. The benchmark sketch contained only tinyls::init() in setup() and
 tinyls::tick() in loop(). The listed SRAM and Flash values are approximate
 reference figures intended for rough capacity planning, not exact guarantees.
 Actual build results may vary slightly with the Arduino AVR core, compiler and
 toolchain version, LTO, build options, sketch structure, and later library
 revisions. Use the compiler build report for the exact size of a particular
 project.

 Minimal profile builds with all ordinary queue subsystems enabled:

   Profile    P32 / PARTED off   P16 / PARTED off
              SRAM    Flash       SRAM    Flash
   TLS_TINY   ~65 B   ~1790 B      ~60 B   ~1800 B
   TLS_MINI   ~85 B   ~1790 B      ~75 B   ~1800 B
   TLS_MID   ~145 B   ~1790 B     ~130 B   ~1800 B
   TLS_DEF   ~245 B   ~1790 B     ~215 B   ~1800 B
   TLS_MAX   ~445 B   ~1790 B     ~395 B   ~1800 B

   Profile    P32 / PARTED on    P16 / PARTED on
              SRAM    Flash       SRAM    Flash
   TLS_TINY   ~65 B   ~1840 B      ~60 B   ~1870 B
   TLS_MINI   ~85 B   ~1840 B      ~80 B   ~1870 B
   TLS_MID   ~145 B   ~1840 B     ~130 B   ~1870 B
   TLS_DEF   ~245 B   ~1840 B     ~220 B   ~1870 B
   TLS_MAX   ~445 B   ~1840 B     ~395 B   ~1870 B

 P32 and P16 mean TLS_PERIOD=32 and TLS_PERIOD=16. Profile capacity changes
 SRAM but normally does not change Flash because the same queue algorithms are
 compiled for every capacity.

 Approximate SRAM planning cost per physical queue slot:

   PERIODIC, TLS_PERIOD=32       ~15 B
   PERIODIC, TLS_PERIOD=16       ~10 B
   DELAYED                       ~10 B
   POLL, PARTED disabled          ~5 B
   POLL, PARTED enabled           ~5 B
   POSTED                         ~5 B
   ISR request                    ~5 B

 These per-slot values are rough planning estimates intended to show the
 relative SRAM cost of each queue type. PERIODIC and DELAYED include their
 order-table entry. Each enabled queue also has a small fixed counter/state
 cost. PARTED adds one state byte per physical POLL slot and a small fixed
 active-slot state. Actual values may vary slightly with the compiler, ABI,
 build configuration, and later library revisions. Use the compiler build
 report for the exact SRAM consumption of a particular project.

 Minimal feature-stripping builds with TLS_MANUAL capacities set to 2:

   Configuration                         SRAM     Flash
   Empty Arduino sketch                  ~10 B     ~445 B
   Scheduler core, all classes disabled  ~15 B     ~585 B
   PERIODIC only                         ~40 B    ~1210 B
   DELAYED only                          ~35 B    ~1070 B
   POSTED only                           ~15 B     ~615 B
   POLL only                             ~15 B     ~635 B
   ISR only                              ~20 B     ~660 B
   POLL + PARTED                         ~15 B     ~685 B
   All ordinary subsystems, PARTED off      ~65 B    ~1790 B

 These are approximate minimal init()/tick() builds. AVR LTO may remove enabled
 queue storage or API code that is provably unused. Class-only Flash sizes are
 complete build totals and must not be added together. Compile the real sketch
 to obtain final SRAM and Flash usage.
*/

/*
 --------------------------------------------------------------------------
 Reference CPU performance
 --------------------------------------------------------------------------
*/

/*
 Reference CPU measurements were produced on Arduino Uno / ATmega328P at
 16 MHz using Timer1 with prescaler 1. One timer count equals one CPU cycle.

 Every scenario was sampled 32 times. Timer measurement overhead was removed.
 AVR IDLE sleep was explicitly suppressed during measured intervals. The listed
 cycle counts are approximate reference values intended to show the expected
 order of magnitude and relative scheduler cost, not exact guarantees. Results
 may vary slightly with the compiler and toolchain version, Arduino AVR core,
 LTO, enabled subsystems, queue capacities, application code layout, benchmark
 conditions, and later library revisions.

 Basic scheduler cost:

   Core-only empty tick                         ~40 cycles
   All ordinary subsystems enabled, empty tick        ~105 cycles
   Minimal direct callback                      ~20 cycles

 Approximate tick cost growth with a minimal callback:

   POLL                                         ~50 cycles per active job
   POSTED                                       ~30 cycles per queued job
   Deferred ISR                                 ~45 cycles per request
   PARTED                                       ~70..75 cycles per active step

 Sorted timing queues inspect only the earliest deadline. When that deadline
 is not due, the steady PERIODIC or DELAYED pass has O(1) cost for the tested
 range of 1..8 waiting jobs.

 DELAYED tombstone compaction is approximately:

   ~25 + N * ~20 cycles

 PERIODIC and DELAYED deadline rebuilds use insertion sort:

   already ordered input                        O(N)
   reverse-ordered input                        O(N^2)

 With eight reverse-ordered timing jobs, the measured complete rebuild tick
 remained approximately 0.11 ms on a 16 MHz ATmega328P.

 A mixed tick executing one PERIODIC, DELAYED, POSTED, POLL, deferred ISR,
 and PARTED callback required about 535 cycles, or about 33 us.

 These figures describe scheduler overhead with minimal test callbacks.
 Real callback execution time must be added separately. See
 docs/PERFORMANCE.md for the benchmark methodology and complete results.
*/

/*
 --------------------------------------------------------------------------
 Period storage width
 --------------------------------------------------------------------------
*/

/*
 Internal period and delay input width.

 TLS_PERIOD = 16:
   Use this only when every PERIODIC interval and DELAYED timeout is within
   0..65,535 ms, approximately 65.535 seconds. PERIODIC records store their
   repeat period as uint16_t, which saves two bytes per PERIODIC record.
   DELAYED records store only a 32-bit absolute deadline, but after() still
   narrows its input through the same internal Period type. Deadline arithmetic
   remains 32-bit in both modes.

 TLS_PERIOD = 32:
   Use this for supported intervals longer than 65,535 ms.

 Public every() and after() parameters remain uint32_t in both modes, giving
 TLS_DECL_ONLY translation units stable declarations. The implementation does
 not clamp or reject values that exceed the selected storage width. In
 TLS_PERIOD=16 mode, C++ uint16_t narrowing applies modulo 65,536; for example,
 65,536 becomes 0 and 65,537 becomes 1. Callers must enforce the documented
 range before registration.

 Absolute deadlines use uint32_t because Arduino millis() is 32-bit. Deadline
 tests use signed-delta arithmetic:

       (int32_t)(now - deadline_ms) >= 0

 This comparison is rollover-safe only when the effective stored period or
 delay does not exceed INT32_MAX milliseconds. Therefore the supported maximum
 is 65,535 ms in TLS_PERIOD=16 mode and 2,147,483,647 ms (about 24.855 days) in
 TLS_PERIOD=32 mode. Larger TLS_PERIOD=32 values are accepted by the API but
 have unsupported deadline semantics and may appear due at the wrong time.

 Longer calendar-scale waits should be implemented with a shorter supported
 scheduler interval plus an application counter. No runtime range validation
 or saturation is performed.
*/

#ifndef TLS_PERIOD
#define TLS_PERIOD 32
#endif

#if TLS_PERIOD != 16 && TLS_PERIOD != 32
#warning "MTW TinyLoopScheduler: invalid TLS_PERIOD value; clamped to 32"
#undef TLS_PERIOD
#define TLS_PERIOD 32
#endif

/*
 --------------------------------------------------------------------------
 Dynamic IDLE selection
 --------------------------------------------------------------------------
*/

/*
 Dynamic IDLE threshold selection.

 TLS_IDLE selects a load-gate threshold; it does not enable or disable the
 IDLE backend. At the beginning of tick(), the scheduler measures the elapsed
 interval between the current and previous tick() entry using the low 16 bits
 of millis(). If that interval is greater than the selected threshold, IDLE is
 skipped for the current pass.

 Public selector:

   #define TLS_IDLE TLS_AGGRESSIVE  //  3 ms threshold
   #define TLS_IDLE TLS_NORMAL      //  8 ms threshold
   #define TLS_IDLE TLS_LAZY        // 16 ms threshold

 TLS_NORMAL is the default. The comparison is strictly greater-than, so an
 interval equal to the threshold does not set the load skip bit.

 IDLE is entered when no deferred ISR callback was executed, no PERIODIC
 callback was due, no DELAYED callback was due, and the tick-entry interval
 did not exceed the threshold. POSTED entries and persistent POLL
 callbacks intentionally do not suppress IDLE; they run after an interrupt
 wakes the MCU.

 The 16-bit interval is a lightweight heuristic and wraps modulo 65,536 ms.
 This wrap does not affect scheduler deadlines, which remain 32-bit, but the
 load gate is not meaningful for gaps of 65.536 seconds or longer.
 
 TLS_SLEEP_GUARD controls how the fixed SLEEP_MODE_IDLE selection is
 maintained. init() selects SLEEP_MODE_IDLE in both configurations.

   #define TLS_SLEEP_GUARD TLS_ENABLE

     Safe default. Before every actual sleep entry, tick() selects
     SLEEP_MODE_IDLE again. This protects the scheduler if application code or
     another library changed the MCU sleep-mode selection after init().

   #define TLS_SLEEP_GUARD TLS_DISABLE

     Optimized policy. SLEEP_MODE_IDLE is selected only by init(); tick() does
     not repeat the mode-selection write before sleeping. After init(), the
     application and third-party libraries must not change the MCU sleep mode.
     sleep_enable(), sleep_cpu(), and sleep_disable() still run for every
     actual sleep entry in both configurations. 
*/

#define TLS_AGGRESSIVE 1
#define TLS_NORMAL     2
#define TLS_LAZY       3

#ifndef TLS_IDLE
#define TLS_IDLE TLS_NORMAL
#endif

#if TLS_IDLE != TLS_AGGRESSIVE && \
    TLS_IDLE != TLS_NORMAL && \
    TLS_IDLE != TLS_LAZY
#warning "MTW TinyLoopScheduler: invalid TLS_IDLE value; clamped to TLS_NORMAL"
#undef TLS_IDLE
#define TLS_IDLE TLS_NORMAL
#endif

#undef TLS_IDLE_THRESHOLD_MS

#if TLS_IDLE == TLS_AGGRESSIVE
#define TLS_IDLE_THRESHOLD_MS 3
#elif TLS_IDLE == TLS_NORMAL
#define TLS_IDLE_THRESHOLD_MS 8
#else
#define TLS_IDLE_THRESHOLD_MS 16
#endif

/*
 --------------------------------------------------------------------------
 Compile-time range clamps
 --------------------------------------------------------------------------
*/

/*
 Every physical queue capacity is clamped independently to 2..255, including
 macros belonging to disabled ordinary subsystems. Disabled queues still do
 not allocate storage.

 Keep each enabled physical queue at least one entry above the expected maximum
 number of simultaneously active or pending jobs. Predefined profiles already
 follow that recommendation. uint8_t counts support capacities up to 255 while
 keeping SRAM and AVR instruction cost low.
*/

#if TLS_MAX_PERIODIC < 2
#warning "MTW TinyLoopScheduler: TLS_MAX_PERIODIC was below 2; clamped to 2"
#undef TLS_MAX_PERIODIC
#define TLS_MAX_PERIODIC 2
#endif

#if TLS_MAX_PERIODIC > 255
#warning "MTW TinyLoopScheduler: TLS_MAX_PERIODIC was above 255; clamped to 255"
#undef TLS_MAX_PERIODIC
#define TLS_MAX_PERIODIC 255
#endif

#if TLS_MAX_DELAYED < 2
#warning "MTW TinyLoopScheduler: TLS_MAX_DELAYED was below 2; clamped to 2"
#undef TLS_MAX_DELAYED
#define TLS_MAX_DELAYED 2
#endif

#if TLS_MAX_DELAYED > 255
#warning "MTW TinyLoopScheduler: TLS_MAX_DELAYED was above 255; clamped to 255"
#undef TLS_MAX_DELAYED
#define TLS_MAX_DELAYED 255
#endif

#if TLS_MAX_POLL < 2
#warning "MTW TinyLoopScheduler: TLS_MAX_POLL was below 2; clamped to 2"
#undef TLS_MAX_POLL
#define TLS_MAX_POLL 2
#endif

#if TLS_MAX_POLL > 255
#warning "MTW TinyLoopScheduler: TLS_MAX_POLL was above 255; clamped to 255"
#undef TLS_MAX_POLL
#define TLS_MAX_POLL 255
#endif

#if TLS_MAX_POSTED < 2
#warning "MTW TinyLoopScheduler: TLS_MAX_POSTED was below 2; clamped to 2"
#undef TLS_MAX_POSTED
#define TLS_MAX_POSTED 2
#endif

#if TLS_MAX_POSTED > 255
#warning "MTW TinyLoopScheduler: TLS_MAX_POSTED was above 255; clamped to 255"
#undef TLS_MAX_POSTED
#define TLS_MAX_POSTED 255
#endif

#if TLS_MAX_ISR_REQUESTS < 2
#warning "MTW TinyLoopScheduler: TLS_MAX_ISR_REQUESTS was below 2; clamped to 2"
#undef TLS_MAX_ISR_REQUESTS
#define TLS_MAX_ISR_REQUESTS 2
#endif

#if TLS_MAX_ISR_REQUESTS > 255
#warning "MTW TinyLoopScheduler: TLS_MAX_ISR_REQUESTS was above 255; clamped to 255"
#undef TLS_MAX_ISR_REQUESTS
#define TLS_MAX_ISR_REQUESTS 255
#endif

#endif // MTW_TINY_LOOP_SCHEDULER_CONFIG_H

/*
 --------------------------------------------------------------------------
 Scheduler implementation
 --------------------------------------------------------------------------
*/

namespace tinyls {

/*
 Public cached time definition.
*/
uint32_t now = 0u;

namespace kernel {

/*
 Internal period storage type.
*/
#if TLS_PERIOD == 16
typedef uint16_t Period;
#else
typedef uint32_t Period;
#endif

/*
 One persistent scheduler control byte.

 Bits 0..3 are current-pass IDLE-skip causes. tick() clears them at entry while
 preserving bits 4..7. IDLE is entered when all four lower bits remain zero
 after the ISR, PERIODIC, and DELAYED phases.

 Bits 4..7 request PERIODIC or DELAYED reordering/compaction. They persist
 across tick() calls because every(), after(), removeEvery(), and removeAfter()
 may update queues in main context outside the corresponding maintenance phase.
*/
enum {
    STATE_SKIP_ISR          = 0x01u,
    STATE_SKIP_DELTA        = 0x02u,
    STATE_SKIP_PERIODIC     = 0x04u,
    STATE_SKIP_DELAYED      = 0x08u,

    STATE_REBUILD_PERIODIC  = 0x10u,
    STATE_COMPACT_PERIODIC  = 0x20u,
    STATE_REBUILD_DELAYED   = 0x40u,
    STATE_COMPACT_DELAYED   = 0x80u,

    STATE_IDLE_MASK         = 0x0Fu,
    STATE_MAINT_MASK        = 0xF0u
};

/*
 --------------------------------------------------------------------------
 Internal job records
 --------------------------------------------------------------------------
*/

#if TLS_PERIODIC == TLS_ENABLE

typedef struct {
    Fn       fn;
    Period   period_ms;
    uint32_t deadline_ms;
} PeriodicJob;

#endif // TLS_PERIODIC

#if TLS_DELAYED == TLS_ENABLE

typedef struct {
    Fn       fn;
    uint32_t deadline_ms;
} DelayedJob;

#endif // TLS_DELAYED

/*
 --------------------------------------------------------------------------
 Scheduler storage
 --------------------------------------------------------------------------
*/

/*
 Each ordinary queue is emitted only when its subsystem is enabled. Profile
 capacity macros remain defined and validated for configuration consistency,
 but a disabled queue has no array or count and therefore consumes no queue
 SRAM. PARTED, when enabled, adds poll_part[] alongside the enabled POLL queue.
*/

#if TLS_POSTED == TLS_ENABLE
static Fn posted[TLS_MAX_POSTED];
static uint8_t posted_count = 0u;
#endif

#if TLS_ISR == TLS_ENABLE
static volatile Fn isr_requests[TLS_MAX_ISR_REQUESTS];
static volatile uint8_t isr_request_count = 0u;
#endif

#if TLS_PERIODIC == TLS_ENABLE
static PeriodicJob periodic[TLS_MAX_PERIODIC];
static uint8_t periodic_order[TLS_MAX_PERIODIC];
static uint8_t periodic_count = 0u;
#endif

#if TLS_DELAYED == TLS_ENABLE
static DelayedJob delayed[TLS_MAX_DELAYED];
static uint8_t delayed_order[TLS_MAX_DELAYED];
static uint8_t delayed_count = 0u;
#endif

#if TLS_POLL == TLS_ENABLE
static Fn poll_jobs[TLS_MAX_POLL];
static uint8_t poll_count = 0u;
#endif

#if TLS_PARTED == TLS_ENABLE
/*
 poll_slot and poll_part are shared implementation symbols. External linkage
 allows PARTED macros expanded in TLS_DECL_ONLY .cpp translation units to use
 the single state instance owned by the implementation translation unit.
*/
uint8_t poll_slot = POLL_SLOT_NONE;
uint8_t poll_part[TLS_MAX_POLL];
#endif

/*
 Persistent state byte.

 Lower bits are cleared on every tick. Upper maintenance bits survive until
 the corresponding PERIODIC or DELAYED phase services them.
*/
static uint8_t state_mask = 0u;

/*
 Low 16 bits of the previous tick-entry timestamp. Unsigned subtraction from
 the next low-16 snapshot yields the modulo-65,536 ms interval used only by the
 IDLE load heuristic. Scheduler deadlines do not use this value.
*/
static uint16_t idle_prev_ms = 0u;

/*
 --------------------------------------------------------------------------
 Critical-section helpers
 --------------------------------------------------------------------------
*/

/*
 Save SREG, disable interrupts for a short operation, then restore the exact
 previous SREG value. If interrupts were already disabled before entry, they
 remain disabled after exit.
*/
static inline uint8_t enterCritical(void)
{
    uint8_t state = SREG;
    cli();
    return state;
}

static inline void exitCritical(uint8_t state)
{
    SREG = state;
}

} // namespace kernel

/*
 --------------------------------------------------------------------------
 Public scheduler implementation
 --------------------------------------------------------------------------
*/

void init(void)
{
    /*
     Read millis() before disabling interrupts to keep the critical section
     short.
    */
    uint32_t initial_now = (uint32_t)millis();

    /*
     Select the scheduler sleep mode in both guard configurations.
     With TLS_SLEEP_GUARD disabled, this is the only mode-selection write.
    */
    set_sleep_mode(SLEEP_MODE_IDLE);
 
    uint8_t state = kernel::enterCritical();

    now = initial_now;
    kernel::idle_prev_ms = (uint16_t)initial_now;

#if TLS_POSTED == TLS_ENABLE
    kernel::posted_count = 0u;
#endif
#if TLS_ISR == TLS_ENABLE
    kernel::isr_request_count = 0u;
#endif
#if TLS_PERIODIC == TLS_ENABLE
    kernel::periodic_count = 0u;
#endif
#if TLS_DELAYED == TLS_ENABLE
    kernel::delayed_count = 0u;
#endif
#if TLS_POLL == TLS_ENABLE
    kernel::poll_count = 0u;
#endif
#if TLS_PARTED == TLS_ENABLE
    kernel::poll_slot = kernel::POLL_SLOT_NONE;
#endif
    kernel::state_mask = 0u;

    kernel::exitCritical(state);
}

void tick(void)
{
    /*
     Preserve queue-maintenance bits and clear only per-pass IDLE-skip bits.
    */
    kernel::state_mask &= kernel::STATE_MAINT_MASK;

    /*
     Capture one common timestamp before any callback executes. Scheduler
     deadlines and callbacks dispatched in this pass use the same tinyls::now
     value. The low-16 subtraction below is only the IDLE load heuristic.
    */
    now = (uint32_t)millis();

    uint16_t current_ms16 = (uint16_t)now;
    uint16_t tick_delta16 =
        (uint16_t)(current_ms16 - kernel::idle_prev_ms);

    kernel::idle_prev_ms = current_ms16;

    if (tick_delta16 > (uint8_t)TLS_IDLE_THRESHOLD_MS) {
        kernel::state_mask |= kernel::STATE_SKIP_DELTA;
    }

#if TLS_ISR == TLS_ENABLE
    /*
     ----------------------------------------------------------------------
     ISR request phase
     ----------------------------------------------------------------------
    */

    /*
     Fast path: avoid entering a critical section on the common empty-buffer
     path. A request arriving immediately after this check is processed by the
     next tick(), which is equivalent to a request arriving after this phase.
    */
    if (kernel::isr_request_count > 0u) {
        /*
            At least one deferred ISR callback will be executed, so
            this pass must not enter IDLE before POSTED and POLL.
        */

        kernel::state_mask |= kernel::STATE_SKIP_ISR;
     
        while (1) {
            Fn fn;
            uint8_t state = kernel::enterCritical();

            if (kernel::isr_request_count == 0u) {
                kernel::exitCritical(state);
                break;
            }

            /*
             Pop under the critical section, then execute outside it.
             Interrupts remain disabled for only a few instructions, and the
             deferred callback runs in normal main context.
            */
            --kernel::isr_request_count;
            fn = kernel::isr_requests[kernel::isr_request_count];

            kernel::exitCritical(state);

            fn();
        }
    }

#endif // TLS_ISR

#if TLS_PERIODIC == TLS_ENABLE || \
    TLS_DELAYED == TLS_ENABLE || \
    TLS_POLL == TLS_ENABLE
    uint8_t phase_count;
#endif

#if TLS_PERIODIC == TLS_ENABLE
    /*
     ----------------------------------------------------------------------
     PERIODIC maintenance: compact/rebuild at phase start
     ----------------------------------------------------------------------
     */

    if (kernel::state_mask &
        (kernel::STATE_COMPACT_PERIODIC | kernel::STATE_REBUILD_PERIODIC)) {

        if (kernel::state_mask & kernel::STATE_COMPACT_PERIODIC) {
            uint8_t write = 0u;

            /*
             Compact pass: remove tombstones left by removeEvery(). Physical
             records may move, so rebuild periodic_order[] as packed indices.
            */
            for (uint8_t read = 0u; read < kernel::periodic_count; ++read) {
                if (kernel::periodic[read].fn == 0) {
                    continue;
                }

                if (write != read) {
                    kernel::periodic[write] = kernel::periodic[read];
                }

                kernel::periodic_order[write] = write;
                ++write;
            }

            kernel::periodic_count = write;
        }

        /*
         Common sort path.

         Without compaction, physical indices remain valid. New jobs were
         already appended to periodic_order[], and rescheduled jobs keep their
         existing indices. Re-sort the existing order instead of rebuilding
         it as 0..periodic_count-1.

         After compaction, periodic_order[] was rebuilt above because physical
         record indices changed.
        */
        for (uint8_t i = 1u; i < kernel::periodic_count; ++i) {
            uint8_t key = kernel::periodic_order[i];
            uint32_t key_deadline = kernel::periodic[key].deadline_ms;
            uint8_t j = i;

            while (j > 0u) {
                uint8_t prev = kernel::periodic_order[(uint8_t)(j - 1u)];

                if ((int32_t)(key_deadline -
                      kernel::periodic[prev].deadline_ms) >= 0) {
                    break;
                }

                kernel::periodic_order[j] = prev;
                --j;
            }

            kernel::periodic_order[j] = key;
        }

        kernel::state_mask &= (uint8_t)~(kernel::STATE_COMPACT_PERIODIC |
                                         kernel::STATE_REBUILD_PERIODIC);
    }

    /*
     ----------------------------------------------------------------------
     PERIODIC due phase
     ----------------------------------------------------------------------
     */

    if (kernel::periodic_count > 0u) {
        phase_count = kernel::periodic_count;

        uint8_t index = kernel::periodic_order[0];

        /*
         Deadline comparison is signed-delta based. This is the standard
         millis()-safe form: it remains correct across uint32_t wrap-around
         as long as compared intervals are below 2^31 ms.
         */
        if ((int32_t)(now - kernel::periodic[index].deadline_ms) >= 0) {
            kernel::state_mask |= kernel::STATE_SKIP_PERIODIC;

            Fn fn = kernel::periodic[index].fn;

            if (fn != 0) {
                kernel::Period period_ms = kernel::periodic[index].period_ms;
                uint32_t old_deadline = kernel::periodic[index].deadline_ms;
                uint32_t next_deadline = old_deadline + (uint32_t)period_ms;

                /*
                 If the scheduler is only slightly late, keep the old cadence
                 by adding one period to the previous deadline. If it is very
                 late, reschedule from now to avoid a catch-up burst.
                 */
                if ((int32_t)(now - next_deadline) >= 0) {
                    kernel::periodic[index].deadline_ms =
                        now + (uint32_t)period_ms;
                } else {
                    kernel::periodic[index].deadline_ms = next_deadline;
                }

                fn();
            }

            for (uint8_t i = 1u; i < phase_count; ++i) {
                index = kernel::periodic_order[i];
                fn = kernel::periodic[index].fn;

                if (fn == 0) {
                    continue;
                }

                if ((int32_t)(now - kernel::periodic[index].deadline_ms) < 0) {
                    break;
                }

                kernel::Period period_ms = kernel::periodic[index].period_ms;
                uint32_t old_deadline = kernel::periodic[index].deadline_ms;
                uint32_t next_deadline = old_deadline + (uint32_t)period_ms;

                if ((int32_t)(now - next_deadline) >= 0) {
                    kernel::periodic[index].deadline_ms =
                        now + (uint32_t)period_ms;
                } else {
                    kernel::periodic[index].deadline_ms = next_deadline;
                }

                fn();
            }

            kernel::state_mask |= kernel::STATE_REBUILD_PERIODIC;
        }
    }

#endif // TLS_PERIODIC

#if TLS_DELAYED == TLS_ENABLE
    /*
     ----------------------------------------------------------------------
     DELAYED maintenance: compact/rebuild at phase start
     ----------------------------------------------------------------------
     */

    if (kernel::state_mask &
        (kernel::STATE_COMPACT_DELAYED | kernel::STATE_REBUILD_DELAYED)) {

        if (kernel::state_mask & kernel::STATE_COMPACT_DELAYED) {
            uint8_t write = 0u;

            /*
             Compact pass: remove DELAYED jobs that have already fired or
             were deleted. Physical records may move, so rebuild
             delayed_order[] as packed indices.
            */
            for (uint8_t read = 0u; read < kernel::delayed_count; ++read) {
                if (kernel::delayed[read].fn == 0) {
                    continue;
                }

                if (write != read) {
                    kernel::delayed[write] = kernel::delayed[read];
                }

                kernel::delayed_order[write] = write;
                ++write;
            }

            kernel::delayed_count = write;
        }

        /*
         Common sort path.

         Without compaction, physical indices remain valid. New jobs were
         already appended to delayed_order[]. Re-sort the existing order
         instead of rebuilding it as 0..delayed_count-1.

         After compaction, delayed_order[] was rebuilt above because physical
         record indices changed.
        */
     
        for (uint8_t i = 1u; i < kernel::delayed_count; ++i) {
            uint8_t key = kernel::delayed_order[i];
            uint32_t key_deadline = kernel::delayed[key].deadline_ms;
            uint8_t j = i;

            while (j > 0u) {
                uint8_t prev = kernel::delayed_order[(uint8_t)(j - 1u)];

                if ((int32_t)(key_deadline -
                      kernel::delayed[prev].deadline_ms) >= 0) {
                    break;
                }

                kernel::delayed_order[j] = prev;
                --j;
            }

            kernel::delayed_order[j] = key;
        }

        kernel::state_mask &= (uint8_t)~(kernel::STATE_COMPACT_DELAYED |
                                         kernel::STATE_REBUILD_DELAYED);
    }

    /*
     ----------------------------------------------------------------------
     DELAYED due phase
     ----------------------------------------------------------------------
     */

    if (kernel::delayed_count > 0u) {
        phase_count = kernel::delayed_count;

        uint8_t index = kernel::delayed_order[0];

        if ((int32_t)(now - kernel::delayed[index].deadline_ms) >= 0) {
            kernel::state_mask |= kernel::STATE_SKIP_DELAYED;

            Fn fn = kernel::delayed[index].fn;

            if (fn != 0) {
                /*
                 Mark the record expired before calling user code. This lets
                 the same function pass the DELAYED duplicate check if it calls
                 after() again. Re-registration can still fail when the physical
                 queue is full because this tombstone remains counted until the
                 next DELAYED maintenance pass.
                 */
                kernel::delayed[index].fn = 0;
                fn();
            }

            for (uint8_t i = 1u; i < phase_count; ++i) {
                index = kernel::delayed_order[i];
                fn = kernel::delayed[index].fn;

                if (fn == 0) {
                    continue;
                }

                if ((int32_t)(now - kernel::delayed[index].deadline_ms) < 0) {
                    break;
                }

                kernel::delayed[index].fn = 0;
                fn();
            }

            kernel::state_mask |= (kernel::STATE_COMPACT_DELAYED |
                                   kernel::STATE_REBUILD_DELAYED);
        }
    }

#endif // TLS_DELAYED

    /*
     ----------------------------------------------------------------------
     IDLE backend
     ----------------------------------------------------------------------
    */

    /*
     IDLE intentionally precedes POSTED and POLL. The lower state bits suppress
     sleep when a deferred ISR callback ran, PERIODIC work was due, DELAYED work
     was due, or the interval between tick() entries exceeded the selected
     threshold. Queue maintenance alone does not suppress sleep.

     POSTED and POLL deliberately do not set skip bits. Even when either has
     pending work, the MCU enters SLEEP_MODE_IDLE here when the IDLE skip mask
     is zero and continues with those phases only after a normal interrupt
     wakes it.
    */
    if ((kernel::state_mask & kernel::STATE_IDLE_MASK) == 0u) {
#if TLS_SLEEP_GUARD == TLS_ENABLE
        /*
         Reaffirm IDLE immediately before sleep in case external code changed
         the global MCU sleep-mode selection after init().
        */
        set_sleep_mode(SLEEP_MODE_IDLE);
#endif
        sleep_enable();
        sleep_cpu();
        sleep_disable();
    }

#if TLS_POSTED == TLS_ENABLE
    /*
     ----------------------------------------------------------------------
     POSTED phase
     ----------------------------------------------------------------------
     */

    /*
     POSTED is a fully drained LIFO buffer. Each callback is popped before it
     runs, giving O(1) removal without array shifting. New callbacks posted
     during this phase are also drained before the phase ends.

     A callback must not repost itself or participate in a repost cycle because
     that would keep this while loop running indefinitely. Persistent repeated
     execution belongs to POLL.
    */
    while (kernel::posted_count > 0u) {
        --kernel::posted_count;

        Fn fn = kernel::posted[kernel::posted_count];
        fn();
    }

#endif // TLS_POSTED

#if TLS_POLL == TLS_ENABLE
    /*
     ----------------------------------------------------------------------
     POLL phase
     ----------------------------------------------------------------------
    */

    /*
     phase_count snapshots the number of callbacks present at POLL entry and is
     the maximum invocation budget for this phase. Appending callbacks does not
     increase that budget. Immediate removals can reduce the queue while the
     phase is running. When a callback removes itself, the next entry shifts
     into the same index; the index is advanced only if the original callback
     still occupies that slot after it returns.
    */
    phase_count = kernel::poll_count;

#if TLS_PARTED == TLS_ENABLE
    /*
     PARTED macros need the active POLL slot as shared state because the macro
     expansion may live in a TLS_DECL_ONLY translation unit while this loop and
     the queue storage live in the implementation owner.
    */
    kernel::poll_slot = 0u;

    while (phase_count > 0u &&
           kernel::poll_slot < kernel::poll_count) {

        --phase_count;

        uint8_t index = kernel::poll_slot;
        Fn fn = kernel::poll_jobs[index];

        fn();

        if (index < kernel::poll_count &&
            kernel::poll_jobs[index] == fn) {

            ++kernel::poll_slot;
        }
    }

    kernel::poll_slot = kernel::POLL_SLOT_NONE;
#else
    /*
     Without PARTED, keep the active POLL index local so it can remain in a
     register and no persistent poll_slot byte is allocated.
    */
    uint8_t poll_index = 0u;

    while (phase_count > 0u &&
           poll_index < kernel::poll_count) {

        --phase_count;

        uint8_t index = poll_index;
        Fn fn = kernel::poll_jobs[index];

        fn();

        if (index < kernel::poll_count &&
            kernel::poll_jobs[index] == fn) {

            ++poll_index;
        }
    }
#endif
#endif // TLS_POLL
}

#if TLS_PERIODIC == TLS_ENABLE

/*
 Register one PERIODIC record.

 The initial deadline is cached tinyls::now plus the narrowed stored period;
 this function does not call millis(). Identity is the function pointer within
 PERIODIC. The uint32_t API parameter is cast to kernel::Period without range
 validation, so TLS_PERIOD=16 uses modulo-65,536 narrowing.

 A zero stored period is valid and remains due on every scheduler pass. The
 due phase invokes a record at most once per tick and advances or skips its
 deadline instead of issuing catch-up calls. POLL is the preferred API for
 ordinary per-pass execution when available.
*/
uint8_t every(Fn fn, uint32_t period_ms)
{
    if (fn == 0) {
        return 0u;
    }

    if (kernel::periodic_count >= TLS_MAX_PERIODIC) {
        return 0u;
    }

    for (uint8_t i = 0u; i < kernel::periodic_count; ++i) {
        if (kernel::periodic[i].fn == fn) {
            return 0u;
        }
    }

    uint8_t index = kernel::periodic_count;
    kernel::Period stored_period = (kernel::Period)period_ms;

    kernel::periodic[index].fn = fn;
    kernel::periodic[index].period_ms = stored_period;
    kernel::periodic[index].deadline_ms =
        now + (uint32_t)stored_period;
    kernel::periodic_order[index] = index;

    ++kernel::periodic_count;
    kernel::state_mask |= kernel::STATE_REBUILD_PERIODIC;

    return 1u;
}

#endif // TLS_PERIODIC


#if TLS_DELAYED == TLS_ENABLE

/*
 Register one DELAYED record.

 The deadline is cached tinyls::now plus the delay narrowed through
 kernel::Period; this function does not call millis(). Identity is the function
 pointer within DELAYED. TLS_PERIOD=16 therefore applies modulo-65,536 narrowing
 to delay_ms even though the resulting absolute deadline is stored as uint32_t.
*/
uint8_t after(Fn fn, uint32_t delay_ms)
{
    if (fn == 0) {
        return 0u;
    }

    if (kernel::delayed_count >= TLS_MAX_DELAYED) {
        return 0u;
    }

    for (uint8_t i = 0u; i < kernel::delayed_count; ++i) {
        if (kernel::delayed[i].fn == fn) {
            return 0u;
        }
    }

    uint8_t index = kernel::delayed_count;
    kernel::Period stored_delay = (kernel::Period)delay_ms;

    kernel::delayed[index].fn = fn;
    kernel::delayed[index].deadline_ms =
        now + (uint32_t)stored_delay;
    kernel::delayed_order[index] = index;

    ++kernel::delayed_count;
    kernel::state_mask |= kernel::STATE_REBUILD_DELAYED;

    return 1u;
}

#endif // TLS_DELAYED


#if TLS_POSTED == TLS_ENABLE

/*
 Append one callback to the POSTED LIFO buffer.

 A duplicate already waiting in POSTED is rejected. The callback is popped
 before invocation, and the POSTED phase also consumes callbacks added while it
 is draining. Self-reposting and repost cycles are therefore prohibited because
 they can prevent tick() from returning. Use POLL for persistent execution.
*/
uint8_t post(Fn fn)
{
    if (fn == 0) {
        return 0u;
    }

    if (kernel::posted_count >= TLS_MAX_POSTED) {
        return 0u;
    }

    for (uint8_t i = 0u; i < kernel::posted_count; ++i) {
        if (kernel::posted[i] == fn) {
            return 0u;
        }
    }

    kernel::posted[kernel::posted_count] = fn;
    ++kernel::posted_count;

    return 1u;
}

#endif // TLS_POSTED


#if TLS_POLL == TLS_ENABLE

/*
 Append one persistent POLL callback.

 Duplicate function pointers are rejected within POLL. When PARTED is enabled,
 the corresponding poll_part[] slot is initialized to the internal start state
 even when the callback is an ordinary non-PARTED POLL function.
*/
uint8_t poll(Fn fn)
{
    if (fn == 0) {
        return 0u;
    }

    if (kernel::poll_count >= TLS_MAX_POLL) {
        return 0u;
    }

    for (uint8_t i = 0u; i < kernel::poll_count; ++i) {
        if (kernel::poll_jobs[i] == fn) {
            return 0u;
        }
    }

    kernel::poll_jobs[kernel::poll_count] = fn;

#if TLS_PARTED == TLS_ENABLE
    kernel::poll_part[kernel::poll_count] = TLS_PART_STATE_START;
#endif

    ++kernel::poll_count;

    return 1u;
}

#endif // TLS_POLL


#if TLS_ISR == TLS_ENABLE

/*
 Append one deferred request from a normal AVR hardware ISR.

 A regular AVR ISR runs with global interrupts disabled, so this ISR-only path
 does not open another critical section. Main-context calls, nested interrupts,
 ISR_NOBLOCK handlers, and manually re-enabled interrupts are unsupported.

 The function pointer is written before the volatile count publishes the new
 entry. No duplicate scan, sort, millis() call, or ordinary queue insertion is
 performed. tick() later pops the request under a short critical section and
 invokes the callback outside that section in main context.
*/
uint8_t postISR(Fn fn)
{
    if (fn == 0) {
        return 0u;
    }

    uint8_t index = kernel::isr_request_count;

    if (index >= TLS_MAX_ISR_REQUESTS) {
        return 0u;
    }

    kernel::isr_requests[index] = fn;
    kernel::isr_request_count = (uint8_t)(index + 1u);

    return 1u;
}

#endif // TLS_ISR


#if TLS_PERIODIC == TLS_ENABLE

/*
 Mark one PERIODIC record as a tombstone.

 The record remains included in periodic_count until the next PERIODIC
 maintenance pass compacts physical storage and rebuilds the order table.
 Therefore removal from a full queue does not immediately create reusable
 capacity. A due-phase scan skips a tombstone if removal occurs before that
 record would otherwise execute.
*/
uint8_t removeEvery(Fn fn)
{
    if (fn == 0) {
        return 0u;
    }

    for (uint8_t i = 0u; i < kernel::periodic_count; ++i) {
        if (kernel::periodic[i].fn == fn) {
            kernel::periodic[i].fn = 0;
            kernel::state_mask |=
                (kernel::STATE_COMPACT_PERIODIC |
                 kernel::STATE_REBUILD_PERIODIC);
            return 1u;
        }
    }

    return 0u;
}

#endif // TLS_PERIODIC


#if TLS_DELAYED == TLS_ENABLE

/*
 Mark one DELAYED record as a tombstone.

 The record remains included in delayed_count until the next DELAYED
 maintenance pass compacts physical storage and rebuilds the order table.
 Therefore removal from a full queue does not immediately create reusable
 capacity. A due-phase scan skips a tombstone if removal occurs before that
 record would otherwise execute.
*/
uint8_t removeAfter(Fn fn)
{
    if (fn == 0) {
        return 0u;
    }

    for (uint8_t i = 0u; i < kernel::delayed_count; ++i) {
        if (kernel::delayed[i].fn == fn) {
            kernel::delayed[i].fn = 0;
            kernel::state_mask |=
                (kernel::STATE_COMPACT_DELAYED |
                 kernel::STATE_REBUILD_DELAYED);
            return 1u;
        }
    }

    return 0u;
}

#endif // TLS_DELAYED


#if TLS_POLL == TLS_ENABLE

/*
 Remove one POLL callback and compact the array immediately.

 When PARTED is enabled, poll_part[] entries shift in lockstep with their
 callbacks. Ordinary POLL callbacks may remove themselves; the POLL loop checks
 whether the original function still occupies the active index before advancing
 that index.

 Do not call removePoll() directly from inside a PARTED step. Immediate slot
 movement can invalidate the index captured by TLS_PART_START() and can make a
 later transition write another callback's state. Use TLS_FINISH() for current
 PARTED self-removal, or defer unrelated POLL changes until after return.
*/
uint8_t removePoll(Fn fn)
{
    if (fn == 0) {
        return 0u;
    }

    for (uint8_t i = 0u; i < kernel::poll_count; ++i) {
        if (kernel::poll_jobs[i] == fn) {
            for (uint8_t j = i;
                 (uint8_t)(j + 1u) < kernel::poll_count;
                 ++j) {

                kernel::poll_jobs[j] =
                    kernel::poll_jobs[(uint8_t)(j + 1u)];

#if TLS_PARTED == TLS_ENABLE
                kernel::poll_part[j] =
                    kernel::poll_part[(uint8_t)(j + 1u)];
#endif
            }

            --kernel::poll_count;
            return 1u;
        }
    }

    return 0u;
}

#endif // TLS_POLL


} // namespace tinyls

#endif // !TLS_DECL_ONLY && !MTW_TINY_LOOP_SCHEDULER_IMPLEMENTATION_H


/*
                                                   ...                    .;UUn;
                                               :!zzzUUzz: ,:;;;:,      .:!zUz:!z
                                            ,znUUUzzzUn!zUzUnfffnUz:,;zzUnfnz:;U
                                          ,nBUUfBDDDDMMB;!nfMBnUz!;zUUnfBDUzMB;U
                          !;.            zBU!fMMBnzzUMDfUz!fMBfU. ;UBMMDB: BMM!U
                         ,fzU!:     .,.,nB;zDMBBfffnz:UB:;nBBBU.;fMDMMMBz:;zMM!U
                         ,n;!nnUz::zzzzzU:;BUnBMDMMMDM;!.!fBMfU;;,UBMMfnMM;:MM!U!,
                          n:!nfMDMBfnUz!!,;.!fBfnU!:;UM;;MfnfBDDBU,:fn,fMMn;fB!z!fU:
                          zzfMnUBDMMDDDBU;.UMMfBfUfMfnUfBzUBBfBMMDMU:.:fMMf:ffz; .!nn:
                        .!zznDM:.UMMMMMn.:MDMBDUzMM;.;zfn!zUBDMMMMMDB; UMMn:Bz!U    ;fn,
                       !fU.U!MMU;nnnBB! zDMMMDzUDB,!f;;ffUMBUUMMMMMBMDU:BMU!M!znz .   !fz
                     ;fU,  z!fDf!fMz,z.UDMMMDzUDM!fDzfDBBMMMDfUBMMMMBfDn;f;zM:ffz!     .Uf:
                   .Uf:    .n!Dn!BMB: UDBMMDf!DMnnDBnDBMDznDMDMfMMMMMBUBB, fB.,DUn,      :fU
                  ;Bz       !zUM;nDD:zDBMMMM!MMMnMMBBM,nD,,MMMMMMMBMMMMzUf,nz;;UDzU        nf.
                 Uf:         U!fU;fU!DfBMMDnnDMBBMMMMB .f, nDMMMMMMfMMMDn!!...BzMfU; . .zz. !B:
               .fn         . ;n;M;.:DffMMMMUMMMMMMMBMf  ,. ;MMBMMBfMfMMMDMn;  zBBMz:;; !UU!  :B!
              ,Bz   ;;    zz  UzUn.MBnMMMMBfMBMMMMMfMf .,;;.nBBfDMnBMnBMBfMDMnznnfU!:  .!UU   .Bz
             :B; . ;Uzz  Uz   .n;,BBzMMBMMfBMBMMMMMnBf,nBMD;!UMnnDMUBMfBMBU!UBBfnUU!:     !n.   fz
            :B: .  !nz: :n   . ;UnM:BDfBMMfMBfMMMBBnff!DMMMB:U;M!UDB!MDBBMMf;    nDnU  .   ;f:   fz
           :B: .  !n,   ,n    :nfM,nDBnMMMfDfnBMMMBUnBzMMMMDn!!;B:zDf:fDMMMMDBU;:!MfU,!,    .n;  .B!
          ,B: .  UU     U!   !BnM:;DMfBMMBnDnUfBMMBn!MUMMMMMMn!z:U:;DB.!BDDMBBBfnzzz!n!   !U;.n!  ,B:
         .B; . .n!  . ,n!  .nB!M;.BMMnMMMfnDz!nUDMBf,fzMMMMMDf;:n;U:,fM, !nBMMMBffnBzn   .n!n  n,  :B.
         f! . :n:    ;n:  :Bn,B! zDBBnMMDnUD!,f:DMBB,zzBMMDB!!MBnBnU;.!fz  .,;!!!;UD;;z . ,!U: U: . zn
        Un    U; .  zn.  ;B!.BU.:BBBBfMMMnzD! n,UDMM;,;fDDU:nDDMn!!:;,: !Uz, .:!U,UDU:n: .  ;z U,    f!
       ;B     U:  .Uz   :B: Bf,nzDnBfBMMBn;D;!U!.nMMzz;;Dz;BBfz,  :.!zU  ;:z!:;:.;!Df!fn: . ;z n:    ,B.
       B:     U: ,n;    f; UD:nznDzBfBBMff.M!Uf;U:UDU!M,UBBfz.:  ,fzMU:,;:UU;  , nzMM:Ufn:  ;z :n:::  zn
      Un  :zz U: U;    ;U .Bz!!!BM:MnfBMnn.!z!z::! !M!fM!BBz,zU!!nzzn,f;n,fM;  !.!BfDz:nnn! .n! .fzU:  B:
     ,B.  Uzn,U: U:    !: ;n;;:UMB.MnUBfB!::UfDDn!n;:n!MMBMUff;!!!Un.;B;B!nf...!!.MBMf!fUnnU. Uz :nU,  !n
     UU    n! U: U:    :: z!!.;UBU ff:MzM;,zz:!!zUUBfzzUMMUzUnBfBMDD!nU;D!B! ,z,f nMMD!U:zUzU: zU z;    f:
    ,B.    n, U: U:     , !!; :Uf; zM ffUn ,.:;.;  ;nBMUz:nMMDMMMMMM;n.zDzz   f,n,:DMMfz; :zzn: n z;    zn
    zU .  UU  n: U:       :U,..Uf,:.M;.M!U!.,!Df!!zzn::zz,nDMMMMDDMU;;,fD;:...fn,f.UDMMzU.  z!U,n ;n    .B.
    f:  .n!  UU  n: ,,     U!  ;f:U zf :B;;,;nzffUfBDU!DDM:nMMMBnUzUn!zBD:,U.!zD;!B,BMMfzz   Uz!!U ;n, . n!
   :f  :n: .nz .n! :UU;    .n;  U;U, n! :f;. UDBBMMMDUUMMMMzzzzUnfMDf:fBM: z:UzMM:nB!MMDUn;  !Uz !n.,n;  !n
   UU .f, :n: :n:  :UU;      ;; :n;.; z: .zz: UDDMMBn!BfMMMDMMMDDMMMz,BBM: z:UnfDB,BMnMMMnf: !nz  :n:.n! ,f.
   f; .n  n: ;n. .  f;  :.      UD;zU  :,  ,;:,znUzUnMMBMMMMMBMMMMMM,!MBM!;U ;MfMDU fDBMMMnn;nn, . .z..n  f;
  ,B. .n. U: U; . ,n;  :f      !Mf;D! .    , .; UMMMDMMMMMMBBBMMMMM; UBMM,;,  BMMMD! UMMMMMBU!,     ;..n  Uz
  ;f  .n. U: z;  ;n: . Un    .!BBzMM.,,  .!! !; ;UMDMMMMBBMBMMMMDB;n.ffMB,  , ;DfMMD! ;fMMBMB!...  .zz.n  !n
  zn  .n. U: z; !n.    !MUzzzfMBBMDz n. .!M, U, z ,nMDDMMMBfMMMDB.nB;ffMB:  .; nBnDMDz UBMMfBDMU!!!zfU n  :f
  Uz  .n. U: z; n.      zfffBnUBMDf.Bz  !MM, z!.U   .!nMDDMMMMDn zDfnUBMBz . z: fnUDMDU,BMMMnznMMMBnn,:f, ,f
  n!  znz n: z; n,       ,:;,!BMDf;Bf  :UBM;; zU,      .!nBDDM! !DMfn:MMfn   .n. UzzMMD!.BDBDBU;:;!z: ;n; .f.
  f;  :z.;n. U; n,      :;znBMDMnzMf . z:fDnz,          :,.!z, !DMBn!:DMBnz . :n, !;:fDM:,BBnDDMn;..!n:   .f.
  f:    zn. !n. n,    :znBBBBDf;nDn.:. !;!DMnf;    ..   .U    zDMMff,.MMMfn! ,.,z, .. ;fU ,Mn!MMMMMU .Uz  .f.
  f:  .Uz  UU  ;n.   !nBMUzBDU,BB: :.   !;nDDnU!  .;.  . nz .nDMMDnn; UDMDzU; ,  ,,. ...:  :D:zDMBUMD! U! .f.
  f;  ;U .nz  zU    znBf:;MDz.f! .!Unnnz.::zDM:n: :.     UMUBDMMMMB!n; BMMB:n!!:UUznfBMBfn! zB fMMf.nDU n.,f
  n;  ;z n! .nz    ;nBU :DDU z, UMDDDDDfnn!.BD!;;;!;!zUzfBMDMMMDMMBU!U!;MMM;znUffzBDMMMMMDDB!; ;BnDU UD!z:,f
  n!  ;U n,.n;     UnU, fDB ..,BDMMMMMUnBUU!MD,;zUMDDDBfDMMMMMBnnnfBfUz,MMM,z!fDzBDMMMMMMMMMDz .nzMB. nMU,;n
  zU  ;z n,,n     .nU!,.MBz  ,MMMMMDBUUfUnfMM;.nDMBBMMMfBMMBfBBBBMMMDD!UMDU;nzDfUDMMMMMMMMMMMDz U:MM, ,MU.zU
  !n  ;U n,,n      n!z .Mn: .BMMMMDfUnnMMUzU!zBMMMMMMfnUznBBMMMMMMMMBnfDMU;fUfDUBMMMMMMMMMMMMMM,:,Mf,, Bn.n;
  :f .Un,U,,n   .  !z;: nB. UDMMMDfUBzzD:;fBMDMMMMMMn!..,;nMMMMMMMBnnBBM!zBMzMMUMMMMMMMMMMMMMMDU :D!!..B; f,
  .B, zz.U,,n ,n:   zUU,.B!.MMMDMnzn!!zn:fMMMMMMMMMf!:  :;nMMMMMMMUUnBzznDDnUDfnDMMMMMMMMMMMMMMB zUz! zf.:f
   n!    U,,n Uz     ,n: ,!zDMMfzzz:zMMfnUfMMMMMMMBz;. ,;zMMMMMMDnf;nDU,nMMzfDUfMMMMMMMMMMMMMMMM:.;B.zf: Uz
   zn . .f.,U.n;.    :z.. .BBnnUUzUfMMMMMDMMMMMMMMMz;;;;!fMMMMMMMfznzMDfUn;!BMzBMMMMMMMMMMMMMMMD; fB:;   f:z.
   ,B. :n; :n.nfz!;;!!;!  .UUfnUnBDDMMMMMMMMMMMMMMMBfBnnBBBBMMMMMMfUzzfMDMfUnfzMMMMMMMMMMMMMMMMD! BM.   !n Uz
    n! z: :n: ;UMBffffn. :UBBzUMDMMMMMMMMMMMMMMMBMMMDDBfnfBMMMMMMMMMBn::BMfnn!UMMMMBBMMMMMMMMMMD! nDn::,;;!zn.
    ;f   ;n,   .,Uffn! .zfff!fDMMMMMMMMMMMMMMMMMfBMDBUUfMMMMMMMMMMMMMDD;;DM:UUUMMfMnMMMMMMMMMMMD; ,MDfzzzznnn.
     f; :U  .  .;:    ,UUzn!BDMMMMMMMMMMMMMMMMMMBUMzzfMMMMMMMMMMMMMMMMDf;DD!;nUMMBzUDMMMMMMMMMMM:  ,fDDMMMnU!
     !f :f:  ,!UU!. ::z!UfzBMMMMMMMMMMMMMMMMMMMDf,zfDMMMMMMMMMMMMMBDMMMzfBB!!!nBMD!zDMMMMMMMMMMM. U, :!zzzz!
      f! :nnnBMBU..!!!:zUUnDMMMMMMMMMMMMMMMMMMMM!zMDMMMMMMMMMMMMMM!fBfUBBUU:!:nfMMMzMMMMMMMMMMMf  nM! ;n!!:
      :U ;nMDDn: ,Uz!.U UzMMMMMMMMMMMMMMMMMMMMMUfDMMMMMMMMMMMMMMMMf!UnfU;U!n!:fnMMDnnDMMMMMMMMDn  :DDf!!z,
       !nBMDB;  :n!! U,:fUDMMMMMMMMMMMMMMMMMMMUBDMMMMMMMMMMMMMMMMMMMfnUUBnnU;!nnfMMMzMMMMMMMMMDz   UDMDfzUz
     .nUnMDU.. :n;z U:zznnDMMMMMMMMMMMMMMMMMMzBMMMMMMMMMMMMMMMMMMMMMMDDMffU :!!nnMMDnnDMMMMMMMM: .  fMMBMnzU
    .f;nDDz;U. n;z ;!z::nnDMMMMMMMMMMMMMMMMMzfDMMMMMMMMMMMMMMMMMMMMMMBffn!  zz!znBMMBzMMMMMMMMB. .; ,MMMUMfzz
    n:UDMU,B, !;!;:U U.,nnDMMMMMMMMMMMMMMMDUnDMMMMMMMMMMMMMMMMMMMMMBffn!.   !n!!UfMMDzBMMMMMMDn . !; UDMf:DUU;
   !z:DMn nz ,z,U!zz.U, fUMMMMMMMMMMMMMMMMBzDMMMMMMMMMMMMMMMMMMMBfffn! ;U!  ,z,z!fMMDnnDMMMMMDz . ,n ;MMD:!D!U
   n,nDM,.f. !::!z U:U  UUBMMMMMMMMMMMMMMDzBMMMMMMMMMMMMMMMMMMBfffU:   zUz  :U.z,ffMMfUDMMMMMM;   .f.,BMD! MUn
   n.fDf ,U ,!!U U.U,U,U;nnDMMMMMMMMMMMMMfzDMMMMMMMMMMMMMMMMBffn!. .!; ,n  !U.:U nnMMBzDMMMMMM.   .f.:fMM: fUU
   n,nDU ,! ;;zU:z.U zz!;fUBMMMMMMMMMMMMDznDMMMMMMMMMMMMMMBnfn;.!!.!UU :z !U ;U. znMMBzDMMMMDf    ,f ;BDf  fUU
   ;U;MU  , z:!:,z n. !z:,fnMMMMMMMMMMMMM;BMMMMMMMMMMMMMBnfn; ,Unn,.n. !! U,:U !:;nBDfzDMMMMDU    z! fDB, ,Un:
    !zUf:   U,!,.n.:n. ,n !fnDMMMMMMMMMMB;MMMMMMMMMMMMMnff! :UU:,. U!  U; U.!:,nz:nfDnUDMMMMD;   .z nDn:, ;n!
     :!zn:  n,;z :n :nz.z! UnfDMMMMMMMMDn:DMMMMMMMMMMfnfz..Uz.   ;U! .zU  n z: n.,nfDUnDMMMMM,   , !DU U;!U;
  ..   .zz; Uz Uz :U.!z..U! nnBDMMMMMMMDz:DMMMMMMMMMnfn;, z; :zzU!  !U; .U;.n ,z !nfD!BMMMMMf      BB ;U!;.
 ;U   . z:n.!B!.!z :U ,;  !U nnBDMMMMMMM,,MMMMMMMDBnB!.z;,n. UUU  ;n! :Un::n: z;:fUMfzDMMMMDU  ,. ,MU!,U    :,
 U!    ,U:M,,nz; z: U: U;  !z,nnfDMMMMDn  fMMMMMMnff. zz!U,  !z  zU. :fU:;U  z! .UUD!nDMMMMD!  !.  BfB,;U:,;n!
 Uz,.,:z;BM. U!z:.U ,n. ;U. nn:znfMMMDB.  zDMMMMnfz ;n:.,   ,n   n  !U...n ;n;,U:Uff:MMMMMMM,  z:  ;MMM!;zUzU:
 !nnUnnnMM:,..f;z ;U .U!::U:.;U;znnMDB,   .BMMMnB; UU .!!!!!U: .U! ,n   U; zU.zUUUB,UDMMMMMB   !U .,:fMDMfnnz
  zUfBBBU.;U  ;n!! :U,.nn. U!  ;znnUz.   ! :DMnf,:n: !n:.,::  !n: :n: :U!.!:  U:Un!.MMMMMMDn   ,B, U,.zzUUU!
   ::,,.:;M,   ;n!! .U!,,:: !U!, ;nBz  .ff. ;fn,zU ,n! .!;:,;n! .UU. zU. nUz z;UU! nDMMMMMDz    B; Un f;;:.
     . z!Bn  .  :nzz: ;!zz!z..,!U:.;Bf:zUzU ;n:U; zU. zU;:;!!  !n: :n; :U;:.!;Uz: :MMMMMMMD; . ,M; nB z,
      .U!D! :: . .zUnz:,,;.:z!!::;!:.zf!!fnzB;z.,n; ;n; .;:,,;n! .Uz  zU.  ,zn!   fMMMMMMMM, . UB .Mf z;
      :zzD! !;     ,;znnz:::..,!!:!nz!;zfU!zznz!U..zz .zU;;;!!. !n: ;n;   ,nf;   zDMMMMMMMB.  !B, nD; n,
      .U:Dn :n     nn;,,!zzzUz!!z!::UfBn;,,,::!UUUn! ;z: .;::,;U!  U!  .;nfz ,; ,MMMMMMMMMB .UU .fDz Uz
       zzzMU !z.  ;DDMBz   ....  .zBBn!!znfffnUz!!!znnz:!U;:;;;. :U;,!nfn;.  !: nDMMMMMMMDf z: ;MB:.Uz
        zzzfU;:,  fMMMMDU  ;, .,;nfUUfBMMMMMMMMMBfU!;:!zUnUUUUUUnffnnz;. :.  !.;DMMMMMMMMDn , ;Dz ,!:
         ,!!zzU, zDMMMMMD! ,! .:UfBMMMMMMMMBMMMMMMMMBnz;,.,,::;;:,.    ,z:  .z fMMMMMMMMMDf   fU  .
              !z.MMMMMMMMf  z. fDMMMMMMMMMMfMMMMMMMMMMMBfU;;:.    ,;:.,;.   :z,MMMMMMMMMMMf   n;
*/
