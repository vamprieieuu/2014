#include "2014_engine.h"

namespace y2014 {

namespace {

static const std::array<CameraSpec, static_cast<size_t>(CameraModel::Count)> kCameraSpecs = {{
    // 0: iPhone 5 (2012) - 8MP 1/3.2" 1.4um f/2.4, strong HDR exposure blending, occasional ghosting/fringing
    {
        CameraModel::IPhone5,
        "iPhone 5",
        "8MP 1/3.2\" 1.4um f/2.4 (iOS 6/7 ISP)",
        0.42f, // highlightRollOff
        0.05f, // shadowLift
        0.16f, // microContrast
        0.38f, // hdrBlendStrength
        0.14f, // hdrGhostFringe
        0.08f, // colorQuantization
        0.06f, // warmthBias
       -0.04f, // tintBias
        0.94f, // saturationResponse
        0.00f, // wideColorGamut
        0.11f, // colorShadingEdge
        {
            1.04f, -0.02f, -0.02f,
           -0.01f,  1.01f,  0.00f,
            0.01f, -0.04f,  1.03f
        },
        0.48f, // shotReadBalance (more read noise in dark areas)
        0.62f, // lumaChromaBalance
        0.34f, // baseSensorNoise
        0.42f, // lowLightDenoise
        0.45f, // detailLossLowLight
        0.42f, // ispSharpening
        0.22f, // ispRinging
        0.70f, // chromaCorrelation
        0.16f, // prnuPatternAmount
        0.18f, // jpegBlockResidue
        0.22f, // lensVignette
        0.20f, // cornerSoftness
        0.58f, // defaultTemporalCoherence
        { 0.26f, 0.96f, 0.64f, 0.28f, 0.08f }
    },
    // 1: iPhone 5s (2013) - 8MP 1/3" 1.5um f/2.2, light NR at low ISO, visible dark luma noise, strong low-light NR detail loss
    {
        CameraModel::IPhone5s,
        "iPhone 5s",
        "8MP 1/3\" 1.5um f/2.2 (A7 ISP)",
        0.36f, // highlightRollOff
        0.04f, // shadowLift
        0.15f, // microContrast
        0.30f, // hdrBlendStrength
        0.10f, // hdrGhostFringe
        0.06f, // colorQuantization
        0.05f, // warmthBias
       -0.02f, // tintBias
        0.96f, // saturationResponse
        0.00f, // wideColorGamut
        0.09f, // colorShadingEdge
        {
            1.03f, -0.015f, -0.015f,
           -0.01f,  1.015f, -0.005f,
            0.00f, -0.025f,  1.025f
        },
        0.54f, // shotReadBalance
        0.68f, // lumaChromaBalance
        0.30f, // baseSensorNoise
        0.52f, // lowLightDenoise
        0.48f, // detailLossLowLight
        0.38f, // ispSharpening
        0.18f, // ispRinging
        0.66f, // chromaCorrelation
        0.14f, // prnuPatternAmount
        0.15f, // jpegBlockResidue
        0.19f, // lensVignette
        0.17f, // cornerSoftness
        0.62f, // defaultTemporalCoherence
        { 0.24f, 0.92f, 0.58f, 0.24f, 0.06f }
    },
    // 2: iPhone 6 (2014) - 8MP 1/3" 1.5um f/2.2 Focus Pixels, luma noise in low light, HDR quantization/ghosting/fringing
    {
        CameraModel::IPhone6,
        "iPhone 6",
        "8MP 1/3\" 1.5um f/2.2 Focus Pixels (A8 ISP)",
        0.34f, // highlightRollOff
        0.04f, // shadowLift
        0.14f, // microContrast
        0.32f, // hdrBlendStrength
        0.12f, // hdrGhostFringe
        0.09f, // colorQuantization
        0.04f, // warmthBias
       -0.01f, // tintBias
        0.98f, // saturationResponse
        0.00f, // wideColorGamut
        0.08f, // colorShadingEdge
        {
            1.025f, -0.015f, -0.010f,
           -0.008f,  1.012f, -0.004f,
            0.002f, -0.020f,  1.018f
        },
        0.58f, // shotReadBalance
        0.72f, // lumaChromaBalance
        0.26f, // baseSensorNoise
        0.50f, // lowLightDenoise
        0.42f, // detailLossLowLight
        0.36f, // ispSharpening
        0.16f, // ispRinging
        0.64f, // chromaCorrelation
        0.12f, // prnuPatternAmount
        0.13f, // jpegBlockResidue
        0.17f, // lensVignette
        0.15f, // cornerSoftness
        0.64f, // defaultTemporalCoherence
        { 0.20f, 0.88f, 0.55f, 0.22f, 0.05f }
    },
    // 3: iPhone 6s (2015) - 12MP 1.22um f/2.2, low chroma noise, visible low-light luma noise, detail loss, slight ringing
    {
        CameraModel::IPhone6s,
        "iPhone 6s",
        "12MP 1/3\" 1.22um f/2.2 Deep Trench Isolation (A9 ISP)",
        0.31f, // highlightRollOff
        0.03f, // shadowLift
        0.15f, // microContrast
        0.28f, // hdrBlendStrength
        0.09f, // hdrGhostFringe
        0.05f, // colorQuantization
        0.03f, // warmthBias
       -0.01f, // tintBias
        0.99f, // saturationResponse
        0.00f, // wideColorGamut
        0.10f, // colorShadingEdge
        {
            1.020f, -0.010f, -0.010f,
           -0.006f,  1.010f, -0.004f,
            0.000f, -0.015f,  1.015f
        },
        0.60f, // shotReadBalance
        0.80f, // lumaChromaBalance (low chroma noise, prominent luma noise)
        0.27f, // baseSensorNoise (smaller 1.22um pixels)
        0.56f, // lowLightDenoise
        0.46f, // detailLossLowLight
        0.40f, // ispSharpening
        0.21f, // ispRinging (documented slight ringing)
        0.60f, // chromaCorrelation
        0.11f, // prnuPatternAmount
        0.11f, // jpegBlockResidue
        0.16f, // lensVignette
        0.14f, // cornerSoftness
        0.66f, // defaultTemporalCoherence
        { 0.19f, 0.86f, 0.54f, 0.21f, 0.05f }
    },
    // 4: iPhone 7 (2016) - 12MP f/1.8 OIS, wide color, advanced ISP, low-light luma noise + fine detail loss
    {
        CameraModel::IPhone7,
        "iPhone 7",
        "12MP 1/3\" f/1.8 OIS Wide Color (A10 ISP)",
        0.28f, // highlightRollOff
        0.03f, // shadowLift
        0.13f, // microContrast
        0.25f, // hdrBlendStrength
        0.06f, // hdrGhostFringe
        0.04f, // colorQuantization
        0.03f, // warmthBias
        0.00f, // tintBias
        1.03f, // saturationResponse
        0.18f, // wideColorGamut
        0.06f, // colorShadingEdge
        {
            1.030f, -0.018f, -0.012f,
           -0.010f,  1.022f, -0.012f,
           -0.004f, -0.016f,  1.020f
        },
        0.64f, // shotReadBalance
        0.78f, // lumaChromaBalance
        0.21f, // baseSensorNoise
        0.58f, // lowLightDenoise
        0.40f, // detailLossLowLight
        0.33f, // ispSharpening
        0.14f, // ispRinging
        0.58f, // chromaCorrelation
        0.09f, // prnuPatternAmount
        0.09f, // jpegBlockResidue
        0.13f, // lensVignette
        0.12f, // cornerSoftness
        0.70f, // defaultTemporalCoherence
        { 0.16f, 0.78f, 0.48f, 0.19f, 0.04f }
    },
    // 5: iPhone 7 Plus (2016) - Dual 12MP f/1.8 Wide + f/2.8 Tele, multi-frame low-light stacking
    {
        CameraModel::IPhone7Plus,
        "iPhone 7 Plus",
        "Dual 12MP f/1.8 Wide + Multi-Frame Low-Light ISP",
        0.27f, // highlightRollOff
        0.03f, // shadowLift
        0.14f, // microContrast
        0.28f, // hdrBlendStrength
        0.07f, // hdrGhostFringe
        0.04f, // colorQuantization
        0.03f, // warmthBias
        0.00f, // tintBias
        1.04f, // saturationResponse
        0.20f, // wideColorGamut
        0.06f, // colorShadingEdge
        {
            1.032f, -0.018f, -0.014f,
           -0.010f,  1.024f, -0.014f,
           -0.004f, -0.016f,  1.020f
        },
        0.66f, // shotReadBalance
        0.80f, // lumaChromaBalance
        0.19f, // baseSensorNoise
        0.62f, // lowLightDenoise (multi-frame low-light smoothing)
        0.42f, // detailLossLowLight
        0.34f, // ispSharpening
        0.14f, // ispRinging
        0.60f, // chromaCorrelation
        0.08f, // prnuPatternAmount
        0.08f, // jpegBlockResidue
        0.12f, // lensVignette
        0.11f, // cornerSoftness
        0.74f, // defaultTemporalCoherence
        { 0.15f, 0.75f, 0.46f, 0.18f, 0.04f }
    },
    // 6: iPhone 8 (2017) - Larger/faster 12MP sensor, new color filter, deeper pixels, OIS, A11 Bionic ISP
    {
        CameraModel::IPhone8,
        "iPhone 8",
        "12MP Larger/Faster Sensor f/1.8 Deeper Pixels (A11 ISP)",
        0.24f, // highlightRollOff
        0.02f, // shadowLift
        0.12f, // microContrast
        0.22f, // hdrBlendStrength
        0.04f, // hdrGhostFringe
        0.02f, // colorQuantization
        0.04f, // warmthBias
        0.01f, // tintBias
        1.05f, // saturationResponse
        0.25f, // wideColorGamut
        0.04f, // colorShadingEdge
        {
            1.035f, -0.020f, -0.015f,
           -0.012f,  1.028f, -0.016f,
           -0.006f, -0.014f,  1.020f
        },
        0.72f, // shotReadBalance
        0.82f, // lumaChromaBalance
        0.15f, // baseSensorNoise
        0.54f, // lowLightDenoise
        0.30f, // detailLossLowLight
        0.28f, // ispSharpening
        0.10f, // ispRinging
        0.54f, // chromaCorrelation
        0.06f, // prnuPatternAmount
        0.06f, // jpegBlockResidue
        0.10f, // lensVignette
        0.09f, // cornerSoftness
        0.76f, // defaultTemporalCoherence
        { 0.12f, 0.66f, 0.40f, 0.15f, 0.03f }
    },
    // 7: iPhone 8 Plus (2017) - Dual 12MP f/1.8 + f/2.8, A11 Bionic advanced pixel processing & multiband NR
    {
        CameraModel::IPhone8Plus,
        "iPhone 8 Plus",
        "Dual 12MP f/1.8 + f/2.8 A11 Multiband ISP",
        0.23f, // highlightRollOff
        0.02f, // shadowLift
        0.12f, // microContrast
        0.22f, // hdrBlendStrength
        0.03f, // hdrGhostFringe
        0.02f, // colorQuantization
        0.04f, // warmthBias
        0.01f, // tintBias
        1.06f, // saturationResponse
        0.26f, // wideColorGamut
        0.04f, // colorShadingEdge
        {
            1.036f, -0.020f, -0.016f,
           -0.012f,  1.028f, -0.016f,
           -0.006f, -0.014f,  1.020f
        },
        0.74f, // shotReadBalance
        0.84f, // lumaChromaBalance
        0.14f, // baseSensorNoise
        0.56f, // lowLightDenoise
        0.28f, // detailLossLowLight
        0.27f, // ispSharpening
        0.09f, // ispRinging
        0.52f, // chromaCorrelation
        0.05f, // prnuPatternAmount
        0.05f, // jpegBlockResidue
        0.09f, // lensVignette
        0.08f, // cornerSoftness
        0.78f, // defaultTemporalCoherence
        { 0.11f, 0.64f, 0.38f, 0.14f, 0.03f }
    }
}};

} // namespace

const CameraSpec& getCameraSpec(CameraModel model) {
    int idx = static_cast<int>(model);
    if (idx < 0 || idx >= static_cast<int>(CameraModel::Count)) {
        idx = static_cast<int>(CameraModel::IPhone5s);
    }
    return kCameraSpecs[static_cast<size_t>(idx)];
}

const char* getCameraModelName(CameraModel model) {
    return getCameraSpec(model).name;
}

void applyCameraDefaultsToGrain(CameraModel model, GrainParams& grain) {
    const CameraSpec& spec = getCameraSpec(model);
    grain.shotReadBalance = spec.shotReadBalance * 100.0f;
    grain.lumaChromaBalance = spec.lumaChromaBalance * 100.0f;
    grain.denoiseInteraction = spec.lowLightDenoise * 100.0f;
    grain.sharpeningInteraction = spec.ispSharpening * 100.0f;
    grain.chromaCorrelation = spec.chromaCorrelation * 100.0f;
    grain.patternAmount = spec.prnuPatternAmount * 100.0f;
    grain.temporalCoherence = spec.defaultTemporalCoherence * 100.0f;
    grain.blackResponse = spec.tonalGrainCurve[0] * 100.0f;
    grain.shadows = spec.tonalGrainCurve[1] * 100.0f;
    grain.midtones = spec.tonalGrainCurve[2] * 100.0f;
    grain.highlights = spec.tonalGrainCurve[3] * 100.0f;
    grain.whiteResponse = spec.tonalGrainCurve[4] * 100.0f;
}

void evaluateCameraPixel(
    float inR, float inG, float inB,
    float normX, float normY,
    const CameraSpec& spec,
    const CameraParams& params,
    float localLumaMean,
    float localDetail,
    float& outR, float& outG, float& outB
) {
    float strength = clamp01(params.profileStrength * 0.01f);
    if (strength <= 1e-5f) {
        outR = inR;
        outG = inG;
        outB = inB;
        return;
    }

    // 1. Exposure trim in camera domain
    float expStops = (params.exposureTrim / 100.0f) * 1.25f;
    float expGain = std::pow(2.0f, expStops);
    float r = inR * expGain;
    float g = inG * expGain;
    float b = inB * expGain;

    // 2. Camera sensor color matrix & warmth/tint response
    float mR = spec.colorMatrix[0] * r + spec.colorMatrix[1] * g + spec.colorMatrix[2] * b;
    float mG = spec.colorMatrix[3] * r + spec.colorMatrix[4] * g + spec.colorMatrix[5] * b;
    float mB = spec.colorMatrix[6] * r + spec.colorMatrix[7] * g + spec.colorMatrix[8] * b;

    float warmTotal = spec.warmthBias + (params.cameraWarmth / 100.0f) * 0.22f;
    float tintTotal = spec.tintBias + (params.cameraTint / 100.0f) * 0.18f;
    mR *= (1.0f + warmTotal * 0.35f + tintTotal * 0.15f);
    mG *= (1.0f - tintTotal * 0.25f);
    mB *= (1.0f - warmTotal * 0.35f + tintTotal * 0.12f);

    // 3. Sensor edge color shading (slight warm/cool radial shift observed on early iPhone lenses)
    float dx = normX - 0.5f;
    float dy = normY - 0.5f;
    float r2 = dx * dx + dy * dy;
    float radial = clamp01(r2 * 2.0f);
    if (spec.colorShadingEdge > 0.0f) {
        float shade = spec.colorShadingEdge * radial;
        mR *= (1.0f - shade * 0.25f);
        mG *= (1.0f + shade * 0.10f);
        mB *= (1.0f + shade * 0.20f);
    }

    // 4. Camera HDR exposure-blending & highlight rolloff
    float hdrScale = clampf(params.hdrCharacter / 50.0f, 0.0f, 2.0f);
    float toneScale = clampf(params.toneCompression / 50.0f, 0.0f, 2.0f);
    float rollOff = spec.highlightRollOff * toneScale;
    float hdrBlend = spec.hdrBlendStrength * hdrScale;

    auto applyCameraTone = [&](float c, float localMean) -> float {
        c = std::max(0.0f, c);
        // Shadow lift characteristic of phone auto-exposure in dim scenes
        float shadowFactor = (1.0f - smoothstepf(0.0f, 0.35f, localMean));
        c += spec.shadowLift * toneScale * shadowFactor * (1.0f - c);

        // Dual-exposure HDR highlight recovery approximation
        float compressed = c / (1.0f + rollOff * c) * (1.0f + rollOff);
        float hdrRecovered = compressed - hdrBlend * 0.18f * smoothstepf(0.55f, 1.0f, c) * (c - localMean);
        // Micro-contrast S-curve
        float centered = hdrRecovered - 0.5f;
        float sCurve = hdrRecovered + spec.microContrast * centered * (1.0f - 4.0f * centered * centered);
        return clamp01(sCurve);
    };

    float tR = applyCameraTone(mR, localLumaMean);
    float tG = applyCameraTone(mG, localLumaMean);
    float tB = applyCameraTone(mB, localLumaMean);

    // 5. Subtle HDR ghost/chroma fringing & quantization on high-contrast transitions (iPhone 5/6)
    float edgeMag = std::fabs(localDetail);
    if (spec.hdrGhostFringe > 0.0f && hdrScale > 0.0f && edgeMag > 0.04f) {
        float fringe = spec.hdrGhostFringe * hdrScale * smoothstepf(0.04f, 0.35f, edgeMag);
        tR = clamp01(tR + fringe * 0.06f * localDetail);
        tB = clamp01(tB - fringe * 0.08f * localDetail);
    }

    float jpegScale = clampf(params.jpegEraArtifact / 50.0f, 0.0f, 2.0f);
    float quantStrength = (spec.colorQuantization * hdrScale + spec.jpegBlockResidue * 0.25f * jpegScale);
    if (quantStrength > 0.02f) {
        float levels = lerpf(256.0f, 72.0f, clamp01(quantStrength));
        tR = std::floor(tR * levels + 0.5f) / levels;
        tG = std::floor(tG * levels + 0.5f) / levels;
        tB = std::floor(tB * levels + 0.5f) / levels;
    }

    // 6. Camera-specific low-light denoising vs ISP sharpening & ringing
    float luma = luminance709(tR, tG, tB);
    float darkWeight = 1.0f - smoothstepf(0.05f, 0.48f, luma);
    float nrUserScale = clampf(params.lowLightDenoise / 50.0f, 0.0f, 2.0f);
    float sharpUserScale = clampf(params.ispSharpening / 50.0f, 0.0f, 2.0f);
    float softEdgeScale = clampf(params.edgeSoftness / 50.0f, 0.0f, 2.0f);

    // In low light, iPhone 5s/6/6s/7 NR suppresses fine detail; in well-lit edges, ISP sharpening & slight ringing dominate
    float detailLoss = clamp01(spec.detailLossLowLight * spec.lowLightDenoise * nrUserScale * darkWeight);
    float cornerSoft = clamp01(spec.cornerSoftness * softEdgeScale * radial);
    float totalSmoothing = clamp01(detailLoss * 0.65f + cornerSoft * 0.55f);

    float sharpGain = spec.ispSharpening * sharpUserScale * (1.0f - detailLoss * 0.7f) * (1.0f - cornerSoft * 0.5f);
    float ringHalo = spec.ispRinging * sharpUserScale * smoothstepf(0.08f, 0.30f, edgeMag);
    float netDetailDelta = localDetail * (sharpGain + ringHalo * 0.35f - totalSmoothing);

    tR = clamp01(tR + netDetailDelta);
    tG = clamp01(tG + netDetailDelta);
    tB = clamp01(tB + netDetailDelta);

    // 7. Saturation / Wide-color response
    float newLuma = luminance709(tR, tG, tB);
    float satMult = spec.saturationResponse + spec.wideColorGamut * 0.15f * smoothstepf(0.15f, 0.60f, std::max({tR, tG, tB}) - std::min({tR, tG, tB}));
    // Low-light chroma suppression
    satMult *= (1.0f - 0.18f * darkWeight * spec.lowLightDenoise * nrUserScale);
    tR = clamp01(newLuma + (tR - newLuma) * satMult);
    tG = clamp01(newLuma + (tG - newLuma) * satMult);
    tB = clamp01(newLuma + (tB - newLuma) * satMult);

    // 8. Optical lens falloff / vignette
    float vigScale = clampf(params.lensVignette / 50.0f, 0.0f, 2.0f);
    float vigAmount = spec.lensVignette * vigScale;
    float vigFactor = std::max(0.0f, 1.0f - vigAmount * smoothstepf(0.15f, 1.05f, radial));
    tR *= vigFactor;
    tG *= vigFactor;
    tB *= vigFactor;

    outR = lerpf(inR, tR, strength);
    outG = lerpf(inG, tG, strength);
    outB = lerpf(inB, tB, strength);
}

} // namespace y2014
