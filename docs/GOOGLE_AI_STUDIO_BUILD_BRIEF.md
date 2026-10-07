# Google AI Studio Build Brief — FINAL

Build a real native Adobe After Effects effect named `2014`.

## Hard rules

- Do not replace it with JSX.
- Do not implement the result by stacking AE built-in effects.
- Do not use reference images as textures.
- Do not use FilmConvert or Dehancer binaries, source, LUTs or proprietary profiles.
- Do not claim access to Apple's private ISP algorithms.
- Implement the image processing independently.
- Keep Camera Model, Filters and Grain as separate engines.
- Preserve all user controls in the UI.

## Architecture

Input
→ camera model
→ camera tone/color
→ sensor/process texture
→ lens/detail
→ 2014 filter
→ adaptive animated iPhone-like grain/noise
→ output

## Camera

Dropdown with iPhone 5 through iPhone 8/8 Plus. Profile strength and advanced editable controls are mandatory.

## Filters

Use original city-name presets from `spec/filter_profiles.json`. They are inspired by the 2014–2015 social-photo era and supplied references, not copies of Instagram filters.

## Grain

This is the highest-priority subsystem.

It must be a camera-aware, image-dependent, temporally animated procedural noise model. It should include signal-dependent shot noise, read noise, correlated chroma noise, low-frequency pattern variation, fine and coarse components, tonal response, camera-specific denoising/sharpening interaction, and temporal coherence.

Main controls:
- Amount
- Size
- Density
- Scale/Zoom
- Softness
- Speed
- Animation
- Color/Chroma
- Seed

Advanced:
- Shadows
- Midtones
- Highlights
- Fine Grain
- Coarse Grain
- Luma/Chroma balance
- Clustering
- Spatial correlation
- Temporal coherence
- Pattern amount/scale
- Shot/read noise balance
- Denoise interaction
- Sharpening interaction

The default must look like early iPhone low-light texture, not generic film grain and not TV static.

## Video references

If a video is supplied, preserve it under `reference/video/original/` and also extract calibration frames into `reference/video/frames/` with timestamps. The extracted frames and metadata are the primary coding-agent calibration artifacts.

## Build

Target Windows x64 / After Effects 23.x. Use the Adobe SDK supplied by the user under `extern/AfterEffectsSDK/`. Generate a genuine `.aex` with proper registration/PiPL and pixel processing.

If the SDK is absent, do not pretend an AEX was built.
