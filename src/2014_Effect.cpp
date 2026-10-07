#include "2014_Effect.h"

#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace {

static void CopyParamName(A_char* dst, const char* src) {
    if (!dst) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    std::strncpy(dst, src, PF_MAX_EFFECT_PARAM_NAME_LEN);
    dst[PF_MAX_EFFECT_PARAM_NAME_LEN] = '\0';
}

static PF_Err AddTopicGroup(
    PF_InData* in_data,
    const char* name,
    A_long disk_id,
    bool start_collapsed = false
) {
    PF_ParamDef def;
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_GROUP_START;
    CopyParamName(def.name, name);
    def.uu.id = disk_id;
    def.flags = start_collapsed ? PF_ParamFlag_START_COLLAPSED : 0;
    return PF_ADD_PARAM(in_data, -1, &def);
}

static PF_Err EndTopicGroup(
    PF_InData* in_data,
    A_long disk_id
) {
    PF_ParamDef def;
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_GROUP_END;
    def.uu.id = disk_id;
    return PF_ADD_PARAM(in_data, -1, &def);
}

static PF_Err AddPopupControl(
    PF_InData* in_data,
    const char* name,
    A_short num_choices,
    A_short default_choice,
    const char* choices_str,
    A_long disk_id,
    bool supervise = false
) {
    PF_ParamDef def;
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_POPUP;
    CopyParamName(def.name, name);
    def.uu.id = disk_id;
    def.flags = supervise ? PF_ParamFlag_SUPERVISE : 0;
    def.u.pd.num_choices = num_choices;
    def.u.pd.dephault = default_choice;
    def.u.pd.value = default_choice;
    def.u.pd.u.namesptr = choices_str;
    return PF_ADD_PARAM(in_data, -1, &def);
}

static PF_Err AddFloatSliderControl(
    PF_InData* in_data,
    const char* name,
    PF_FpShort valid_min,
    PF_FpShort valid_max,
    PF_FpShort slider_min,
    PF_FpShort slider_max,
    PF_FpShort default_val,
    A_short precision,
    PF_ValueDisplayFlags display_flags,
    A_long disk_id
) {
    PF_ParamDef def;
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_FLOAT_SLIDER;
    CopyParamName(def.name, name);
    def.uu.id = disk_id;
    def.u.fs_d.valid_min = valid_min;
    def.u.fs_d.valid_max = valid_max;
    def.u.fs_d.slider_min = slider_min;
    def.u.fs_d.slider_max = slider_max;
    def.u.fs_d.value = default_val;
    def.u.fs_d.dephault = default_val;
    def.u.fs_d.precision = precision;
    def.u.fs_d.display_flags = display_flags;
    def.u.fs_d.fs_flags = PF_FSliderFlag_NONE;
    def.u.fs_d.curve_tolerance = AEFX_AUDIO_DEFAULT_CURVE_TOLERANCE;
    return PF_ADD_PARAM(in_data, -1, &def);
}

static PF_Err AddIntSliderControl(
    PF_InData* in_data,
    const char* name,
    PF_ParamValue valid_min,
    PF_ParamValue valid_max,
    PF_ParamValue slider_min,
    PF_ParamValue slider_max,
    PF_ParamValue default_val,
    A_long disk_id
) {
    PF_ParamDef def;
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_SLIDER;
    CopyParamName(def.name, name);
    def.uu.id = disk_id;
    def.u.sd.valid_min = valid_min;
    def.u.sd.valid_max = valid_max;
    def.u.sd.slider_min = slider_min;
    def.u.sd.slider_max = slider_max;
    def.u.sd.value = default_val;
    def.u.sd.dephault = default_val;
    return PF_ADD_PARAM(in_data, -1, &def);
}

static void UnionLRectLocal(const PF_LRect* src, PF_LRect* dst) {
    if (!src || !dst) return;
    if (dst->left == 0 && dst->top == 0 && dst->right == 0 && dst->bottom == 0) {
        *dst = *src;
        return;
    }
    dst->left   = std::min(dst->left, src->left);
    dst->top    = std::min(dst->top, src->top);
    dst->right  = std::max(dst->right, src->right);
    dst->bottom = std::max(dst->bottom, src->bottom);
}

