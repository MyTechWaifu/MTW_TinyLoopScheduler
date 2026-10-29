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
