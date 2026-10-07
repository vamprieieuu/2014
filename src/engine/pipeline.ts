import type { CameraProfile, FilterParams, GrainParams } from './types.ts';

function clamp01(v: number): number {
  return v < 0 ? 0 : v > 1 ? 1 : v;
}

function lerp(a: number, b: number, t: number): number {
  return a + (b - a) * t;
}

function fade(x: number): number {
  return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
}

// Exact bit-exact port of y2014::hash32 & u01 from src/grain_engine.cpp
export function hash32(x: number): number {
  let v = x >>> 0;
  v = (v ^ (v >>> 16)) >>> 0;
  v = Math.imul(v, 0x7feb352d) >>> 0;
  v = (v ^ (v >>> 15)) >>> 0;
  v = Math.imul(v, 0x846ca68b) >>> 0;
  v = (v ^ (v >>> 16)) >>> 0;
  return v;
}

export function u01(x: number): number {
  return (hash32(x) & 0x00ffffff) / 16777216.0;
}

// Exact port of y2014::continuousNoise3D from src/grain_engine.cpp
export function continuousNoise3D(x: number, y: number, t: number, seed: number): number {
  const x0 = Math.floor(x);
  const y0 = Math.floor(y);
  const z0 = Math.floor(t);
  const fx = x - x0;
  const fy = y - y0;
  const fz = t - z0;

  const s = seed >>> 0;
  const n = (xi: number, yi: number, zi: number): number => {
    const h =
      (Math.imul(xi, 0x9e3779b9) ^
        Math.imul(yi, 0x85ebca6b) ^
        Math.imul(zi, 0xc2b2ae35) ^
        s) >>>
      0;
    return u01(h) * 2.0 - 1.0;
  };

  const c000 = n(x0, y0, z0);
  const c100 = n(x0 + 1, y0, z0);
  const c010 = n(x0, y0 + 1, z0);
  const c110 = n(x0 + 1, y0 + 1, z0);
  const c001 = n(x0, y0, z0 + 1);
  const c101 = n(x0 + 1, y0, z0 + 1);
  const c011 = n(x0, y0 + 1, z0 + 1);
  const c111 = n(x0 + 1, y0 + 1, z0 + 1);

  const u = fade(fx);
  const v = fade(fy);
  const w = fade(fz);

  const a = lerp(lerp(c000, c100, u), lerp(c010, c110, u), v);
  const b = lerp(lerp(c001, c101, u), lerp(c011, c111, u), v);
  return lerp(a, b, w);
}

export interface PipelineMetrics {
  meanLuma: number;
  shadowNoiseRms: number;
  chromaSpread: number;
  histogramR: number[];
  histogramG: number[];
  histogramB: number[];
  histogramY: number[];
}

/**
 * Full 6-stage pixel pipeline matching docs/GOOGLE_AI_STUDIO_BUILD_BRIEF.md:
 * Input
 *  -> 1. Camera Model (lens distortion, CA, vignette, ISP color/tone & HDR local character)
 *  -> 2. Sensor/Process Texture (low-light NR & edge ringing/sharpening interaction)
 *  -> 3. 2014 Filter Engine (exposure, brilliance, split-toning, fade, microcontrast)
 *  -> 4. Adaptive Animated iPhone-like Grain/Noise (Shot + Read + Correlated Chroma + PRNU + 3D Coherent/Stochastic)
 *  -> Output
 */