static y2014::PipelineConfig ExtractPipelineConfigFromParams(
    PF_InData* in_data,
    PF_ParamDef* params[]
) {
    y2014::PipelineConfig cfg{};

    if (in_data) {
        A_long step = (in_data->time_step > 0) ? in_data->time_step : 1;
        cfg.frameIndex = static_cast<int>(in_data->current_time / step);
        if (in_data->time_scale > 0) {
            cfg.frameTimeSeconds = static_cast<float>(in_data->current_time) / static_cast<float>(in_data->time_scale);
        } else {
            cfg.frameTimeSeconds = static_cast<float>(cfg.frameIndex) / 24.0f;
        }
    }

    if (!params) {
        return cfg;
    }

    // 1. Camera Model
    int camPopup = static_cast<int>(params[Y2014_CAMERA_MODEL]->u.pd.value) - 1;
    camPopup = std::max(0, std::min(static_cast<int>(y2014::CameraModel::Count) - 1, camPopup));
    cfg.camera.model            = static_cast<y2014::CameraModel>(camPopup);
    cfg.camera.profileStrength  = static_cast<float>(params[Y2014_CAMERA_STRENGTH]->u.fs_d.value);
    cfg.camera.exposureTrim     = static_cast<float>(params[Y2014_CAMERA_EXPOSURE_TRIM]->u.fs_d.value);
    cfg.camera.hdrCharacter     = static_cast<float>(params[Y2014_CAMERA_HDR_CHARACTER]->u.fs_d.value);
    cfg.camera.toneCompression  = static_cast<float>(params[Y2014_CAMERA_TONE_COMPRESSION]->u.fs_d.value);
    cfg.camera.cameraWarmth     = static_cast<float>(params[Y2014_CAMERA_WARMTH]->u.fs_d.value);
    cfg.camera.cameraTint       = static_cast<float>(params[Y2014_CAMERA_TINT]->u.fs_d.value);
    cfg.camera.lowLightDenoise  = static_cast<float>(params[Y2014_CAMERA_LOWLIGHT_DENOISE]->u.fs_d.value);
    cfg.camera.ispSharpening    = static_cast<float>(params[Y2014_CAMERA_ISP_SHARPENING]->u.fs_d.value);
    cfg.camera.lensVignette     = static_cast<float>(params[Y2014_CAMERA_LENS_VIGNETTE]->u.fs_d.value);
    cfg.camera.edgeSoftness     = static_cast<float>(params[Y2014_CAMERA_EDGE_SOFTNESS]->u.fs_d.value);
    cfg.camera.jpegEraArtifact  = static_cast<float>(params[Y2014_CAMERA_JPEG_ARTIFACT]->u.fs_d.value);

    // 2. 2014 Filter
    int fltPopup = static_cast<int>(params[Y2014_FILTER_PRESET]->u.pd.value) - 1;
    fltPopup = std::max(0, std::min(static_cast<int>(y2014::FilterPreset::Count) - 1, fltPopup));
    y2014::FilterPreset preset  = static_cast<y2014::FilterPreset>(fltPopup);
    y2014::FilterParams presetBase = y2014::getFilterPresetParams(preset);

    cfg.filter.preset           = preset;
    cfg.filter.filterStrength   = static_cast<float>(params[Y2014_FILTER_STRENGTH]->u.fs_d.value);
    cfg.filter.exposure         = static_cast<float>(params[Y2014_FILTER_EXPOSURE]->u.fs_d.value);
    cfg.filter.brilliance       = static_cast<float>(params[Y2014_FILTER_BRILLIANCE]->u.fs_d.value);
    cfg.filter.highlights       = static_cast<float>(params[Y2014_FILTER_HIGHLIGHTS]->u.fs_d.value);
    cfg.filter.shadows          = static_cast<float>(params[Y2014_FILTER_SHADOWS]->u.fs_d.value);
    cfg.filter.contrast         = static_cast<float>(params[Y2014_FILTER_CONTRAST]->u.fs_d.value);
    cfg.filter.brightness       = static_cast<float>(params[Y2014_FILTER_BRIGHTNESS]->u.fs_d.value);
    cfg.filter.blackPoint       = static_cast<float>(params[Y2014_FILTER_BLACK_POINT]->u.fs_d.value);
    cfg.filter.fade             = static_cast<float>(params[Y2014_FILTER_FADE]->u.fs_d.value);
    cfg.filter.saturation       = static_cast<float>(params[Y2014_FILTER_SATURATION]->u.fs_d.value);
    cfg.filter.vibrance         = static_cast<float>(params[Y2014_FILTER_VIBRANCE]->u.fs_d.value);
    cfg.filter.warmth           = static_cast<float>(params[Y2014_FILTER_WARMTH]->u.fs_d.value);
    cfg.filter.tint             = static_cast<float>(params[Y2014_FILTER_TINT]->u.fs_d.value);
    cfg.filter.sharpness        = static_cast<float>(params[Y2014_FILTER_SHARPNESS]->u.fs_d.value);
    cfg.filter.definition       = static_cast<float>(params[Y2014_FILTER_DEFINITION]->u.fs_d.value);
    cfg.filter.vignette         = static_cast<float>(params[Y2014_FILTER_VIGNETTE]->u.fs_d.value);
    cfg.filter.centerBrightness = static_cast<float>(params[Y2014_FILTER_CENTER_BRIGHTNESS]->u.fs_d.value);
    cfg.filter.softening        = static_cast<float>(params[Y2014_FILTER_SOFTENING]->u.fs_d.value);
    cfg.filter.splitBalance     = static_cast<float>(params[Y2014_FILTER_SPLIT_BALANCE]->u.fs_d.value);
    cfg.filter.shadowTint       = presetBase.shadowTint;
    cfg.filter.highlightTint    = presetBase.highlightTint;

    // 3. Grain / Noise (Main)
    int colorModePopup = static_cast<int>(params[Y2014_GRAIN_COLOR_MODE]->u.pd.value) - 1;
    colorModePopup = std::max(0, std::min(static_cast<int>(y2014::GrainColorMode::Count) - 1, colorModePopup));

    cfg.grain.amount            = static_cast<float>(params[Y2014_GRAIN_AMOUNT]->u.fs_d.value);
    cfg.grain.size              = static_cast<float>(params[Y2014_GRAIN_SIZE]->u.fs_d.value);
    cfg.grain.density           = static_cast<float>(params[Y2014_GRAIN_DENSITY]->u.fs_d.value);
    cfg.grain.scaleZoom         = static_cast<float>(params[Y2014_GRAIN_SCALE_ZOOM]->u.fs_d.value);
    cfg.grain.softness          = static_cast<float>(params[Y2014_GRAIN_SOFTNESS]->u.fs_d.value);
    cfg.grain.speed             = static_cast<float>(params[Y2014_GRAIN_SPEED]->u.fs_d.value);
    cfg.grain.animation         = static_cast<float>(params[Y2014_GRAIN_ANIMATION]->u.fs_d.value);
    cfg.grain.colorMode         = static_cast<y2014::GrainColorMode>(colorModePopup);
    cfg.grain.chromaAmount      = static_cast<float>(params[Y2014_GRAIN_CHROMA_AMOUNT]->u.fs_d.value);
    cfg.grain.seed              = static_cast<int32_t>(params[Y2014_GRAIN_SEED]->u.sd.value);

    // 4. Grain / Noise (Advanced)
    cfg.grain.shadows               = static_cast<float>(params[Y2014_GRAIN_SHADOWS]->u.fs_d.value);
    cfg.grain.midtones              = static_cast<float>(params[Y2014_GRAIN_MIDTONES]->u.fs_d.value);
    cfg.grain.highlights            = static_cast<float>(params[Y2014_GRAIN_HIGHLIGHTS]->u.fs_d.value);
    cfg.grain.blackResponse         = static_cast<float>(params[Y2014_GRAIN_BLACK_RESPONSE]->u.fs_d.value);
    cfg.grain.whiteResponse         = static_cast<float>(params[Y2014_GRAIN_WHITE_RESPONSE]->u.fs_d.value);
    cfg.grain.fineGrain             = static_cast<float>(params[Y2014_GRAIN_FINE]->u.fs_d.value);
    cfg.grain.coarseGrain           = static_cast<float>(params[Y2014_GRAIN_COARSE]->u.fs_d.value);
    cfg.grain.lumaChromaBalance     = static_cast<float>(params[Y2014_GRAIN_LUMA_CHROMA_BAL]->u.fs_d.value);
    cfg.grain.clustering            = static_cast<float>(params[Y2014_GRAIN_CLUSTERING]->u.fs_d.value);
    cfg.grain.spatialCorrelation    = static_cast<float>(params[Y2014_GRAIN_SPATIAL_CORR]->u.fs_d.value);
    cfg.grain.temporalCoherence     = static_cast<float>(params[Y2014_GRAIN_TEMPORAL_COHERENCE]->u.fs_d.value);
    cfg.grain.patternAmount         = static_cast<float>(params[Y2014_GRAIN_PATTERN_AMOUNT]->u.fs_d.value);
    cfg.grain.patternScale          = static_cast<float>(params[Y2014_GRAIN_PATTERN_SCALE]->u.fs_d.value);
    cfg.grain.shotReadBalance       = static_cast<float>(params[Y2014_GRAIN_SHOT_READ_BAL]->u.fs_d.value);
    cfg.grain.shadowFloor           = static_cast<float>(params[Y2014_GRAIN_SHADOW_FLOOR]->u.fs_d.value);
    cfg.grain.chromaCorrelation     = static_cast<float>(params[Y2014_GRAIN_CHROMA_CORR]->u.fs_d.value);
    cfg.grain.chromaHueBias         = static_cast<float>(params[Y2014_GRAIN_CHROMA_HUE_BIAS]->u.fs_d.value);
    cfg.grain.denoiseInteraction    = static_cast<float>(params[Y2014_GRAIN_DENOISE_INTERACT]->u.fs_d.value);
    cfg.grain.sharpeningInteraction = static_cast<float>(params[Y2014_GRAIN_SHARPEN_INTERACT]->u.fs_d.value);

    // 5. Performance Mode
    int perfPopup = static_cast<int>(params[Y2014_PERFORMANCE_MODE]->u.pd.value) - 1;
    perfPopup = std::max(0, std::min(static_cast<int>(y2014::PerformanceMode::Count) - 1, perfPopup));
    cfg.grain.performanceMode = static_cast<y2014::PerformanceMode>(perfPopup);

    return cfg;
}

