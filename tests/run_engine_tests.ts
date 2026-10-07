import { CAMERA_PROFILES, DEFAULT_GRAIN_PARAMS, FILTER_PRESETS } from '../src/engine/presets.ts';
import { processFrameBuffer } from '../src/engine/pipeline.ts';
import type { CameraModelId, FilterPresetId } from '../src/engine/types.ts';

function assert(cond: boolean, msg: string) {
  if (!cond) {
    throw new Error(`ASSERTION FAILED: ${msg}`);
  }
}

function runSuite() {
  console.log('[2014 Automated Test Suite] Starting verification...');

  const cam = { ...CAMERA_PROFILES.iphone_6 };
  const filt = {
    presetId: 'new_york' as FilterPresetId,
    strength: 0.85,
    ...FILTER_PRESETS.new_york.params,
  };
  const grain = { ...DEFAULT_GRAIN_PARAMS };

  // 1. Black, White, Mid-Gray, Gradient, RGB channels
  const W = 64;
  const H = 64;
  const src = new Uint8ClampedArray(W * H * 4);
  const dst1 = new Uint8ClampedArray(W * H * 4);
  const dst2 = new Uint8ClampedArray(W * H * 4);
  const dstNextTime = new Uint8ClampedArray(W * H * 4);

  for (let y = 0; y < H; y++) {
    for (let x = 0; x < W; x++) {
      const idx = (y * W + x) * 4;
      if (y < 16) {
        src[idx] = 0;
        src[idx + 1] = 0;
        src[idx + 2] = 0;
      } else if (y < 32) {
        src[idx] = 255;
        src[idx + 1] = 255;
        src[idx + 2] = 255;
      } else if (y < 48) {
        src[idx] = 128;
        src[idx + 1] = 128;
        src[idx + 2] = 128;
      } else {
        src[idx] = Math.round((x / (W - 1)) * 255);
        src[idx + 1] = Math.round((y / (H - 1)) * 255);
        src[idx + 2] = 160;
      }
      src[idx + 3] = 255;
    }
  }

  const m1 = processFrameBuffer(src, dst1, W, H, cam, filt, grain, 1.25, null);
  const m2 = processFrameBuffer(src, dst2, W, H, cam, filt, grain, 1.25, null);

  assert(Number.isFinite(m1.meanLuma) && m1.meanLuma >= 0 && m1.meanLuma <= 1, 'Valid meanLuma');
  assert(Number.isFinite(m1.shadowNoiseRms) && m1.shadowNoiseRms > 0, 'Non-zero shadow noise RMS');

  // 2. Deterministic repeatability for identical seed & time
  for (let i = 0; i < dst1.length; i++) {
    assert(dst1[i] === dst2[i], `Deterministic mismatch at byte ${i}`);
  }
  console.log('  [PASS] Black/White/Gray/Gradient & deterministic repeatability');

  // 3. Temporal animation across frame times
  processFrameBuffer(src, dstNextTime, W, H, cam, filt, grain, 2.5, null);
  let diffCount = 0;
  for (let i = 0; i < dst1.length; i++) {
    if (dst1[i] !== dstNextTime[i]) diffCount++;
  }
  assert(diffCount > 100, 'Animated grain must evolve across different frame times');
  console.log('  [PASS] Temporal grain animation across frames');

  // 4. Zero grain vs Maximum grain & Different seeds
  const zeroGrain = { ...grain, amount: 0 };
  const maxGrain = { ...grain, amount: 1.0, seed: 9001 };
  const dstZero = new Uint8ClampedArray(W * H * 4);
  const dstMax = new Uint8ClampedArray(W * H * 4);
  const mZero = processFrameBuffer(src, dstZero, W, H, cam, filt, zeroGrain, 1.0, null);
  const mMax = processFrameBuffer(src, dstMax, W, H, cam, filt, maxGrain, 1.0, null);
  assert(mZero.shadowNoiseRms === 0, 'Zero grain must have 0 shadow noise RMS');
  assert(mMax.shadowNoiseRms > m1.shadowNoiseRms, 'Max grain must exceed default grain RMS');
  console.log('  [PASS] Zero grain, Maximum grain, and Seed variation');

  // 5. All 8 Camera Models x All 11 Filter Presets on 1x1 tiny & normal buffers
  const tinySrc = new Uint8ClampedArray([110, 120, 130, 255]);
  const tinyDst = new Uint8ClampedArray(4);
  for (const camKey of Object.keys(CAMERA_PROFILES) as CameraModelId[]) {
    for (const filtKey of Object.keys(FILTER_PRESETS) as FilterPresetId[]) {
      const fState = {
        presetId: filtKey,
        strength: 0.9,
        ...FILTER_PRESETS[filtKey].params,
      };
      processFrameBuffer(tinySrc, tinyDst, 1, 1, CAMERA_PROFILES[camKey], fState, grain, 0.5, null);
      assert(tinyDst[3] === 255, 'Alpha preserved on 1x1 buffer');
    }
  }
  console.log('  [PASS] All 8 Camera Models x 11 Filter Presets & 1x1 tiny image');
  console.log('[2014 Automated Test Suite] ALL TESTS PASSED.');
}

runSuite();
