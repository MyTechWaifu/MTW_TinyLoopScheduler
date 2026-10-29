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