static y2014::PipelineConfig CheckoutPipelineConfigAtCurrentTime(PF_InData* in_data) {
    PF_ParamDef defArray[Y2014_NUM_PARAMS];
    PF_ParamDef* paramPtrs[Y2014_NUM_PARAMS];

    for (int i = 0; i < Y2014_NUM_PARAMS; ++i) {
        AEFX_CLR_STRUCT(defArray[i]);
        paramPtrs[i] = &defArray[i];
        if (i > Y2014_INPUT) {
            PF_CHECKOUT_PARAM(
                in_data,
                i,
                in_data->current_time,
                in_data->time_step,
                in_data->time_scale,
                &defArray[i]
            );
        }
    }

    y2014::PipelineConfig cfg = ExtractPipelineConfigFromParams(in_data, paramPtrs);

    for (int i = 1; i < Y2014_NUM_PARAMS; ++i) {
        PF_CHECKIN_PARAM(in_data, &defArray[i]);
    }

    return cfg;
}

static PF_Err ProcessEffectWorld8(
    PF_EffectWorld* input_worldP,
    PF_EffectWorld* output_worldP,
    const y2014::PipelineConfig& cfg
) {
    if (!input_worldP || !output_worldP || !input_worldP->data || !output_worldP->data) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    const int width  = static_cast<int>(output_worldP->width);
    const int height = static_cast<int>(output_worldP->height);
    const int inW    = static_cast<int>(input_worldP->width);
    const int inH    = static_cast<int>(input_worldP->height);

    const char* srcBase = reinterpret_cast<const char*>(input_worldP->data);
    char* dstBase       = reinterpret_cast<char*>(output_worldP->data);

    auto sampleSrc8 = [&](int sx, int sy) -> y2014::RGBAf {
        int cx = std::max(0, std::min(inW - 1, sx));
        int cy = std::max(0, std::min(inH - 1, sy));
        const PF_Pixel8* p = reinterpret_cast<const PF_Pixel8*>(srcBase + static_cast<ptrdiff_t>(cy) * input_worldP->rowbytes) + cx;
        constexpr float inv255 = 1.0f / static_cast<float>(PF_MAX_CHAN8);
        return {
            static_cast<float>(p->red)   * inv255,
            static_cast<float>(p->green) * inv255,
            static_cast<float>(p->blue)  * inv255,
            static_cast<float>(p->alpha) * inv255
        };
    };

    for (int y = 0; y < height; ++y) {
        PF_Pixel8* dstRow = reinterpret_cast<PF_Pixel8*>(dstBase + static_cast<ptrdiff_t>(y) * output_worldP->rowbytes);
        for (int x = 0; x < width; ++x) {
            y2014::RGBAf c  = sampleSrc8(x, y);
            y2014::RGBAf l1 = sampleSrc8(x - 1, y);
            y2014::RGBAf r1 = sampleSrc8(x + 1, y);
            y2014::RGBAf u1 = sampleSrc8(x, y - 1);
            y2014::RGBAf d1 = sampleSrc8(x, y + 1);
            y2014::RGBAf l2 = sampleSrc8(x - 2, y);
            y2014::RGBAf r2 = sampleSrc8(x + 2, y);
            y2014::RGBAf u2 = sampleSrc8(x, y - 2);
            y2014::RGBAf d2 = sampleSrc8(x, y + 2);

            y2014::RGBAf localBlur{
                (c.r * 2.0f + (l1.r + r1.r + u1.r + d1.r) + 0.5f * (l2.r + r2.r + u2.r + d2.r)) * 0.125f,
                (c.g * 2.0f + (l1.g + r1.g + u1.g + d1.g) + 0.5f * (l2.g + r2.g + u2.g + d2.g)) * 0.125f,
                (c.b * 2.0f + (l1.b + r1.b + u1.b + d1.b) + 0.5f * (l2.b + r2.b + u2.b + d2.b)) * 0.125f,
                c.a
            };

            float detail = y2014::luminance709(c.r, c.g, c.b) - y2014::luminance709(localBlur.r, localBlur.g, localBlur.b);
            y2014::RGBAf outPx = y2014::evaluatePipelinePixel(x, y, width, height, c, localBlur, detail, cfg);

            dstRow[x].alpha = static_cast<A_u_char>(y2014::clamp01(outPx.a) * PF_MAX_CHAN8 + 0.5f);
            dstRow[x].red   = static_cast<A_u_char>(y2014::clamp01(outPx.r) * PF_MAX_CHAN8 + 0.5f);
            dstRow[x].green = static_cast<A_u_char>(y2014::clamp01(outPx.g) * PF_MAX_CHAN8 + 0.5f);
            dstRow[x].blue  = static_cast<A_u_char>(y2014::clamp01(outPx.b) * PF_MAX_CHAN8 + 0.5f);
        }
    }

    return PF_Err_NONE;
}

