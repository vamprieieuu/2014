#if defined(Y2014_HAS_AE_SDK)

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif

#include "AEConfig.h"
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"

#include "2014_Plugin.h"
#include "pipeline_core.h"

#include <cstring>
#include <cmath>
#include <algorithm>

static inline void UnionRectSafe(const PF_LRect* src, PF_LRect* dst) noexcept {
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

static PF_Err About(
    PF_InData* in_data,
    PF_OutData* out_data,
    PF_ParamDef* params[],
    PF_LayerDef* output)
{
    (void)params;
    (void)output;
    if (in_data && out_data && in_data->utils && in_data->utils->ansi.sprintf) {
        in_data->utils->ansi.sprintf(
            out_data->return_msg,
            "%s v%d.%d\rEarly iPhone Camera Model, 2014 Filter & Animated Sensor Grain Engine.",
            Y2014_PLUGIN_NAME,
            Y2014_MAJOR_VERSION,
            Y2014_MINOR_VERSION);
    }
    return PF_Err_NONE;
}

static PF_Err GlobalSetup(
    PF_InData* in_data,
    PF_OutData* out_data,
    PF_ParamDef* params[],
    PF_LayerDef* output)
{
    (void)in_data;
    (void)params;
    (void)output;
    if (!out_data) {
        return PF_Err_INVALID_INDEX;
    }

    out_data->my_version = PF_VERSION(
        Y2014_MAJOR_VERSION,
        Y2014_MINOR_VERSION,
        Y2014_BUG_VERSION,
        Y2014_STAGE_VERSION,
        Y2014_BUILD_VERSION);

    out_data->out_flags =
        PF_OutFlag_DEEP_COLOR_AWARE |
        PF_OutFlag_NON_PARAM_VARY |
        PF_OutFlag_SEND_UPDATE_PARAMS_UI;

    out_data->out_flags2 =
        PF_OutFlag2_FLOAT_COLOR_AWARE |
        PF_OutFlag2_SUPPORTS_SMART_RENDER |
        PF_OutFlag2_SUPPORTS_THREADED_RENDERING;

    return PF_Err_NONE;
}

static PF_Err AddSlider(
    const char* name,
    PF_FpShort validMin,
    PF_FpShort validMax,
    PF_FpShort sliderMin,
    PF_FpShort sliderMax,
    PF_FpShort dflt,
    A_short precision,
    PF_ParamFlags flags,
    A_long diskId,
    PF_InData* in_data)
{
    PF_ParamDef def;
    AEFX_CLR_STRUCT(def);
    def.flags = flags;
    PF_ADD_FLOAT_SLIDERX(
        name,
        validMin,
        validMax,
        sliderMin,
        sliderMax,
        dflt,
        precision,
        PF_ValueDisplayFlag_NONE,
        flags,
        diskId);
    return PF_Err_NONE;
}

static PF_Err ParamsSetup(
    PF_InData* in_data,
    PF_OutData* out_data,
    PF_ParamDef* params[],
    PF_LayerDef* output)
{
    (void)params;
    (void)output;
    PF_Err err = PF_Err_NONE;
    PF_ParamDef def;

    // ===================== CAMERA MODEL GROUP =====================
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Camera Model", PARAM_CAMERA_TOPIC_START);

    AEFX_CLR_STRUCT(def);
    def.flags = PF_ParamFlag_SUPERVISE;
    PF_ADD_POPUP(
        "Camera Model",
        8,
        3, // Default: iPhone 6
        "iPhone 5|iPhone 5s|iPhone 6|iPhone 6s|iPhone 7|iPhone 7 Plus|iPhone 8|iPhone 8 Plus",
        PARAM_CAMERA_MODEL);

    ERR(AddSlider("Profile Strength", 0.0f, 100.0f, 0.0f, 100.0f, 85.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_STRENGTH, in_data));
    ERR(AddSlider("Camera Exposure", -2.0f, 2.0f, -2.0f, 2.0f, 0.0f, 2, PF_ParamFlag_NONE, PARAM_CAMERA_EXPOSURE, in_data));
    ERR(AddSlider("Camera Brightness", -100.0f, 100.0f, -100.0f, 100.0f, 0.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_BRIGHTNESS, in_data));
    ERR(AddSlider("Camera Contrast", -100.0f, 100.0f, -100.0f, 100.0f, 0.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_CONTRAST, in_data));
    ERR(AddSlider("Camera Saturation", -100.0f, 100.0f, -100.0f, 100.0f, 10.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_SATURATION, in_data));
    ERR(AddSlider("Camera Temperature", -100.0f, 100.0f, -100.0f, 100.0f, 20.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_TEMPERATURE, in_data));
    ERR(AddSlider("Camera Tint", -100.0f, 100.0f, -100.0f, 100.0f, 0.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_TINT, in_data));
    ERR(AddSlider("Camera Highlights", -100.0f, 100.0f, -100.0f, 100.0f, 0.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_HIGHLIGHTS, in_data));
    ERR(AddSlider("Camera Shadows", -100.0f, 100.0f, -100.0f, 100.0f, 0.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_SHADOWS, in_data));
    ERR(AddSlider("Camera Whites", -100.0f, 100.0f, -100.0f, 100.0f, 0.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_WHITES, in_data));
    ERR(AddSlider("Camera Blacks", -100.0f, 100.0f, -100.0f, 100.0f, 0.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_BLACKS, in_data));
    ERR(AddSlider("HDR Character", 0.0f, 100.0f, 0.0f, 100.0f, 68.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_HDR_CHARACTER, in_data));
    ERR(AddSlider("Low-Light Noise Reduction", 0.0f, 100.0f, 0.0f, 100.0f, 58.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_LOWLIGHT_NR, in_data));
    ERR(AddSlider("ISP Edge Sharpening", 0.0f, 120.0f, 0.0f, 120.0f, 62.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_SHARPENING, in_data));
    ERR(AddSlider("Corner Lens Softness", 0.0f, 100.0f, 0.0f, 100.0f, 14.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_LENS_SOFTNESS, in_data));
    ERR(AddSlider("Chromatic Aberration", 0.0f, 100.0f, 0.0f, 100.0f, 12.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_CHROMATIC_ABERRATION, in_data));
    ERR(AddSlider("Camera Vignette", 0.0f, 100.0f, 0.0f, 100.0f, 19.0f, 1, PF_ParamFlag_NONE, PARAM_CAMERA_VIGNETTE, in_data));

    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(PARAM_CAMERA_TOPIC_END);

    // ===================== 2014 FILTER GROUP =====================
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("2014 Filter", PARAM_FILTER_TOPIC_START);

    AEFX_CLR_STRUCT(def);
    def.flags = PF_ParamFlag_SUPERVISE;
    PF_ADD_POPUP(
        "Filter Preset",
        11,
        2, // Default: New York
        "Bypass (None)|New York|Miami|Los Angeles|Tokyo|Paris|London|Seattle|Berlin|Miami Night|Nostalgic Nights Reference",
        PARAM_FILTER_PRESET);

    ERR(AddSlider("Filter Strength", 0.0f, 100.0f, 0.0f, 100.0f, 85.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_STRENGTH, in_data));
    ERR(AddSlider("Exposure", -100.0f, 100.0f, -100.0f, 100.0f, -24.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_EXPOSURE, in_data));
    ERR(AddSlider("Brilliance", -100.0f, 100.0f, -100.0f, 100.0f, 12.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_BRILLIANCE, in_data));
    ERR(AddSlider("Brightness", -100.0f, 100.0f, -100.0f, 100.0f, 28.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_BRIGHTNESS, in_data));
    ERR(AddSlider("Contrast", -100.0f, 100.0f, -100.0f, 100.0f, 24.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_CONTRAST, in_data));
    ERR(AddSlider("Temperature", -100.0f, 100.0f, -100.0f, 100.0f, -38.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_TEMPERATURE, in_data));
    ERR(AddSlider("Tint", -100.0f, 100.0f, -100.0f, 100.0f, -22.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_TINT, in_data));
    ERR(AddSlider("Saturation", -100.0f, 100.0f, -100.0f, 100.0f, 12.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_SATURATION, in_data));
    ERR(AddSlider("Vibrance", -100.0f, 100.0f, -100.0f, 100.0f, -28.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_VIBRANCE, in_data));
    ERR(AddSlider("Highlights", -100.0f, 100.0f, -100.0f, 100.0f, 45.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_HIGHLIGHTS, in_data));
    ERR(AddSlider("Shadows", -100.0f, 100.0f, -100.0f, 100.0f, -22.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_SHADOWS, in_data));
    ERR(AddSlider("Whites", -100.0f, 100.0f, -100.0f, 100.0f, 18.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_WHITES, in_data));
    ERR(AddSlider("Blacks", -100.0f, 100.0f, -100.0f, 100.0f, 22.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_BLACKS, in_data));
    ERR(AddSlider("Fade", 0.0f, 100.0f, 0.0f, 100.0f, 12.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_FADE, in_data));
    ERR(AddSlider("Highlight Roll-Off", 0.0f, 100.0f, 0.0f, 100.0f, 25.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_HIGHLIGHT_ROLLOFF, in_data));
    ERR(AddSlider("Microcontrast", -100.0f, 100.0f, -100.0f, 100.0f, 48.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_MICROCONTRAST, in_data));
    ERR(AddSlider("Softness", 0.0f, 100.0f, 0.0f, 100.0f, 6.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_SOFTNESS, in_data));
    ERR(AddSlider("Vignette", 0.0f, 100.0f, 0.0f, 100.0f, 34.0f, 1, PF_ParamFlag_NONE, PARAM_FILTER_VIGNETTE, in_data));

    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(PARAM_FILTER_TOPIC_END);

    // ===================== GRAIN / SENSOR TEXTURE GROUP =====================
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Grain / Sensor Texture", PARAM_GRAIN_TOPIC_START);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(
        "Color Mode",
        6,
        1, // Default: Camera Profile
        "Camera Profile|Monochrome|Neutral Luma|Warm Chroma|Cool Chroma|Full Chroma",
        PARAM_GRAIN_COLOR_MODE);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP(
        "Render Mode",
        3,
        1, // Default: Accurate
        "Accurate|Preview|Draft",
        PARAM_GRAIN_RENDER_MODE);

    ERR(AddSlider("Amount", 0.0f, 100.0f, 0.0f, 100.0f, 42.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_AMOUNT, in_data));
    ERR(AddSlider("Size", 0.5f, 3.0f, 0.5f, 3.0f, 1.25f, 2, PF_ParamFlag_NONE, PARAM_GRAIN_SIZE, in_data));
    ERR(AddSlider("Density", 0.1f, 2.0f, 0.1f, 2.0f, 1.10f, 2, PF_ParamFlag_NONE, PARAM_GRAIN_DENSITY, in_data));
    ERR(AddSlider("Scale / Zoom", 0.5f, 2.5f, 0.5f, 2.5f, 1.00f, 2, PF_ParamFlag_NONE, PARAM_GRAIN_SCALE, in_data));
    ERR(AddSlider("Softness", 0.0f, 100.0f, 0.0f, 100.0f, 28.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_SOFTNESS, in_data));
    ERR(AddSlider("Speed", 0.0f, 3.0f, 0.0f, 3.0f, 1.00f, 2, PF_ParamFlag_NONE, PARAM_GRAIN_SPEED, in_data));
    ERR(AddSlider("Animation", 0.0f, 100.0f, 0.0f, 100.0f, 85.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_ANIMATION, in_data));
    ERR(AddSlider("Color / Chroma", 0.0f, 100.0f, 0.0f, 100.0f, 48.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_CHROMA, in_data));
    ERR(AddSlider("Seed", 0.0f, 99999.0f, 0.0f, 9999.0f, 2014.0f, 0, PF_ParamFlag_NONE, PARAM_GRAIN_SEED, in_data));

    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Advanced Grain Controls", PARAM_GRAIN_ADV_TOPIC_START);

    ERR(AddSlider("Shadows", 0.0f, 200.0f, 0.0f, 200.0f, 135.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_SHADOWS, in_data));
    ERR(AddSlider("Midtones", 0.0f, 200.0f, 0.0f, 200.0f, 95.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_MIDTONES, in_data));
    ERR(AddSlider("Highlights", 0.0f, 200.0f, 0.0f, 200.0f, 35.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_HIGHLIGHTS, in_data));
    ERR(AddSlider("Fine", 0.0f, 200.0f, 0.0f, 200.0f, 110.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_FINE, in_data));
    ERR(AddSlider("Coarse", 0.0f, 200.0f, 0.0f, 200.0f, 55.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_COARSE, in_data));
    ERR(AddSlider("Luma", 0.0f, 200.0f, 0.0f, 200.0f, 100.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_LUMA, in_data));
    ERR(AddSlider("Clustering", 0.0f, 100.0f, 0.0f, 100.0f, 42.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_CLUSTERING, in_data));
    ERR(AddSlider("Spatial Correlation", 0.0f, 100.0f, 0.0f, 100.0f, 52.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_SPATIAL_CORR, in_data));
    ERR(AddSlider("Temporal Coherence", 0.0f, 100.0f, 0.0f, 100.0f, 65.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_TEMPORAL_COHERENCE, in_data));
    ERR(AddSlider("Pattern Amount", 0.0f, 50.0f, 0.0f, 50.0f, 12.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_PATTERN_AMOUNT, in_data));
    ERR(AddSlider("Pattern Scale", 0.5f, 4.0f, 0.5f, 4.0f, 1.40f, 2, PF_ParamFlag_NONE, PARAM_GRAIN_PATTERN_SCALE, in_data));
    ERR(AddSlider("Shot Noise", 0.0f, 100.0f, 0.0f, 100.0f, 45.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_SHOT_NOISE, in_data));
    ERR(AddSlider("Read Noise", 0.0f, 100.0f, 0.0f, 100.0f, 62.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_READ_NOISE, in_data));
    ERR(AddSlider("Denoise Interaction", 0.0f, 100.0f, 0.0f, 100.0f, 45.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_DENOISE_INTERACTION, in_data));
    ERR(AddSlider("Sharpening Interaction", 0.0f, 100.0f, 0.0f, 100.0f, 50.0f, 1, PF_ParamFlag_NONE, PARAM_GRAIN_SHARPEN_INTERACTION, in_data));

    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(PARAM_GRAIN_ADV_TOPIC_END);

    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(PARAM_GRAIN_TOPIC_END);

    out_data->num_params = PARAM_COUNT;
    return err;
}

