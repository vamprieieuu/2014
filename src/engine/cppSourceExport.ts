import { CameraProfile, FilterParams, GrainParams } from './types';

export function generateCppSourceBundle(
  camera: CameraProfile,
  filter: FilterParams,
  grain: GrainParams
): { filename: string; title: string; code: string }[] {
  const grainHeader = `#pragma once
#include <cstdint>

namespace y2014 {

struct GrainParams {
    float amount = ${grain.amount.toFixed(3)}f;
    float size = ${grain.size.toFixed(3)}f;
    float density = ${grain.density.toFixed(3)}f;
    float scale = ${grain.scale.toFixed(3)}f;
    float softness = ${grain.softness.toFixed(3)}f;
    float speed = ${grain.speed.toFixed(3)}f;
    float animation = ${grain.animation.toFixed(3)}f;
    float chroma = ${grain.chroma.toFixed(3)}f;
    float shadows = ${grain.shadows.toFixed(3)}f;
    float midtones = ${grain.midtones.toFixed(3)}f;
    float highlights = ${grain.highlights.toFixed(3)}f;
    float fine = ${grain.fine.toFixed(3)}f;
    float coarse = ${grain.coarse.toFixed(3)}f;
    float clustering = ${grain.clustering.toFixed(3)}f;
    float spatialCorrelation = ${grain.spatialCorrelation.toFixed(3)}f;
    float temporalCoherence = ${grain.temporalCoherence.toFixed(3)}f;
    float shotNoise = ${grain.shotNoise.toFixed(3)}f;
    float readNoise = ${grain.readNoise.toFixed(3)}f;
    float chromaNoise = ${grain.chromaNoise.toFixed(3)}f;
    float patternAmount = ${grain.patternAmount.toFixed(3)}f;
    float patternScale = ${grain.patternScale.toFixed(3)}f;
    uint32_t seed = ${grain.seed}U;
};

// Deterministic continuous 3D noise sample used by the final AE renderer.
float continuousNoise3D(float x, float y, float t, uint32_t seed);

} // namespace y2014`;

  const grainCpp = `#include "grain_engine.h"
#include <cmath>
#include <cstdint>

namespace y2014 {
static inline uint32_t hash32(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352dU;
    x ^= x >> 15; x *= 0x846ca68bU;
    x ^= x >> 16; return x;
}
static inline float u01(uint32_t x) { return (hash32(x) & 0x00ffffffU) / 16777216.0f; }
static inline float fade(float x) { return x*x*x*(x*(x*6.0f-15.0f)+10.0f); }
static inline float lerp(float a,float b,float t){ return a+(b-a)*t; }

float continuousNoise3D(float x, float y, float t, uint32_t seed) {
    const int x0 = (int)std::floor(x), y0=(int)std::floor(y), z0=(int)std::floor(t);
    const float fx=x-x0, fy=y-y0, fz=t-z0;
    auto n=[&](int xi,int yi,int zi){
        uint32_t h=(uint32_t)xi*0x9e3779b9U ^ (uint32_t)yi*0x85ebca6bU ^ (uint32_t)zi*0xc2b2ae35U ^ seed;
        return u01(h)*2.0f-1.0f;
    };
    float c000=n(x0,y0,z0), c100=n(x0+1,y0,z0), c010=n(x0,y0+1,z0), c110=n(x0+1,y0+1,z0);
    float c001=n(x0,y0,z0+1), c101=n(x0+1,y0,z0+1), c011=n(x0,y0+1,z0+1), c111=n(x0+1,y0+1,z0+1);
    float u=fade(fx),v=fade(fy),w=fade(fz);
    float a=lerp(lerp(c000,c100,u),lerp(c010,c110,u),v);
    float b=lerp(lerp(c001,c101,u),lerp(c011,c111,u),v);
    return lerp(a,b,w);
}
} // namespace y2014`;

  const cameraCpp = `#include <algorithm>
#include <cmath>

namespace y2014 {
struct CameraProfile {
    float warmth, saturation, lowLightNR, detail, sharpening;
    float highlightRollOff, shadowLift, lensSoftness, vignette;
    float distortion, chromaNoise, hdrCharacter;
};

// Active preset: ${camera.name}
static const CameraProfile kActiveCameraProfile = {
    ${camera.warmth.toFixed(3)}f, ${camera.saturation.toFixed(3)}f, ${camera.lowLightNR.toFixed(3)}f,
    ${camera.detail.toFixed(3)}f, ${camera.sharpening.toFixed(3)}f, ${camera.highlightRollOff.toFixed(3)}f,
    ${camera.shadowLift.toFixed(3)}f, ${camera.lensSoftness.toFixed(3)}f, ${camera.vignette.toFixed(3)}f,
    ${camera.distortion.toFixed(3)}f, ${camera.chromaNoise.toFixed(3)}f, ${camera.hdrCharacter.toFixed(3)}f
};

static float clamp01(float v){ return std::max(0.0f,std::min(1.0f,v)); }

void applyCameraCharacter(float& r,float& g,float& b,const CameraProfile& p,float strength){
    strength=clamp01(strength);
    const float y=0.2126f*r+0.7152f*g+0.0722f*b;
    const float sat=1.0f+p.saturation*strength;
    r=y+(r-y)*sat; g=y+(g-y)*sat; b=y+(b-y)*sat;
    r=clamp01(r+p.warmth*0.02f*strength);
    b=clamp01(b-p.warmth*0.01f*strength);
}
} // namespace y2014`;

  const filterStateJson = JSON.stringify(
    {
      activeCamera: camera.id,
      cameraStrength: camera.strength,
      activeFilter: filter.presetId,
      filterStrength: filter.strength,
      filterControls: filter,
      grainControls: grain,
    },
    null,
    2
  );

  return [
    { filename: 'src/grain_engine.h', title: 'grain_engine.h', code: grainHeader },
    { filename: 'src/grain_engine.cpp', title: 'grain_engine.cpp', code: grainCpp },
    { filename: 'src/camera_profiles.cpp', title: 'camera_profiles.cpp', code: cameraCpp },
    { filename: 'spec/active_preset_state.json', title: 'active_preset_state.json', code: filterStateJson },
  ];
}