static PF_Err ProcessEffectWorld16(
    PF_EffectWorld* input_worldP,
    PF_EffectWorld* output_worldP,
    const y2014::PipelineConfig& cfg
) {
    if (!input_worldP || !output_worldP || !input_worldP->data || !output_worldP->data) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    const int width  = static_cast<int>(output_worldP->width);
    const int height = static_cast<int>(output_worldP->height);
    const int inW    = static_cast<int>(input_worldP->width);
    const int inH    = static_cast<int>(input_worldP->height);

    const char* srcBase = reinterpret_cast<const char*>(input_worldP->data);
    char* dstBase       = reinterpret_cast<char*>(output_worldP->data);

    auto sampleSrc16 = [&](int sx, int sy) -> y2014::RGBAf {
        int cx = std::max(0, std::min(inW - 1, sx));
        int cy = std::max(0, std::min(inH - 1, sy));
        const PF_Pixel16* p = reinterpret_cast<const PF_Pixel16*>(srcBase + static_cast<ptrdiff_t>(cy) * input_worldP->rowbytes) + cx;
        constexpr float inv32768 = 1.0f / static_cast<float>(PF_MAX_CHAN16);
        return {
            static_cast<float>(p->red)   * inv32768,
            static_cast<float>(p->green) * inv32768,
            static_cast<float>(p->blue)  * inv32768,
            static_cast<float>(p->alpha) * inv32768
        };
    };

    for (int y = 0; y < height; ++y) {
        PF_Pixel16* dstRow = reinterpret_cast<PF_Pixel16*>(dstBase + static_cast<ptrdiff_t>(y) * output_worldP->rowbytes);
        for (int x = 0; x < width; ++x) {
            y2014::RGBAf c  = sampleSrc16(x, y);
            y2014::RGBAf l1 = sampleSrc16(x - 1, y);
            y2014::RGBAf r1 = sampleSrc16(x + 1, y);
            y2014::RGBAf u1 = sampleSrc16(x, y - 1);
            y2014::RGBAf d1 = sampleSrc16(x, y + 1);
            y2014::RGBAf l2 = sampleSrc16(x - 2, y);
            y2014::RGBAf r2 = sampleSrc16(x + 2, y);
            y2014::RGBAf u2 = sampleSrc16(x, y - 2);
            y2014::RGBAf d2 = sampleSrc16(x, y + 2);

            y2014::RGBAf localBlur{
                (c.r * 2.0f + (l1.r + r1.r + u1.r + d1.r) + 0.5f * (l2.r + r2.r + u2.r + d2.r)) * 0.125f,
                (c.g * 2.0f + (l1.g + r1.g + u1.g + d1.g) + 0.5f * (l2.g + r2.g + u2.g + d2.g)) * 0.125f,
                (c.b * 2.0f + (l1.b + r1.b + u1.b + d1.b) + 0.5f * (l2.b + r2.b + u2.b + d2.b)) * 0.125f,
                c.a
            };

            float detail = y2014::luminance709(c.r, c.g, c.b) - y2014::luminance709(localBlur.r, localBlur.g, localBlur.b);
            y2014::RGBAf outPx = y2014::evaluatePipelinePixel(x, y, width, height, c, localBlur, detail, cfg);

            dstRow[x].alpha = static_cast<A_u_short>(y2014::clamp01(outPx.a) * PF_MAX_CHAN16 + 0.5f);
            dstRow[x].red   = static_cast<A_u_short>(y2014::clamp01(outPx.r) * PF_MAX_CHAN16 + 0.5f);
            dstRow[x].green = static_cast<A_u_short>(y2014::clamp01(outPx.g) * PF_MAX_CHAN16 + 0.5f);
            dstRow[x].blue  = static_cast<A_u_short>(y2014::clamp01(outPx.b) * PF_MAX_CHAN16 + 0.5f);
        }
    }

    return PF_Err_NONE;
}

static PF_Err ProcessEffectWorld32f(
    PF_EffectWorld* input_worldP,
    PF_EffectWorld* output_worldP,
    const y2014::PipelineConfig& cfg
) {
    if (!input_worldP || !output_worldP || !input_worldP->data || !output_worldP->data) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    const int width  = static_cast<int>(output_worldP->width);
    const int height = static_cast<int>(output_worldP->height);
    const int inW    = static_cast<int>(input_worldP->width);
    const int inH    = static_cast<int>(input_worldP->height);

    const char* srcBase = reinterpret_cast<const char*>(input_worldP->data);
    char* dstBase       = reinterpret_cast<char*>(output_worldP->data);

    auto sampleSrc32 = [&](int sx, int sy) -> y2014::RGBAf {
        int cx = std::max(0, std::min(inW - 1, sx));
        int cy = std::max(0, std::min(inH - 1, sy));
        const PF_PixelFloat* p = reinterpret_cast<const PF_PixelFloat*>(srcBase + static_cast<ptrdiff_t>(cy) * input_worldP->rowbytes) + cx;
        return {
            static_cast<float>(p->red),
            static_cast<float>(p->green),
            static_cast<float>(p->blue),
            static_cast<float>(p->alpha)
        };
    };

    for (int y = 0; y < height; ++y) {
        PF_PixelFloat* dstRow = reinterpret_cast<PF_PixelFloat*>(dstBase + static_cast<ptrdiff_t>(y) * output_worldP->rowbytes);
        for (int x = 0; x < width; ++x) {
            y2014::RGBAf c  = sampleSrc32(x, y);
            y2014::RGBAf l1 = sampleSrc32(x - 1, y);
            y2014::RGBAf r1 = sampleSrc32(x + 1, y);
            y2014::RGBAf u1 = sampleSrc32(x, y - 1);
            y2014::RGBAf d1 = sampleSrc32(x, y + 1);
            y2014::RGBAf l2 = sampleSrc32(x - 2, y);
            y2014::RGBAf r2 = sampleSrc32(x + 2, y);
            y2014::RGBAf u2 = sampleSrc32(x, y - 2);
            y2014::RGBAf d2 = sampleSrc32(x, y + 2);

            y2014::RGBAf localBlur{
                (c.r * 2.0f + (l1.r + r1.r + u1.r + d1.r) + 0.5f * (l2.r + r2.r + u2.r + d2.r)) * 0.125f,
                (c.g * 2.0f + (l1.g + r1.g + u1.g + d1.g) + 0.5f * (l2.g + r2.g + u2.g + d2.g)) * 0.125f,
                (c.b * 2.0f + (l1.b + r1.b + u1.b + d1.b) + 0.5f * (l2.b + r2.b + u2.b + d2.b)) * 0.125f,
                c.a
            };

            float detail = y2014::luminance709(c.r, c.g, c.b) - y2014::luminance709(localBlur.r, localBlur.g, localBlur.b);
            y2014::RGBAf outPx = y2014::evaluatePipelinePixel(x, y, width, height, c, localBlur, detail, cfg);

            dstRow[x].alpha = static_cast<PF_FpShort>(outPx.a);
            dstRow[x].red   = static_cast<PF_FpShort>(outPx.r);
            dstRow[x].green = static_cast<PF_FpShort>(outPx.g);
            dstRow[x].blue  = static_cast<PF_FpShort>(outPx.b);
        }
    }

    return PF_Err_NONE;
}

