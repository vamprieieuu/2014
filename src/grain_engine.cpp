#include "grain_engine.h"
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
} // namespace y2014
