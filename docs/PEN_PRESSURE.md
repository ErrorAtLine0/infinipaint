# Per-sample pressure for pen brush strokes

Depends on the Windows input change described in WINDOWS_PEN_INPUT.md.
No wobble filter, prediction, or ARM64 build changes are included.

## Behavior

Upstream already supports pressure. Its temporary-point width maxima,
backwards width smoothing and final tip correction can widen previous parts
of the same stroke. This proposal deliberately changes that behavior for pen
brush strokes; it is not a claim that pressure smoothing is always undesirable.

New pen strokes append each contact sample with its own pressure-derived width.
The same mapping is used at contact-down and motion. The pressure factor and
minimum size remain configurable; disabling pressure gives a constant width.
Pressure changes do not run backwards through existing pen points.

The direct pen path also bypasses brush-size-dependent midpoint spacing and
Catmull-Rom interpolation. Retaining a dense per-sample path is necessary here
to keep width associated with the corresponding sampled position. Coincident
vertices are compacted only for outline construction; overlapping marks can
still visually cover one another.

The eraser and mouse/touch paths retain upstream smoothing. UI text clarifies
that the old pressure smoothing control no longer controls pen brush strokes.
Changing camera transforms, window position or DPI ends the active pen stroke;
lift and start again rather than reinterpreting old screen samples.

Existing drawings are unchanged. The existing mesh format persists the resulting
variable-width geometry, not the original pen pressure reports.

## Validation and review questions

Automated tests cover the pure pressure mapping (minimum size, constant-width
mode, invalid inputs and a light-heavy-light sequence). They do not compile or
exercise the full brush/Skia renderer. The complete fork has received a positive
user drawing report; this isolated PR still needs end-to-end app testing.

Reproduce with a continuous light-heavy-light line: earlier light sections
should not expand when pressure increases later. Also test stationary pressure,
dots, sharp turns, overlapping/translucent strokes, pan/zoom interruption, undo,
save/reopen, exports and collaboration. Long dense strokes need performance checks.

Maintainer decision: this draft changes the pen brush's default stroke-generation
policy. Keeping a selectable legacy pressure-smoothing policy could be preferable
for compatibility; no claim is made that the new behavior suits every brush.
