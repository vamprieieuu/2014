#pragma once
#ifndef Y2014_EFFECT_H
#define Y2014_EFFECT_H

#define PF_DEEP_COLOR_AWARE 1

#if !defined(_MSC_VER) && !defined(__MINGW32__) && !defined(__int64)
    #define __int64 long long
#endif

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__ANDROID__)
    #define Y2014_TEMP_WIN32_FOR_AECONFIG 1
    #define _WIN32 1
#endif
#ifdef AE_OS_WIN
    #undef AE_OS_WIN
#endif
#ifdef AE_PROC_INTELx64
    #undef AE_PROC_INTELx64
#endif

#include "AEConfig.h"

#ifdef Y2014_TEMP_WIN32_FOR_AECONFIG
    #undef _WIN32
    #undef Y2014_TEMP_WIN32_FOR_AECONFIG
#endif

#ifdef AE_OS_WIN
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #if defined(_MSC_VER)
        #include <Windows.h>
    #elif defined(__has_include)
        #if __has_include(<windows.h>)
            #include <windows.h>
        #elif __has_include(<Windows.h>)
            #include <Windows.h>
        #endif
    #endif
#endif

#if defined(__has_include)
    #if __has_include("entry.h")
        #include "entry.h"
    #endif
#endif

#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_EffectCBSuites.h"
#include "AE_EffectSuites.h"
#include "AE_Macros.h"
#include "AE_PluginData.h"
#include "AEFX_SuiteHandlerTemplate.h"
#include "PrSDKAESupport.h"

#if defined(__has_include)
    #if __has_include("Param_Utils.h")
        #include "Param_Utils.h"
    #endif
#endif

#include "2014_engine.h"

#ifndef DllExport
    #if defined(_MSC_VER) || defined(__MINGW32__)
        #define DllExport __declspec(dllexport)
    #elif defined(__GNUC__)
        #define DllExport __attribute__((visibility("default")))
    #else
        #define DllExport
    #endif
#endif

#ifndef AE_RESERVED_INFO
    #define AE_RESERVED_INFO 8
#endif

#ifndef PF_REGISTER_EFFECT_EXT2
    #define PF_REGISTER_EFFECT_EXT2(IN_PTR, CB_PTR, NAME, MATCH_NAME, CATEGORY, RESERVED_INFO, ENTRY_POINT, SUPPORT_URL) \
        (*(CB_PTR))( \
            (IN_PTR), \
            reinterpret_cast<const A_u_char*>(NAME), \
            reinterpret_cast<const A_u_char*>(MATCH_NAME), \
            reinterpret_cast<const A_u_char*>(CATEGORY), \
            reinterpret_cast<const A_u_char*>(ENTRY_POINT), \
            0x65464B54UL, \
            PF_AE_PLUG_IN_VERSION, \
            PF_AE_PLUG_IN_SUBVERS, \
            (RESERVED_INFO), \
            reinterpret_cast<const A_u_char*>(SUPPORT_URL))
#endif

#define Y2014_MAJOR_VERSION 1
#define Y2014_MINOR_VERSION 0
#define Y2014_BUG_VERSION   0
#define Y2014_STAGE_VERSION PF_Stage_RELEASE
#define Y2014_BUILD_VERSION 1

#define Y2014_EFFECT_NAME        "2014"
#define Y2014_EFFECT_MATCH_NAME  "ADBE 2014"
#define Y2014_EFFECT_CATEGORY    "2014 Nostalgia"
#define Y2014_EFFECT_DESCRIPTION "2014 Camera Model, City Filter & Procedural iPhone Sensor Grain Engine."

enum Y2014ParamIndex {
    Y2014_INPUT = 0,

    // Group 1: Camera Model
    Y2014_TOPIC_CAMERA_START,
    Y2014_CAMERA_MODEL,
    Y2014_CAMERA_STRENGTH,
    Y2014_CAMERA_EXPOSURE_TRIM,
    Y2014_CAMERA_HDR_CHARACTER,
    Y2014_CAMERA_TONE_COMPRESSION,
    Y2014_CAMERA_WARMTH,
    Y2014_CAMERA_TINT,
    Y2014_CAMERA_LOWLIGHT_DENOISE,
    Y2014_CAMERA_ISP_SHARPENING,
    Y2014_CAMERA_LENS_VIGNETTE,
    Y2014_CAMERA_EDGE_SOFTNESS,
    Y2014_CAMERA_JPEG_ARTIFACT,
    Y2014_TOPIC_CAMERA_END,

