#include "camera_profiles.h"
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

CameraProfile getCameraPreset(CameraModelId modelId) noexcept {
    CameraProfile p{};
    p.id = modelId;
    p.strength = 0.85f;

    switch (modelId) {
        case CameraModelId::IPHONE_5:
            p.warmth = 0.28f;
            p.temperature = 0.28f;
            p.saturation = 0.14f;
            p.lowLightNR = 0.35f;
            p.detail = 0.55f;
            p.sharpening = 0.65f;
            p.highlightRollOff = 0.30f;
            p.shadowLift = 0.15f;
            p.lensSoftness = 0.25f;
            p.vignette = 0.28f;
            p.distortion = 0.12f;
            p.chromaticAberration = 0.18f;
            p.chromaNoise = 0.62f;
            p.hdrCharacter = 0.70f;
            break;

        case CameraModelId::IPHONE_5S:
            p.warmth = 0.22f;
            p.temperature = 0.22f;
            p.saturation = 0.12f;
            p.lowLightNR = 0.52f;
            p.detail = 0.62f;
            p.sharpening = 0.58f;
            p.highlightRollOff = 0.42f;
            p.shadowLift = 0.18f;
            p.lensSoftness = 0.18f;
            p.vignette = 0.22f;
            p.distortion = 0.09f;
            p.chromaticAberration = 0.14f;
            p.chromaNoise = 0.48f;
            p.hdrCharacter = 0.62f;
            break;

        case CameraModelId::IPHONE_6:
            p.warmth = 0.20f;
            p.temperature = 0.20f;
            p.saturation = 0.10f;
            p.lowLightNR = 0.58f;
            p.detail = 0.68f;
            p.sharpening = 0.62f;
            p.highlightRollOff = 0.48f;
            p.shadowLift = 0.20f;
            p.lensSoftness = 0.14f;
            p.vignette = 0.19f;
            p.distortion = 0.08f;
            p.chromaticAberration = 0.12f;
            p.chromaNoise = 0.42f;
            p.hdrCharacter = 0.68f;
            break;

        case CameraModelId::IPHONE_6S:
            p.warmth = 0.15f;
            p.temperature = 0.15f;
            p.saturation = 0.08f;
            p.lowLightNR = 0.65f;
            p.detail = 0.72f;
            p.sharpening = 0.66f;
            p.highlightRollOff = 0.52f;
            p.shadowLift = 0.16f;
            p.lensSoftness = 0.12f;
            p.vignette = 0.16f;
            p.distortion = 0.06f;
            p.chromaticAberration = 0.10f;
            p.chromaNoise = 0.28f;
            p.hdrCharacter = 0.58f;
            break;

        case CameraModelId::IPHONE_7:
            p.strength = 0.80f;
            p.warmth = 0.14f;
            p.temperature = 0.14f;
            p.saturation = 0.12f;
            p.lowLightNR = 0.68f;
            p.detail = 0.76f;
            p.sharpening = 0.54f;
            p.highlightRollOff = 0.60f;
            p.shadowLift = 0.22f;
            p.lensSoftness = 0.09f;
            p.vignette = 0.13f;
            p.distortion = 0.05f;
            p.chromaticAberration = 0.08f;
            p.chromaNoise = 0.22f;
            p.hdrCharacter = 0.64f;
            break;

        case CameraModelId::IPHONE_7_PLUS:
            p.strength = 0.80f;
            p.warmth = 0.14f;
            p.temperature = 0.14f;
            p.saturation = 0.13f;
            p.lowLightNR = 0.74f;
            p.detail = 0.78f;
            p.sharpening = 0.52f;
            p.highlightRollOff = 0.64f;
            p.shadowLift = 0.24f;
            p.lensSoftness = 0.08f;
            p.vignette = 0.12f;
            p.distortion = 0.04f;
            p.chromaticAberration = 0.07f;
            p.chromaNoise = 0.19f;
            p.hdrCharacter = 0.70f;
            break;

        case CameraModelId::IPHONE_8:
            p.strength = 0.75f;
            p.warmth = 0.18f;
            p.temperature = 0.18f;
            p.saturation = 0.15f;
            p.lowLightNR = 0.78f;
            p.detail = 0.84f;
            p.sharpening = 0.46f;
            p.highlightRollOff = 0.74f;
            p.shadowLift = 0.26f;
            p.lensSoftness = 0.06f;
            p.vignette = 0.10f;
            p.distortion = 0.03f;
            p.chromaticAberration = 0.05f;
            p.chromaNoise = 0.14f;
            p.hdrCharacter = 0.76f;
            break;

        case CameraModelId::IPHONE_8_PLUS:
        default:
            p.strength = 0.75f;
            p.warmth = 0.19f;
            p.temperature = 0.19f;
            p.saturation = 0.16f;
            p.lowLightNR = 0.82f;
            p.detail = 0.86f;
            p.sharpening = 0.44f;
            p.highlightRollOff = 0.78f;
            p.shadowLift = 0.28f;
            p.lensSoftness = 0.05f;
            p.vignette = 0.09f;
            p.distortion = 0.03f;
            p.chromaticAberration = 0.04f;
            p.chromaNoise = 0.12f;
            p.hdrCharacter = 0.80f;
            break;
    }
    return p;
}