static PF_Err UserChangedParam(
    PF_InData* in_data,
    PF_OutData* out_data,
    PF_ParamDef* params[],
    PF_LayerDef* output,
    const PF_UserChangedParamExtra* which_hitP)
{
    (void)in_data;
    (void)out_data;
    (void)output;
    if (!which_hitP || !params) {
        return PF_Err_NONE;
    }

    if (which_hitP->param_index == PARAM_CAMERA_MODEL) {
        const auto modelId = static_cast<y2014::CameraModelId>(params[PARAM_CAMERA_MODEL]->u.pd.value);
        const y2014::CameraProfile preset = y2014::getCameraPreset(modelId);

        params[PARAM_CAMERA_STRENGTH]->u.fs_d.value = preset.strength * 100.0;
        params[PARAM_CAMERA_SATURATION]->u.fs_d.value = preset.saturation * 100.0;
        params[PARAM_CAMERA_TEMPERATURE]->u.fs_d.value = preset.temperature * 100.0;
        params[PARAM_CAMERA_HDR_CHARACTER]->u.fs_d.value = preset.hdrCharacter * 100.0;
        params[PARAM_CAMERA_LOWLIGHT_NR]->u.fs_d.value = preset.lowLightNR * 100.0;
        params[PARAM_CAMERA_SHARPENING]->u.fs_d.value = preset.sharpening * 100.0;
        params[PARAM_CAMERA_LENS_SOFTNESS]->u.fs_d.value = preset.lensSoftness * 100.0;
        params[PARAM_CAMERA_CHROMATIC_ABERRATION]->u.fs_d.value = preset.chromaticAberration * 100.0;
        params[PARAM_CAMERA_VIGNETTE]->u.fs_d.value = preset.vignette * 100.0;

        params[PARAM_CAMERA_STRENGTH]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        params[PARAM_CAMERA_SATURATION]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        params[PARAM_CAMERA_TEMPERATURE]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        params[PARAM_CAMERA_HDR_CHARACTER]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        params[PARAM_CAMERA_LOWLIGHT_NR]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        params[PARAM_CAMERA_SHARPENING]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        params[PARAM_CAMERA_LENS_SOFTNESS]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        params[PARAM_CAMERA_CHROMATIC_ABERRATION]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        params[PARAM_CAMERA_VIGNETTE]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
    } else if (which_hitP->param_index == PARAM_FILTER_PRESET) {
        const auto presetId = static_cast<y2014::FilterPresetId>(params[PARAM_FILTER_PRESET]->u.pd.value);
        const y2014::FilterParams f = y2014::getFilterPreset(presetId);

        params[PARAM_FILTER_STRENGTH]->u.fs_d.value = f.strength * 100.0;
        params[PARAM_FILTER_EXPOSURE]->u.fs_d.value = f.exposure;
        params[PARAM_FILTER_BRILLIANCE]->u.fs_d.value = f.brilliance;
        params[PARAM_FILTER_BRIGHTNESS]->u.fs_d.value = f.brightness;
        params[PARAM_FILTER_CONTRAST]->u.fs_d.value = f.contrast;
        params[PARAM_FILTER_TEMPERATURE]->u.fs_d.value = f.temperature;
        params[PARAM_FILTER_TINT]->u.fs_d.value = f.tint;
        params[PARAM_FILTER_SATURATION]->u.fs_d.value = f.saturation;
        params[PARAM_FILTER_VIBRANCE]->u.fs_d.value = f.vibrance;
        params[PARAM_FILTER_HIGHLIGHTS]->u.fs_d.value = f.highlights;
        params[PARAM_FILTER_SHADOWS]->u.fs_d.value = f.shadows;
        params[PARAM_FILTER_WHITES]->u.fs_d.value = f.whites;
        params[PARAM_FILTER_BLACKS]->u.fs_d.value = f.blacks;
        params[PARAM_FILTER_FADE]->u.fs_d.value = f.fade;
        params[PARAM_FILTER_HIGHLIGHT_ROLLOFF]->u.fs_d.value = f.highlightRolloff;
        params[PARAM_FILTER_MICROCONTRAST]->u.fs_d.value = f.microcontrast;
        params[PARAM_FILTER_SOFTNESS]->u.fs_d.value = f.softness;
        params[PARAM_FILTER_VIGNETTE]->u.fs_d.value = f.vignette;

        for (int idx = PARAM_FILTER_STRENGTH; idx <= PARAM_FILTER_VIGNETTE; ++idx) {
            params[idx]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        }
    }

    return PF_Err_NONE;
}

