#pragma once
#include <cstdint>

namespace y2014 {

enum class CameraModelId : int32_t {
    IPHONE_5 = 1,
    IPHONE_5S = 2,
    IPHONE_6 = 3,
    IPHONE_6S = 4,
    IPHONE_7 = 5,
    IPHONE_7_PLUS = 6,
    IPHONE_8 = 7,
    IPHONE_8_PLUS = 8,
    COUNT = 8
};

struct CameraProfile {
    CameraModelId id = CameraModelId::IPHONE_6;
    float strength = 0.85f;

    // Basic Camera Controls
    float exposure = 0.0f;       // -2.0 to +2.0 EV
    float brightness = 0.0f;     // -100 to +100
    float contrast = 0.0f;       // -100 to +100
    float saturation = 0.10f;    // -1.0 to +1.0
    float temperature = 0.20f;   // -1.0 to +1.0 (warmth)
    float tint = 0.0f;           // -1.0 to +1.0
    float highlights = 0.0f;     // -100 to +100
    float shadows = 0.0f;        // -100 to +100
    float whites = 0.0f;         // -100 to +100
    float blacks = 0.0f;         // -100 to +100

    // Advanced ISP / Optics / Sensor Characteristics
    float warmth = 0.20f;
    float lowLightNR = 0.58f;
    float detail = 0.68f;
    float sharpening = 0.62f;
    float highlightRollOff = 0.48f;
    float shadowLift = 0.20f;
    float lensSoftness = 0.14f;
    float vignette = 0.19f;
    float distortion = 0.08f;
    float chromaticAberration = 0.12f;
    float chromaNoise = 0.42f;
    float hdrCharacter = 0.68f;
};

CameraProfile getCameraPreset(CameraModelId modelId) noexcept;

void applyCameraCharacter(
    float& r,
    float& g,
    float& b,
    float highFreqLuma,
    float radialSq,
    const CameraProfile& p,
    float strength) noexcept;

} // namespace y2014
