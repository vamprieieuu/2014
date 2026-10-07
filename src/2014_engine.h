#pragma once
#ifndef Y2014_ENGINE_H
#define Y2014_ENGINE_H

#include <cstdint>
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <array>
#include <vector>
#include <string>

namespace y2014 {

// ============================================================================
// 1. Enumerations
// ============================================================================

enum class CameraModel : int {
    IPhone5 = 0,
    IPhone5s = 1,
    IPhone6 = 2,
    IPhone6s = 3,
    IPhone7 = 4,
    IPhone7Plus = 5,
    IPhone8 = 6,
    IPhone8Plus = 7,
    Count = 8
};

enum class FilterPreset : int {
    None = 0,
    NewYork = 1,
    Miami = 2,
    LosAngeles = 3,
    Tokyo = 4,
    Paris = 5,
    London = 6,
    Seattle = 7,
    Berlin = 8,
    MiamiNight = 9,
    NostalgicNights = 10,
    Count = 11
};

enum class GrainColorMode : int {
    Monochrome = 0,
    NeutralLuma = 1,
    WarmChroma = 2,
    CoolChroma = 3,
    FullChroma = 4,
    CameraProfile = 5,
    Count = 6
};

enum class PerformanceMode : int {
    Accurate = 0,
    Preview = 1,
    Draft = 2,
    Count = 3
};

// ============================================================================
// 2. Floating-Point RGB / RGBA Structures
// ============================================================================

struct RGBf {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};

struct RGBAf {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float clamp01(float v) {
    return clampf(v, 0.0f, 1.0f);
}

inline float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

inline float smoothstepf(float edge0, float edge1, float x) {
    if (std::fabs(edge1 - edge0) < 1e-6f) {
        return x >= edge1 ? 1.0f : 0.0f;
    }
    float t = clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

inline float luminance709(float r, float g, float b) {
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

// ============================================================================
// 3. Subsystem 1: Camera Model Specification & User Controls
// ============================================================================

struct CameraSpec {
    CameraModel model = CameraModel::IPhone5s;
    const char* name = "iPhone 5s";
    const char* sensorSummary = "8MP 1/3\" 1.5um f/2.2";

    // Tone & Exposure / HDR blending character
    float highlightRollOff = 0.35f;
    float shadowLift = 0.04f;
    float microContrast = 0.14f;
    float hdrBlendStrength = 0.25f;
    float hdrGhostFringe = 0.08f;
    float colorQuantization = 0.05f;

    // Color response (matrix & cast approximations)
    float warmthBias = 0.05f;
    float tintBias = -0.02f;
    float saturationResponse = 0.96f;
    float wideColorGamut = 0.0f;
    float colorShadingEdge = 0.08f;
    std::array<float, 9> colorMatrix = {
        1.02f, -0.01f, -0.01f,
       -0.01f,  1.01f,  0.00f,
        0.00f, -0.02f,  1.02f
    };

    // Sensor & ISP processing defaults (used by both Camera stage and Grain Engine)
    float shotReadBalance = 0.55f;       // 0 = read-dominated, 1 = shot-dominated
    float lumaChromaBalance = 0.68f;     // 0 = pure chroma, 1 = pure luma
    float baseSensorNoise = 0.28f;
    float lowLightDenoise = 0.48f;       // Bilateral/chroma NR strength in dark areas
    float detailLossLowLight = 0.42f;    // Smoothing of fine shadow detail when NR engages
    float ispSharpening = 0.38f;         // Edge sharpening / halo character
    float ispRinging = 0.18f;            // Slight overshoot halo around high-contrast edges
    float chromaCorrelation = 0.65f;     // Demosaic/ISP spatial correlation of chroma noise
    float prnuPatternAmount = 0.12f;     // Fixed-pattern sensor non-uniformity
    float jpegBlockResidue = 0.14f;      // Subtle 2014-era JPEG quantization/chroma subsampling residue
    float lensVignette = 0.18f;
    float cornerSoftness = 0.16f;
    float defaultTemporalCoherence = 0.62f;

    // 5-point camera tonal grain response: Black, Shadow, Midtone, Highlight, White
    std::array<float, 5> tonalGrainCurve = { 0.22f, 0.92f, 0.58f, 0.24f, 0.06f };
};

struct CameraParams {
    CameraModel model = CameraModel::IPhone5s;
    float profileStrength = 100.0f;      // 0..100%

    // Editable camera overrides (normalized -100..+100 or 0..100%)
    float exposureTrim = 0.0f;           // -100..+100
    float hdrCharacter = 50.0f;          // 0..100%
    float toneCompression = 50.0f;       // 0..100%
    float cameraWarmth = 0.0f;           // -100..+100
    float cameraTint = 0.0f;             // -100..+100
    float lowLightDenoise = 50.0f;       // 0..100%
    float ispSharpening = 50.0f;         // 0..100%
    float lensVignette = 50.0f;          // 0..100%
    float edgeSoftness = 40.0f;          // 0..100%
    float jpegEraArtifact = 25.0f;       // 0..100%
};

const CameraSpec& getCameraSpec(CameraModel model);
const char* getCameraModelName(CameraModel model);

void evaluateCameraPixel(
    float inR, float inG, float inB,
    float normX, float normY,
    const CameraSpec& spec,
    const CameraParams& params,
    float localLumaMean,
    float localDetail,
    float& outR, float& outG, float& outB
);

// ============================================================================
// 4. Subsystem 2: 2014 Filter Specification & User Controls
// ============================================================================

struct FilterParams {
    FilterPreset preset = FilterPreset::NewYork;
    float filterStrength = 100.0f;       // 0..100%

    // Editable 2014 filter parameters (-100..+100)
    float exposure = -18.0f;
    float brilliance = 12.0f;
    float highlights = 35.0f;
    float shadows = -14.0f;
    float contrast = 22.0f;
    float brightness = 15.0f;
    float blackPoint = 24.0f;
    float fade = 8.0f;
    float saturation = -8.0f;
    float vibrance = -22.0f;
    float warmth = -16.0f;
    float tint = -18.0f;
    float sharpness = 12.0f;
    float definition = 42.0f;
    float vignette = 28.0f;
    float centerBrightness = 14.0f;
    float softening = 6.0f;

    // Split toning (R, G, B in -1..+1)
    RGBf shadowTint = { 0.08f, -0.05f, 0.22f };
    RGBf highlightTint = { 0.16f, 0.07f, -0.08f };
    float splitBalance = -10.0f;         // -100..+100
};

FilterParams getFilterPresetParams(FilterPreset preset);
const char* getFilterPresetName(FilterPreset preset);

void evaluateFilterPixel(
    float inR, float inG, float inB,
    float normX, float normY,
    float localBlurR, float localBlurG, float localBlurB,
    const FilterParams& params,
    float& outR, float& outG, float& outB
);

// ============================================================================
// 5. Subsystem 3: Procedural Camera-Aware Sensor Grain / Noise Engine
// ============================================================================

struct GrainParams {
    // Main controls
    float amount = 45.0f;                // 0..100% (up to 200% slider max)
    float size = 38.0f;                  // 1..100 (grain particle scale in image space)
    float density = 62.0f;               // 0..100% (cluster occurrence frequency)
    float scaleZoom = 100.0f;            // 10..400% (sampling scale of generated texture)
    float softness = 32.0f;              // 0..100% (spatial filtering of grain structure)
    float speed = 50.0f;                 // 0..200% (temporal evolution speed)
    float animation = 75.0f;             // 0..100% (0 = static, 100 = full temporal motion)
    GrainColorMode colorMode = GrainColorMode::CameraProfile;
    float chromaAmount = 48.0f;          // 0..100%
    int32_t seed = 2014;                 // User base seed

    // Advanced tonal response controls
    float blackResponse = 22.0f;         // 0..100%
    float shadows = 88.0f;               // 0..200%
    float midtones = 58.0f;              // 0..200%
    float highlights = 24.0f;            // 0..200%
    float whiteResponse = 6.0f;          // 0..100%

    // Advanced structural & sensor controls
    float fineGrain = 68.0f;             // 0..100%
    float coarseGrain = 46.0f;           // 0..100%
    float lumaChromaBalance = 68.0f;     // 0..100% (0 = chroma, 100 = luma)
    float clustering = 42.0f;            // 0..100%
    float spatialCorrelation = 55.0f;    // 0..100%
    float temporalCoherence = 62.0f;     // 0..100% (0 = independent per-frame, 100 = smooth 3D advected)
    float patternAmount = 14.0f;         // 0..100% (PRNU sensor pattern)
    float patternScale = 50.0f;          // 10..200%
    float shotReadBalance = 55.0f;       // 0..100% (0 = read noise, 100 = Poisson shot noise)
    float shadowFloor = 18.0f;           // 0..100%
    float chromaCorrelation = 65.0f;     // 0..100%
    float chromaHueBias = -12.0f;        // -100..+100 (cool/violet vs warm)
    float denoiseInteraction = 52.0f;    // 0..100%
    float sharpeningInteraction = 44.0f; // 0..100%

    PerformanceMode performanceMode = PerformanceMode::Accurate;
};

struct GrainSampleResult {
    float deltaR = 0.0f;
    float deltaG = 0.0f;
    float deltaB = 0.0f;
    float lumaGrain = 0.0f;
    float cbGrain = 0.0f;
    float crGrain = 0.0f;
    float tonalWeight = 0.0f;
};

float evaluateTonalGrainResponse(
    float luma,
    const CameraSpec& camSpec,
    const GrainParams& grainParams
);

GrainSampleResult evaluateGrainAtPixel(
    int x,
    int y,
    int width,
    int height,
    int frameIndex,
    float frameTimeSeconds,
    float pixelR,
    float pixelG,
    float pixelB,
    float localEdgeDetail,
    const CameraSpec& camSpec,
    const GrainParams& grainParams
);

// ============================================================================
// 6. Complete Pipeline State & Frame Processor
// ============================================================================

struct PipelineConfig {
    CameraParams camera;
    FilterParams filter;
    GrainParams grain;
    int frameIndex = 0;
    float frameTimeSeconds = 0.0f;
    bool usePresetDefaultsForFilter = false;
    bool applyCameraGrainDefaults = false;
};

// Synchronizes GrainParams defaults from a selected CameraModel when requested
void applyCameraDefaultsToGrain(CameraModel model, GrainParams& grain);

// Full frame processor operating on floating-point RGBA buffer (row-major, width * height)
void renderFrameRGBAf(
    const RGBAf* srcBuffer,
    RGBAf* dstBuffer,
    int width,
    int height,
    int srcStridePixels,
    int dstStridePixels,
    const PipelineConfig& config
);

// Single-pixel evaluation helper (with optional neighborhood estimates)
RGBAf evaluatePipelinePixel(
    int x,
    int y,
    int width,
    int height,
    const RGBAf& centerPx,
    const RGBAf& localBlurPx,
    float localDetail,
    const PipelineConfig& config
);

} // namespace y2014

#endif // Y2014_ENGINE_H