static y2014::RenderContext BuildRenderContextFromParams(
    PF_InData* in_data,
    PF_ParamDef* params[]) noexcept
{
    y2014::RenderContext ctx{};

    if (in_data && in_data->time_scale > 0) {
        ctx.timeSeconds = static_cast<float>(in_data->current_time) / static_cast<float>(in_data->time_scale);
    }

    if (!params) {
        return ctx;
    }

    const auto camModelId = static_cast<y2014::CameraModelId>(
        params[PARAM_CAMERA_MODEL] ? params[PARAM_CAMERA_MODEL]->u.pd.value : 3);
    ctx.camera = y2014::getCameraPreset(camModelId);

    if (params[PARAM_CAMERA_STRENGTH]) ctx.camera.strength = static_cast<float>(params[PARAM_CAMERA_STRENGTH]->u.fs_d.value) * 0.01f;
    if (params[PARAM_CAMERA_EXPOSURE]) ctx.camera.exposure = static_cast<float>(params[PARAM_CAMERA_EXPOSURE]->u.fs_d.value);
    if (params[PARAM_CAMERA_BRIGHTNESS]) ctx.camera.brightness = static_cast<float>(params[PARAM_CAMERA_BRIGHTNESS]->u.fs_d.value);
    if (params[PARAM_CAMERA_CONTRAST]) ctx.camera.contrast = static_cast<float>(params[PARAM_CAMERA_CONTRAST]->u.fs_d.value);
    if (params[PARAM_CAMERA_SATURATION]) ctx.camera.saturation = static_cast<float>(params[PARAM_CAMERA_SATURATION]->u.fs_d.value) * 0.01f;
    if (params[PARAM_CAMERA_TEMPERATURE]) {
        ctx.camera.temperature = static_cast<float>(params[PARAM_CAMERA_TEMPERATURE]->u.fs_d.value) * 0.01f;
        ctx.camera.warmth = ctx.camera.temperature;
    }
    if (params[PARAM_CAMERA_TINT]) ctx.camera.tint = static_cast<float>(params[PARAM_CAMERA_TINT]->u.fs_d.value) * 0.01f;
    if (params[PARAM_CAMERA_HIGHLIGHTS]) ctx.camera.highlights = static_cast<float>(params[PARAM_CAMERA_HIGHLIGHTS]->u.fs_d.value);
    if (params[PARAM_CAMERA_SHADOWS]) ctx.camera.shadows = static_cast<float>(params[PARAM_CAMERA_SHADOWS]->u.fs_d.value);
    if (params[PARAM_CAMERA_WHITES]) ctx.camera.whites = static_cast<float>(params[PARAM_CAMERA_WHITES]->u.fs_d.value);
    if (params[PARAM_CAMERA_BLACKS]) ctx.camera.blacks = static_cast<float>(params[PARAM_CAMERA_BLACKS]->u.fs_d.value);
    if (params[PARAM_CAMERA_HDR_CHARACTER]) ctx.camera.hdrCharacter = static_cast<float>(params[PARAM_CAMERA_HDR_CHARACTER]->u.fs_d.value) * 0.01f;
    if (params[PARAM_CAMERA_LOWLIGHT_NR]) ctx.camera.lowLightNR = static_cast<float>(params[PARAM_CAMERA_LOWLIGHT_NR]->u.fs_d.value) * 0.01f;
    if (params[PARAM_CAMERA_SHARPENING]) ctx.camera.sharpening = static_cast<float>(params[PARAM_CAMERA_SHARPENING]->u.fs_d.value) * 0.01f;
    if (params[PARAM_CAMERA_LENS_SOFTNESS]) ctx.camera.lensSoftness = static_cast<float>(params[PARAM_CAMERA_LENS_SOFTNESS]->u.fs_d.value) * 0.01f;
    if (params[PARAM_CAMERA_CHROMATIC_ABERRATION]) ctx.camera.chromaticAberration = static_cast<float>(params[PARAM_CAMERA_CHROMATIC_ABERRATION]->u.fs_d.value) * 0.01f;
    if (params[PARAM_CAMERA_VIGNETTE]) ctx.camera.vignette = static_cast<float>(params[PARAM_CAMERA_VIGNETTE]->u.fs_d.value) * 0.01f;

    const auto filterPresetId = static_cast<y2014::FilterPresetId>(
        params[PARAM_FILTER_PRESET] ? params[PARAM_FILTER_PRESET]->u.pd.value : 2);
    ctx.filter = y2014::getFilterPreset(filterPresetId);

    if (params[PARAM_FILTER_STRENGTH]) ctx.filter.strength = static_cast<float>(params[PARAM_FILTER_STRENGTH]->u.fs_d.value) * 0.01f;
    if (params[PARAM_FILTER_EXPOSURE]) ctx.filter.exposure = static_cast<float>(params[PARAM_FILTER_EXPOSURE]->u.fs_d.value);
    if (params[PARAM_FILTER_BRILLIANCE]) ctx.filter.brilliance = static_cast<float>(params[PARAM_FILTER_BRILLIANCE]->u.fs_d.value);
    if (params[PARAM_FILTER_BRIGHTNESS]) ctx.filter.brightness = static_cast<float>(params[PARAM_FILTER_BRIGHTNESS]->u.fs_d.value);
    if (params[PARAM_FILTER_CONTRAST]) ctx.filter.contrast = static_cast<float>(params[PARAM_FILTER_CONTRAST]->u.fs_d.value);
    if (params[PARAM_FILTER_TEMPERATURE]) ctx.filter.temperature = static_cast<float>(params[PARAM_FILTER_TEMPERATURE]->u.fs_d.value);
    if (params[PARAM_FILTER_TINT]) ctx.filter.tint = static_cast<float>(params[PARAM_FILTER_TINT]->u.fs_d.value);
    if (params[PARAM_FILTER_SATURATION]) ctx.filter.saturation = static_cast<float>(params[PARAM_FILTER_SATURATION]->u.fs_d.value);
    if (params[PARAM_FILTER_VIBRANCE]) ctx.filter.vibrance = static_cast<float>(params[PARAM_FILTER_VIBRANCE]->u.fs_d.value);
    if (params[PARAM_FILTER_HIGHLIGHTS]) ctx.filter.highlights = static_cast<float>(params[PARAM_FILTER_HIGHLIGHTS]->u.fs_d.value);
    if (params[PARAM_FILTER_SHADOWS]) ctx.filter.shadows = static_cast<float>(params[PARAM_FILTER_SHADOWS]->u.fs_d.value);
    if (params[PARAM_FILTER_WHITES]) ctx.filter.whites = static_cast<float>(params[PARAM_FILTER_WHITES]->u.fs_d.value);
    if (params[PARAM_FILTER_BLACKS]) ctx.filter.blacks = static_cast<float>(params[PARAM_FILTER_BLACKS]->u.fs_d.value);
    if (params[PARAM_FILTER_FADE]) ctx.filter.fade = static_cast<float>(params[PARAM_FILTER_FADE]->u.fs_d.value);
    if (params[PARAM_FILTER_HIGHLIGHT_ROLLOFF]) ctx.filter.highlightRolloff = static_cast<float>(params[PARAM_FILTER_HIGHLIGHT_ROLLOFF]->u.fs_d.value);
    if (params[PARAM_FILTER_MICROCONTRAST]) ctx.filter.microcontrast = static_cast<float>(params[PARAM_FILTER_MICROCONTRAST]->u.fs_d.value);
    if (params[PARAM_FILTER_SOFTNESS]) ctx.filter.softness = static_cast<float>(params[PARAM_FILTER_SOFTNESS]->u.fs_d.value);
    if (params[PARAM_FILTER_VIGNETTE]) ctx.filter.vignette = static_cast<float>(params[PARAM_FILTER_VIGNETTE]->u.fs_d.value);

    if (params[PARAM_GRAIN_COLOR_MODE]) ctx.grain.colorMode = static_cast<y2014::GrainColorMode>(params[PARAM_GRAIN_COLOR_MODE]->u.pd.value);
    if (params[PARAM_GRAIN_RENDER_MODE]) ctx.grain.renderMode = static_cast<y2014::GrainRenderMode>(params[PARAM_GRAIN_RENDER_MODE]->u.pd.value);
    if (params[PARAM_GRAIN_AMOUNT]) ctx.grain.amount = static_cast<float>(params[PARAM_GRAIN_AMOUNT]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_SIZE]) ctx.grain.size = static_cast<float>(params[PARAM_GRAIN_SIZE]->u.fs_d.value);
    if (params[PARAM_GRAIN_DENSITY]) ctx.grain.density = static_cast<float>(params[PARAM_GRAIN_DENSITY]->u.fs_d.value);
    if (params[PARAM_GRAIN_SCALE]) ctx.grain.scale = static_cast<float>(params[PARAM_GRAIN_SCALE]->u.fs_d.value);
    if (params[PARAM_GRAIN_SOFTNESS]) ctx.grain.softness = static_cast<float>(params[PARAM_GRAIN_SOFTNESS]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_SPEED]) ctx.grain.speed = static_cast<float>(params[PARAM_GRAIN_SPEED]->u.fs_d.value);
    if (params[PARAM_GRAIN_ANIMATION]) ctx.grain.animation = static_cast<float>(params[PARAM_GRAIN_ANIMATION]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_CHROMA]) {
        ctx.grain.chroma = static_cast<float>(params[PARAM_GRAIN_CHROMA]->u.fs_d.value) * 0.01f;
        ctx.grain.chromaNoise = ctx.grain.chroma;
    }
    if (params[PARAM_GRAIN_SEED]) ctx.grain.seed = static_cast<uint32_t>(std::max(0.0, params[PARAM_GRAIN_SEED]->u.fs_d.value));
    if (params[PARAM_GRAIN_SHADOWS]) ctx.grain.shadows = static_cast<float>(params[PARAM_GRAIN_SHADOWS]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_MIDTONES]) ctx.grain.midtones = static_cast<float>(params[PARAM_GRAIN_MIDTONES]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_HIGHLIGHTS]) ctx.grain.highlights = static_cast<float>(params[PARAM_GRAIN_HIGHLIGHTS]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_FINE]) ctx.grain.fine = static_cast<float>(params[PARAM_GRAIN_FINE]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_COARSE]) ctx.grain.coarse = static_cast<float>(params[PARAM_GRAIN_COARSE]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_LUMA]) ctx.grain.luma = static_cast<float>(params[PARAM_GRAIN_LUMA]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_CLUSTERING]) ctx.grain.clustering = static_cast<float>(params[PARAM_GRAIN_CLUSTERING]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_SPATIAL_CORR]) ctx.grain.spatialCorrelation = static_cast<float>(params[PARAM_GRAIN_SPATIAL_CORR]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_TEMPORAL_COHERENCE]) ctx.grain.temporalCoherence = static_cast<float>(params[PARAM_GRAIN_TEMPORAL_COHERENCE]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_PATTERN_AMOUNT]) ctx.grain.patternAmount = static_cast<float>(params[PARAM_GRAIN_PATTERN_AMOUNT]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_PATTERN_SCALE]) ctx.grain.patternScale = static_cast<float>(params[PARAM_GRAIN_PATTERN_SCALE]->u.fs_d.value);
    if (params[PARAM_GRAIN_SHOT_NOISE]) ctx.grain.shotNoise = static_cast<float>(params[PARAM_GRAIN_SHOT_NOISE]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_READ_NOISE]) ctx.grain.readNoise = static_cast<float>(params[PARAM_GRAIN_READ_NOISE]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_DENOISE_INTERACTION]) ctx.grain.denoiseInteraction = static_cast<float>(params[PARAM_GRAIN_DENOISE_INTERACTION]->u.fs_d.value) * 0.01f;
    if (params[PARAM_GRAIN_SHARPEN_INTERACTION]) ctx.grain.sharpeningInteraction = static_cast<float>(params[PARAM_GRAIN_SHARPEN_INTERACTION]->u.fs_d.value) * 0.01f;

    return ctx;
}

