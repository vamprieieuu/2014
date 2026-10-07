#pragma once
#include <cstdint>
#include "camera_profiles.h"

namespace y2014 {

enum class GrainColorMode : int32_t {
    CAMERA_PROFILE = 1,
    MONOCHROME = 2,
    NEUTRAL_LUMA = 3,
    WARM_CHROMA = 4,
    COOL_CHROMA = 5,
    FULL_CHROMA = 6
};

enum class GrainRenderMode : int32_t {
    ACCURATE = 1,
    PREVIEW = 2,
    DRAFT = 3
};

struct GrainParams {
    // Main controls
    float amount = 0.42f;
    float size = 1.25f;
    float density = 1.10f;
    float scale = 1.00f;
    float softness = 0.28f;
    float speed = 1.00f;
    float animation = 0.85f;
    float chroma = 0.48f;
    uint32_t seed = 2014U;

    // Advanced tonal controls
    float shadows = 1.35f;
    float midtones = 0.95f;
    float highlights = 0.35f;

    // Advanced structure & sensor controls
    float fine = 1.10f;
    float coarse = 0.55f;
    float luma = 1.00f;
    float clustering = 0.42f;
    float spatialCorrelation = 0.52f;
    float temporalCoherence = 0.65f;
    float shotNoise = 0.45f;
    float readNoise = 0.62f;
    float chromaNoise = 0.48f;
    float patternAmount = 0.12f;
    float patternScale = 1.40f;
    float denoiseInteraction = 0.45f;
    float sharpeningInteraction = 0.50f;

    GrainColorMode colorMode = GrainColorMode::CAMERA_PROFILE;
    GrainRenderMode renderMode = GrainRenderMode::ACCURATE;
};

// Deterministic 32-bit hash & continuous 3D noise
uint32_t hash32(uint32_t x) noexcept;
float u01(uint32_t x) noexcept;
float continuousNoise3D(float x, float y, float t, uint32_t seed) noexcept;

// Resolution-independent, camera-aware, temporally coherent sensor grain evaluation
void applySensorGrainPixel(
    float& r,
    float& g,
    float& b,
    int32_t x,
    int32_t y,
    int32_t frameWidth,
    int32_t frameHeight,
    float highFreqLuma,
    float timeSeconds,
    const GrainParams& grain,
    const CameraProfile& camera) noexcept;

} // namespace y2014
