# Product

<!-- impeccable:product-schema 1 -->

## Platform

desktop

Native JUCE audio plugin (AU/VST3/Standalone, macOS 11+, universal binary). Sibling of the MXA suite; family design authority: `../PhaserMXA/DESIGN.md`; family product context: `../PhaserMXA/PRODUCT.md`.

## Product Purpose

A splice-head pitch shifter (tape-splice / H910 method): two read heads sweep a circular buffer at the transposition rate, half a window apart, sin/cos crossfaded; feedback reshifts each repeat (the spiral).

## Capabilities and Constraints

- Exactly six parameters: `pitch` (±12 st), `fine` (±100 ct), `grain` (splice window 20–120 ms), `snap` (Free/Semitone/Octave quantize choice), `fb` (0–70 %, reshifting), `mix`.
- Ratio and grain slewed ~30 ms against clicks; both channels share one head pair (coherent stereo); splice artifacts on transients are inherent to the method (documented character, not a defect).
- UI truth taps: atomic live ratio (`uiRatio`) and head phase (`uiPhase`); static `quantizedSemis()` / `ratioFor()` / `semisFor()` / `fadeGainFor()` — the single source of truth for FIG. 1's sawtooth head trajectories (slope = shift, reconstructed at the live rate) and FIG. 2's rotating read heads and printed ratio.
- Editor: Service Manual family sheet, 820×470, spot ink emerald-green #44C493, DWG NO. MXA-PT-01.

## Brand Commitments

Inherits the family's: MXAudio, "BY MESCALINA" credit, one spot ink per sibling (Pitch = emerald-green).

## Evidence on Hand

Working DSP (`Source/PitchEngine.*`). No users/testimonials — nothing may be fabricated.
