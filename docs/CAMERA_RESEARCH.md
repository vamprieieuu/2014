# Camera Research Summary

## iPhone 5
Public testing reports HDR exposure blending, occasional ghosting, and strong tonal-range behavior in HDR scenes. The implementation should therefore keep HDR as a separate camera-processing characteristic, not a generic contrast curve.

## iPhone 5s
Independent testing reports light-handed noise reduction at low ISO, visible luminance noise in dark areas, and stronger detail loss when low-light noise reduction becomes active. This is a key reason the profile should model both noise and denoising instead of simply adding noise.

## iPhone 6
Testing reports luminance noise in low light and visible HDR artifacts including color quantization, ghosting and color fringing.

## iPhone 6s
Testing reports low chroma noise, visible luminance noise in low light, loss of fine detail as light falls, color shading, occasional HDR ghosting, and slight ringing.

## iPhone 7 / 7 Plus
Apple moved to a 12MP f/1.8 main camera with wide color and a more advanced ISP. Independent testing reports good dynamic range/color but visible low-light luminance noise and some fine-detail loss; the 7 Plus wide camera was also observed to use multi-frame methods in low light.

## iPhone 8 / 8 Plus
Apple documents a larger/faster 12MP sensor, new color filter, deeper pixels, OIS and a new ISP with advanced pixel processing and better HDR. The profile should therefore be cleaner and more refined than the 5/5s profiles while retaining generation-specific processing.

## Evidence tiers

- documented = manufacturer specification
- observed = independent test/sample evidence
- estimated = implementation approximation needed to reproduce output

Never describe estimated coefficients as Apple's internal values.
