#include "grain_engine.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace y2014 {

static inline float sanitize(float v, float fallback = 0.0f) noexcept {
    return std::isfinite(v) ? v : fallback;
}

static inline float clamp01(float v) noexcept {
    v = sanitize(v, 0.0f);
    return std::max(0.0f, std::min(1.0f, v));
}

static inline float fade(float x) noexcept {
    return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

static inline float lerp(float a, float b, float t) noexcept {
    return a + (b - a) * t;
}

uint32_t hash32(uint32_t x) noexcept {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

float u01(uint32_t x) noexcept {
    return static_cast<float>(hash32(x) & 0x00ffffffU) / 16777216.0f;
}

float continuousNoise3D(float x, float y, float t, uint32_t seed) noexcept {
    x = sanitize(x, 0.0f);
    y = sanitize(y, 0.0f);
    t = sanitize(t, 0.0f);

    const int32_t x0 = static_cast<int32_t>(std::floor(x));
    const int32_t y0 = static_cast<int32_t>(std::floor(y));
    const int32_t z0 = static_cast<int32_t>(std::floor(t));
    const float fx = x - static_cast<float>(x0);
    const float fy = y - static_cast<float>(y0);
    const float fz = t - static_cast<float>(z0);

    auto n = [&](int32_t xi, int32_t yi, int32_t zi) noexcept -> float {
        const uint32_t h =
            (static_cast<uint32_t>(xi) * 0x9e3779b9U) ^
            (static_cast<uint32_t>(yi) * 0x85ebca6bU) ^
            (static_cast<uint32_t>(zi) * 0xc2b2ae35U) ^
            seed;
        return u01(h) * 2.0f - 1.0f;
    };

    const float c000 = n(x0,     y0,     z0);
    const float c100 = n(x0 + 1, y0,     z0);
    const float c010 = n(x0,     y0 + 1, z0);
    const float c110 = n(x0 + 1, y0 + 1, z0);
    const float c001 = n(x0,     y0,     z0 + 1);
    const float c101 = n(x0 + 1, y0,     z0 + 1);
    const float c011 = n(x0,     y0 + 1, z0 + 1);
    const float c111 = n(x0 + 1, y0 + 1, z0 + 1);

    const float u = fade(fx);
    const float v = fade(fy);
    const float w = fade(fz);

    const float a = lerp(lerp(c000, c100, u), lerp(c010, c110, u), v);
    const float b = lerp(lerp(c001, c101, u), lerp(c011, c111, u), v);
    return lerp(a, b, w);
}

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
    const CameraProfile& camera) noexcept
{
    const float amount = std::max(0.0f, std::min(2.0f, sanitize(grain.amount, 0.0f)));
    if (amount <= 0.0001f) {
        r = clamp01(r);
        g = clamp01(g);
        b = clamp01(b);
        return;
    }

    r = clamp01(r);
    g = clamp01(g);
    b = clamp01(b);
    highFreqLuma = sanitize(highFreqLuma, 0.0f);
    timeSeconds = std::max(0.0f, sanitize(timeSeconds, 0.0f));

    const float luma = clamp01(0.2126f * r + 0.7152f * g + 0.0722f * b);

    // Resolution normalization against 1280x720 reference so 720p/1080p/4K maintain proportional particle size
    const float safeW = static_cast<float>(std::max<int32_t>(16, frameWidth));
    const float safeH = static_cast<float>(std::max<int32_t>(16, frameHeight));
    const float resNorm = std::sqrt((safeW * safeH) / (1280.0f * 720.0f));

    const float particleSize = std::max(0.25f, sanitize(grain.size, 1.0f));
    const float zoomScale = std::max(0.25f, sanitize(grain.scale, 1.0f));
    const float effectiveSize = std::max(0.30f, particleSize * zoomScale * resNorm);
    const float invFineFreq = 1.0f / effectiveSize;
    const float invCoarseFreq = 1.0f / (effectiveSize * 3.1f);
    const float invPatternFreq = 1.0f / (std::max(0.4f, sanitize(grain.patternScale, 1.0f)) * 18.0f * resNorm);

    // Temporal animation coordinates (coherent 3D advection + controlled per-frame stochastic term)
    const float animAmt = clamp01(grain.animation);
    const float speed = std::max(0.0f, sanitize(grain.speed, 1.0f));
    const float tCoord = (animAmt > 0.001f) ? (timeSeconds * speed * 5.5f * animAmt) : 0.0f;
    const uint32_t frameIdx = (animAmt > 0.001f)
        ? static_cast<uint32_t>(std::floor(timeSeconds * 24.0f * std::max(0.2f, speed)))
        : 0U;
    const float coherence = clamp01(grain.temporalCoherence);

    // Tonal response curve: Black -> Shadow -> Midtone -> Highlight -> White
    const float blackFloor = clamp01(luma / 0.06f + 0.25f);
    const float whiteCeil = clamp01((1.0f - luma) / 0.08f + 0.15f);
    const float shZone = std::pow(1.0f - luma, 1.7f) * std::max(0.0f, sanitize(grain.shadows, 1.0f));
    const float midZone = 4.0f * luma * (1.0f - luma) * std::max(0.0f, sanitize(grain.midtones, 1.0f));
    const float hiZone = std::pow(luma, 1.8f) * std::max(0.0f, sanitize(grain.highlights, 1.0f));
    const float tonalWeight = (shZone * 0.55f + midZone * 0.35f + hiZone * 0.25f) * blackFloor * whiteCeil;

    // Physical sensor model: signal-dependent Shot Noise + floor Read Noise
    const float shotComp = std::sqrt(std::max(0.02f, luma)) * clamp01(grain.shotNoise);
    const float readComp = (1.0f - luma * 0.65f) * clamp01(grain.readNoise);
    const float sensorGain = 0.35f + shotComp * 0.55f + readComp * 0.65f;

    float fineSample = 0.0f;
    float coarseSample = 0.0f;

    if (grain.renderMode == GrainRenderMode::DRAFT) {
        const uint32_t h =
            (static_cast<uint32_t>(x) * 0x9e3779b9U) ^
            (static_cast<uint32_t>(y) * 0x85ebca6bU) ^
            (frameIdx * 0xc2b2ae35U) ^
            grain.seed;
        fineSample = u01(h) * 2.0f - 1.0f;
    } else {
        const float nx = static_cast<float>(x) * invFineFreq;
        const float ny = static_cast<float>(y) * invFineFreq;
        const float coherentFine = continuousNoise3D(nx, ny, tCoord, grain.seed);

        const uint32_t stochHash =
            (static_cast<uint32_t>(std::floor(nx * 1.5f)) * 0x9e3779b9U) ^
            (static_cast<uint32_t>(std::floor(ny * 1.5f)) * 0x85ebca6bU) ^
            (frameIdx * 0x27d4eb2dU) ^
            grain.seed;
        const float stochFine = u01(stochHash) * 2.0f - 1.0f;
        fineSample = lerp(stochFine, coherentFine, coherence);

        if (grain.renderMode == GrainRenderMode::ACCURATE &&
            (grain.coarse > 0.01f || grain.clustering > 0.01f)) {
            const float cx = static_cast<float>(x) * invCoarseFreq;
            const float cy = static_cast<float>(y) * invCoarseFreq;
            coarseSample = continuousNoise3D(cx, cy, tCoord * 0.45f, grain.seed + 1013U);

            const float clust = clamp01(grain.clustering);
            if (clust > 0.01f) {
                const float density = std::max(0.1f, sanitize(grain.density, 1.0f));
                const float shaped = std::tanh(coarseSample * (1.2f + clust * 2.2f) * density);
                coarseSample = lerp(coarseSample, shaped, clust);
            }
        }
    }

    // Subtle row-level PRNU sensor variation
    const float patAmt = std::max(0.0f, std::min(1.0f, sanitize(grain.patternAmount, 0.0f)));
    const float rowNoise = (patAmt > 0.001f)
        ? ((u01(static_cast<uint32_t>(y) * 2654435761U + grain.seed) * 2.0f - 1.0f) * patAmt * 0.04f)
        : 0.0f;

    const float prnu = (patAmt > 0.005f && grain.renderMode == GrainRenderMode::ACCURATE)
        ? (continuousNoise3D(
               static_cast<float>(x) * invPatternFreq,
               static_cast<float>(y) * invPatternFreq,
               tCoord * 0.05f,
               grain.seed + 7919U) * patAmt * 0.25f)
        : 0.0f;

    const float spatialBlend = clamp01(clamp01(grain.softness) * 0.5f + clamp01(grain.spatialCorrelation) * 0.35f);
    const float density = std::max(0.1f, std::min(3.0f, sanitize(grain.density, 1.0f)));
    const float lumaWeight = std::max(0.0f, std::min(2.0f, sanitize(grain.luma, 1.0f)));

    const float combinedLumaNoise =
        (fineSample * std::max(0.0f, sanitize(grain.fine, 1.0f)) * (1.0f - spatialBlend * 0.35f) +
         coarseSample * std::max(0.0f, sanitize(grain.coarse, 0.5f)) * (0.65f + spatialBlend * 0.55f) +
         rowNoise) *
        density * lumaWeight;

    // Interaction with camera ISP denoising and edge sharpening
    const float edgeFactor = std::min(1.0f, std::fabs(highFreqLuma) * 5.0f);
    const float ispMod =
        1.0f -
        clamp01(grain.denoiseInteraction) * (1.0f - edgeFactor) * clamp01(camera.lowLightNR) * 0.35f +
        clamp01(grain.sharpeningInteraction) * edgeFactor * clamp01(camera.sharpening) * 0.45f;

    const float finalLumaDelta =
        (combinedLumaNoise + prnu) * amount * 0.22f * tonalWeight * sensorGain * ispMod;

    // Correlated YCbCr chroma noise
    float cbNoise = 0.0f;
    float crNoise = 0.0f;
    const float isLowLight = clamp01(1.0f - luma * 1.65f);

    if (grain.colorMode != GrainColorMode::MONOCHROME &&
        grain.colorMode != GrainColorMode::NEUTRAL_LUMA &&
        (grain.chroma > 0.01f || grain.chromaNoise > 0.01f))
    {
        const float chromaFreq = invCoarseFreq * 0.75f;
        const float cbRaw = continuousNoise3D(
            static_cast<float>(x) * chromaFreq + 17.3f,
            static_cast<float>(y) * chromaFreq + 31.7f,
            tCoord * 0.55f,
            grain.seed + 31337U);
        const float crRaw = continuousNoise3D(
            static_cast<float>(x) * chromaFreq + 59.1f,
            static_cast<float>(y) * chromaFreq + 11.9f,
            tCoord * 0.55f,
            grain.seed + 65537U);

        float chromaStrength =
            amount * 0.16f *
            ((clamp01(grain.chroma) + clamp01(grain.chromaNoise)) * 0.5f) *
            tonalWeight * (0.6f + isLowLight * 0.7f);

        if (grain.colorMode == GrainColorMode::CAMERA_PROFILE) {
            chromaStrength *= (0.55f + clamp01(camera.chromaNoise) * 0.90f);
        } else if (grain.colorMode == GrainColorMode::FULL_CHROMA) {
            chromaStrength *= 1.35f;
        }

        cbNoise = cbRaw * chromaStrength;
        crNoise = crRaw * chromaStrength;

        if (grain.colorMode == GrainColorMode::WARM_CHROMA) {
            crNoise += std::fabs(crRaw) * chromaStrength * 0.45f;
            cbNoise -= std::fabs(cbRaw) * chromaStrength * 0.25f;
        } else if (grain.colorMode == GrainColorMode::COOL_CHROMA) {
            cbNoise += std::fabs(cbRaw) * chromaStrength * 0.45f;
            crNoise -= std::fabs(crRaw) * chromaStrength * 0.20f;
        }
    }

    if (grain.colorMode == GrainColorMode::MONOCHROME) {
        const float mono = clamp01(luma + finalLumaDelta);
        r = mono;
        g = mono;
        b = mono;
    } else {
        r = clamp01(r + finalLumaDelta + 1.402f * crNoise);
        g = clamp01(g + finalLumaDelta - 0.344136f * cbNoise - 0.714136f * crNoise);
        b = clamp01(b + finalLumaDelta + 1.772f * cbNoise);
    }
}

} // namespace y2014