static PF_Err RenderLayerWorlds(
    PF_InData* in_data,
    PF_EffectWorld* input_worldP,
    PF_EffectWorld* output_worldP,
    const y2014::RenderContext& ctx)
{
    if (!input_worldP || !output_worldP || !input_worldP->data || !output_worldP->data) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    const int32_t width = std::min<int32_t>(input_worldP->width, output_worldP->width);
    const int32_t height = std::min<int32_t>(input_worldP->height, output_worldP->height);
    if (width <= 0 || height <= 0) {
        return PF_Err_NONE;
    }

    PF_PixelFormat format = PF_PixelFormat_ARGB32;
    if (in_data && in_data->pica_basicP) {
        const PF_WorldSuite2* wsP = nullptr;
        if (in_data->pica_basicP->AcquireSuite(
                kPFWorldSuite,
                kPFWorldSuiteVersion2,
                reinterpret_cast<const void**>(&wsP)) == kSPNoError && wsP)
        {
            wsP->PF_GetPixelFormat(output_worldP, &format);
            in_data->pica_basicP->ReleaseSuite(kPFWorldSuite, kPFWorldSuiteVersion2);
        } else if (PF_WORLD_IS_DEEP(output_worldP)) {
            format = PF_PixelFormat_ARGB64;
        }
    } else if (PF_WORLD_IS_DEEP(output_worldP)) {
        format = PF_PixelFormat_ARGB64;
    }

    const auto* srcBytes = reinterpret_cast<const uint8_t*>(input_worldP->data);
    auto* dstBytes = reinterpret_cast<uint8_t*>(output_worldP->data);

    switch (format) {
        case PF_PixelFormat_ARGB128:
            y2014::renderBufferFloat(
                srcBytes,
                input_worldP->rowbytes,
                dstBytes,
                output_worldP->rowbytes,
                width,
                height,
                ctx);
            break;

        case PF_PixelFormat_ARGB64:
            y2014::renderBuffer16(
                srcBytes,
                input_worldP->rowbytes,
                dstBytes,
                output_worldP->rowbytes,
                width,
                height,
                ctx);
            break;

        case PF_PixelFormat_ARGB32:
        default:
            y2014::renderBuffer8(
                srcBytes,
                input_worldP->rowbytes,
                dstBytes,
                output_worldP->rowbytes,
                width,
                height,
                ctx);
            break;
    }

    return PF_Err_NONE;
}