void applyCameraCharacter(
    float& r,
    float& g,
    float& b,
    float highFreqLuma,
    float radialSq,
    const CameraProfile& p,
    float strength) noexcept
{
    strength = clamp01(strength);
    if (strength <= 0.0001f) {
        r = clamp01(r);
        g = clamp01(g);
        b = clamp01(b);
        return;
    }

    r = clamp01(r);
    g = clamp01(g);
    b = clamp01(b);
    highFreqLuma = sanitizeFloat(highFreqLuma, 0.0f);
    radialSq = std::max(0.0f, sanitizeFloat(radialSq, 0.0f));

    // 1. Camera exposure & basic tone adjustments
    const float expFactor = std::pow(2.0f, std::max(-4.0f, std::min(4.0f, sanitizeFloat(p.exposure) * strength)));
    r *= expFactor;
    g *= expFactor;
    b *= expFactor;

    const float br = (sanitizeFloat(p.brightness) / 100.0f) * 0.30f * strength;
    if (std::fabs(br) > 0.0001f) {
        r += br * (1.0f - clamp01(r) * 0.5f);
        g += br * (1.0f - clamp01(g) * 0.5f);
        b += br * (1.0f - clamp01(b) * 0.5f);
    }

    float y = clamp01(0.2126f * r + 0.7152f * g + 0.0722f * b);

    // 2. HDR local tone mapping + shadow/highlight response
    const float shMask = (1.0f - y) * (1.0f - y);
    const float hiMask = y * y;
    const float hdrLift = (sanitizeFloat(p.shadowLift) * 0.18f + (sanitizeFloat(p.shadows) / 100.0f) * 0.25f) * strength * shMask;
    const float hdrPull = (sanitizeFloat(p.hdrCharacter) * 0.14f - (sanitizeFloat(p.highlights) / 100.0f) * 0.25f) * strength * hiMask;
    const float whiteBlackDelta =
        ((sanitizeFloat(p.whites) / 100.0f) * 0.20f * hiMask * y -
         (sanitizeFloat(p.blacks) / 100.0f) * 0.20f * shMask * (1.0f - y)) * strength;

    r += hdrLift - hdrPull + whiteBlackDelta;
    g += hdrLift - hdrPull + whiteBlackDelta;
    b += hdrLift - hdrPull + whiteBlackDelta;

    // 3. Contrast pivot
    const float cAmt = (sanitizeFloat(p.contrast) / 100.0f) * 0.50f * strength;
    if (std::fabs(cAmt) > 0.0001f) {
        r = 0.46f + (r - 0.46f) * (1.0f + cAmt);
        g = 0.46f + (g - 0.46f) * (1.0f + cAmt);
        b = 0.46f + (b - 0.46f) * (1.0f + cAmt);
    }

    // 4. Sensor color response & warmth/tint
    y = clamp01(0.2126f * r + 0.7152f * g + 0.0722f * b);
    const float sat = std::max(0.0f, 1.0f + sanitizeFloat(p.saturation) * strength);
    r = y + (r - y) * sat;
    g = y + (g - y) * sat;
    b = y + (b - y) * sat;

    const float combinedWarmth = (sanitizeFloat(p.warmth) + sanitizeFloat(p.temperature) * 0.5f) * strength;
    const float tintShift = sanitizeFloat(p.tint) * 0.05f * strength;
    r += combinedWarmth * 0.045f - tintShift * 0.35f;
    g += tintShift * 0.65f;
    b -= combinedWarmth * 0.032f + tintShift * 0.45f;

    // 5. ISP low-light noise reduction vs edge sharpening/ringing & corner lens softness
    const float lowLightWeight = clamp01(1.0f - y * 1.65f);
    const float nrSmooth = clamp01(p.lowLightNR) * lowLightWeight * strength * 0.55f;
    const float netSharpness =
        (sanitizeFloat(p.sharpening) * (1.0f - nrSmooth * 0.6f) -
         sanitizeFloat(p.lensSoftness) * radialSq * 0.85f) * strength * 0.90f;

    const float detailDelta = highFreqLuma * netSharpness - highFreqLuma * nrSmooth;
    r += detailDelta;
    g += detailDelta;
    b += detailDelta;

    // 6. Highlight roll-off & optical vignette
    const float rollOff = clamp01(p.highlightRollOff) * strength;
    if (rollOff > 0.001f) {
        if (r > 0.68f) r = 0.68f + (r - 0.68f) / (1.0f + (r - 0.68f) * rollOff * 2.0f);
        if (g > 0.68f) g = 0.68f + (g - 0.68f) / (1.0f + (g - 0.68f) * rollOff * 2.0f);
        if (b > 0.68f) b = 0.68f + (b - 0.68f) / (1.0f + (b - 0.68f) * rollOff * 2.0f);
    }

    const float vig = clamp01(p.vignette) * strength * 0.45f;
    if (vig > 0.001f) {
        const float vigGain = std::max(0.0f, 1.0f - radialSq * vig * 1.35f);
        r *= vigGain;
        g *= vigGain;
        b *= vigGain;
    }

    r = clamp01(r);
    g = clamp01(g);
    b = clamp01(b);
}

} // namespace y2014
