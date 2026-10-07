#include <algorithm>
#include <cmath>

namespace y2014 {
struct CameraProfile {
    float warmth, saturation, lowLightNR, detail, sharpening;
    float highlightRollOff, shadowLift, lensSoftness, vignette;
    float distortion, chromaNoise, hdrCharacter;
};
static float clamp01(float v){ return std::max(0.0f,std::min(1.0f,v)); }

// Research-stage camera transform. Final values must be calibrated against
// the supplied reference set; they are intentionally not presented as
// Apple's proprietary ISP coefficients.
void applyCameraCharacter(float& r,float& g,float& b,const CameraProfile& p,float strength){
    strength=clamp01(strength);
    const float y=0.2126f*r+0.7152f*g+0.0722f*b;
    const float sat=1.0f+p.saturation*strength;
    r=y+(r-y)*sat; g=y+(g-y)*sat; b=y+(b-y)*sat;
    r=clamp01(r+p.warmth*0.02f*strength);
    b=clamp01(b-p.warmth*0.01f*strength);
}
}
