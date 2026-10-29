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

