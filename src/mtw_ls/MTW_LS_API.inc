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