static PF_Err About(
    PF_InData* in_data,
    PF_OutData* out_data
) {
    if (out_data) {
        if (in_data && in_data->utils && in_data->utils->ansi.sprintf) {
            in_data->utils->ansi.sprintf(
                out_data->return_msg,
                "%s v%d.%d\r%s",
                Y2014_EFFECT_NAME,
                Y2014_MAJOR_VERSION,
                Y2014_MINOR_VERSION,
                Y2014_EFFECT_DESCRIPTION
            );
        } else {
            std::snprintf(
                out_data->return_msg,
                sizeof(out_data->return_msg),
                "%s v%d.%d\r%s",
                Y2014_EFFECT_NAME,
                Y2014_MAJOR_VERSION,
                Y2014_MINOR_VERSION,
                Y2014_EFFECT_DESCRIPTION
            );
        }
    }
    return PF_Err_NONE;
}

static PF_Err GlobalSetup(
    PF_InData* /*in_data*/,
    PF_OutData* out_data
) {
    if (!out_data) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    out_data->my_version = PF_VERSION(
        Y2014_MAJOR_VERSION,
        Y2014_MINOR_VERSION,
        Y2014_BUG_VERSION,
        Y2014_STAGE_VERSION,
        Y2014_BUILD_VERSION
    );

    // Matches 2014_PiPL.r:
    // AE_Effect_Global_OutFlags   = 0x02000004 (PF_OutFlag_DEEP_COLOR_AWARE | PF_OutFlag_NON_PARAM_VARY)
    // AE_Effect_Global_OutFlags_2 = 0x08001408 (PF_OutFlag2_PARAM_GROUP_START_COLLAPSED_FLAG |
    //                                           PF_OutFlag2_SUPPORTS_SMART_RENDER |
    //                                           PF_OutFlag2_FLOAT_COLOR_AWARE |
    //                                           PF_OutFlag2_SUPPORTS_THREADED_RENDERING)
    out_data->out_flags =
        PF_OutFlag_DEEP_COLOR_AWARE |
        PF_OutFlag_NON_PARAM_VARY;

    out_data->out_flags2 =
        PF_OutFlag2_PARAM_GROUP_START_COLLAPSED_FLAG |
        PF_OutFlag2_SUPPORTS_SMART_RENDER |
        PF_OutFlag2_FLOAT_COLOR_AWARE |
        PF_OutFlag2_SUPPORTS_THREADED_RENDERING;

    return PF_Err_NONE;
}