    // Group 2: 2014 Filter
    Y2014_TOPIC_FILTER_START,
    Y2014_FILTER_PRESET,
    Y2014_FILTER_STRENGTH,
    Y2014_FILTER_EXPOSURE,
    Y2014_FILTER_BRILLIANCE,
    Y2014_FILTER_HIGHLIGHTS,
    Y2014_FILTER_SHADOWS,
    Y2014_FILTER_CONTRAST,
    Y2014_FILTER_BRIGHTNESS,
    Y2014_FILTER_BLACK_POINT,
    Y2014_FILTER_FADE,
    Y2014_FILTER_SATURATION,
    Y2014_FILTER_VIBRANCE,
    Y2014_FILTER_WARMTH,
    Y2014_FILTER_TINT,
    Y2014_FILTER_SHARPNESS,
    Y2014_FILTER_DEFINITION,
    Y2014_FILTER_VIGNETTE,
    Y2014_FILTER_CENTER_BRIGHTNESS,
    Y2014_FILTER_SOFTENING,
    Y2014_FILTER_SPLIT_BALANCE,
    Y2014_TOPIC_FILTER_END,

    // Group 3: Grain / Noise (Main)
    Y2014_TOPIC_GRAIN_MAIN_START,
    Y2014_GRAIN_AMOUNT,
    Y2014_GRAIN_SIZE,
    Y2014_GRAIN_DENSITY,
    Y2014_GRAIN_SCALE_ZOOM,
    Y2014_GRAIN_SOFTNESS,
    Y2014_GRAIN_SPEED,
    Y2014_GRAIN_ANIMATION,
    Y2014_GRAIN_COLOR_MODE,
    Y2014_GRAIN_CHROMA_AMOUNT,
    Y2014_GRAIN_SEED,
    Y2014_TOPIC_GRAIN_MAIN_END,

    // Group 4: Grain / Noise (Advanced)
    Y2014_TOPIC_GRAIN_ADV_START,
    Y2014_GRAIN_SHADOWS,
    Y2014_GRAIN_MIDTONES,
    Y2014_GRAIN_HIGHLIGHTS,
    Y2014_GRAIN_BLACK_RESPONSE,
    Y2014_GRAIN_WHITE_RESPONSE,
    Y2014_GRAIN_FINE,
    Y2014_GRAIN_COARSE,
    Y2014_GRAIN_LUMA_CHROMA_BAL,
    Y2014_GRAIN_CLUSTERING,
    Y2014_GRAIN_SPATIAL_CORR,
    Y2014_GRAIN_TEMPORAL_COHERENCE,
    Y2014_GRAIN_PATTERN_AMOUNT,
    Y2014_GRAIN_PATTERN_SCALE,
    Y2014_GRAIN_SHOT_READ_BAL,
    Y2014_GRAIN_SHADOW_FLOOR,
    Y2014_GRAIN_CHROMA_CORR,
    Y2014_GRAIN_CHROMA_HUE_BIAS,
    Y2014_GRAIN_DENOISE_INTERACT,
    Y2014_GRAIN_SHARPEN_INTERACT,
    Y2014_TOPIC_GRAIN_ADV_END,

    // Performance Mode
    Y2014_PERFORMANCE_MODE,

    Y2014_NUM_PARAMS
};

extern "C" {

DllExport
PF_Err
EffectMain(
    PF_Cmd          cmd,
    PF_InData       *in_data,
    PF_OutData      *out_data,
    PF_ParamDef     *params[],
    PF_LayerDef     *output,
    void            *extra);

DllExport
PF_Err
PluginDataEntryFunction2(
    PF_PluginDataPtr    inPtr,
    PF_PluginDataCB2    inPluginDataCallBackPtr,
    SPBasicSuite*       inSPBasicSuitePtr,
    const char*         inHostName,
    const char*         inHostVersion);

}

#endif // Y2014_EFFECT_H
