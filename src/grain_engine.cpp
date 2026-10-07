#include "2014_engine.h"

namespace y2014 {

namespace {

// Fast, high-quality 32-bit PCG/Murmur-inspired integer hash for non-repeating procedural sensor noise
inline uint32_t hash32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

inline uint32_t hashCoords3D(int32_t x, int32_t y, int32_t z, uint32_t seed) {
    uint32_t h = seed + 0x9e3779b9U;
    h ^= static_cast<uint32_t>(x) * 0x85ebca6bU;
    h = hash32(h);
    h ^= static_cast<uint32_t>(y) * 0xc2b2ae35U;
    h = hash32(h);
    h ^= static_cast<uint32_t>(z) * 0x27d4eb2fU;
    return hash32(h);
}

// Uniform float in [-1.0, +1.0]
inline float hashSignedFloat(int32_t x, int32_t y, int32_t z, uint32_t seed) {
    uint32_t h = hashCoords3D(x, y, z, seed);
    return static_cast<float>(h & 0x00FFFFFFU) * (2.0f / 16777215.0f) - 1.0f;
}

// Approximate zero-mean unit-variance Gaussian via sum of 3 independent uniform variates (Irwin-Hall)
inline float hashGaussian(int32_t x, int32_t y, int32_t z, uint32_t seed) {
    uint32_t h0 = hashCoords3D(x, y, z, seed);
    uint32_t h1 = hash32(h0 ^ 0xA5A5A5A5U);
    uint32_t h2 = hash32(h1 ^ 0x5A5A5A5AU);
    float u0 = static_cast<float>(h0 & 0x00FFFFFFU) * (1.0f / 16777215.0f) - 0.5f;
    float u1 = static_cast<float>(h1 & 0x00FFFFFFU) * (1.0f / 16777215.0f) - 0.5f;
    float u2 = static_cast<float>(h2 & 0x00FFFFFFU) * (1.0f / 16777215.0f) - 0.5f;
    // Sum of 3 uniforms in [-0.5, 0.5] has variance 3/12 = 1/4, std dev = 0.5; multiply by 2.0 for unit variance
    return (u0 + u1 + u2) * 2.0f;
}

inline float quinticFade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// Continuous 3D value-gradient noise in [-1, +1] for coherent temporal evolution without flicker
float coherentNoise3D(float x, float y, float z, uint32_t seed) {
    int32_t xi = static_cast<int32_t>(std::floor(x));
    int32_t yi = static_cast<int32_t>(std::floor(y));
    int32_t zi = static_cast<int32_t>(std::floor(z));

    float xf = x - static_cast<float>(xi);
    float yf = y - static_cast<float>(yi);
    float zf = z - static_cast<float>(zi);

    float u = quinticFade(xf);
    float v = quinticFade(yf);
    float w = quinticFade(zf);

    float c000 = hashSignedFloat(xi,     yi,     zi,     seed);
    float c100 = hashSignedFloat(xi + 1, yi,     zi,     seed);
    float c010 = hashSignedFloat(xi,     yi + 1, zi,     seed);
    float c110 = hashSignedFloat(xi + 1, yi + 1, zi,     seed);
    float c001 = hashSignedFloat(xi,     yi,     zi + 1, seed);
    float c101 = hashSignedFloat(xi + 1, yi,     zi + 1, seed);
    float c011 = hashSignedFloat(xi,     yi + 1, zi + 1, seed);
    float c111 = hashSignedFloat(xi + 1, yi + 1, zi + 1, seed);

    float x00 = lerpf(c000, c100, u);
    float x10 = lerpf(c010, c110, u);
    float x01 = lerpf(c001, c101, u);
    float x11 = lerpf(c011, c111, u);

    float y0 = lerpf(x00, x10, v);
    float y1 = lerpf(x01, x11, v);

    // Normalize variance so output has ~unit standard deviation
    return lerpf(y0, y1, w) * 1.45f;
}

// Spatial low-pass filtered stochastic field on continuous 2D coordinates for a given integer frame
float filteredStochastic2D(float x, float y, int32_t frameSlice, float softnessNorm, uint32_t seed) {
    int32_t xi = static_cast<int32_t>(std::floor(x));
    int32_t yi = static_cast<int32_t>(std::floor(y));
    float xf = x - static_cast<float>(xi);
    float yf = y - static_cast<float>(yi);

    float u = smoothstepf(0.0f, 1.0f, xf);
    float v = smoothstepf(0.0f, 1.0f, yf);

    float n00 = hashGaussian(xi,     yi,     frameSlice, seed);
    float n10 = hashGaussian(xi + 1, yi,     frameSlice, seed);
    float n01 = hashGaussian(xi,     yi + 1, frameSlice, seed);
    float n11 = hashGaussian(xi + 1, yi + 1, frameSlice, seed);

    float bilinear = lerpf(lerpf(n00, n10, u), lerpf(n01, n11, u), v);

    if (softnessNorm <= 0.05f) {
        return lerpf(n00, bilinear, clamp01(softnessNorm * 20.0f));
    }

    // 5-tap cross spatial correlation kernel for soft sensor/NR blotchiness
    float nL = hashGaussian(xi - 1, yi,     frameSlice, seed);
    float nR = hashGaussian(xi + 2, yi,     frameSlice, seed);
    float nU = hashGaussian(xi,     yi - 1, frameSlice, seed);
    float nD = hashGaussian(xi,     yi + 2, frameSlice, seed);
    float crossAvg = (bilinear * 2.0f + 0.25f * (nL + nR + nU + nD)) * 0.45f;
    return lerpf(bilinear, crossAvg, clamp01(softnessNorm));
}

} // namespace

float evaluateTonalGrainResponse(
    float luma,
    const CameraSpec& camSpec,
    const GrainParams& grainParams
) {
    float y = clamp01(luma);

    // 5-point editable curve: Black (0.0), Shadow (0.18), Midtone (0.50), Highlight (0.80), White (1.0)
    float pBlack     = clampf(grainParams.blackResponse * 0.01f, 0.0f, 2.0f);
    float pShadow    = clampf(grainParams.shadows * 0.01f, 0.0f, 2.5f);
    float pMidtone   = clampf(grainParams.midtones * 0.01f, 0.0f, 2.5f);
    float pHighlight = clampf(grainParams.highlights * 0.01f, 0.0f, 2.5f);
    float pWhite     = clampf(grainParams.whiteResponse * 0.01f, 0.0f, 2.0f);

    // Blend slightly with camera profile's native tonal curve
    pBlack     = lerpf(pBlack,     camSpec.tonalGrainCurve[0], 0.25f);
    pShadow    = lerpf(pShadow,    camSpec.tonalGrainCurve[1], 0.20f);
    pMidtone   = lerpf(pMidtone,   camSpec.tonalGrainCurve[2], 0.20f);
    pHighlight = lerpf(pHighlight, camSpec.tonalGrainCurve[3], 0.20f);
    pWhite     = lerpf(pWhite,     camSpec.tonalGrainCurve[4], 0.25f);

    float curveVal = 0.0f;
    if (y < 0.18f) {
        float t = smoothstepf(0.0f, 0.18f, y);
        curveVal = lerpf(pBlack, pShadow, t);
    } else if (y < 0.50f) {
        float t = smoothstepf(0.18f, 0.50f, y);
        curveVal = lerpf(pShadow, pMidtone, t);
    } else if (y < 0.80f) {
        float t = smoothstepf(0.50f, 0.80f, y);
        curveVal = lerpf(pMidtone, pHighlight, t);
    } else {
        float t = smoothstepf(0.80f, 1.0f, y);
        curveVal = lerpf(pHighlight, pWhite, t);
    }

    // Pure black and pure white fade so clipped extremes never receive an artificial opaque static overlay
    float shadowFloor = clamp01(grainParams.shadowFloor * 0.01f);
    float blackProtection = lerpf(shadowFloor * 0.35f, 1.0f, smoothstepf(0.0f, 0.045f, y));
    float whiteProtection = 1.0f - 0.85f * smoothstepf(0.92f, 1.0f, y);

    return std::max(0.0f, curveVal * blackProtection * whiteProtection);
}

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
) {
    GrainSampleResult res{};

    float amountNorm = clampf(grainParams.amount * 0.01f, 0.0f, 2.0f);
    if (amountNorm <= 1e-5f) {
        return res;
    }

    // 1. Resolution normalization relative to 1920x1080 so grain never becomes microscopic at 4K or huge at 720p
    float maxDim = static_cast<float>(std::max(1, std::max(width, height)));
    float resScale = clampf(maxDim / 1920.0f, 0.35f, 4.0f);

    // Size = physical particle scale in image space; Scale/Zoom = sampling zoom of generated texture
    float sizeFactor = lerpf(0.65f, 3.2f, clamp01(grainParams.size * 0.01f));
    float zoomFactor = clampf(grainParams.scaleZoom * 0.01f, 0.1f, 4.0f);
    float effectiveCellSize = std::max(0.55f, resScale * sizeFactor * zoomFactor);

    float sx = static_cast<float>(x) / effectiveCellSize;
    float sy = static_cast<float>(y) / effectiveCellSize;

    // 2. Temporal coordinates: Coherent 3D advected layer + controlled per-frame stochastic layer
    float animAmt = clamp01(grainParams.animation * 0.01f);
    float speedNorm = clampf(grainParams.speed * 0.01f, 0.0f, 2.5f);
    float coherence = clamp01(grainParams.temporalCoherence * 0.01f);

    float effectiveTime = (frameTimeSeconds > 0.0f)
        ? frameTimeSeconds
        : (static_cast<float>(frameIndex) / 24.0f);
    float tCoord = effectiveTime * speedNorm * animAmt * 6.5f;

    int32_t stochasticFrame = (animAmt > 0.001f && speedNorm > 0.001f)
        ? static_cast<int32_t>(std::floor(effectiveTime * 24.0f * std::max(0.25f, speedNorm))) + frameIndex
        : 0;

    uint32_t baseSeed = static_cast<uint32_t>(grainParams.seed) ^
                        (static_cast<uint32_t>(camSpec.model) * 0x45d9f3bU);

    // 3. Signal level & Poisson-Gaussian Shot + Read Noise Balance
    float luma = clamp01(luminance709(pixelR, pixelG, pixelB));
    float shotRead = clamp01(grainParams.shotReadBalance * 0.01f);
    float shadowFloor = clamp01(grainParams.shadowFloor * 0.01f);

    // Shot noise scales with sqrt(signal) in sensor domain, normalized to visible post-ISP tone curve
    float poissonEnv = std::sqrt(std::max(0.02f, luma)) * (1.0f - 0.55f * luma);
    // Read noise dominates dark regions
    float readEnv = (0.35f + 0.65f * shadowFloor) * (1.0f - 0.65f * smoothstepf(0.05f, 0.60f, luma));
    float sensorNoiseEnv = lerpf(readEnv, poissonEnv * 1.45f, shotRead);

    // 4. Tonal response curve weight
    float tonalWeight = evaluateTonalGrainResponse(luma, camSpec, grainParams);
    res.tonalWeight = tonalWeight;

    // 5. Camera-specific Denoise & Sharpening Interaction
    float darkMask = 1.0f - smoothstepf(0.04f, 0.48f, luma);
    float nrInteract = clamp01(grainParams.denoiseInteraction * 0.01f) * camSpec.lowLightDenoise;
    float sharpInteract = clamp01(grainParams.sharpeningInteraction * 0.01f) * camSpec.ispSharpening;

    // In low light, NR increases effective softness and suppresses ultra-fine grain while leaving coarse blotches
    float softnessNorm = clamp01(
        grainParams.softness * 0.01f +
        grainParams.spatialCorrelation * 0.0045f +
        nrInteract * darkMask * 0.42f
    );

    // 6. Performance Mode Dispatch
    if (grainParams.performanceMode == PerformanceMode::Draft) {
        // Fast single-layer preview noise (never silently used for Accurate final render)
        float fastLuma = filteredStochastic2D(sx, sy, stochasticFrame, softnessNorm, baseSeed);
        float amp = amountNorm * 0.11f * tonalWeight * (0.6f + 0.4f * sensorNoiseEnv);
        res.lumaGrain = fastLuma * amp;
        res.deltaR = res.lumaGrain;
        res.deltaG = res.lumaGrain;
        res.deltaB = res.lumaGrain;
        return res;
    }

    // 7. Fine Stochastic + Coherent Component
    float fineCoherent = coherentNoise3D(
        sx * 1.15f + 0.37f * tCoord,
        sy * 1.15f - 0.29f * tCoord,
        tCoord,
        baseSeed + 101U
    );
    float fineStochastic = filteredStochastic2D(
        sx * 1.10f,
        sy * 1.10f,
        stochasticFrame,
        softnessNorm * 0.75f,
        baseSeed + 202U
    );
    // Blend coherent 3D evolution with per-frame stochastic sensor component to avoid TV-static flicker
    float fineField = lerpf(fineStochastic, fineCoherent, coherence * 0.78f);

    // ISP sharpening accentuates fine/mid-frequency grain especially near detail transitions
    float edgeBoost = 1.0f + sharpInteract * (0.35f + 1.25f * clamp01(std::fabs(localEdgeDetail) * 4.0f));
    // Low-light NR dampens fine grain in dark regions
    float fineNRDamp = 1.0f - nrInteract * darkMask * 0.52f;
    float fineWeight = clamp01(grainParams.fineGrain * 0.01f) * edgeBoost * fineNRDamp;

    // 8. Coarse Clustered Low-Light Component
    float coarseFreq = 1.0f / lerpf(2.2f, 4.8f, clamp01(grainParams.clustering * 0.01f));
    float coarseField = coherentNoise3D(
        sx * coarseFreq - 0.15f * tCoord,
        sy * coarseFreq + 0.18f * tCoord,
        tCoord * 0.55f,
        baseSeed + 303U
    );

    if (grainParams.performanceMode == PerformanceMode::Accurate) {
        float coarseSub = filteredStochastic2D(
            sx * coarseFreq * 1.35f,
            sy * coarseFreq * 1.35f,
            stochasticFrame,
            0.85f,
            baseSeed + 404U
        );
        coarseField = lerpf(coarseSub, coarseField, 0.35f + 0.65f * coherence);
    }

    // Density & Clustering shape how frequently visible low-light clusters occur
    float densityNorm = clamp01(grainParams.density * 0.01f);
    float clusterNorm = clamp01(grainParams.clustering * 0.01f);
    float clusterGate = smoothstepf(0.85f - densityNorm * 0.75f, 1.35f - densityNorm * 0.45f, std::fabs(coarseField) + 0.35f * densityNorm);
    float shapedCoarse = coarseField * lerpf(1.0f, clusterGate * 1.45f, clusterNorm);
    // Low-light NR leaves behind low-frequency blotches in shadows
    float coarseNRBoost = 1.0f + nrInteract * darkMask * 0.45f;
    float coarseWeight = clamp01(grainParams.coarseGrain * 0.01f) * coarseNRBoost;

    // 9. Sensor Pattern / PRNU-like Fixed-Pattern Non-Uniformity Component
    float prnuField = 0.0f;
    float patternAmt = clamp01(grainParams.patternAmount * 0.01f);
    if (patternAmt > 0.005f && grainParams.performanceMode == PerformanceMode::Accurate) {
        float patScale = std::max(0.25f, (grainParams.patternScale * 0.01f) * 3.5f);
        // Very slow temporal drift so PRNU feels anchored to sensor geometry without looking frozen or tiled
        float slowDrift = effectiveTime * 0.12f * animAmt;
        prnuField = coherentNoise3D(
            (static_cast<float>(x) / (resScale * 18.0f * patScale)),
            (static_cast<float>(y) / (resScale * 18.0f * patScale)),
            slowDrift,
            baseSeed + 505U
        ) * patternAmt * 0.38f;
    }

    // Combine luminance grain components
    float combinedLumaNoise = (fineField * fineWeight * 0.72f + shapedCoarse * coarseWeight * 0.58f + prnuField);

    // Scale by master amount, camera base noise character, tonal weight, and sensor shot/read envelope
    float cameraNoiseFactor = 0.72f + 0.95f * camSpec.baseSensorNoise;
    float masterAmplitude = amountNorm * 0.135f * cameraNoiseFactor * tonalWeight * (0.52f + 0.48f * sensorNoiseEnv);

    float lumaBalance = clamp01(grainParams.lumaChromaBalance * 0.01f);
    float lumaGrain = combinedLumaNoise * masterAmplitude * (0.35f + 0.65f * lumaBalance);
    res.lumaGrain = lumaGrain;

    // 10. Correlated Chroma Noise in YCbCr / Opponent Space
    float chromaUserAmt = clamp01(grainParams.chromaAmount * 0.01f);
    float chromaWeight = chromaUserAmt * (1.0f - 0.65f * lumaBalance);

    if (grainParams.colorMode == GrainColorMode::Monochrome ||
        grainParams.colorMode == GrainColorMode::NeutralLuma ||
        chromaWeight <= 0.002f) {
        // Pure luminance grain
        if (grainParams.colorMode == GrainColorMode::NeutralLuma) {
            // Preserve local chromaticity ratio slightly while modulating luminance
            res.deltaR = lumaGrain * (0.7f + 0.3f * pixelR);
            res.deltaG = lumaGrain * (0.7f + 0.3f * pixelG);
            res.deltaB = lumaGrain * (0.7f + 0.3f * pixelB);
        } else {
            res.deltaR = lumaGrain;
            res.deltaG = lumaGrain;
            res.deltaB = lumaGrain;
        }
        return res;
    }

    // Chroma noise is spatially broader (demosaicing + ISP chroma NR correlation)
    float chromaCorr = clamp01(grainParams.chromaCorrelation * 0.01f);
    float chromaCellScale = 1.0f / lerpf(1.6f, 4.2f, chromaCorr);
    float cx = sx * chromaCellScale;
    float cy = sy * chromaCellScale;
    float ct = tCoord * 0.65f;

    float cbRaw = coherentNoise3D(cx + 13.7f, cy - 9.2f, ct, baseSeed + 606U);
    float crRaw = coherentNoise3D(cx - 21.4f, cy + 17.8f, ct, baseSeed + 707U);

    if (grainParams.performanceMode == PerformanceMode::Accurate) {
        float cbStoch = filteredStochastic2D(cx, cy, stochasticFrame, 0.85f, baseSeed + 808U);
        float crStoch = filteredStochastic2D(cx, cy, stochasticFrame, 0.85f, baseSeed + 909U);
        cbRaw = lerpf(cbStoch, cbRaw, 0.45f + 0.55f * coherence);
        crRaw = lerpf(crStoch, crRaw, 0.45f + 0.55f * coherence);
    }

    // Apply color mode & hue bias (2014 low-light iPhone references show violet/magenta-blue shadow chroma blotches)
    float hueBias = grainParams.chromaHueBias * 0.01f;
    float modeCbScale = 1.0f;
    float modeCrScale = 1.0f;
    float modeBiasCb = 0.0f;
    float modeBiasCr = 0.0f;

    switch (grainParams.colorMode) {
        case GrainColorMode::WarmChroma:
            modeCbScale = 0.70f;
            modeCrScale = 1.25f;
            modeBiasCb = -0.10f;
            modeBiasCr =  0.16f;
            break;
        case GrainColorMode::CoolChroma:
            modeCbScale = 1.30f;
            modeCrScale = 0.80f;
            modeBiasCb =  0.18f;
            modeBiasCr = -0.08f;
            break;
        case GrainColorMode::FullChroma:
            modeCbScale = 1.25f;
            modeCrScale = 1.25f;
            break;
        case GrainColorMode::CameraProfile:
        default:
            // Camera-specific chroma character: iPhone 5/5s/6 have stronger blue-magenta low-light chroma blotches;
            // iPhone 6s/7/8 have tighter ISP chroma suppression.
            modeCbScale = 1.15f * (1.35f - camSpec.lumaChromaBalance);
            modeCrScale = 1.05f * (1.35f - camSpec.lumaChromaBalance);
            modeBiasCb = -hueBias * 0.14f + 0.05f * darkMask;
            modeBiasCr =  hueBias * 0.14f + 0.04f * darkMask;
            break;
    }

    // Chroma noise is strongest in dark/shadow regions where sensor read noise differs across R/G/B channels
    float chromaShadowBoost = 0.65f + 0.75f * darkMask;
    float chromaAmp = masterAmplitude * chromaWeight * 0.85f * chromaShadowBoost;

    float cbGrain = (cbRaw * modeCbScale + modeBiasCb * std::fabs(cbRaw)) * chromaAmp;
    float crGrain = (crRaw * modeCrScale + modeBiasCr * std::fabs(crRaw)) * chromaAmp;
    res.cbGrain = cbGrain;
    res.crGrain = crGrain;

    // Convert YCbCr grain perturbation back to RGB delta (BT.709 opponent conversion)
    res.deltaR = lumaGrain + 1.5748f * crGrain;
    res.deltaG = lumaGrain - 0.1873f * cbGrain - 0.4681f * crGrain;
    res.deltaB = lumaGrain + 1.8556f * cbGrain;

    return res;
}

} // namespace y2014
