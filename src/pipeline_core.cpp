#include "2014_engine.h"

namespace y2014 {

RGBAf evaluatePipelinePixel(
    int x,
    int y,
    int width,
    int height,
    const RGBAf& centerPx,
    const RGBAf& localBlurPx,
    float localDetail,
    const PipelineConfig& config
) {
    float normX = (width > 1) ? (static_cast<float>(x) / static_cast<float>(width - 1)) : 0.5f;
    float normY = (height > 1) ? (static_cast<float>(y) / static_cast<float>(height - 1)) : 0.5f;

    const CameraSpec& camSpec = getCameraSpec(config.camera.model);
    float localLumaMean = luminance709(localBlurPx.r, localBlurPx.g, localBlurPx.b);

    // Stage 1-4: Camera Model (camera color response -> camera tone/HDR -> sensor/NR/ISP sharpening -> lens detail)
    float camR = centerPx.r;
    float camG = centerPx.g;
    float camB = centerPx.b;
    evaluateCameraPixel(
        centerPx.r, centerPx.g, centerPx.b,
        normX, normY,
        camSpec,
        config.camera,
        localLumaMean,
        localDetail,
        camR, camG, camB
    );

    // Also compute camera-transformed local blur for the 2014 Filter's Definition & Softening stage
    float camBlurR = localBlurPx.r;
    float camBlurG = localBlurPx.g;
    float camBlurB = localBlurPx.b;
    evaluateCameraPixel(
        localBlurPx.r, localBlurPx.g, localBlurPx.b,
        normX, normY,
        camSpec,
        config.camera,
        localLumaMean,
        0.0f,
        camBlurR, camBlurG, camBlurB
    );

    // Stage 5: 2014 Filter Look (city preset / editable creative parameters)
    FilterParams activeFilter = config.usePresetDefaultsForFilter
        ? getFilterPresetParams(config.filter.preset)
        : config.filter;
    if (config.usePresetDefaultsForFilter) {
        activeFilter.filterStrength = config.filter.filterStrength;
    }

    float fltR = camR;
    float fltG = camG;
    float fltB = camB;
    evaluateFilterPixel(
        camR, camG, camB,
        normX, normY,
        camBlurR, camBlurG, camBlurB,
        activeFilter,
        fltR, fltG, fltB
    );

    // Stage 6: Adaptive Animated iPhone-like Sensor Grain / Noise Engine
    GrainParams activeGrain = config.grain;
    if (config.applyCameraGrainDefaults) {
        applyCameraDefaultsToGrain(config.camera.model, activeGrain);
    }

    GrainSampleResult grainSample = evaluateGrainAtPixel(
        x, y,
        width, height,
        config.frameIndex,
        config.frameTimeSeconds,
        fltR, fltG, fltB,
        localDetail,
        camSpec,
        activeGrain
    );

    // Soft-clipping shoulder so added sensor grain never causes harsh digital overflow/clipping artifacts
    auto softClamp = [](float baseVal, float delta) -> float {
        float v = baseVal + delta;
        if (v < 0.0f) {
            return 0.0f;
        }
        if (v > 1.0f) {
            return 1.0f - 0.15f / (1.0f + 6.0f * (v - 1.0f));
        }
        return clamp01(v);
    };

    RGBAf out;
    out.r = softClamp(fltR, grainSample.deltaR);
    out.g = softClamp(fltG, grainSample.deltaG);
    out.b = softClamp(fltB, grainSample.deltaB);
    out.a = centerPx.a;
    return out;
}

void renderFrameRGBAf(
    const RGBAf* srcBuffer,
    RGBAf* dstBuffer,
    int width,
    int height,
    int srcStridePixels,
    int dstStridePixels,
    const PipelineConfig& config
) {
    if (!srcBuffer || !dstBuffer || width <= 0 || height <= 0) {
        return;
    }
    if (srcStridePixels <= 0) srcStridePixels = width;
    if (dstStridePixels <= 0) dstStridePixels = width;

    auto sampleClamped = [&](int sx, int sy) -> const RGBAf& {
        int cx = std::max(0, std::min(width - 1, sx));
        int cy = std::max(0, std::min(height - 1, sy));
        return srcBuffer[static_cast<size_t>(cy) * static_cast<size_t>(srcStridePixels) + static_cast<size_t>(cx)];
    };

    for (int y = 0; y < height; ++y) {
        RGBAf* dstRow = dstBuffer + static_cast<size_t>(y) * static_cast<size_t>(dstStridePixels);
        for (int x = 0; x < width; ++x) {
            const RGBAf& c  = sampleClamped(x, y);
            const RGBAf& l1 = sampleClamped(x - 1, y);
            const RGBAf& r1 = sampleClamped(x + 1, y);
            const RGBAf& u1 = sampleClamped(x, y - 1);
            const RGBAf& d1 = sampleClamped(x, y + 1);
            const RGBAf& l2 = sampleClamped(x - 2, y);
            const RGBAf& r2 = sampleClamped(x + 2, y);
            const RGBAf& u2 = sampleClamped(x, y - 2);
            const RGBAf& d2 = sampleClamped(x, y + 2);

            RGBAf localBlur;
            localBlur.r = (c.r * 2.0f + (l1.r + r1.r + u1.r + d1.r) + 0.5f * (l2.r + r2.r + u2.r + d2.r)) * (1.0f / 8.0f);
            localBlur.g = (c.g * 2.0f + (l1.g + r1.g + u1.g + d1.g) + 0.5f * (l2.g + r2.g + u2.g + d2.g)) * (1.0f / 8.0f);
            localBlur.b = (c.b * 2.0f + (l1.b + r1.b + u1.b + d1.b) + 0.5f * (l2.b + r2.b + u2.b + d2.b)) * (1.0f / 8.0f);
            localBlur.a = c.a;

            float centerLuma = luminance709(c.r, c.g, c.b);
            float blurLuma = luminance709(localBlur.r, localBlur.g, localBlur.b);
            float localDetail = centerLuma - blurLuma;

            dstRow[x] = evaluatePipelinePixel(
                x, y,
                width, height,
                c,
                localBlur,
                localDetail,
                config
            );
        }
    }
}

} // namespace y2014
