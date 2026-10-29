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