static PF_Err Render(
    PF_InData* in_data,
    PF_OutData* out_data,
    PF_ParamDef* params[],
    PF_LayerDef* output)
{
    (void)out_data;
    if (!in_data || !params || !params[PARAM_INPUT] || !output) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    const y2014::RenderContext ctx = BuildRenderContextFromParams(in_data, params);
    return RenderLayerWorlds(in_data, &params[PARAM_INPUT]->u.ld, output, ctx);
}

static PF_Err PreRender(
    PF_InData* in_data,
    PF_OutData* out_data,
    PF_PreRenderExtra* extra)
{
    (void)out_data;
    if (!in_data || !extra || !extra->input || !extra->output || !extra->cb) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    PF_RenderRequest req = extra->input->output_request;
    PF_CheckoutResult in_result;
    AEFX_CLR_STRUCT(in_result);

    PF_Err err = extra->cb->checkout_layer(
        in_data->effect_ref,
        PARAM_INPUT,
        PARAM_INPUT,
        &req,
        in_data->current_time,
        in_data->time_step,
        in_data->time_scale,
        &in_result);

    if (!err) {
        UnionRectSafe(&in_result.result_rect, &extra->output->result_rect);
        UnionRectSafe(&in_result.max_result_rect, &extra->output->max_result_rect);
    }
    return err;
}

