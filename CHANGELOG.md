# Changelog

All notable changes to MTW TinyLoopScheduler are documented in this file.

The project follows Semantic Versioning.

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
