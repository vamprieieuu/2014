#include "pipeline_core.h"
#include <algorithm>
#include <cmath>

namespace y2014 {

static inline float sanitizeFloat(float v, float fallback = 0.0f) noexcept {
    return std::isfinite(v) ? v : fallback;
}

static inline float clamp01(float v) noexcept {
    v = sanitizeFloat(v, 0.0f);
    return std::max(0.0f, std::min(1.0f, v));
}

void processPixelFloat(
    float inR,
    float inG,
    float inB,
    float inA,
    float neighborAvgR,
    float neighborAvgG,
    float neighborAvgB,
    int32_t x,
    int32_t y,
    const RenderContext& ctx,
    float& outR,
    float& outG,
    float& outB,
    float& outA) noexcept
{
    outA = clamp01(inA);
    float r = clamp01(inR);
    float g = clamp01(inG);
    float b = clamp01(inB);

    const float avgR = clamp01(neighborAvgR);
    const float avgG = clamp01(neighborAvgG);
    const float avgB = clamp01(neighborAvgB);

    const float luma = 0.2126f * r + 0.7152f * g + 0.0722f * b;
    const float avgLuma = 0.2126f * avgR + 0.7152f * avgG + 0.0722f * avgB;
    const float highFreqLuma = sanitizeFloat(luma - avgLuma, 0.0f);

    const float cx = static_cast<float>(std::max<int32_t>(1, ctx.width)) * 0.5f;
    const float cy = static_cast<float>(std::max<int32_t>(1, ctx.height)) * 0.5f;
    const float invMaxR = 1.0f / std::max(1.0f, std::sqrt(cx * cx + cy * cy));
    const float dx = (static_cast<float>(x) - cx) * invMaxR;
    const float dy = (static_cast<float>(y) - cy) * invMaxR;
    const float radialSq = dx * dx + dy * dy;

    // Stage 1 & 2: Camera Model ISP + Tone + NR/Sharpening + Optics
    applyCameraCharacter(r, g, b, highFreqLuma, radialSq, ctx.camera, ctx.camera.strength);

    // Stage 3: 2014 Editable Filter Engine
    applyFilter2014(r, g, b, highFreqLuma, radialSq, ctx.filter);

    // Stage 4: Adaptive Animated iPhone-Like Sensor Grain/Noise Engine
    applySensorGrainPixel(
        r,
        g,
        b,
        x,
        y,
        ctx.width,
        ctx.height,
        highFreqLuma,
        ctx.timeSeconds,
        ctx.grain,
        ctx.camera);

    outR = clamp01(r);
    outG = clamp01(g);
    outB = clamp01(b);
}

void renderBuffer8(
    const uint8_t* srcBase,
    int32_t srcRowBytes,
    uint8_t* dstBase,
    int32_t dstRowBytes,
    int32_t width,
    int32_t height,
    const RenderContext& ctx) noexcept
{
    if (!srcBase || !dstBase || width <= 0 || height <= 0 || srcRowBytes <= 0 || dstRowBytes <= 0) {
        return;
    }

    RenderContext localCtx = ctx;
    localCtx.width = width;
    localCtx.height = height;

    const float inv255 = 1.0f / 255.0f;
    const float cx = static_cast<float>(width) * 0.5f;
    const float cy = static_cast<float>(height) * 0.5f;
    const float invMaxR = 1.0f / std::max(1.0f, std::sqrt(cx * cx + cy * cy));
    const float caStrength = clamp01(ctx.camera.chromaticAberration) * clamp01(ctx.camera.strength);

    for (int32_t y = 0; y < height; ++y) {
        const auto* srcRow = reinterpret_cast<const Pixel8*>(srcBase + static_cast<std::ptrdiff_t>(y) * srcRowBytes);
        const auto* srcRowUp = reinterpret_cast<const Pixel8*>(srcBase + static_cast<std::ptrdiff_t>(std::max<int32_t>(0, y - 1)) * srcRowBytes);
        const auto* srcRowDn = reinterpret_cast<const Pixel8*>(srcBase + static_cast<std::ptrdiff_t>(std::min<int32_t>(height - 1, y + 1)) * srcRowBytes);
        auto* dstRow = reinterpret_cast<Pixel8*>(dstBase + static_cast<std::ptrdiff_t>(y) * dstRowBytes);

        const float dy = (static_cast<float>(y) - cy) * invMaxR;

        for (int32_t x = 0; x < width; ++x) {
            const int32_t xL = std::max<int32_t>(0, x - 1);
            const int32_t xR = std::min<int32_t>(width - 1, x + 1);

            const Pixel8 center = srcRow[x];
            float inR = static_cast<float>(center.red) * inv255;
            const float inG = static_cast<float>(center.green) * inv255;
            float inB = static_cast<float>(center.blue) * inv255;
            const float inA = static_cast<float>(center.alpha) * inv255;

            if (caStrength > 0.005f) {
                const float dx = (static_cast<float>(x) - cx) * invMaxR;
                const float rSq = dx * dx + dy * dy;
                const int32_t shift = static_cast<int32_t>(std::round(rSq * caStrength * 4.0f));
                if (shift > 0) {
                    const int32_t rx = std::min<int32_t>(width - 1, std::max<int32_t>(0, x + (dx >= 0.0f ? shift : -shift)));
                    const int32_t bx = std::min<int32_t>(width - 1, std::max<int32_t>(0, x - (dx >= 0.0f ? shift : -shift)));
                    inR = static_cast<float>(srcRow[rx].red) * inv255;
                    inB = static_cast<float>(srcRow[bx].blue) * inv255;
                }
            }

            const float avgR = (static_cast<float>(srcRow[xL].red) + static_cast<float>(srcRow[xR].red) +
                                static_cast<float>(srcRowUp[x].red) + static_cast<float>(srcRowDn[x].red)) * (0.25f * inv255);
            const float avgG = (static_cast<float>(srcRow[xL].green) + static_cast<float>(srcRow[xR].green) +
                                static_cast<float>(srcRowUp[x].green) + static_cast<float>(srcRowDn[x].green)) * (0.25f * inv255);
            const float avgB = (static_cast<float>(srcRow[xL].blue) + static_cast<float>(srcRow[xR].blue) +
                                static_cast<float>(srcRowUp[x].blue) + static_cast<float>(srcRowDn[x].blue)) * (0.25f * inv255);

            float outR = 0.0f, outG = 0.0f, outB = 0.0f, outA = 1.0f;
            processPixelFloat(inR, inG, inB, inA, avgR, avgG, avgB, x, y, localCtx, outR, outG, outB, outA);

            dstRow[x].alpha = center.alpha;
            dstRow[x].red   = static_cast<uint8_t>(std::round(clamp01(outR) * 255.0f));
            dstRow[x].green = static_cast<uint8_t>(std::round(clamp01(outG) * 255.0f));
            dstRow[x].blue  = static_cast<uint8_t>(std::round(clamp01(outB) * 255.0f));
        }
    }
}

void renderBuffer16(
    const uint8_t* srcBase,
    int32_t srcRowBytes,
    uint8_t* dstBase,
    int32_t dstRowBytes,
    int32_t width,
    int32_t height,
    const RenderContext& ctx) noexcept
{
    if (!srcBase || !dstBase || width <= 0 || height <= 0 || srcRowBytes <= 0 || dstRowBytes <= 0) {
        return;
    }

    RenderContext localCtx = ctx;
    localCtx.width = width;
    localCtx.height = height;

    // After Effects 16-bit channel range is 0..32768 (PF_MAX_CHAN16)
    constexpr float kMax16 = 32768.0f;
    constexpr float kInv16 = 1.0f / kMax16;

    for (int32_t y = 0; y < height; ++y) {
        const auto* srcRow = reinterpret_cast<const Pixel16*>(srcBase + static_cast<std::ptrdiff_t>(y) * srcRowBytes);
        const auto* srcRowUp = reinterpret_cast<const Pixel16*>(srcBase + static_cast<std::ptrdiff_t>(std::max<int32_t>(0, y - 1)) * srcRowBytes);
        const auto* srcRowDn = reinterpret_cast<const Pixel16*>(srcBase + static_cast<std::ptrdiff_t>(std::min<int32_t>(height - 1, y + 1)) * srcRowBytes);
        auto* dstRow = reinterpret_cast<Pixel16*>(dstBase + static_cast<std::ptrdiff_t>(y) * dstRowBytes);

        for (int32_t x = 0; x < width; ++x) {
            const int32_t xL = std::max<int32_t>(0, x - 1);
            const int32_t xR = std::min<int32_t>(width - 1, x + 1);

            const Pixel16 center = srcRow[x];
            const float inR = static_cast<float>(center.red) * kInv16;
            const float inG = static_cast<float>(center.green) * kInv16;
            const float inB = static_cast<float>(center.blue) * kInv16;
            const float inA = static_cast<float>(center.alpha) * kInv16;

            const float avgR = (static_cast<float>(srcRow[xL].red) + static_cast<float>(srcRow[xR].red) +
                                static_cast<float>(srcRowUp[x].red) + static_cast<float>(srcRowDn[x].red)) * (0.25f * kInv16);
            const float avgG = (static_cast<float>(srcRow[xL].green) + static_cast<float>(srcRow[xR].green) +
                                static_cast<float>(srcRowUp[x].green) + static_cast<float>(srcRowDn[x].green)) * (0.25f * kInv16);
            const float avgB = (static_cast<float>(srcRow[xL].blue) + static_cast<float>(srcRow[xR].blue) +
                                static_cast<float>(srcRowUp[x].blue) + static_cast<float>(srcRowDn[x].blue)) * (0.25f * kInv16);

            float outR = 0.0f, outG = 0.0f, outB = 0.0f, outA = 1.0f;
            processPixelFloat(inR, inG, inB, inA, avgR, avgG, avgB, x, y, localCtx, outR, outG, outB, outA);

            dstRow[x].alpha = center.alpha;
            dstRow[x].red   = static_cast<uint16_t>(std::min<int32_t>(32768, std::max<int32_t>(0, static_cast<int32_t>(std::round(clamp01(outR) * kMax16)))));
            dstRow[x].green = static_cast<uint16_t>(std::min<int32_t>(32768, std::max<int32_t>(0, static_cast<int32_t>(std::round(clamp01(outG) * kMax16)))));
            dstRow[x].blue  = static_cast<uint16_t>(std::min<int32_t>(32768, std::max<int32_t>(0, static_cast<int32_t>(std::round(clamp01(outB) * kMax16)))));
        }
    }
}

void renderBufferFloat(
    const uint8_t* srcBase,
    int32_t srcRowBytes,
    uint8_t* dstBase,
    int32_t dstRowBytes,
    int32_t width,
    int32_t height,
    const RenderContext& ctx) noexcept
{
    if (!srcBase || !dstBase || width <= 0 || height <= 0 || srcRowBytes <= 0 || dstRowBytes <= 0) {
        return;
    }

    RenderContext localCtx = ctx;
    localCtx.width = width;
    localCtx.height = height;

    for (int32_t y = 0; y < height; ++y) {
        const auto* srcRow = reinterpret_cast<const PixelFloat*>(srcBase + static_cast<std::ptrdiff_t>(y) * srcRowBytes);
        const auto* srcRowUp = reinterpret_cast<const PixelFloat*>(srcBase + static_cast<std::ptrdiff_t>(std::max<int32_t>(0, y - 1)) * srcRowBytes);
        const auto* srcRowDn = reinterpret_cast<const PixelFloat*>(srcBase + static_cast<std::ptrdiff_t>(std::min<int32_t>(height - 1, y + 1)) * srcRowBytes);
        auto* dstRow = reinterpret_cast<PixelFloat*>(dstBase + static_cast<std::ptrdiff_t>(y) * dstRowBytes);

        for (int32_t x = 0; x < width; ++x) {
            const int32_t xL = std::max<int32_t>(0, x - 1);
            const int32_t xR = std::min<int32_t>(width - 1, x + 1);

            const PixelFloat center = srcRow[x];
            const float avgR = (sanitizeFloat(srcRow[xL].red) + sanitizeFloat(srcRow[xR].red) +
                                sanitizeFloat(srcRowUp[x].red) + sanitizeFloat(srcRowDn[x].red)) * 0.25f;
            const float avgG = (sanitizeFloat(srcRow[xL].green) + sanitizeFloat(srcRow[xR].green) +
                                sanitizeFloat(srcRowUp[x].green) + sanitizeFloat(srcRowDn[x].green)) * 0.25f;
            const float avgB = (sanitizeFloat(srcRow[xL].blue) + sanitizeFloat(srcRow[xR].blue) +
                                sanitizeFloat(srcRowUp[x].blue) + sanitizeFloat(srcRowDn[x].blue)) * 0.25f;

            float outR = 0.0f, outG = 0.0f, outB = 0.0f, outA = 1.0f;
            processPixelFloat(
                center.red,
                center.green,
                center.blue,
                center.alpha,
                avgR,
                avgG,
                avgB,
                x,
                y,
                localCtx,
                outR,
                outG,
                outB,
                outA);

            dstRow[x].alpha = sanitizeFloat(center.alpha, 1.0f);
            dstRow[x].red   = outR;
            dstRow[x].green = outG;
            dstRow[x].blue  = outB;
        }
    }
}

} // namespace y2014
