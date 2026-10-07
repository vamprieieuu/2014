#pragma once
#include <cstdint>

namespace y2014 {

struct GrainParams {
    float amount = 0.0f;
    float size = 1.0f;
    float density = 1.0f;
    float scale = 1.0f;
    float softness = 0.0f;
    float speed = 1.0f;
    float animation = 1.0f;
    float chroma = 0.0f;
    float shadows = 1.0f;
    float midtones = 1.0f;
    float highlights = 1.0f;
    float fine = 1.0f;
    float coarse = 0.0f;
    float clustering = 0.0f;
    float spatialCorrelation = 0.5f;
    float temporalCoherence = 0.65f;
    float shotNoise = 0.0f;
    float readNoise = 0.0f;
    float chromaNoise = 0.0f;
    float patternAmount = 0.0f;
    float patternScale = 1.0f;
    uint32_t seed = 0;
};

// Deterministic continuous 3D noise sample used by the final AE renderer.
float continuousNoise3D(float x, float y, float t, uint32_t seed);

} // namespace y2014
