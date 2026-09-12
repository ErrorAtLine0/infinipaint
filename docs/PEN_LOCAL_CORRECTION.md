# Optional shared-source pen correction

Depends on PR #96 (native input) and #97 (pressure response/sample path).
The original Smoothed pressure mode with correction Off remains the default.
Graphite styling, movable panels and cursor changes are NOT part of this PR.

## Shared library, not another copy

The implementation lives in [pen-stabilizer](https://github.com/alexiokay/pen-stabilizer),
MIT-licensed C++17 header-only source. Git submodule deps/pen-stabilizer pins
**adbdce4e902433fcd14fba16e08863f2ec909f79**, package **v0.1.0**, algorithm revision **1**.
src/PenStabilizer.hpp is only the InfiniPaint validation/type adapter.
CMake links an INTERFACE target: no DLL, service, executable or runtime download.

On the build machine, before configuring:
```sh
git submodule update --init deps/pen-stabilizer
```
Source ZIPs from GitHub omit submodule contents; use a recursive clone or supply
the exact pinned source with its LICENSE. Normal recursive setup also works.
A missing dependency produces an explicit CMake error. Build scripts/ARM64
configuration from the creator remain unchanged.

[PenTraceLab](https://github.com/alexiokay/pen-trace-lab) 0.4.1 pins the same revision
and calls filterBatch for its local-correction candidate. InfiniPaint uses append
for live contact reports. Other Lab comparison candidates are not this filter.
The CI oracle intentionally remains an independent older diagnostic implementation
at ef6555a6defd12b8dde5afc408df4975eb4492b2, not the shared code testing itself.
[Shared overview](https://github.com/alexiokay/pen-tools) explains components and versioning.

## Pressure and position are independent

Enable Local wobble correction in the Brush panel (desktop or phone). It works
with all three pressure modes; changing pressure mode does not toggle correction.
Settings and width policy are captured at contact-down.

| Pressure response | Correction Off | Correction On |
| --- | --- | --- |
| Smoothed (default) | Unchanged original generation | Corrected sample path with width propagation |
| Preserve samples | Raw dense sample path, individual widths | Corrected positions, individual widths |
| Uniform peak width | Raw dense sample path, whole-stroke peak | Corrected positions, whole-stroke peak |

Smoothed + On uses dense reports, not upstream midpoint spacing/Catmull-Rom.
The same propagation factor can therefore feel different because sample density
differs. This is not advertised as exact original interpolation with correction On.
Earlier widths may change in Smoothed/Peak while old corrected positions freeze.
Source positions, timestamps and widths are never rewritten.

A one-time config migration preserves effective behavior of older fork builds:
a saved enabled filter that was inactive under Original is switched Off, once.
Preserve/Peak retain their saved filter choice. A persisted correctionIndependent
marker prevents later mode changes or reloads from switching it off again.
Fresh configuration defaults Off.

## Algorithm and limits

Symmetric arc-length neighborhoods, local-normal projection, bounded displacement,
corner/endpoint tapering and a frozen prefix. Defaults: radius 12 DIP, recent-tail
revision window 0.120 seconds, correction cap 4 DIP. The newest contact tip is exact;
the recent trail may revise as future samples arrive. No prediction, global line
snapping or post-lift catch-up. Timestamp discontinuities split neighborhoods.
Eraser/mouse/touch are not filtered. Destructive erasing must not use a revisable trail.

This cannot recover unknown physical ground truth or guarantee eliminating slow
diagonal wobble. Larger windows may erase intended detail. Whole-stroke storage
and mesh rebuilding costs remain; revision duration does not bound sample count.
Existing drawing formats store derived meshes, not raw reports.

## Validation and update policy

CI: independent oracle at 60/120/240/672 Hz, endpoint/frozen-prefix/cap/rotation/Off
invariants, source-width immutability, all pressure modes, migration, reset,
Linux sanitizers, patched SDL x64/ARM64 compilation and native x64 input tests.
These are NOT full InfiniPaint builds or GUI/physical-pen acceptance tests.
Test slow diagonals, handwriting, corners, transparency, cancel, pan/zoom/DPI,
long strokes, selection/deletion, erasing, undo, save/reopen, export and collaboration.

To update: review a library release, explicitly change the pinned commit, run the
oracle/invariants, replay Lab recordings locally, then build/device-test consumers.
Each app has its own version. Algorithm changes need a documented behavior revision.
Do not automatically follow library main. Retain the pinned MIT license on vendoring.
