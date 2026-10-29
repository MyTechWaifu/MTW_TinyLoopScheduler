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