static PF_Err SmartRender(
    PF_InData* in_data,
    PF_OutData* out_data,
    PF_SmartRenderExtra* extra)
{
    (void)out_data;
    if (!in_data || !extra || !extra->cb) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    PF_Err err = PF_Err_NONE;
    PF_Err err2 = PF_Err_NONE;

    PF_EffectWorld* input_worldP = nullptr;
    PF_EffectWorld* output_worldP = nullptr;

    ERR(extra->cb->checkout_layer_pixels(in_data->effect_ref, PARAM_INPUT, &input_worldP));
    ERR(extra->cb->checkout_output(in_data->effect_ref, &output_worldP));

    if (!err && input_worldP && output_worldP) {
        PF_ParamDef* paramPtrs[PARAM_COUNT] = {nullptr};
        PF_ParamDef checkedOutParams[PARAM_COUNT];
        for (int i = 0; i < PARAM_COUNT; ++i) {
            AEFX_CLR_STRUCT(checkedOutParams[i]);
        }

        if (in_data->inter.checkout_param) {
            for (int i = 1; i < PARAM_COUNT; ++i) {
                if (in_data->inter.checkout_param(
                        in_data->effect_ref,
                        i,
                        in_data->current_time,
                        in_data->time_step,
                        in_data->time_scale,
                        &checkedOutParams[i]) == PF_Err_NONE)
                {
                    paramPtrs[i] = &checkedOutParams[i];
                }
            }
        }

        const y2014::RenderContext ctx = BuildRenderContextFromParams(in_data, paramPtrs);
        ERR(RenderLayerWorlds(in_data, input_worldP, output_worldP, ctx));

        if (in_data->inter.checkin_param) {
            for (int i = 1; i < PARAM_COUNT; ++i) {
                if (paramPtrs[i]) {
                    ERR2(in_data->inter.checkin_param(in_data->effect_ref, paramPtrs[i]));
                }
            }
        }
    }

    if (input_worldP) {
        ERR2(extra->cb->checkin_layer_pixels(in_data->effect_ref, PARAM_INPUT));
    }
    return err;
}

