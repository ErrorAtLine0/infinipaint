# Windows pen input

This change preserves reported pen history and contact ownership without adding
a positional filter or changing ARM64/Vulkan build scripts.

## Acquisition and lifecycle

- The pinned SDL 3.4.16 recipe reads pointer history before the message advances,
  retries bounded buffer growth, and falls back to the latest report on failure.
- Reports are processed oldest-first. A 4096-report cache compares report data,
  rather than discarding distinct pressure/state reports with equal timestamps.
- Fractional HIMETRIC mapping accounts for device, display and client origins.
  These are Windows-reported coordinates, not electrical sensor measurements.
- Stable QPC calibration preserves intervals when reports run slightly ahead of
  receipt. Unusable timing is marked with timestamp 1; consumers must not use it
  to claim measured speed/latency.
- Pressure is cached before the corresponding motion. Stationary contact reports
  are retained. Lift/cancel ends contact before hover movement.
- App callbacks retain the timestamp and SDL pen id. Pointer/device changes,
  proximity loss and focus loss terminate the preceding contact. Mouse/touch
  releases cannot consume a pen's release, and the synthetic release used when
  starting a pan retains its originating device.
- An eraser release applies any pending measured motion before committing, even
  if motion and release arrived in the same event batch before a render update.
- SDL's Windows backend still represents native pens as one logical pen. This
  patch does not implement independent multi-pen tracking.

## Build and checks

Re-export the SDL recipe and rebuild that dependency: recompiling the app against
an old SDL binary does not apply this patch. The creator's build scripts already
export the recipe.

The GitHub workflow checksum-verifies the SDL source archive, applies the actual
Conan source patch and compiles SDL for Windows x64 and ARM64. Native x64 tests
exercise exact duplicate detection, equal-time attribute/state changes, ring
wraparound and property cleanup. ARM64 is compile-only on the x64 runner.

This is not a complete application build or an end-to-end device test. Runtime
acceptance should cover contact/down/up ordering, stationary pressure, cancellation,
pan while drawing, touch/mouse coexistence, display scaling and non-zero origins.
History retrieval failure/race handling also warrants targeted device testing.

Pressure-preserving stroke generation and optional stabilization are separate
dependent changes; no claim of hardware wobble elimination is made here.