static PF_Err ParamsSetup(
    PF_InData* in_data,
    PF_OutData* out_data
) {
    PF_Err err = PF_Err_NONE;

    // ------------------------------------------------------------------------
    // Topic 1: Camera Model (iPhone 5 through iPhone 8/8 Plus)
    // ------------------------------------------------------------------------
    ERR(AddTopicGroup(in_data, "Camera Model", Y2014_TOPIC_CAMERA_START, false));

    ERR(AddPopupControl(
        in_data,
        "Camera Profile",
        8,
        2, // Default: iPhone 5s
        "iPhone 5|iPhone 5s|iPhone 6|iPhone 6s|iPhone 7|iPhone 7 Plus|iPhone 8|iPhone 8 Plus",
        Y2014_CAMERA_MODEL,
        true
    ));
    ERR(AddFloatSliderControl(in_data, "Profile Strength",       0.0f, 100.0f,    0.0f, 100.0f, 100.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_CAMERA_STRENGTH));
    ERR(AddFloatSliderControl(in_data, "Camera Exposure Trim", -100.0f, 100.0f, -100.0f, 100.0f,   0.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_CAMERA_EXPOSURE_TRIM));
    ERR(AddFloatSliderControl(in_data, "HDR Blending",           0.0f, 100.0f,    0.0f, 100.0f,  50.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_CAMERA_HDR_CHARACTER));
    ERR(AddFloatSliderControl(in_data, "Tone Compression",       0.0f, 100.0f,    0.0f, 100.0f,  50.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_CAMERA_TONE_COMPRESSION));
    ERR(AddFloatSliderControl(in_data, "Camera Warmth",        -100.0f, 100.0f, -100.0f, 100.0f,   0.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_CAMERA_WARMTH));
    ERR(AddFloatSliderControl(in_data, "Camera Tint",          -100.0f, 100.0f, -100.0f, 100.0f,   0.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_CAMERA_TINT));
    ERR(AddFloatSliderControl(in_data, "Low-Light Denoise",      0.0f, 100.0f,    0.0f, 100.0f,  50.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_CAMERA_LOWLIGHT_DENOISE));
    ERR(AddFloatSliderControl(in_data, "ISP Sharpening",         0.0f, 100.0f,    0.0f, 100.0f,  50.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_CAMERA_ISP_SHARPENING));
    ERR(AddFloatSliderControl(in_data, "Lens Vignette",          0.0f, 100.0f,    0.0f, 100.0f,  50.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_CAMERA_LENS_VIGNETTE));
    ERR(AddFloatSliderControl(in_data, "Edge Softness",          0.0f, 100.0f,    0.0f, 100.0f,  40.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_CAMERA_EDGE_SOFTNESS));
    ERR(AddFloatSliderControl(in_data, "JPEG Era Residue",       0.0f, 100.0f,    0.0f, 100.0f,  25.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_CAMERA_JPEG_ARTIFACT));

    ERR(EndTopicGroup(in_data, Y2014_TOPIC_CAMERA_END));

    // ------------------------------------------------------------------------
    // Topic 2: 2014 Filters (City Presets + Full Editable Controls)
    // ------------------------------------------------------------------------
    ERR(AddTopicGroup(in_data, "2014 Filter", Y2014_TOPIC_FILTER_START, false));

    ERR(AddPopupControl(
        in_data,
        "2014 City Preset",
        11,
        2, // Default: New York
        "None|New York|Miami|Los Angeles|Tokyo|Paris|London|Seattle|Berlin|Miami Night|Nostalgic Nights (IOS)",
        Y2014_FILTER_PRESET,
        true
    ));
    ERR(AddFloatSliderControl(in_data, "Filter Strength",        0.0f, 100.0f,    0.0f, 100.0f, 100.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_FILTER_STRENGTH));
    ERR(AddFloatSliderControl(in_data, "Exposure",            -100.0f, 100.0f, -100.0f, 100.0f, -18.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_FILTER_EXPOSURE));
    ERR(AddFloatSliderControl(in_data, "Brilliance",          -100.0f, 100.0f, -100.0f, 100.0f,  12.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_FILTER_BRILLIANCE));
    ERR(AddFloatSliderControl(in_data, "Highlights",          -100.0f, 100.0f, -100.0f, 100.0f,  35.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_FILTER_HIGHLIGHTS));
    ERR(AddFloatSliderControl(in_data, "Shadows",             -100.0f, 100.0f, -100.0f, 100.0f, -14.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_FILTER_SHADOWS));
    ERR(AddFloatSliderControl(in_data, "Contrast",            -100.0f, 100.0f, -100.0f, 100.0f,  22.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_FILTER_CONTRAST));
    ERR(AddFloatSliderControl(in_data, "Brightness",          -100.0f, 100.0f, -100.0f, 100.0f,  15.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_FILTER_BRIGHTNESS));
    ERR(AddFloatSliderControl(in_data, "Black Point",         -100.0f, 100.0f, -100.0f, 100.0f,  24.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_FILTER_BLACK_POINT));
    ERR(AddFloatSliderControl(in_data, "Matte Fade",             0.0f, 100.0f,    0.0f, 100.0f,   8.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_FILTER_FADE));
    ERR(AddFloatSliderControl(in_data, "Saturation",          -100.0f, 100.0f, -100.0f, 100.0f,  -8.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_FILTER_SATURATION));
    ERR(AddFloatSliderControl(in_data, "Vibrance",            -100.0f, 100.0f, -100.0f, 100.0f, -22.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_FILTER_VIBRANCE));
    ERR(AddFloatSliderControl(in_data, "Warmth",              -100.0f, 100.0f, -100.0f, 100.0f, -16.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_FILTER_WARMTH));
    ERR(AddFloatSliderControl(in_data, "Tint",                -100.0f, 100.0f, -100.0f, 100.0f, -18.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_FILTER_TINT));
    ERR(AddFloatSliderControl(in_data, "Sharpness",           -100.0f, 100.0f, -100.0f, 100.0f,  12.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_FILTER_SHARPNESS));
    ERR(AddFloatSliderControl(in_data, "Definition",          -100.0f, 100.0f, -100.0f, 100.0f,  42.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_FILTER_DEFINITION));
    ERR(AddFloatSliderControl(in_data, "Vignette",               0.0f, 100.0f,    0.0f, 100.0f,  28.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_FILTER_VIGNETTE));
    ERR(AddFloatSliderControl(in_data, "Center Brightness",      0.0f, 100.0f,    0.0f, 100.0f,  14.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_FILTER_CENTER_BRIGHTNESS));
    ERR(AddFloatSliderControl(in_data, "Softening",              0.0f, 100.0f,    0.0f, 100.0f,   6.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_FILTER_SOFTENING));
    ERR(AddFloatSliderControl(in_data, "Split Balance",       -100.0f, 100.0f, -100.0f, 100.0f, -10.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_FILTER_SPLIT_BALANCE));

    ERR(EndTopicGroup(in_data, Y2014_TOPIC_FILTER_END));

    // ------------------------------------------------------------------------
    // Topic 3: Grain / Noise (Main Controls)
    // ------------------------------------------------------------------------
    ERR(AddTopicGroup(in_data, "Grain / Noise (Main)", Y2014_TOPIC_GRAIN_MAIN_START, false));

    ERR(AddFloatSliderControl(in_data, "Amount",                 0.0f, 200.0f,    0.0f, 100.0f,  45.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_AMOUNT));
    ERR(AddFloatSliderControl(in_data, "Size",                   1.0f, 100.0f,    1.0f, 100.0f,  38.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_GRAIN_SIZE));
    ERR(AddFloatSliderControl(in_data, "Density",                0.0f, 100.0f,    0.0f, 100.0f,  62.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_DENSITY));
    ERR(AddFloatSliderControl(in_data, "Scale / Zoom",          10.0f, 400.0f,   25.0f, 200.0f, 100.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_SCALE_ZOOM));
    ERR(AddFloatSliderControl(in_data, "Softness",               0.0f, 100.0f,    0.0f, 100.0f,  32.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_SOFTNESS));
    ERR(AddFloatSliderControl(in_data, "Speed",                  0.0f, 200.0f,    0.0f, 150.0f,  50.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_SPEED));
    ERR(AddFloatSliderControl(in_data, "Animation",              0.0f, 100.0f,    0.0f, 100.0f,  75.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_ANIMATION));
    ERR(AddPopupControl(
        in_data,
        "Color / Chroma Mode",
        6,
        6, // Default: Camera Profile
        "Monochrome|Neutral Luma|Warm Chroma|Cool Chroma|Full Chroma|Camera Profile",
        Y2014_GRAIN_COLOR_MODE,
        false
    ));
    ERR(AddFloatSliderControl(in_data, "Chroma Amount",          0.0f, 100.0f,    0.0f, 100.0f,  48.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_CHROMA_AMOUNT));
    ERR(AddIntSliderControl(  in_data, "Seed",                      0,  99999,       0,  10000,   2014, Y2014_GRAIN_SEED));

    ERR(EndTopicGroup(in_data, Y2014_TOPIC_GRAIN_MAIN_END));

    // ------------------------------------------------------------------------
    // Topic 4: Grain / Noise (Advanced Controls)
    // ------------------------------------------------------------------------
    ERR(AddTopicGroup(in_data, "Grain / Noise (Advanced)", Y2014_TOPIC_GRAIN_ADV_START, true));

    ERR(AddFloatSliderControl(in_data, "Shadows",                0.0f, 200.0f,    0.0f, 150.0f,  88.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_SHADOWS));
    ERR(AddFloatSliderControl(in_data, "Midtones",               0.0f, 200.0f,    0.0f, 150.0f,  58.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_MIDTONES));
    ERR(AddFloatSliderControl(in_data, "Highlights",             0.0f, 200.0f,    0.0f, 150.0f,  24.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_HIGHLIGHTS));
    ERR(AddFloatSliderControl(in_data, "Black Response",         0.0f, 100.0f,    0.0f, 100.0f,  22.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_BLACK_RESPONSE));
    ERR(AddFloatSliderControl(in_data, "White Response",         0.0f, 100.0f,    0.0f, 100.0f,   6.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_WHITE_RESPONSE));
    ERR(AddFloatSliderControl(in_data, "Fine Grain",             0.0f, 100.0f,    0.0f, 100.0f,  68.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_FINE));
    ERR(AddFloatSliderControl(in_data, "Coarse Grain",           0.0f, 100.0f,    0.0f, 100.0f,  46.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_COARSE));
    ERR(AddFloatSliderControl(in_data, "Luma / Chroma Balance",  0.0f, 100.0f,    0.0f, 100.0f,  68.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_LUMA_CHROMA_BAL));
    ERR(AddFloatSliderControl(in_data, "Clustering",             0.0f, 100.0f,    0.0f, 100.0f,  42.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_CLUSTERING));
    ERR(AddFloatSliderControl(in_data, "Spatial Correlation",    0.0f, 100.0f,    0.0f, 100.0f,  55.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_SPATIAL_CORR));
    ERR(AddFloatSliderControl(in_data, "Temporal Coherence",     0.0f, 100.0f,    0.0f, 100.0f,  62.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_TEMPORAL_COHERENCE));
    ERR(AddFloatSliderControl(in_data, "Pattern Amount",         0.0f, 100.0f,    0.0f, 100.0f,  14.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_PATTERN_AMOUNT));
    ERR(AddFloatSliderControl(in_data, "Pattern Scale",         10.0f, 200.0f,   10.0f, 200.0f,  50.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_PATTERN_SCALE));
    ERR(AddFloatSliderControl(in_data, "Shot / Read Balance",    0.0f, 100.0f,    0.0f, 100.0f,  55.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_SHOT_READ_BAL));
    ERR(AddFloatSliderControl(in_data, "Shadow Floor",           0.0f, 100.0f,    0.0f, 100.0f,  18.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_SHADOW_FLOOR));
    ERR(AddFloatSliderControl(in_data, "Chroma Correlation",     0.0f, 100.0f,    0.0f, 100.0f,  65.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_CHROMA_CORR));
    ERR(AddFloatSliderControl(in_data, "Chroma Hue Bias",     -100.0f, 100.0f, -100.0f, 100.0f, -12.0f, 1, PF_ValueDisplayFlag_NONE,    Y2014_GRAIN_CHROMA_HUE_BIAS));
    ERR(AddFloatSliderControl(in_data, "Denoise Interaction",    0.0f, 100.0f,    0.0f, 100.0f,  52.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_DENOISE_INTERACT));
    ERR(AddFloatSliderControl(in_data, "Sharpening Interaction", 0.0f, 100.0f,    0.0f, 100.0f,  44.0f, 1, PF_ValueDisplayFlag_PERCENT, Y2014_GRAIN_SHARPEN_INTERACT));

    ERR(EndTopicGroup(in_data, Y2014_TOPIC_GRAIN_ADV_END));

    // ------------------------------------------------------------------------
    // Performance Mode
    // ------------------------------------------------------------------------
    ERR(AddPopupControl(
        in_data,
        "Performance Mode",
        3,
        1, // Default: Accurate
        "Accurate (Final)|Preview|Draft (Fast Interactive)",
        Y2014_PERFORMANCE_MODE,
        false
    ));

    out_data->num_params = Y2014_NUM_PARAMS;
    return err;
}

