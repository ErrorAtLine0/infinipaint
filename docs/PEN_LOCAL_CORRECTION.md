# Optional local pen correction

Depends on the input and direct-pressure-path changes documented in
WINDOWS_PEN_INPUT.md and PEN_PRESSURE.md. Correction is opt-in in this upstream
proposal; it is not an update to saved drawings or the eraser.

## Algorithm and settings

Streaming port of PenTraceLab 0.4.0 localFilter, commit
ef6555a6defd12b8dde5afc408df4975eb4492b2 (MIT notice included in app licenses).

The filter integrates symmetric arc-length neighborhoods, applies only the
local-normal component of displacement, tapers at corners/endpoints and clamps
the maximum correction. Suggested settings are 12 DIP radius, 120 ms live-tail
revision window and 4 DIP cap, independent of brush width and canvas zoom.
Settings are captured at contact-down; configuration has a versioned key.

The current tip stays at the last in-contact report. A recent tail can revise
as future measured samples arrive; older points freeze. There is no prediction,
global line snapping, or post-lift catch-up. Correction requires the Brush panel's
**Preserve per-point pen pressure** option, which defaults off. With preservation
off, upstream's original smoothing is used even if a saved filter setting is on;
the settings UI explains that correction is inactive. With preservation on,
turning correction off retains the direct unfiltered pen path. Both choices are
captured at stroke start; the filter never silently opts a brush into preservation.

This is not recovery of ground-truth pen movement. Larger windows/radii can
soften intended detail. Slow diagonals are not guaranteed to become straight.
No universal hardware-wobble elimination or zero-latency claim is made.

## Scope and evidence

Source positions/timestamps/widths remain immutable while generating a stroke.
The existing file/network format stores derived mesh geometry, not recoverable
raw reports. Pressure is not re-filtered by the positional algorithm.
Transform changes end a stroke; timestamp discontinuities split filter runs.
The eraser is excluded because revising an already-applied erase path is unsafe.

A separate diagnostic comparison on a user's device motivated the defaults,
and the complete fork received positive user feedback. This is not controlled
independent physical-ground-truth validation; private recordings are not included.

CI checks every live prefix against the pinned original diagnostic implementation
at 60/120/240/672 Hz for noisy paths, circles, corners, stationary points, equal
timestamps and gaps. Other invariants cover endpoint preservation, frozen prefixes,
rotation equivariance, Off mode, displacement caps and unchanged sample widths.
Linux tests run with ASan/UBSan. Windows input compilation/testing is inherited
from the first PR. These are not full application builds or GUI/device tests.

## Before considering a default change

Review handwriting detail, slow diagonals, corners, pressure, transparency,
cancellation, DPI/zoom, long-stroke performance, undo, save/reopen, exports and
collaboration. Mesh generation still rebuilds the whole live stroke per frame;
the bounded filter tail is not a complete rendering-performance redesign.

Please treat this draft as an optional feature proposal. Input correctness and
pressure preservation can be considered independently of accepting this filter.