extern "C" DllExport
PF_Err PluginDataEntryFunction2(
    PF_PluginDataPtr inPtr,
    PF_PluginDataCB2 inPluginDataCallBackPtr,
    SPBasicSuite* inSPBasicSuitePtr,
    const char* inHostName,
    const char* inHostVersion)
{
    (void)inSPBasicSuitePtr;
    (void)inHostName;
    (void)inHostVersion;
    PF_Err result = PF_Err_INVALID_CALLBACK;
    if (inPluginDataCallBackPtr) {
        result = PF_REGISTER_EFFECT_EXT2(
            inPtr,
            inPluginDataCallBackPtr,
            Y2014_PLUGIN_NAME,
            Y2014_PLUGIN_MATCH_NAME,
            Y2014_PLUGIN_CATEGORY,
            AE_RESERVED_INFO,
            "EffectMain",
            "https://github.com/vamprieieuu/2014");
    }
    return result;
}

extern "C" DllExport
PF_Err EffectMain(
    PF_Cmd cmd,
    PF_InData* in_data,
    PF_OutData* out_data,
    PF_ParamDef* params[],
    PF_LayerDef* output,
    void* extra)
{
    PF_Err err = PF_Err_NONE;
    try {
        switch (cmd) {
            case PF_Cmd_ABOUT:
                err = About(in_data, out_data, params, output);
                break;
            case PF_Cmd_GLOBAL_SETUP:
                err = GlobalSetup(in_data, out_data, params, output);
                break;
            case PF_Cmd_PARAMS_SETUP:
                err = ParamsSetup(in_data, out_data, params, output);
                break;
            case PF_Cmd_USER_CHANGED_PARAM:
                err = UserChangedParam(
                    in_data,
                    out_data,
                    params,
                    output,
                    reinterpret_cast<const PF_UserChangedParamExtra*>(extra));
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
    } catch (...) {
        err = PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
    return err;
}

#endif // Y2014_HAS_AE_SDK
