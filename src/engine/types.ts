export type CameraModelId =
  | 'iphone_5'
  | 'iphone_5s'
  | 'iphone_6'
  | 'iphone_6s'
  | 'iphone_7'
  | 'iphone_7_plus'
  | 'iphone_8'
  | 'iphone_8_plus';

export interface CameraProfile {
  id: CameraModelId;
  name: string;
  year: number;
  sensorSpec: string;
  evidenceNotes: string;
  strength: number;
  warmth: number;
  saturation: number;
  lowLightNR: number;
  detail: number;
  sharpening: number;
  highlightRollOff: number;
  shadowLift: number;
  lensSoftness: number;
  vignette: number;
  distortion: number;
  chromaticAberration: number;
  chromaNoise: number;
  hdrCharacter: number;
}

export type FilterPresetId =
  | 'none'
  | 'nostalgic_nights_ref'
  | 'new_york'
  | 'miami'
  | 'los_angeles'
  | 'tokyo'
  | 'paris'
  | 'london'
  | 'seattle'
  | 'berlin'
  | 'miami_night';

export interface FilterParams {
  presetId: FilterPresetId;
  strength: number;
  exposure: number; // -100 to 100
  brilliance: number; // -100 to 100
  brightness: number; // -100 to 100
  contrast: number; // -100 to 100
  temperature: number; // -100 to 100
  tint: number; // -100 to 100
  saturation: number; // -100 to 100
  vibrance: number; // -100 to 100
  highlights: number; // -100 to 100
  shadows: number; // -100 to 100
  whites: number; // -100 to 100
  blacks: number; // -100 to 100
  fade: number; // 0 to 100
  highlightRolloff: number; // 0 to 100
  microcontrast: number; // -100 to 100
  softness: number; // 0 to 100
  vignette: number; // 0 to 100
  shadowColor: [number, number, number]; // RGB bias -1 to 1
  midtoneColor: [number, number, number];
  highlightColor: [number, number, number];
}

export type GrainColorMode =
  | 'camera_profile'
  | 'monochrome'
  | 'neutral_luma'
  | 'warm_chroma'
  | 'cool_chroma'
  | 'full_chroma';

export type GrainRenderMode = 'accurate' | 'preview' | 'draft';

export interface GrainParams {
  amount: number; // 0 to 1
  size: number; // 0.5 to 3.0
  density: number; // 0.1 to 2.0
  scale: number; // 0.5 to 2.5
  softness: number; // 0 to 1
  speed: number; // 0 to 3
  animation: number; // 0 to 1
  chroma: number; // 0 to 1
  shadows: number; // 0 to 2
  midtones: number; // 0 to 2
  highlights: number; // 0 to 2
  fine: number; // 0 to 2
  coarse: number; // 0 to 2
  clustering: number; // 0 to 1
  spatialCorrelation: number; // 0 to 1
  temporalCoherence: number; // 0 to 1
  shotNoise: number; // 0 to 1
  readNoise: number; // 0 to 1
  chromaNoise: number; // 0 to 1
  patternAmount: number; // 0 to 0.5
  patternScale: number; // 0.5 to 4.0
  denoiseInteraction: number; // 0 to 1
  sharpeningInteraction: number; // 0 to 1
  seed: number;
  colorMode: GrainColorMode;
  renderMode: GrainRenderMode;
}

export interface ReferenceAsset {
  id: string;
  title: string;
  category: '2014_looks' | 'ios_5' | 'ios_5_filtered' | 'ios_6' | 'ios_7';
  path: string;
  notes: string;
}
