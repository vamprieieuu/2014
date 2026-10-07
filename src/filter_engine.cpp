#include "filter_engine.h"
#include <algorithm>
#include <cmath>

namespace y2014 {

static inline float sanitize(float v, float fallback = 0.0f) noexcept {
    return std::isfinite(v) ? v : fallback;
}

static inline float clamp01(float v) noexcept {
    v = sanitize(v, 0.0f);
    return std::max(0.0f, std::min(1.0f, v));
}

FilterParams getFilterPreset(FilterPresetId presetId) noexcept {
    FilterParams f{};
    f.presetId = presetId;
    f.strength = (presetId == FilterPresetId::NONE) ? 0.0f : 0.85f;

    switch (presetId) {
        case FilterPresetId::NONE:
            f.exposure = 0.0f; f.brilliance = 0.0f; f.brightness = 0.0f; f.contrast = 0.0f;
            f.temperature = 0.0f; f.tint = 0.0f; f.saturation = 0.0f; f.vibrance = 0.0f;
            f.highlights = 0.0f; f.shadows = 0.0f; f.whites = 0.0f; f.blacks = 0.0f;
            f.fade = 0.0f; f.highlightRolloff = 0.0f; f.microcontrast = 0.0f;
            f.softness = 0.0f; f.vignette = 0.0f;
            f.shadowColorR = 0.0f; f.shadowColorG = 0.0f; f.shadowColorB = 0.0f;
            f.midtoneColorR = 0.0f; f.midtoneColorG = 0.0f; f.midtoneColorB = 0.0f;
            f.highlightColorR = 0.0f; f.highlightColorG = 0.0f; f.highlightColorB = 0.0f;
            break;

        case FilterPresetId::NOSTALGIC_NIGHTS_REF:
            // Exact values from spec/filter_profiles.json & IMG_3448.JPG
            f.strength = 1.0f;
            f.exposure = -59.0f; f.brilliance = 9.0f; f.highlights = 100.0f; f.shadows = -17.0f;
            f.contrast = 18.0f; f.brightness = 100.0f; f.blacks = 35.0f; f.whites = 15.0f;
            f.saturation = 26.0f; f.vibrance = -57.0f; f.temperature = -100.0f; f.tint = -30.0f;
            f.microcontrast = 100.0f; f.softness = 8.0f; f.vignette = 30.0f;
            f.fade = 14.0f; f.highlightRolloff = 20.0f;
            f.shadowColorR = 0.06f; f.shadowColorG = -0.04f; f.shadowColorB = 0.18f;
            f.midtoneColorR = -0.03f; f.midtoneColorG = -0.01f; f.midtoneColorB = 0.08f;
            f.highlightColorR = 0.04f; f.highlightColorG = 0.02f; f.highlightColorB = 0.08f;
            break;

        case FilterPresetId::MIAMI:
            f.exposure = -8.0f; f.brilliance = 18.0f; f.brightness = 22.0f; f.contrast = 10.0f;
            f.temperature = 24.0f; f.tint = -18.0f; f.saturation = 28.0f; f.vibrance = 16.0f;
            f.highlights = 20.0f; f.shadows = 18.0f; f.whites = 8.0f; f.blacks = -18.0f;
            f.fade = 24.0f; f.highlightRolloff = 32.0f; f.microcontrast = 22.0f;
            f.softness = 12.0f; f.vignette = 22.0f;
            f.shadowColorR = -0.12f; f.shadowColorG = 0.08f; f.shadowColorB = 0.16f;
            f.midtoneColorR = 0.06f; f.midtoneColorG = -0.03f; f.midtoneColorB = 0.05f;
            f.highlightColorR = 0.16f; f.highlightColorG = 0.08f; f.highlightColorB = -0.08f;
            break;

        case FilterPresetId::LOS_ANGELES:
            f.exposure = 4.0f; f.brilliance = 16.0f; f.brightness = 18.0f; f.contrast = -14.0f;
            f.temperature = 36.0f; f.tint = -24.0f; f.saturation = 18.0f; f.vibrance = -12.0f;
            f.highlights = -18.0f; f.shadows = 26.0f; f.whites = 6.0f; f.blacks = -24.0f;
            f.fade = 32.0f; f.highlightRolloff = 45.0f; f.microcontrast = -12.0f;
            f.softness = 20.0f; f.vignette = 26.0f;
            f.shadowColorR = 0.09f; f.shadowColorG = -0.04f; f.shadowColorB = 0.15f;
            f.midtoneColorR = 0.08f; f.midtoneColorG = 0.02f; f.midtoneColorB = -0.02f;
            f.highlightColorR = 0.20f; f.highlightColorG = 0.09f; f.highlightColorB = -0.10f;
            break;

        case FilterPresetId::TOKYO:
            f.exposure = -18.0f; f.brilliance = 14.0f; f.brightness = 32.0f; f.contrast = 16.0f;
            f.temperature = -52.0f; f.tint = -34.0f; f.saturation = 16.0f; f.vibrance = -20.0f;
            f.highlights = 52.0f; f.shadows = -12.0f; f.whites = 20.0f; f.blacks = 14.0f;
            f.fade = 18.0f; f.highlightRolloff = 22.0f; f.microcontrast = 36.0f;
            f.softness = 10.0f; f.vignette = 28.0f;
            f.shadowColorR = 0.10f; f.shadowColorG = -0.06f; f.shadowColorB = 0.24f;
            f.midtoneColorR = 0.05f; f.midtoneColorG = -0.04f; f.midtoneColorB = 0.12f;
            f.highlightColorR = 0.06f; f.highlightColorG = 0.04f; f.highlightColorB = 0.14f;
            break;

        case FilterPresetId::PARIS:
            f.exposure = 6.0f; f.brilliance = 12.0f; f.brightness = 24.0f; f.contrast = -22.0f;
            f.temperature = 16.0f; f.tint = -16.0f; f.saturation = -14.0f; f.vibrance = -18.0f;
            f.highlights = -24.0f; f.shadows = 32.0f; f.whites = -6.0f; f.blacks = -28.0f;
            f.fade = 38.0f; f.highlightRolloff = 54.0f; f.microcontrast = -24.0f;
            f.softness = 28.0f; f.vignette = 18.0f;
            f.shadowColorR = 0.06f; f.shadowColorG = 0.01f; f.shadowColorB = 0.08f;
            f.midtoneColorR = 0.07f; f.midtoneColorG = 0.02f; f.midtoneColorB = 0.03f;
            f.highlightColorR = 0.14f; f.highlightColorG = 0.06f; f.highlightColorB = 0.06f;
            break;

        case FilterPresetId::LONDON:
            f.exposure = -28.0f; f.brilliance = -8.0f; f.brightness = 14.0f; f.contrast = 22.0f;
            f.temperature = -44.0f; f.tint = 8.0f; f.saturation = -28.0f; f.vibrance = -34.0f;
            f.highlights = -20.0f; f.shadows = -18.0f; f.whites = -10.0f; f.blacks = 24.0f;
            f.fade = 22.0f; f.highlightRolloff = 40.0f; f.microcontrast = 30.0f;
            f.softness = 8.0f; f.vignette = 32.0f;
            f.shadowColorR = -0.06f; f.shadowColorG = 0.02f; f.shadowColorB = 0.14f;
            f.midtoneColorR = -0.04f; f.midtoneColorG = 0.0f; f.midtoneColorB = 0.06f;
            f.highlightColorR = -0.02f; f.highlightColorG = 0.02f; f.highlightColorB = 0.04f;
            break;

        case FilterPresetId::SEATTLE:
            f.exposure = -20.0f; f.brilliance = -4.0f; f.brightness = 16.0f; f.contrast = 8.0f;
            f.temperature = -36.0f; f.tint = 26.0f; f.saturation = -22.0f; f.vibrance = -26.0f;
            f.highlights = -32.0f; f.shadows = 14.0f; f.whites = -14.0f; f.blacks = -16.0f;
            f.fade = 28.0f; f.highlightRolloff = 48.0f; f.microcontrast = 14.0f;
            f.softness = 14.0f; f.vignette = 26.0f;
            f.shadowColorR = -0.08f; f.shadowColorG = 0.09f; f.shadowColorB = 0.10f;
            f.midtoneColorR = -0.05f; f.midtoneColorG = 0.06f; f.midtoneColorB = 0.04f;
            f.highlightColorR = -0.02f; f.highlightColorG = 0.05f; f.highlightColorB = 0.02f;
            break;

        case FilterPresetId::BERLIN:
            f.exposure = -22.0f; f.brilliance = 6.0f; f.brightness = 18.0f; f.contrast = 38.0f;
            f.temperature = -30.0f; f.tint = -6.0f; f.saturation = -36.0f; f.vibrance = -24.0f;
            f.highlights = 28.0f; f.shadows = -26.0f; f.whites = 16.0f; f.blacks = 30.0f;
            f.fade = 16.0f; f.highlightRolloff = 24.0f; f.microcontrast = 64.0f;
            f.softness = 0.0f; f.vignette = 36.0f;
            f.shadowColorR = -0.04f; f.shadowColorG = 0.0f; f.shadowColorB = 0.10f;
            f.midtoneColorR = -0.03f; f.midtoneColorG = -0.01f; f.midtoneColorB = 0.05f;
            f.highlightColorR = 0.02f; f.highlightColorG = 0.03f; f.highlightColorB = 0.05f;
            break;

        case FilterPresetId::MIAMI_NIGHT:
            f.exposure = -42.0f; f.brilliance = 14.0f; f.brightness = 54.0f; f.contrast = 24.0f;
            f.temperature = -48.0f; f.tint = -28.0f; f.saturation = 24.0f; f.vibrance = -32.0f;
            f.highlights = 68.0f; f.shadows = -20.0f; f.whites = 18.0f; f.blacks = 28.0f;
            f.fade = 18.0f; f.highlightRolloff = 26.0f; f.microcontrast = 58.0f;
            f.softness = 8.0f; f.vignette = 34.0f;
            f.shadowColorR = 0.10f; f.shadowColorG = -0.06f; f.shadowColorB = 0.24f;
            f.midtoneColorR = -0.06f; f.midtoneColorG = 0.04f; f.midtoneColorB = 0.12f;
            f.highlightColorR = 0.18f; f.highlightColorG = 0.07f; f.highlightColorB = -0.06f;
            break;

        case FilterPresetId::NEW_YORK:
        default:
            f.exposure = -24.0f; f.brilliance = 12.0f; f.brightness = 28.0f; f.contrast = 24.0f;
            f.temperature = -38.0f; f.tint = -22.0f; f.saturation = 12.0f; f.vibrance = -28.0f;
            f.highlights = 45.0f; f.shadows = -22.0f; f.whites = 18.0f; f.blacks = 22.0f;
            f.fade = 12.0f; f.highlightRolloff = 25.0f; f.microcontrast = 48.0f;
            f.softness = 6.0f; f.vignette = 34.0f;
            f.shadowColorR = 0.08f; f.shadowColorG = -0.05f; f.shadowColorB = 0.22f;
            f.midtoneColorR = 0.01f; f.midtoneColorG = -0.02f; f.midtoneColorB = 0.06f;
            f.highlightColorR = 0.12f; f.highlightColorG = 0.05f; f.highlightColorB = -0.04f;
            break;
    }
    return f;
}

void applyFilter2014(
    float& r,
    float& g,
    float& b,
    float highFreqLuma,
    float radialSq,
    const FilterParams& f) noexcept
{
    const float str = clamp01(f.strength);
    if (str <= 0.0001f || f.presetId == FilterPresetId::NONE) {
        r = clamp01(r);
        g = clamp01(g);
        b = clamp01(b);
        return;
    }

    r = clamp01(r);
    g = clamp01(g);
    b = clamp01(b);
    highFreqLuma = sanitize(highFreqLuma, 0.0f);
    radialSq = std::max(0.0f, sanitize(radialSq, 0.0f));

    const float expMul = std::pow(2.0f, std::max(-4.0f, std::min(4.0f, (sanitize(f.exposure) / 100.0f) * 1.35f * str)));
    const float brillianceAmt = (sanitize(f.brilliance) / 100.0f) * 0.45f * str;
    const float brightAmt = (sanitize(f.brightness) / 100.0f) * 0.38f * str;
    const float contrastAmt = (sanitize(f.contrast) / 100.0f) * 0.65f * str;
    const float tempAmt = (sanitize(f.temperature) / 100.0f) * 0.16f * str;
    const float tintAmt = (sanitize(f.tint) / 100.0f) * 0.12f * str;
    const float satAmt = (sanitize(f.saturation) / 100.0f) * 0.65f * str;
    const float vibAmt = (sanitize(f.vibrance) / 100.0f) * 0.65f * str;
    const float hiAmt = (sanitize(f.highlights) / 100.0f) * 0.42f * str;
    const float shAmt = (sanitize(f.shadows) / 100.0f) * 0.45f * str;
    const float whitesAmt = (sanitize(f.whites) / 100.0f) * 0.25f * str;
    const float blacksAmt = (sanitize(f.blacks) / 100.0f) * 0.22f * str;
    const float fadeAmt = std::max(0.0f, (sanitize(f.fade) / 100.0f) * 0.24f * str);
    const float rolloffAmt = std::max(0.0f, (sanitize(f.highlightRolloff) / 100.0f) * 0.45f * str);
    const float microAmt = ((sanitize(f.microcontrast) - sanitize(f.softness) * 0.8f) / 100.0f) * 0.55f * str;
    const float vigAmt = std::max(0.0f, (sanitize(f.vignette) / 100.0f) * 0.55f * str);

    r *= expMul;
    g *= expMul;
    b *= expMul;

    if (std::fabs(brightAmt) > 0.0001f) {
        r += brightAmt * (1.0f - clamp01(r) * 0.65f);
        g += brightAmt * (1.0f - clamp01(g) * 0.65f);
        b += brightAmt * (1.0f - clamp01(b) * 0.65f);
    }

    float luma = clamp01(0.2126f * r + 0.7152f * g + 0.0722f * b);
    const float shWeight = std::pow(1.0f - luma, 1.8f);
    const float hiWeight = std::pow(luma, 1.8f);
    const float midWeight = 4.0f * luma * (1.0f - luma);

    const float toneDelta =
        shAmt * shWeight +
        brillianceAmt * (shWeight * 0.6f + midWeight * 0.5f) +
        hiAmt * hiWeight * 0.45f +
        whitesAmt * hiWeight * luma -
        blacksAmt * shWeight * (1.0f - luma) +
        highFreqLuma * microAmt;

    r += toneDelta;
    g += toneDelta;
    b += toneDelta;

    if (std::fabs(contrastAmt) > 0.0001f) {
        r = 0.46f + (r - 0.46f) * (1.0f + contrastAmt);
        g = 0.46f + (g - 0.46f) * (1.0f + contrastAmt);
        b = 0.46f + (b - 0.46f) * (1.0f + contrastAmt);
    }

    // Temperature & tint
    r += tempAmt - tintAmt * 0.35f;
    g += tintAmt * 0.65f;
    b -= tempAmt + tintAmt * 0.45f;

    // 3-Way split toning
    r += (sanitize(f.shadowColorR) * shWeight * 0.45f +
          sanitize(f.midtoneColorR) * midWeight * 0.35f +
          sanitize(f.highlightColorR) * hiWeight * 0.45f) * str;
    g += (sanitize(f.shadowColorG) * shWeight * 0.45f +
          sanitize(f.midtoneColorG) * midWeight * 0.35f +
          sanitize(f.highlightColorG) * hiWeight * 0.45f) * str;
    b += (sanitize(f.shadowColorB) * shWeight * 0.45f +
          sanitize(f.midtoneColorB) * midWeight * 0.35f +
          sanitize(f.highlightColorB) * hiWeight * 0.45f) * str;

    // Saturation & Vibrance
    luma = clamp01(0.2126f * r + 0.7152f * g + 0.0722f * b);
    const float maxC = std::max({r, g, b});
    const float minC = std::min({r, g, b});
    const float curSat = clamp01(maxC - minC);
    const float vibFactor = vibAmt * (1.0f - curSat * curSat);
    const float totalSat = std::max(0.0f, 1.0f + satAmt + vibFactor);

    r = luma + (r - luma) * totalSat;
    g = luma + (g - luma) * totalSat;
    b = luma + (b - luma) * totalSat;

    if (fadeAmt > 0.0001f) {
        r = fadeAmt + r * (1.0f - fadeAmt * 0.85f);
        g = fadeAmt + g * (1.0f - fadeAmt * 0.85f);
        b = fadeAmt + b * (1.0f - fadeAmt * 0.85f);
    }

    if (rolloffAmt > 0.0001f) {
        if (r > 0.65f) r = 0.65f + (r - 0.65f) / (1.0f + (r - 0.65f) * rolloffAmt * 2.2f);
        if (g > 0.65f) g = 0.65f + (g - 0.65f) / (1.0f + (g - 0.65f) * rolloffAmt * 2.2f);
        if (b > 0.65f) b = 0.65f + (b - 0.65f) / (1.0f + (b - 0.65f) * rolloffAmt * 2.2f);
    }

    if (vigAmt > 0.0001f) {
        const float vigMask = std::max(0.0f, 1.0f - radialSq * vigAmt * 1.35f);
        r *= vigMask;
        g *= vigMask;
        b *= vigMask;
    }

    r = clamp01(r);
    g = clamp01(g);
    b = clamp01(b);
}

} // namespace y2014
