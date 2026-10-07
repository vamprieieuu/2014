# 2014 Grain Engine — Final Design

## Goal

The grain/noise in 2014 is **not film grain copied from FilmConvert/Dehancer**. It is a camera-character system intended to reproduce the visible texture of early iPhone JPEG output.

The model is inspired by publicly documented imaging science and by the supplied references, but is independently implemented.

## Why a simple grain overlay is wrong

A digital phone image can contain multiple interacting components:

1. photon/shot noise (signal dependent)
2. read/electronic noise
3. color-channel imbalance and chroma noise
4. sensor non-uniformity / PRNU-like low-frequency variation
5. demosaicing and color interpolation artifacts
6. sharpening/ringing around edges
7. noise reduction that suppresses texture differently by tone and spatial frequency
8. JPEG/compression/resizing artifacts in old social-media images
9. multi-frame/HDR processing in some capture modes

Digital-camera noise is commonly modeled as signal-dependent Poisson plus Gaussian components; modern camera pipelines then transform/denoise that signal. This is a better foundation for the 2014 texture than adding white noise.

## Required pipeline

Input pixels
→ working linear/light-normalized representation
→ camera color response
→ camera tone/exposure response
→ sensor noise formation
→ camera-specific denoise
→ demosaic-like chroma correlation / processing residue
→ sharpening/ringing character
→ optional JPEG-era texture
→ 2014 look
→ final adaptive grain/noise enhancement
→ output

The exact internal order may be optimized for quality/performance, but the implementation must preserve the separation between camera character and creative filter.

## Noise components

### A. Shot noise
Signal-dependent stochastic component. Strength increases with signal level in the sensor domain.

### B. Read noise
Approximately signal-independent base noise. Controls:
- readNoise
- colorReadNoise
- shadowFloor

### C. Chroma noise
Generate correlated chroma variation rather than independent RGB speckles. Use a YCbCr/OKLab-like working representation, then transform back.

Controls:
- chromaAmount
- chromaScale
- chromaCorrelation
- chromaHueBias

### D. Sensor pattern / PRNU-like component
A very-low-amplitude spatial field that stays mostly stable over time, combined with a moving stochastic component. It must never become a visible repeating texture.

Controls:
- patternAmount
- patternScale
- patternSeed
- temporalDrift

### E. Fine stochastic component
High-frequency component that changes continuously over time.

Do NOT regenerate an unrelated random image every frame. That causes ugly digital flicker.

Use a continuous 3D noise domain (x, y, time) plus a small frame-stochastic term.

### F. Coarse clustered component
The supplied low-light references show texture that can read as clusters rather than perfectly independent pixels. Use a second low-frequency noise field and threshold/shape it softly.

Controls:
- fineSize
- coarseSize
- clustering
- softness
- density

## Temporal model

The final grain must move.

Use two temporal layers:

### Coherent layer
3D noise / smoothly advected field. This gives organic movement without popping.

### Stochastic layer
A controlled per-frame component representing independent sensor noise.

The user controls:
- Animation Amount
- Animation Speed
- Temporal Coherence
- Frame Seed

At 0 coherence, the grain can move more independently.
At high coherence, the texture evolves smoothly.

The default should avoid the obvious "TV static" look.

## Tonal response

Grain amount is not constant over the frame.

Use a response curve with editable points:

Black → Shadow → Midtone → Highlight → White

and separate controls:
- Shadows
- Midtones
- Highlights

Pure black and pure white should not suddenly receive an opaque noise layer. The response should fade according to local available signal/detail.

## Size / scaling

The user requested independent control over:
- Size
- Density
- Scale / Zoom
- Softness
- Speed
- Amount
- Color

Important distinction:
- **Size** = physical-looking grain particle scale in image space.
- **Scale/Zoom** = sampling scale of the generated texture.
- **Density** = how frequently visible clusters occur.
- **Softness** = spatial filtering of the grain structure.
- **Amount** = amplitude/opacity-equivalent response.

The plugin must automatically account for comp resolution so the same profile does not become microscopic at 4K or enormous at 720p.

## Color modes

- Monochrome
- Neutral Luma
- Warm Chroma
- Cool Chroma
- Full Chroma
- Camera Profile

`Camera Profile` is the default and uses camera-specific color/noise character.

## Camera-specific defaults

The camera profile should set default values for:
- shot/read balance
- luma/chroma balance
- noise reduction strength
- sharpening
- chroma correlation
- pattern amount
- tonal response
- temporal coherence

The user can override all of them.

## Performance modes

### Accurate
Full multi-component model. Default final render mode.

### Preview
Reduced sample count / simplified noise evaluation.

### Draft
Fast procedural noise for interactive previews only.

Do not silently switch the final render to the Draft algorithm.

## Calibration rule

Reference images can estimate texture size, chroma behavior, clustering and tonal distribution. They cannot prove temporal behavior.

If an original video ever becomes available, use it as temporal calibration evidence. The repository includes a video-reference workflow below.
