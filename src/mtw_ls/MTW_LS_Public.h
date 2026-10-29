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

