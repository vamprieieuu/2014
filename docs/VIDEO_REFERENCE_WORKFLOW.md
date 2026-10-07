# Video Reference Workflow

## Can a video be used?

Yes. Gemini can process uploaded video files. Current Gemini documentation supports common formats including MP4, MOV, AVI, WebM and others. The File API is intended for large/reusable video files; documented limits are up to 2 GB on the free tier and up to 20 GB on paid usage. Static processing samples video at 1 FPS by default, while supported recent Gemini models also have an agentic mode that can navigate the timeline dynamically.

## Important limitation for this repository

A raw video committed to Git is **not the best calibration artifact for a coding agent**. The agent can store the file, but it may not automatically inspect every frame or use it as a quantitative dataset during implementation.

Therefore, if a video is available, keep BOTH:

1. the original video (optional archive/reference)
2. an extracted calibration dataset:
   - frames every 0.25–1.0 seconds for normal motion
   - short frame bursts around low-light regions
   - 100% crops of flat walls/sky/shadows when possible
   - metadata: frame number, timestamp, resolution, fps

The frames are what should be committed under `reference/video/frames/` for the coding agent.

## Why this matters for grain

The temporal character is the key information a still image cannot contain. A short original iPhone clip can reveal:
- frame-to-frame correlation
- whether fine noise changes independently
- whether chroma noise moves differently from luma noise
- denoising persistence
- sharpening shimmer
- low-light temporal smoothing
- stabilization-induced texture changes

## Recommended clip

A 5–20 second original camera clip is enough for calibration if it contains:
- a dark interior or night scene
- a flat wall/sky area
- a face or skin region
- a highlight/light source
- some motion

Do not re-export it through Instagram/TikTok/WhatsApp before calibration; recompression changes the texture.
