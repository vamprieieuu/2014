#pragma once
#include <cstdint>
#include <cstddef>
#include "camera_profiles.h"
#include "filter_engine.h"
#include "grain_engine.h"

namespace y2014 {

// Standard AE pixel layouts (ARGB in PF_Pixel8 / PF_Pixel16 / PF_PixelFloat)
struct Pixel8 {
    uint8_t alpha;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

struct Pixel16 {
    uint16_t alpha;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
};

struct PixelFloat {
    float alpha;
    float red;
    float green;
    float blue;
};

struct RenderContext {
    CameraProfile camera;
    FilterParams filter;
    GrainParams grain;
    float timeSeconds = 0.0f;
    int32_t width = 0;
    int32_t height = 0;
};

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
    float& outA) noexcept;

void renderBuffer8(
    const uint8_t* srcBase,
    int32_t srcRowBytes,
    uint8_t* dstBase,
    int32_t dstRowBytes,
    int32_t width,
    int32_t height,
    const RenderContext& ctx) noexcept;

void renderBuffer16(
    const uint8_t* srcBase,
    int32_t srcRowBytes,
    uint8_t* dstBase,
    int32_t dstRowBytes,
    int32_t width,
    int32_t height,
    const RenderContext& ctx) noexcept;

void renderBufferFloat(
    const uint8_t* srcBase,
    int32_t srcRowBytes,
    uint8_t* dstBase,
    int32_t dstRowBytes,
    int32_t width,
    int32_t height,
    const RenderContext& ctx) noexcept;

} // namespace y2014