static PF_Err UserChangedParam(
    PF_InData* /*in_data*/,
    PF_OutData* /*out_data*/,
    PF_ParamDef* params[],
    const PF_UserChangedParamExtra* extraP
) {
    if (!params || !extraP) {
        return PF_Err_NONE;
    }

    if (extraP->param_index == Y2014_FILTER_PRESET) {
        int fltPopup = static_cast<int>(params[Y2014_FILTER_PRESET]->u.pd.value) - 1;
        fltPopup = std::max(0, std::min(static_cast<int>(y2014::FilterPreset::Count) - 1, fltPopup));
        y2014::FilterParams p = y2014::getFilterPresetParams(static_cast<y2014::FilterPreset>(fltPopup));

        auto setSlider = [&](int idx, float val) {
            params[idx]->u.fs_d.value = val;
            params[idx]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        };

        setSlider(Y2014_FILTER_EXPOSURE,          p.exposure);
        setSlider(Y2014_FILTER_BRILLIANCE,        p.brilliance);
        setSlider(Y2014_FILTER_HIGHLIGHTS,        p.highlights);
        setSlider(Y2014_FILTER_SHADOWS,           p.shadows);
        setSlider(Y2014_FILTER_CONTRAST,          p.contrast);
        setSlider(Y2014_FILTER_BRIGHTNESS,        p.brightness);
        setSlider(Y2014_FILTER_BLACK_POINT,       p.blackPoint);
        setSlider(Y2014_FILTER_FADE,              p.fade);
        setSlider(Y2014_FILTER_SATURATION,        p.saturation);
        setSlider(Y2014_FILTER_VIBRANCE,          p.vibrance);
        setSlider(Y2014_FILTER_WARMTH,            p.warmth);
        setSlider(Y2014_FILTER_TINT,              p.tint);
        setSlider(Y2014_FILTER_SHARPNESS,         p.sharpness);
        setSlider(Y2014_FILTER_DEFINITION,        p.definition);
        setSlider(Y2014_FILTER_VIGNETTE,          p.vignette);
        setSlider(Y2014_FILTER_CENTER_BRIGHTNESS, p.centerBrightness);
        setSlider(Y2014_FILTER_SOFTENING,         p.softening);
        setSlider(Y2014_FILTER_SPLIT_BALANCE,     p.splitBalance);
    } else if (extraP->param_index == Y2014_CAMERA_MODEL) {
        int camPopup = static_cast<int>(params[Y2014_CAMERA_MODEL]->u.pd.value) - 1;
        camPopup = std::max(0, std::min(static_cast<int>(y2014::CameraModel::Count) - 1, camPopup));
        y2014::GrainParams gp{};
        y2014::applyCameraDefaultsToGrain(static_cast<y2014::CameraModel>(camPopup), gp);

        auto setSlider = [&](int idx, float val) {
            params[idx]->u.fs_d.value = val;
            params[idx]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        };

        setSlider(Y2014_GRAIN_SHADOWS,            gp.shadows);
        setSlider(Y2014_GRAIN_MIDTONES,           gp.midtones);
        setSlider(Y2014_GRAIN_HIGHLIGHTS,         gp.highlights);
        setSlider(Y2014_GRAIN_BLACK_RESPONSE,     gp.blackResponse);
        setSlider(Y2014_GRAIN_WHITE_RESPONSE,     gp.whiteResponse);
        setSlider(Y2014_GRAIN_LUMA_CHROMA_BAL,    gp.lumaChromaBalance);
        setSlider(Y2014_GRAIN_TEMPORAL_COHERENCE, gp.temporalCoherence);
        setSlider(Y2014_GRAIN_PATTERN_AMOUNT,     gp.patternAmount);
        setSlider(Y2014_GRAIN_SHOT_READ_BAL,      gp.shotReadBalance);
        setSlider(Y2014_GRAIN_CHROMA_CORR,        gp.chromaCorrelation);
        setSlider(Y2014_GRAIN_DENOISE_INTERACT,   gp.denoiseInteraction);
        setSlider(Y2014_GRAIN_SHARPEN_INTERACT,   gp.sharpeningInteraction);
    }

    return PF_Err_NONE;
}