export function processFrameBuffer(
  srcData: Uint8ClampedArray,
  dstData: Uint8ClampedArray,
  width: number,
  height: number,
  camera: CameraProfile,
  filter: FilterParams,
  grain: GrainParams,
  timeSeconds: number,
  splitRatio: number | null // null = full processed, 0..1 = split wipe X position
): PipelineMetrics {
  const histR = new Array<number>(64).fill(0);
  const histG = new Array<number>(64).fill(0);
  const histB = new Array<number>(64).fill(0);
  const histY = new Array<number>(64).fill(0);

  let sumLuma = 0;
  let shadowNoiseSq = 0;
  let shadowCount = 0;
  let chromaDiffSum = 0;

  const camStrength = clamp01(camera.strength);
  const filtStrength = clamp01(filter.strength);

  // Resolution normalization factor so grain looks consistent across resolutions
  const resNorm = Math.sqrt((width * height) / (1280 * 720));
  const effectiveGrainSize = Math.max(0.35, grain.size * grain.scale * resNorm);
  const invFineFreq = 1.0 / effectiveGrainSize;
  const invCoarseFreq = 1.0 / (effectiveGrainSize * 3.1);
  const invPatternFreq = 1.0 / (Math.max(0.5, grain.patternScale) * 18.0 * resNorm);

  // Temporal coordinate: combines smooth 3D advection + controlled stochastic frame offset
  const animActive = grain.animation > 0.001;
  const tCoord = animActive ? timeSeconds * grain.speed * 5.5 * grain.animation : 0.0;
  const frameIndex = animActive ? Math.floor(timeSeconds * 24.0 * Math.max(0.2, grain.speed)) : 0;
  const coherence = clamp01(grain.temporalCoherence);

  // Precompute filter scalars
  const expMul = Math.pow(2.0, (filter.exposure / 100.0) * 1.35 * filtStrength);
  const brillianceAmt = (filter.brilliance / 100.0) * 0.45 * filtStrength;
  const brightAmt = (filter.brightness / 100.0) * 0.38 * filtStrength;
  const contrastAmt = (filter.contrast / 100.0) * 0.65 * filtStrength;
  const tempAmt = (filter.temperature / 100.0) * 0.16 * filtStrength;
  const tintAmt = (filter.tint / 100.0) * 0.12 * filtStrength;
  const satAmt = (filter.saturation / 100.0) * 0.65 * filtStrength;
  const vibAmt = (filter.vibrance / 100.0) * 0.65 * filtStrength;
  const hiAmt = (filter.highlights / 100.0) * 0.42 * filtStrength;
  const shAmt = (filter.shadows / 100.0) * 0.45 * filtStrength;
  const whitesAmt = (filter.whites / 100.0) * 0.25 * filtStrength;
  const blacksAmt = (filter.blacks / 100.0) * 0.22 * filtStrength;
  const fadeAmt = (filter.fade / 100.0) * 0.24 * filtStrength;
  const rolloffAmt =
    (filter.highlightRolloff / 100.0) * 0.45 * filtStrength +
    camera.highlightRollOff * 0.35 * camStrength;
  const microAmt = (filter.microcontrast / 100.0) * 0.55 * filtStrength;
  const totalVignette =
    (filter.vignette / 100.0) * 0.55 * filtStrength + camera.vignette * 0.45 * camStrength;

  const cx = width * 0.5;
  const cy = height * 0.5;
  const invMaxR = 1.0 / Math.sqrt(cx * cx + cy * cy);

  const splitX = splitRatio !== null ? Math.floor(width * splitRatio) : -1;

  for (let y = 0; y < height; y++) {
    const dy = (y - cy) * invMaxR;
    const rowOffset = y * width;

    // Subtle row-level sensor variation (PRNU row bias)
    const rowNoise =
      grain.patternAmount > 0.001
        ? (u01((y * 2654435761 + grain.seed) >>> 0) * 2.0 - 1.0) * grain.patternAmount * 0.04
        : 0.0;

    for (let x = 0; x < width; x++) {
      const idx = (rowOffset + x) * 4;

      if (splitX >= 0 && x < splitX) {
        dstData[idx] = srcData[idx];
        dstData[idx + 1] = srcData[idx + 1];
        dstData[idx + 2] = srcData[idx + 2];
        dstData[idx + 3] = 255;
        continue;
      }

      const dx = (x - cx) * invMaxR;
      const rSq = dx * dx + dy * dy;

      // 1. Lens Distortion & Chromatic Aberration (Camera Model)
      let r = srcData[idx] / 255.0;
      let g = srcData[idx + 1] / 255.0;
      let b = srcData[idx + 2] / 255.0;

      if (camStrength > 0.01 && (camera.chromaticAberration > 0.01 || camera.lensSoftness > 0.01)) {
        const caShift = Math.round(rSq * camera.chromaticAberration * camStrength * 4.5);
        if (caShift > 0) {
          const rx = Math.min(width - 1, Math.max(0, x + (dx >= 0 ? caShift : -caShift)));
          const bx = Math.min(width - 1, Math.max(0, x - (dx >= 0 ? caShift : -caShift)));
          r = srcData[(rowOffset + rx) * 4] / 255.0;
          b = srcData[(rowOffset + bx) * 4 + 2] / 255.0;
        }
      }

      // Compute local 4-neighbor detail for ISP sharpening/ringing, low-light NR, and filter microcontrast
      const xLeft = x > 0 ? x - 1 : x;
      const xRight = x < width - 1 ? x + 1 : x;
      const yUp = y > 0 ? y - 1 : y;
      const yDown = y < height - 1 ? y + 1 : y;

      const idxL = (rowOffset + xLeft) * 4;
      const idxR = (rowOffset + xRight) * 4;
      const idxU = (yUp * width + x) * 4;
      const idxD = (yDown * width + x) * 4;

      const avgR = (srcData[idxL] + srcData[idxR] + srcData[idxU] + srcData[idxD]) * (0.25 / 255.0);
      const avgG =
        (srcData[idxL + 1] + srcData[idxR + 1] + srcData[idxU + 1] + srcData[idxD + 1]) *
        (0.25 / 255.0);
      const avgB =
        (srcData[idxL + 2] + srcData[idxR + 2] + srcData[idxU + 2] + srcData[idxD + 2]) *
        (0.25 / 255.0);

      let luma = 0.2126 * r + 0.7152 * g + 0.0722 * b;
      const avgLuma = 0.2126 * avgR + 0.7152 * avgG + 0.0722 * avgB;
      const highFreq = luma - avgLuma;

      // 2. Camera Model ISP Character (from src/camera_profiles.cpp + HDR + NR/Sharpening)
      const camSat = 1.0 + camera.saturation * camStrength;
      r = luma + (r - luma) * camSat;
      g = luma + (g - luma) * camSat;
      b = luma + (b - luma) * camSat;

      r += camera.warmth * 0.055 * camStrength;
      b -= camera.warmth * 0.038 * camStrength;

      // Early iPhone HDR / Shadow Lift & Highlight Roll-off
      const shadowMask = (1.0 - luma) * (1.0 - luma);
      const highlightMask = luma * luma;
      const hdrLift = camera.shadowLift * 0.18 * camStrength * shadowMask;
      const hdrPull = camera.hdrCharacter * 0.14 * camStrength * highlightMask;
      r += hdrLift - hdrPull;
      g += hdrLift - hdrPull;
      b += hdrLift - hdrPull;

      // Sensor/ISP Low-light Noise Reduction vs Edge Ringing/Sharpening
      const isLowLight = clamp01(1.0 - luma * 1.65);
      const nrSmooth = camera.lowLightNR * isLowLight * camStrength * 0.55;
      const edgeSharp =
        (camera.sharpening * (1.0 - nrSmooth * 0.6) -
          camera.lensSoftness * rSq * 0.85) *
          camStrength *
          0.95 +
        microAmt -
        (filter.softness / 100.0) * 0.5 * filtStrength;

      const detailDelta = highFreq * edgeSharp - highFreq * nrSmooth;
      r += detailDelta;
      g += detailDelta;
      b += detailDelta;

      // 3. 2014 Filter Engine
      if (filtStrength > 0.001) {
        // Exposure & Brightness (midtone gamma lift)
        r *= expMul;
        g *= expMul;
        b *= expMul;

        if (Math.abs(brightAmt) > 0.001) {
          const midLift = brightAmt * (1.0 - clamp01(r) * 0.65);
          r += midLift;
          g += brightAmt * (1.0 - clamp01(g) * 0.65);
          b += brightAmt * (1.0 - clamp01(b) * 0.65);
        }

        luma = clamp01(0.2126 * r + 0.7152 * g + 0.0722 * b);

        // Brilliance, Shadows, Highlights, Blacks, Whites
        const shWeight = Math.pow(1.0 - luma, 1.8);
        const hiWeight = Math.pow(luma, 1.8);
        const midWeight = 4.0 * luma * (1.0 - luma);

        const toneDelta =
          shAmt * shWeight +
          brillianceAmt * (shWeight * 0.6 + midWeight * 0.5) +
          hiAmt * hiWeight * 0.45 +
          whitesAmt * Math.pow(luma, 3.0) -
          blacksAmt * Math.pow(1.0 - luma, 2.6);

        r += toneDelta;
        g += toneDelta;
        b += toneDelta;

        // Contrast S-Curve around 0.46 pivot
        if (Math.abs(contrastAmt) > 0.001) {
          r = 0.46 + (r - 0.46) * (1.0 + contrastAmt);
          g = 0.46 + (g - 0.46) * (1.0 + contrastAmt);
          b = 0.46 + (b - 0.46) * (1.0 + contrastAmt);
        }

        // Temperature & Tint (2014 warm/cool & green/magenta shift)
        r += tempAmt - tintAmt * 0.35;
        g += tintAmt * 0.65;
        b -= tempAmt + tintAmt * 0.45;

        // 3-Way Color Split (Shadow / Midtone / Highlight casts)
        const sR = filter.shadowColor[0] * shWeight * filtStrength * 0.45;
        const sG = filter.shadowColor[1] * shWeight * filtStrength * 0.45;
        const sB = filter.shadowColor[2] * shWeight * filtStrength * 0.45;

        const mR = filter.midtoneColor[0] * midWeight * filtStrength * 0.35;
        const mG = filter.midtoneColor[1] * midWeight * filtStrength * 0.35;
        const mB = filter.midtoneColor[2] * midWeight * filtStrength * 0.35;

        const hR = filter.highlightColor[0] * hiWeight * filtStrength * 0.45;
        const hG = filter.highlightColor[1] * hiWeight * filtStrength * 0.45;
        const hB = filter.highlightColor[2] * hiWeight * filtStrength * 0.45;

        r += sR + mR + hR;
        g += sG + mG + hG;
        b += sB + mB + hB;

        // Saturation & Vibrance (selective low-sat boost/cut)
        luma = clamp01(0.2126 * r + 0.7152 * g + 0.0722 * b);
        const maxC = Math.max(r, g, b);
        const minC = Math.min(r, g, b);
        const curSat = clamp01(maxC - minC);
        const vibFactor = vibAmt * (1.0 - curSat * curSat);
        const totalSat = Math.max(0.0, 1.0 + satAmt + vibFactor);

        r = luma + (r - luma) * totalSat;
        g = luma + (g - luma) * totalSat;
        b = luma + (b - luma) * totalSat;

        // Fade (lifted black floor characteristic of 2014 presets)
        if (fadeAmt > 0.001) {
          r = fadeAmt + r * (1.0 - fadeAmt * 0.85);
          g = fadeAmt + g * (1.0 - fadeAmt * 0.85);
          b = fadeAmt + b * (1.0 - fadeAmt * 0.85);
        }
      }

      // Highlight Roll-off & Vignette
      if (rolloffAmt > 0.001) {
        if (r > 0.65) r = 0.65 + (r - 0.65) / (1.0 + (r - 0.65) * rolloffAmt * 2.2);
        if (g > 0.65) g = 0.65 + (g - 0.65) / (1.0 + (g - 0.65) * rolloffAmt * 2.2);
        if (b > 0.65) b = 0.65 + (b - 0.65) / (1.0 + (b - 0.65) * rolloffAmt * 2.2);
      }

      if (totalVignette > 0.001) {
        const vigMask = Math.max(0.0, 1.0 - rSq * totalVignette * 1.35);
        r *= vigMask;
        g *= vigMask;
        b *= vigMask;
      }

      r = clamp01(r);
      g = clamp01(g);
      b = clamp01(b);
      luma = 0.2126 * r + 0.7152 * g + 0.0722 * b;

      // 4. Adaptive Animated iPhone-Like Grain/Noise Engine
      if (grain.amount > 0.001) {
        // Tonal response curve: Black -> Shadow -> Midtone -> Highlight -> White
        // Pure black (0.0) and pure white (1.0) roll off smoothly so there is no artificial static overlay
        const blackFloor = clamp01(luma / 0.06 + 0.25);
        const whiteCeil = clamp01((1.0 - luma) / 0.08 + 0.15);
        const shZone = Math.pow(1.0 - luma, 1.7) * grain.shadows;
        const midZone = 4.0 * luma * (1.0 - luma) * grain.midtones;
        const hiZone = Math.pow(luma, 1.8) * grain.highlights;
        const tonalWeight = (shZone * 0.55 + midZone * 0.35 + hiZone * 0.25) * blackFloor * whiteCeil;

        // Sensor physical model: Poisson-like Shot Noise (scales with sqrt(signal)) + Gaussian-like Read Noise
        const shotComponent = Math.sqrt(Math.max(0.02, luma)) * grain.shotNoise;
        const readComponent = (1.0 - luma * 0.65) * grain.readNoise;
        const sensorGain = 0.35 + shotComponent * 0.55 + readComponent * 0.65;

        // Coherent 3D continuous noise + stochastic frame component
        let fineSample = 0.0;
        let coarseSample = 0.0;

        if (grain.renderMode === 'draft') {
          const fastHash =
            (Math.imul(x, 0x9e3779b9) ^
              Math.imul(y, 0x85ebca6b) ^
              Math.imul(frameIndex, 0xc2b2ae35) ^
              grain.seed) >>>
            0;
          fineSample = u01(fastHash) * 2.0 - 1.0;
        } else {
          const nx = x * invFineFreq;
          const ny = y * invFineFreq;
          const coherentFine = continuousNoise3D(nx, ny, tCoord, grain.seed);

          // Stochastic per-frame term blended by (1 - temporalCoherence)
          const stochHash =
            (Math.imul(Math.floor(nx * 1.5), 0x9e3779b9) ^
              Math.imul(Math.floor(ny * 1.5), 0x85ebca6b) ^
              Math.imul(frameIndex, 0x27d4eb2d) ^
              grain.seed) >>>
            0;
          const stochFine = u01(stochHash) * 2.0 - 1.0;
          fineSample = lerp(stochFine, coherentFine, coherence);

          if (grain.renderMode === 'accurate' && (grain.coarse > 0.01 || grain.clustering > 0.01)) {
            const cxCoord = x * invCoarseFreq;
            const cyCoord = y * invCoarseFreq;
            coarseSample = continuousNoise3D(cxCoord, cyCoord, tCoord * 0.45, grain.seed + 1013);
            // Soft cluster shaping
            if (grain.clustering > 0.01) {
              const clusterGate = Math.tanh(coarseSample * (1.2 + grain.clustering * 2.2) * grain.density);
              coarseSample = lerp(coarseSample, clusterGate, grain.clustering);
            }
          }
        }

        // Spatial softness & correlation
        const spatialBlend = clamp01(grain.softness * 0.5 + grain.spatialCorrelation * 0.35);
        const combinedLumaNoise =
          (fineSample * grain.fine * (1.0 - spatialBlend * 0.35) +
            coarseSample * grain.coarse * (0.65 + spatialBlend * 0.55) +
            rowNoise) *
          grain.density;

        // PRNU low-frequency sensor pattern
        const prnu =
          grain.patternAmount > 0.005 && grain.renderMode === 'accurate'
            ? continuousNoise3D(
                x * invPatternFreq,
                y * invPatternFreq,
                tCoord * 0.05,
                grain.seed + 7919
              ) *
              grain.patternAmount *
              0.25
            : 0.0;

        // Interaction with ISP denoise & sharpening around edges
        const edgeFactor = Math.min(1.0, Math.abs(highFreq) * 5.0);
        const ispModulation =
          1.0 -
          grain.denoiseInteraction * (1.0 - edgeFactor) * camera.lowLightNR * 0.35 +
          grain.sharpeningInteraction * edgeFactor * camera.sharpening * 0.45;

        const finalLumaDelta =
          (combinedLumaNoise + prnu) *
          grain.amount *
          0.22 *
          tonalWeight *
          sensorGain *
          ispModulation;

        // Correlated Chroma Noise in YCbCr-like opponent channels (Cb / Cr)
        let cbNoise = 0.0;
        let crNoise = 0.0;

        if (
          grain.colorMode !== 'monochrome' &&
          grain.colorMode !== 'neutral_luma' &&
          (grain.chroma > 0.01 || grain.chromaNoise > 0.01)
        ) {
          const chromaScaleFreq = invCoarseFreq * 0.75;
          const cbRaw = continuousNoise3D(
            x * chromaScaleFreq + 17.3,
            y * chromaScaleFreq + 31.7,
            tCoord * 0.55,
            grain.seed + 31337
          );
          const crRaw = continuousNoise3D(
            x * chromaScaleFreq + 59.1,
            y * chromaScaleFreq + 11.9,
            tCoord * 0.55,
            grain.seed + 65537
          );

          let chromaStrength =
            grain.amount *
            0.16 *
            ((grain.chroma + grain.chromaNoise) * 0.5) *
            tonalWeight *
            (0.6 + isLowLight * 0.7);

          if (grain.colorMode === 'camera_profile') {
            chromaStrength *= 0.55 + camera.chromaNoise * 0.9;
          } else if (grain.colorMode === 'full_chroma') {
            chromaStrength *= 1.35;
          }

          cbNoise = cbRaw * chromaStrength;
          crNoise = crRaw * chromaStrength;

          if (grain.colorMode === 'warm_chroma') {
            crNoise += Math.abs(crRaw) * chromaStrength * 0.45;
            cbNoise -= Math.abs(cbRaw) * chromaStrength * 0.25;
          } else if (grain.colorMode === 'cool_chroma') {
            cbNoise += Math.abs(cbRaw) * chromaStrength * 0.45;
            crNoise -= Math.abs(crRaw) * chromaStrength * 0.2;
          }
        }

        if (grain.colorMode === 'monochrome') {
          const mono = clamp01(luma + finalLumaDelta);
          r = mono;
          g = mono;
          b = mono;
        } else {
          r = clamp01(r + finalLumaDelta + 1.402 * crNoise);
          g = clamp01(g + finalLumaDelta - 0.344136 * cbNoise - 0.714136 * crNoise);
          b = clamp01(b + finalLumaDelta + 1.772 * cbNoise);
        }

        if (luma < 0.35) {
          shadowNoiseSq += finalLumaDelta * finalLumaDelta;
          shadowCount++;
        }
        chromaDiffSum += Math.abs(cbNoise) + Math.abs(crNoise);
      }

      const outR = Math.round(r * 255);
      const outG = Math.round(g * 255);
      const outB = Math.round(b * 255);
      const outY = Math.round(clamp01(0.2126 * r + 0.7152 * g + 0.0722 * b) * 255);

      dstData[idx] = outR;
      dstData[idx + 1] = outG;
      dstData[idx + 2] = outB;
      dstData[idx + 3] = 255;

      histR[outR >> 2]++;
      histG[outG >> 2]++;
      histB[outB >> 2]++;
      histY[outY >> 2]++;
      sumLuma += outY / 255.0;
    }
  }

  const totalPixels = Math.max(1, width * height);
  return {
    meanLuma: sumLuma / totalPixels,
    shadowNoiseRms: shadowCount > 0 ? Math.sqrt(shadowNoiseSq / shadowCount) : 0,
    chromaSpread: chromaDiffSum / totalPixels,
    histogramR: histR,
    histogramG: histG,
    histogramB: histB,
    histogramY: histY,
  };
}
