# Changelog

All notable changes to MTW TinyLoopScheduler are documented in this file.

The project follows Semantic Versioning.

## [0.1.4] - 2026-07-12

### Added

- Added PlatformIO examples.
- Added `platformio.ini` configuration files for the PlatformIO examples.

### Compatibility

- No library runtime behavior changes.
- No public API changes.

## [0.1.3] - 2026-07-11

### Added

- Added the `TLS_SLEEP_GUARD` compile-time option.
- `TLS_SLEEP_GUARD` defaults to `TLS_ENABLE`, preserving the previous safe sleep behavior.
- Added compile-time validation for `TLS_SLEEP_GUARD`; invalid values emit a warning and fall back to `TLS_ENABLE`.

### Changed

- `tinyls::init()` now selects `SLEEP_MODE_IDLE` in all sleep-guard configurations.
- When `TLS_SLEEP_GUARD` is enabled, `tick()` reaffirms `SLEEP_MODE_IDLE` before every actual sleep entry.
- When `TLS_SLEEP_GUARD` is disabled, the sleep mode is selected only during `init()`, avoiding the repeated mode-selection write.
- Updated configuration and API documentation for the new sleep-guard behavior.

### Performance

- Reduced deferred-ISR phase overhead by setting `STATE_SKIP_ISR` once per non-empty ISR phase instead of once per executed callback.
- Added an optional lower-overhead sleep path for applications that exclusively control the MCU sleep mode.

### Compatibility

- No public function API changes.
- Existing projects retain the previous sleep behavior without configuration changes.
- When `TLS_SLEEP_GUARD` is set to `TLS_DISABLE`, application code and third-party libraries must not change the MCU sleep mode after `tinyls::init()`.

## [0.1.2] - 2026-07-08

### Added

- Added `library.json` manifest for PlatformIO package metadata.
- Declared PlatformIO platform compatibility with `atmelavr` and `atmelmegaavr`.

### Changed

- Updated Arduino library metadata to include both `avr` and `megaavr` architectures.
- Added PlatformIO installation instructions through GitHub.

### Compatibility

- No public API changes.
- No scheduler behavior changes.
- No source compatibility breaks for sketches written for `0.1.0` or `0.1.1`.

## [0.1.1] - 2026-06-22

### Performance

* Optimized PERIODIC insertion sorting by caching the current key deadline before the inner sorting loop.
* Optimized DELAYED insertion sorting using the same technique.

### Compatibility

* No public API changes.
* No scheduler behavior or queue-ordering changes.
* Fully compatible with applications written for v0.1.0.

## [0.1.0] - 2026-06-21

### Added

* Initial public release of MTW TinyLoopScheduler.
* Header-only cooperative scheduler for AVR targets using an Arduino-compatible core.
* Static compile-time storage with no heap allocation and no dynamic containers.
* `tinyls::init()` scheduler initialization.
* `tinyls::tick()` cooperative scheduler pass.
* Cached scheduler timestamp exposed as `tinyls::now`.
* PERIODIC callbacks through `tinyls::every()`.
* DELAYED one-shot callbacks through `tinyls::after()`.
* POSTED one-shot callbacks through `tinyls::post()`.
* Persistent POLL callbacks through `tinyls::poll()`.
* Deferred AVR ISR callbacks through `tinyls::postISR()`.
* PERIODIC removal through `tinyls::removeEvery()`.
* DELAYED removal through `tinyls::removeAfter()`.
* POLL removal through `tinyls::removePoll()`.
* Compile-time enable/disable control for PERIODIC, DELAYED, POSTED, POLL, ISR, and PARTED through the corresponding `TLS_*` switches.
* Full compile-time kernel trimming: a disabled class is removed together with its public API, implementation code, static storage, queue state, initialization logic, and `tick()` phase.
* Predefined queue profiles:

  * `TLS_TINY`
  * `TLS_MINI`
  * `TLS_MID`
  * `TLS_DEF`
  * `TLS_MAX`
* Manual queue configuration through `TLS_MANUAL`.
* Independent queue limits for PERIODIC, DELAYED, POLL, POSTED, and ISR requests.
* Compile-time queue-limit validation and clamping to the supported range.
* Selectable 16-bit or 32-bit period storage through `TLS_PERIOD`.
* Rollover-safe deadline comparison within the documented interval limits.
* Mandatory automatic AVR `SLEEP_MODE_IDLE` entry controlled by a dynamic load gate with configurable thresholds:

  * `TLS_AGGRESSIVE`
  * `TLS_NORMAL`
  * `TLS_LAZY`
* Optional PARTED step-based cooperative execution through POLL.
* PARTED transitions:

  * `TLS_NEXT()`
  * `TLS_NEXT(N)`
  * `TLS_RESTART()`
  * `TLS_FINISH()`
* PARTED user step range `0..247`.
* PERIODIC and DELAYED deadline ordering.
* Deferred tombstone compaction for PERIODIC and DELAYED queues.
* LIFO execution for POSTED callbacks.
* LIFO processing for deferred ISR requests.
* Multi-file support through `TLS_DECL_ONLY`.
* Apache License 2.0 licensing.
* Arduino example sketches.
