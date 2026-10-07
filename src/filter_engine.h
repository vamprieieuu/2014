#pragma once
#include <cstdint>

namespace y2014 {

enum class FilterPresetId : int32_t {
    NONE = 1,
    NEW_YORK = 2,
    MIAMI = 3,
    LOS_ANGELES = 4,
    TOKYO = 5,
    PARIS = 6,
    LONDON = 7,
    SEATTLE = 8,
    BERLIN = 9,
    MIAMI_NIGHT = 10,
    NOSTALGIC_NIGHTS_REF = 11,
    COUNT = 11
};

struct FilterParams {
    FilterPresetId presetId = FilterPresetId::NEW_YORK;
    float strength = 0.85f;         // 0.0 to 1.0

    float exposure = -24.0f;        // -100 to +100
    float brilliance = 12.0f;       // -100 to +100
    float brightness = 28.0f;       // -100 to +100
    float contrast = 24.0f;         // -100 to +100
    float temperature = -38.0f;     // -100 to +100
    float tint = -22.0f;            // -100 to +100
    float saturation = 12.0f;       // -100 to +100
    float vibrance = -28.0f;        // -100 to +100
    float highlights = 45.0f;       // -100 to +100
    float shadows = -22.0f;         // -100 to +100
    float whites = 18.0f;           // -100 to +100
    float blacks = 22.0f;           // -100 to +100
    float fade = 12.0f;             // 0 to 100
    float highlightRolloff = 25.0f; // 0 to 100
    float microcontrast = 48.0f;    // -100 to +100
    float softness = 6.0f;          // 0 to 100
    float vignette = 34.0f;         // 0 to 100

    float shadowColorR = 0.08f;
    float shadowColorG = -0.05f;
    float shadowColorB = 0.22f;

    float midtoneColorR = 0.01f;
    float midtoneColorG = -0.02f;
    float midtoneColorB = 0.06f;

    float highlightColorR = 0.12f;
    float highlightColorG = 0.05f;
    float highlightColorB = -0.04f;
};

FilterParams getFilterPreset(FilterPresetId presetId) noexcept;

void applyFilter2014(
    float& r,
    float& g,
    float& b,
    float highFreqLuma,
    float radialSq,
    const FilterParams& f) noexcept;

} // namespace y2014