static PF_Err Render(
    PF_InData* in_data,
    PF_OutData* /*out_data*/,
    PF_ParamDef* params[],
    PF_LayerDef* output
) {
    if (!in_data || !params || !output) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    PF_EffectWorld* srcWorld = &params[Y2014_INPUT]->u.ld;
    y2014::PipelineConfig cfg = ExtractPipelineConfigFromParams(in_data, params);

    if (PF_WORLD_IS_DEEP(output)) {
        return ProcessEffectWorld16(srcWorld, output, cfg);
    }
    return ProcessEffectWorld8(srcWorld, output, cfg);
}

static PF_Err PreRender(
    PF_InData* in_data,
    PF_OutData* /*out_data*/,
    PF_PreRenderExtra* extraP
) {
    if (!in_data || !extraP || !extraP->cb || !extraP->input || !extraP->output) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    PF_Err err = PF_Err_NONE;
    PF_RenderRequest req = extraP->input->output_request;
    PF_CheckoutResult in_result;
    AEFX_CLR_STRUCT(in_result);

    ERR(extraP->cb->checkout_layer(
        in_data->effect_ref,
        Y2014_INPUT,
        Y2014_INPUT,
        &req,
        in_data->current_time,
        in_data->time_step,
        in_data->time_scale,
        &in_result
    ));

    UnionLRectLocal(&in_result.result_rect, &extraP->output->result_rect);
    UnionLRectLocal(&in_result.max_result_rect, &extraP->output->max_result_rect);
    extraP->output->pre_render_data = nullptr;
    extraP->output->delete_pre_render_data_func = nullptr;

    return err;
}

static PF_Err SmartRender(
    PF_InData* in_data,
    PF_OutData* out_data,
    PF_SmartRenderExtra* extraP
) {
    if (!in_data || !extraP || !extraP->cb) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    PF_Err err  = PF_Err_NONE;
    PF_Err err2 = PF_Err_NONE;

    PF_EffectWorld* input_worldP  = nullptr;
    PF_EffectWorld* output_worldP = nullptr;

    ERR(extraP->cb->checkout_layer_pixels(in_data->effect_ref, Y2014_INPUT, &input_worldP));
    ERR(extraP->cb->checkout_output(in_data->effect_ref, &output_worldP));

    if (!err && input_worldP && output_worldP) {
        y2014::PipelineConfig cfg = CheckoutPipelineConfigAtCurrentTime(in_data);

        PF_PixelFormat format = PF_PixelFormat_INVALID;
        if (in_data->pica_basicP) {
            AEFX_SuiteScoper<PF_WorldSuite2> wsP(
                in_data,
                kPFWorldSuite,
                kPFWorldSuiteVersion2,
                out_data
            );
            ERR(wsP->PF_GetPixelFormat(input_worldP, &format));
        } else {
            format = PF_WORLD_IS_DEEP(output_worldP) ? PF_PixelFormat_ARGB64 : PF_PixelFormat_ARGB32;
        }

        if (!err) {
            switch (format) {
                case PF_PixelFormat_ARGB128:
                    err = ProcessEffectWorld32f(input_worldP, output_worldP, cfg);
                    break;
                case PF_PixelFormat_ARGB64:
                    err = ProcessEffectWorld16(input_worldP, output_worldP, cfg);
                    break;
                case PF_PixelFormat_ARGB32:
                default:
                    err = ProcessEffectWorld8(input_worldP, output_worldP, cfg);
                    break;
            }
        }
    }

    ERR2(extraP->cb->checkin_layer_pixels(in_data->effect_ref, Y2014_INPUT));
    return err;
}

} // namespace

extern "C" DllExport
PF_Err PluginDataEntryFunction2(
    PF_PluginDataPtr inPtr,
    PF_PluginDataCB2 inPluginDataCallBackPtr,
    SPBasicSuite* /*inSPBasicSuitePtr*/,
    const char* /*inHostName*/,
    const char* /*inHostVersion*/
) {
    if (!inPluginDataCallBackPtr) {
        return PF_Err_INVALID_CALLBACK;
    }

    return PF_REGISTER_EFFECT_EXT2(
        inPtr,
        inPluginDataCallBackPtr,
        Y2014_EFFECT_NAME,
        Y2014_EFFECT_MATCH_NAME,
        Y2014_EFFECT_CATEGORY,
        AE_RESERVED_INFO,
        "EffectMain",
        "https://www.adobe.com"
    );
}

extern "C" DllExport
PF_Err EffectMain(
    PF_Cmd          cmd,
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output,
    void            *extra
) {
    PF_Err err = PF_Err_NONE;

    try {
        switch (cmd) {
            case PF_Cmd_ABOUT:
                err = About(in_data, out_data);
                break;
            case PF_Cmd_GLOBAL_SETUP:
                err = GlobalSetup(in_data, out_data);
                break;
            case PF_Cmd_PARAMS_SETUP:
                err = ParamsSetup(in_data, out_data);
                break;
            case PF_Cmd_USER_CHANGED_PARAM:
                err = UserChangedParam(
                    in_data,
                    out_data,
                    params,
                    reinterpret_cast<const PF_UserChangedParamExtra*>(extra)
                );
                break;
            case PF_Cmd_RENDER:
                err = Render(in_data, out_data, params, output);
                break;
            case PF_Cmd_SMART_PRE_RENDER:
                err = PreRender(in_data, out_data, reinterpret_cast<PF_PreRenderExtra*>(extra));
                break;
            case PF_Cmd_SMART_RENDER:
                err = SmartRender(in_data, out_data, reinterpret_cast<PF_SmartRenderExtra*>(extra));
                break;
            default:
                break;
        }
    } catch (PF_Err& thrown_err) {
        err = thrown_err;
    } catch (...) {
        err = PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    return err;
}
