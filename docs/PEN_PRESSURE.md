# Pen brush pressure response

This document describes the pressure-only PR #97. On the combined PR #98 branch,
see [PEN_LOCAL_CORRECTION.md](PEN_LOCAL_CORRECTION.md) for independent correction
and the Smoothed + On sample path.

This proposal depends on the native history/input changes in PR #96. It has no
positional filter, source-library dependency, theme, cursor or movable-panel changes.
The companion optional filter is proposed in PR #98.

## Choose in Brush (desktop and phone)

| Mode | Width behavior | Position path in this PR |
| --- | --- | --- |
| Smoothed pressure (default) | Existing upstream width propagation, default 0.707 | Original midpoint spacing and Catmull-Rom |
| Preserve samples | Keep each accepted contact sample's width | Dense measured polyline |
| Uniform peak width | Whole current stroke uses its highest recorded width | Dense measured polyline |

The former Preserve per-point pen pressure checkbox is replaced by explicit modes.
Pressure support already exists upstream; this changes the policy, not the input
capability. Upstream's original behavior is the default. A propagation value of 1
spreads peak width in the original engine; it is not a requirement for preservation.
Preserve and Peak deliberately bypass that propagation slider.

The mode is captured at contact-down. Changing it affects subsequent strokes only.
Pressure-affects-size and minimum width remain shared brush/eraser settings.
Mouse, touch and eraser generation are not opted into the new pen path.
Coincident vertices are collapsed for valid outline normals, retaining the widest
coincident width: invisible overlapping samples cannot each produce a separate pixel.

## Configuration and compatibility

Brush configuration saves pressureResponse as original, preserve or peak. Missing
and unknown modes select Original. The old preservePenPressure boolean migrates to
Preserve when true and Original otherwise; an explicit new mode takes precedence.
The compatibility boolean is still written for older fork builds (Peak cannot be
represented by that old setting). Drawings store derived meshes, not raw pressure.

No operating-system settings, registry entries, build-tool installs or ARM64 build
workarounds are introduced.

## Validation

Pure C++ tests cover configuration roundtrips, legacy migration, pressure mapping,
peak/preserve widths and reset. Source checks cover both panels and contact-down.
CI compiles patched SDL for Windows x64/ARM64 and runs x64 input tests.
These are not full application builds, Skia rendering tests or hardware validation.

On a build machine, check light-heavy-light strokes, stationary pressure, dots,
corners, transparency, long strokes, cancel/pan/zoom/DPI changes, erasing, undo,
save/reopen, export and collaboration. The combined fork received positive subjective
feedback on Surface Pro 11 with Metapen M2; this isolated PR still needs testing.
