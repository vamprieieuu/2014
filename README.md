# 2014 — Native After Effects Camera & Nostalgia Effect

A native After Effects effect focused on reproducing the visual character of early iPhone photography and 2014–2015 nostalgic social-photo looks.

## Core systems

1. Camera Model — iPhone 5 through iPhone 8/8 Plus.
2. 2014 Filters — original city-name presets with full editable controls.
3. iPhone-like Grain/Noise — procedural, image-dependent and animated over time.

## Reference data

- `reference/2014_looks/` — supplied 2014 look references, including the `Nostalgic Nights` settings image.
- `reference/camera_samples/` — supplied iOS 5/6/7 camera references and the filtered subset.
- `reference/video/` — optional original videos and extracted calibration frames.

## Important

The repository contains research and implementation specifications, not Apple's SDK. Put the matching Adobe After Effects SDK under `extern/AfterEffectsSDK/` when building.

The camera models are output-matching approximations. Apple's private ISP implementation is not public.

The final plugin should be an independent native `.aex`, not a JSX wrapper around native AE effects.
