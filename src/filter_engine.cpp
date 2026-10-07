#include "2014_engine.h"

namespace y2014 {

FilterParams getFilterPresetParams(FilterPreset preset) {
    FilterParams p;
    p.preset = preset;
    p.filterStrength = 100.0f;

    switch (preset) {
        case FilterPreset::None:
            p.exposure = 0.0f;
            p.brilliance = 0.0f;
            p.highlights = 0.0f;
            p.shadows = 0.0f;
            p.contrast = 0.0f;
            p.brightness = 0.0f;
            p.blackPoint = 0.0f;
            p.fade = 0.0f;
            p.saturation = 0.0f;
            p.vibrance = 0.0f;
            p.warmth = 0.0f;
            p.tint = 0.0f;
            p.sharpness = 0.0f;
            p.definition = 0.0f;
            p.vignette = 0.0f;
            p.centerBrightness = 0.0f;
            p.softening = 0.0f;
            p.shadowTint = { 0.0f, 0.0f, 0.0f };
            p.highlightTint = { 0.0f, 0.0f, 0.0f };
            p.splitBalance = 0.0f;
            break;

        case FilterPreset::NewYork:
            // Dark nightlife / flash: cooler-purple shadows, warm skin/highlights, denser blacks, moderate vignette
            p.exposure = -18.0f;
            p.brilliance = 12.0f;
            p.highlights = 35.0f;
            p.shadows = -14.0f;
            p.contrast = 22.0f;
            p.brightness = 15.0f;
            p.blackPoint = 24.0f;
            p.fade = 8.0f;
            p.saturation = -8.0f;
            p.vibrance = -22.0f;
            p.warmth = -16.0f;
            p.tint = -18.0f;
            p.sharpness = 12.0f;
            p.definition = 42.0f;
            p.vignette = 28.0f;
            p.centerBrightness = 14.0f;
            p.softening = 6.0f;
            p.shadowTint = { 0.08f, -0.05f, 0.22f };
            p.highlightTint = { 0.16f, 0.07f, -0.08f };
            p.splitBalance = -10.0f;
            break;

        case FilterPreset::Miami:
            // Colorful warm/cool split: warm highlights, cyan/teal shadows, magenta accents, lifted blacks
            p.exposure = 4.0f;
            p.brilliance = 18.0f;
            p.highlights = -12.0f;
            p.shadows = 16.0f;
            p.contrast = 14.0f;
            p.brightness = 12.0f;
            p.blackPoint = -10.0f;
            p.fade = 22.0f;
            p.saturation = 18.0f;
            p.vibrance = 24.0f;
            p.warmth = 18.0f;
            p.tint = 14.0f;
            p.sharpness = 8.0f;
            p.definition = 20.0f;
            p.vignette = 18.0f;
            p.centerBrightness = 10.0f;
            p.softening = 10.0f;
            p.shadowTint = { -0.14f, 0.10f, 0.20f };
            p.highlightTint = { 0.22f, 0.08f, -0.12f };
            p.splitBalance = 5.0f;
            break;

        case FilterPreset::LosAngeles:
            // Sunset / soft social-camera look: warm skin, purple-blue skies, orange highlights, softened contrast, gentle fade
            p.exposure = 8.0f;
            p.brilliance = 15.0f;
            p.highlights = -18.0f;
            p.shadows = 22.0f;
            p.contrast = -14.0f;
            p.brightness = 18.0f;
            p.blackPoint = -12.0f;
            p.fade = 28.0f;
            p.saturation = 10.0f;
            p.vibrance = 16.0f;
            p.warmth = 26.0f;
            p.tint = -12.0f;
            p.sharpness = -6.0f;
            p.definition = 10.0f;
            p.vignette = 20.0f;
            p.centerBrightness = 16.0f;
            p.softening = 22.0f;
            p.shadowTint = { 0.10f, -0.04f, 0.18f };
            p.highlightTint = { 0.26f, 0.10f, -0.16f };
            p.splitBalance = 12.0f;
            break;

        case FilterPreset::Tokyo:
            // Violet digital nostalgia: magenta/violet bias, cooler shadows, luminous highlights, medium contrast
            p.exposure = -12.0f;
            p.brilliance = 14.0f;
            p.highlights = 28.0f;
            p.shadows = -8.0f;
            p.contrast = 16.0f;
            p.brightness = 20.0f;
            p.blackPoint = 14.0f;
            p.fade = 14.0f;
            p.saturation = 6.0f;
            p.vibrance = -18.0f;
            p.warmth = -28.0f;
            p.tint = -26.0f;
            p.sharpness = 10.0f;
            p.definition = 36.0f;
            p.vignette = 24.0f;
            p.centerBrightness = 12.0f;
            p.softening = 8.0f;
            p.shadowTint = { 0.12f, -0.08f, 0.28f };
            p.highlightTint = { 0.14f, 0.02f, 0.18f };
            p.splitBalance = -8.0f;
            break;

        case FilterPreset::Paris:
            // Soft pastel nostalgia: warm pink highlights, lifted blacks, gentle contrast, soft detail
            p.exposure = 6.0f;
            p.brilliance = 10.0f;
            p.highlights = -20.0f;
            p.shadows = 24.0f;
            p.contrast = -18.0f;
            p.brightness = 16.0f;
            p.blackPoint = -16.0f;
            p.fade = 32.0f;
            p.saturation = -12.0f;
            p.vibrance = 8.0f;
            p.warmth = 14.0f;
            p.tint = 18.0f;
            p.sharpness = -10.0f;
            p.definition = -8.0f;
            p.vignette = 16.0f;
            p.centerBrightness = 14.0f;
            p.softening = 28.0f;
            p.shadowTint = { 0.08f, 0.02f, 0.10f };
            p.highlightTint = { 0.22f, 0.08f, 0.12f };
            p.splitBalance = 15.0f;
            break;

        case FilterPreset::London:
            // Dark muted cool: lower saturation, cool shadows, dense mids, restrained highlights
            p.exposure = -16.0f;
            p.brilliance = -6.0f;
            p.highlights = -24.0f;
            p.shadows = -12.0f;
            p.contrast = 18.0f;
            p.brightness = -8.0f;
            p.blackPoint = 20.0f;
            p.fade = 12.0f;
            p.saturation = -26.0f;
            p.vibrance = -20.0f;
            p.warmth = -24.0f;
            p.tint = -6.0f;
            p.sharpness = 6.0f;
            p.definition = 28.0f;
            p.vignette = 26.0f;
            p.centerBrightness = 8.0f;
            p.softening = 10.0f;
            p.shadowTint = { -0.08f, 0.02f, 0.18f };
            p.highlightTint = { 0.04f, 0.06f, 0.08f };
            p.splitBalance = -18.0f;
            break;

        case FilterPreset::Seattle:
            // Cool green/cyan dim light: subdued saturation, cyan/green cast, compressed highlights, soft blacks
            p.exposure = -10.0f;
            p.brilliance = -4.0f;
            p.highlights = -28.0f;
            p.shadows = 10.0f;
            p.contrast = 8.0f;
            p.brightness = -4.0f;
            p.blackPoint = 8.0f;
            p.fade = 20.0f;
            p.saturation = -22.0f;
            p.vibrance = -16.0f;
            p.warmth = -20.0f;
            p.tint = 16.0f;
            p.sharpness = 4.0f;
            p.definition = 18.0f;
            p.vignette = 22.0f;
            p.centerBrightness = 8.0f;
            p.softening = 14.0f;
            p.shadowTint = { -0.12f, 0.14f, 0.12f };
            p.highlightTint = { -0.04f, 0.08f, 0.06f };
            p.splitBalance = -12.0f;
            break;

        case FilterPreset::Berlin:
            // Cool hard/desaturated: stronger contrast, cool mids, lower saturation, defined grain
            p.exposure = -14.0f;
            p.brilliance = 6.0f;
            p.highlights = 14.0f;
            p.shadows = -18.0f;
            p.contrast = 32.0f;
            p.brightness = 4.0f;
            p.blackPoint = 26.0f;
            p.fade = 6.0f;
            p.saturation = -32.0f;
            p.vibrance = -24.0f;
            p.warmth = -18.0f;
            p.tint = -8.0f;
            p.sharpness = 18.0f;
            p.definition = 48.0f;
            p.vignette = 28.0f;
            p.centerBrightness = 10.0f;
            p.softening = 0.0f;
            p.shadowTint = { -0.06f, 0.00f, 0.14f };
            p.highlightTint = { 0.06f, 0.06f, 0.08f };
            p.splitBalance = -14.0f;
            break;

        case FilterPreset::MiamiNight:
            // Darker nightlife variant of Miami: violet shadows, warm practicals, cyan/purple separation
            p.exposure = -28.0f;
            p.brilliance = 10.0f;
            p.highlights = 42.0f;
            p.shadows = -16.0f;
            p.contrast = 20.0f;
            p.brightness = 32.0f;
            p.blackPoint = 28.0f;
            p.fade = 10.0f;
            p.saturation = 18.0f;
            p.vibrance = -32.0f;
            p.warmth = -48.0f;
            p.tint = -24.0f;
            p.sharpness = 10.0f;
            p.definition = 56.0f;
            p.vignette = 30.0f;
            p.centerBrightness = 14.0f;
            p.softening = 8.0f;
            p.shadowTint = { 0.14f, -0.08f, 0.28f };
            p.highlightTint = { 0.22f, 0.08f, -0.06f };
            p.splitBalance = -15.0f;
            break;

        case FilterPreset::NostalgicNights:
        default:
            // Exact documented reference values from IMG_3448.JPG ("Nostalgic Nights (IOS)")
            p.exposure = -59.0f;
            p.brilliance = 9.0f;
            p.highlights = 100.0f;
            p.shadows = -17.0f;
            p.contrast = 18.0f;
            p.brightness = 100.0f;
            p.blackPoint = 35.0f;
            p.fade = 8.0f;
            p.saturation = 26.0f;
            p.vibrance = -57.0f;
            p.warmth = -100.0f;
            p.tint = -30.0f;
            p.sharpness = 10.0f;
            p.definition = 100.0f;
            p.vignette = 30.0f;
            p.centerBrightness = 12.0f;
            p.softening = 6.0f;
            p.shadowTint = { 0.14f, -0.06f, 0.30f };
            p.highlightTint = { 0.12f, 0.04f, 0.16f };
            p.splitBalance = -20.0f;
            break;
    }

    return p;
}

const char* getFilterPresetName(FilterPreset preset) {
    switch (preset) {
        case FilterPreset::None:            return "None";
        case FilterPreset::NewYork:         return "New York";
        case FilterPreset::Miami:           return "Miami";
        case FilterPreset::LosAngeles:      return "Los Angeles";
        case FilterPreset::Tokyo:           return "Tokyo";
        case FilterPreset::Paris:           return "Paris";
        case FilterPreset::London:          return "London";
        case FilterPreset::Seattle:         return "Seattle";
        case FilterPreset::Berlin:          return "Berlin";
        case FilterPreset::MiamiNight:      return "Miami Night";
        case FilterPreset::NostalgicNights: return "Nostalgic Nights (IOS)";
        default:                            return "New York";
    }
}

void evaluateFilterPixel(
    float inR, float inG, float inB,
    float normX, float normY,
    float localBlurR, float localBlurG, float localBlurB,
    const FilterParams& params,
    float& outR, float& outG, float& outB
) {
    float mixAmt = clamp01(params.filterStrength * 0.01f);
    if (mixAmt <= 1e-5f) {
        outR = inR;
        outG = inG;
        outB = inB;
        return;
    }

    // 1. Softening vs Definition / Sharpness (local clarity & halation/softness)
    float softAmt = clamp01(params.softening * 0.01f);
    float r = lerpf(inR, localBlurR, softAmt * 0.55f);
    float g = lerpf(inG, localBlurG, softAmt * 0.55f);
    float b = lerpf(inB, localBlurB, softAmt * 0.55f);

    float defAmt = (params.definition * 0.01f) * 0.45f + (params.sharpness * 0.01f) * 0.25f;
    r = clamp01(r + (inR - localBlurR) * defAmt);
    g = clamp01(g + (inG - localBlurG) * defAmt);
    b = clamp01(b + (inB - localBlurB) * defAmt);

    // 2. Exposure & Brightness + Brilliance (iOS-style tonal operator where negative exposure + positive brightness/highlights yields rich moody mids)
    float expN = params.exposure * 0.01f;       // -1..+1
    float briN = params.brightness * 0.01f;     // -1..+1
    float brilN = params.brilliance * 0.01f;    // -1..+1
    float hiN = params.highlights * 0.01f;      // -1..+1
    float shN = params.shadows * 0.01f;         // -1..+1
    float conN = params.contrast * 0.01f;       // -1..+1
    float blkN = params.blackPoint * 0.01f;     // -1..+1
    float fadeN = clamp01(params.fade * 0.01f); // 0..1

    float expMult = std::pow(2.0f, expN * 0.95f);

    auto shapeChannel = [&](float c) -> float {
        c = clamp01(c * expMult);

        // Brightness (gamma-like midtone lift) & Brilliance (midtone clarity + shadow recovery)
        float gamma = std::pow(2.0f, -briN * 0.68f - brilN * 0.28f);
        c = std::pow(std::max(1e-5f, c), gamma);

        // Shadows & Highlights regional recovery/boost
        float shadowMask = (1.0f - smoothstepf(0.0f, 0.55f, c));
        float highlightMask = smoothstepf(0.35f, 1.0f, c);
        c += shN * 0.24f * shadowMask * (1.0f - c);
        c += hiN * 0.22f * highlightMask * (1.0f - 0.35f * c);

        // Black point (positive deepens toe, negative lifts) & Fade (2014 lifted matte blacks)
        if (blkN > 0.0f) {
            float toe = blkN * 0.16f;
            c = std::max(0.0f, (c - toe) / std::max(0.1f, 1.0f - toe * 0.65f));
        } else if (blkN < 0.0f) {
            float lift = -blkN * 0.16f;
            c = lift + c * (1.0f - lift);
        }

        // Contrast S-curve
        float pivot = 0.46f;
        float delta = c - pivot;
        float slope = 1.0f + conN * 0.55f;
        c = clamp01(pivot + delta * slope / (1.0f + std::fabs(conN * 0.35f * delta)));

        // 2014 Matte Fade floor
        float fadeFloor = fadeN * 0.14f;
        c = fadeFloor + c * (1.0f - fadeFloor * 0.85f);
        return clamp01(c);
    };

    r = shapeChannel(r);
    g = shapeChannel(g);
    b = shapeChannel(b);

    // 3. Warmth & Tint (2014 cool/purple or warm/amber cast)
    float warmN = params.warmth * 0.01f; // -1..+1
    float tintN = params.tint * 0.01f;   // -1..+1

    // Negative warmth + negative tint in iOS ("Nostalgic Nights" Warmth -100, Tint -30) produces deep cool blue-violet/purple ambiance
    r = clamp01(r + warmN * 0.11f - tintN * 0.05f);
    g = clamp01(g + warmN * 0.02f + tintN * 0.09f);
    b = clamp01(b - warmN * 0.14f - tintN * 0.07f);

    // 4. Split Toning (Shadow Tint vs Highlight Tint)
    float luma = luminance709(r, g, b);
    float pivotSplit = clampf(0.5f - (params.splitBalance * 0.01f) * 0.25f, 0.15f, 0.85f);
    float shWeight = 1.0f - smoothstepf(0.0f, pivotSplit, luma);
    float hiWeight = smoothstepf(pivotSplit * 0.75f, 1.0f, luma);

    r = clamp01(r + shWeight * params.shadowTint.r * 0.28f + hiWeight * params.highlightTint.r * 0.25f);
    g = clamp01(g + shWeight * params.shadowTint.g * 0.28f + hiWeight * params.highlightTint.g * 0.25f);
    b = clamp01(b + shWeight * params.shadowTint.b * 0.28f + hiWeight * params.highlightTint.b * 0.25f);

    // 5. Saturation & Vibrance (selective skin/muted-tone saturation vs overall saturation)
    float satN = params.saturation * 0.01f;
    float vibN = params.vibrance * 0.01f;
    float curLuma = luminance709(r, g, b);
    float maxC = std::max({r, g, b});
    float minC = std::min({r, g, b});
    float curSat = maxC - minC;

    // Vibrance affects less-saturated colors more strongly
    float vibWeight = (1.0f - clamp01(curSat * 1.35f));
    float totalSatMult = std::max(0.0f, (1.0f + satN * 0.65f) * (1.0f + vibN * 0.65f * vibWeight));

    r = clamp01(curLuma + (r - curLuma) * totalSatMult);
    g = clamp01(curLuma + (g - curLuma) * totalSatMult);
    b = clamp01(curLuma + (b - curLuma) * totalSatMult);

    // 6. Center Brightness & Edge Vignette (classic 2014 social filter radial framing)
    float dx = normX - 0.5f;
    float dy = normY - 0.5f;
    float radial = clamp01((dx * dx + dy * dy) * 2.0f);
    float centerBoost = (params.centerBrightness * 0.01f) * 0.14f * (1.0f - smoothstepf(0.0f, 0.65f, radial));
    float vigDrop = (params.vignette * 0.01f) * 0.42f * smoothstepf(0.18f, 1.05f, radial);
    float radialMult = std::max(0.0f, 1.0f + centerBoost - vigDrop);

    r = clamp01(r * radialMult);
    g = clamp01(g * radialMult);
    b = clamp01(b * radialMult);

    outR = lerpf(inR, r, mixAmt);
    outG = lerpf(inG, g, mixAmt);
    outB = lerpf(inB, b, mixAmt);
}

} // namespace y2014
