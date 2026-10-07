import React, { useState, useEffect, useRef, useCallback } from 'react';
import {
  Camera,
  Sliders,
  Sparkles,
  Play,
  Pause,
  Upload,
  Download,
  RotateCcw,
  SplitSquareVertical,
  ZoomIn,
  Code2,
  Image as ImageIcon,
  Eye,
  Film,
  Layers,
  Check,
  Copy,
} from 'lucide-react';
import {
  CameraModelId,
  CameraProfile,
  FilterParams,
  FilterPresetId,
  GrainColorMode,
  GrainParams,
  GrainRenderMode,
} from './engine/types';
import {
  CAMERA_PROFILES,
  DEFAULT_GRAIN_PARAMS,
  FILTER_PRESETS,
  REFERENCE_ASSETS,
} from './engine/presets';
import { PipelineMetrics, processFrameBuffer } from './engine/pipeline';
import { generateCppSourceBundle } from './engine/cppSourceExport';

function SliderControl({
  label,
  value,
  min,
  max,
  step = 0.01,
  unit = '',
  onChange,
  onReset,
}: {
  label: string;
  value: number;
  min: number;
  max: number;
  step?: number;
  unit?: string;
  onChange: (val: number) => void;
  onReset?: () => void;
}) {
  const pct = Math.max(0, Math.min(100, ((value - min) / (max - min)) * 100));
  return (
    <div className="group py-1.5">
      <div className="flex items-center justify-between text-xs mb-1">
        <span
          onDoubleClick={onReset}
          title={onReset ? 'Double-click to reset parameter' : undefined}
          className="text-zinc-300 font-medium select-none cursor-pointer group-hover:text-white transition-colors"
        >
          {label}
        </span>
        <span className="font-mono text-[11px] text-amber-400/90 tabular-nums">
          {step >= 1 ? Math.round(value) : value.toFixed(2)}
          {unit}
        </span>
      </div>
      <div className="relative flex items-center">
        <input
          type="range"
          min={min}
          max={max}
          step={step}
          value={value}
          onChange={(e) => onChange(parseFloat(e.target.value))}
          className="w-full h-1.5 bg-zinc-800 rounded-lg appearance-none cursor-pointer accent-amber-500"
          style={{
            background: `linear-gradient(to right, #f59e0b 0%, #f59e0b ${pct}%, #27272a ${pct}%, #27272a 100%)`,
          }}
        />
      </div>
    </div>
  );
}

export function App() {
  // 1. Camera State
  const [selectedCameraId, setSelectedCameraId] = useState<CameraModelId>('iphone_6');
  const [cameraProfile, setCameraProfile] = useState<CameraProfile>(() => ({
    ...CAMERA_PROFILES.iphone_6,
  }));

  // 2. Filter State
  const [filterParams, setFilterParams] = useState<FilterParams>(() => ({
    presetId: 'new_york',
    strength: 0.85,
    ...FILTER_PRESETS.new_york.params,
  }));

  // 3. Grain State
  const [grainParams, setGrainParams] = useState<GrainParams>(() => ({
    ...DEFAULT_GRAIN_PARAMS,
  }));

  // Active Inspector Tab
  const [activeTab, setActiveTab] = useState<'camera' | 'filter' | 'grain' | 'references'>('grain');
  const [showAdvancedGrain, setShowAdvancedGrain] = useState(true);
  const [showAdvancedFilter, setShowAdvancedFilter] = useState(false);

  // Viewport & Playback Controls
  const [selectedAssetPath, setSelectedAssetPath] = useState<string>(REFERENCE_ASSETS[0].path);
  const [customImageUrl, setCustomImageUrl] = useState<string | null>(null);
  const [isPlaying, setIsPlaying] = useState<boolean>(true);
  const [splitCompare, setSplitCompare] = useState<boolean>(false);
  const [splitPos, setSplitPos] = useState<number>(0.5);
  const [zoomLevel, setZoomLevel] = useState<1 | 2 | 3>(1);
  const [showCodeModal, setShowCodeModal] = useState<boolean>(false);
  const [activeCodeFileIndex, setActiveCodeFileIndex] = useState<number>(0);
  const [copiedCode, setCopiedCode] = useState<boolean>(false);
  const [refCategoryFilter, setRefCategoryFilter] = useState<string>('all');

  const [metrics, setMetrics] = useState<PipelineMetrics | null>(null);

  // Offscreen source image buffer & output canvas
  const mainCanvasRef = useRef<HTMLCanvasElement | null>(null);
  const sourceBufferRef = useRef<{
    data: Uint8ClampedArray;
    width: number;
    height: number;
  } | null>(null);
  const animFrameRef = useRef<number | null>(null);
  const startTimeRef = useRef<number>(performance.now());
  const fileInputRef = useRef<HTMLInputElement | null>(null);

  // Select Camera Model preset and sync camera-aware grain defaults
  const handleSelectCamera = (id: CameraModelId) => {
    const preset = CAMERA_PROFILES[id];
    setSelectedCameraId(id);
    setCameraProfile({ ...preset });
    setGrainParams((prev) => ({
      ...prev,
      chromaNoise: preset.chromaNoise,
      denoiseInteraction: preset.lowLightNR * 0.8,
      sharpeningInteraction: preset.sharpening * 0.8,
    }));
  };

  // Select 2014 Filter preset
  const handleSelectFilter = (id: FilterPresetId) => {
    const preset = FILTER_PRESETS[id];
    setFilterParams((prev) => ({
      ...preset.params,
      presetId: id,
      strength: id === 'none' ? 0 : prev.strength === 0 ? 0.85 : prev.strength,
    }));
  };

  // Load image into source buffer
  const loadSourceImage = useCallback((url: string) => {
    const img = new Image();
    img.crossOrigin = 'anonymous';
    img.onload = () => {
      const maxDim = 920;
      const scale = Math.min(1, maxDim / Math.max(img.width, img.height));
      const w = Math.max(64, Math.round(img.width * scale));
      const h = Math.max(64, Math.round(img.height * scale));

      const offCanvas = document.createElement('canvas');
      offCanvas.width = w;
      offCanvas.height = h;
      const ctx = offCanvas.getContext('2d');
      if (!ctx) return;
      ctx.drawImage(img, 0, 0, w, h);
      const imgData = ctx.getImageData(0, 0, w, h);
      sourceBufferRef.current = {
        data: imgData.data,
        width: w,
        height: h,
      };

      if (mainCanvasRef.current) {
        mainCanvasRef.current.width = w;
        mainCanvasRef.current.height = h;
      }
    };
    img.src = url;
  }, []);

  useEffect(() => {
    loadSourceImage(customImageUrl || selectedAssetPath);
  }, [selectedAssetPath, customImageUrl, loadSourceImage]);

  // Render frame loop
  useEffect(() => {
    let isMounted = true;

    const renderTick = (now: number) => {
      if (!isMounted) return;
      const src = sourceBufferRef.current;
      const canvas = mainCanvasRef.current;

      if (src && canvas) {
        if (canvas.width !== src.width || canvas.height !== src.height) {
          canvas.width = src.width;
          canvas.height = src.height;
        }
        const ctx = canvas.getContext('2d');
        if (ctx) {
          const outImage = ctx.createImageData(src.width, src.height);
          const elapsedSec = isPlaying ? (now - startTimeRef.current) / 1000.0 : 0.0;
          const computedMetrics = processFrameBuffer(
            src.data,
            outImage.data,
            src.width,
            src.height,
            cameraProfile,
            filterParams,
            grainParams,
            elapsedSec,
            splitCompare ? splitPos : null
          );
          ctx.putImageData(outImage, 0, 0);
          setMetrics(computedMetrics);
        }
      }

      if (isPlaying && grainParams.amount > 0.001 && grainParams.animation > 0.001) {
        animFrameRef.current = requestAnimationFrame(renderTick);
      }
    };

    animFrameRef.current = requestAnimationFrame(renderTick);

    return () => {
      isMounted = false;
      if (animFrameRef.current !== null) {
        cancelAnimationFrame(animFrameRef.current);
      }
    };
  }, [
    cameraProfile,
    filterParams,
    grainParams,
    isPlaying,
    splitCompare,
    splitPos,
    selectedAssetPath,
    customImageUrl,
  ]);

  const handleUploadFile = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;
    const url = URL.createObjectURL(file);
    setCustomImageUrl(url);
  };

  const handleDownloadFrame = () => {
    const canvas = mainCanvasRef.current;
    if (!canvas) return;
    const link = document.createElement('a');
    link.download = `2014_${cameraProfile.id}_${filterParams.presetId}_seed${grainParams.seed}.png`;
    link.href = canvas.toDataURL('image/png');
    link.click();
  };

  const handleResetAll = () => {
    handleSelectCamera('iphone_6');
    handleSelectFilter('new_york');
    setGrainParams({ ...DEFAULT_GRAIN_PARAMS });
  };

  const cppFiles = generateCppSourceBundle(cameraProfile, filterParams, grainParams);

  const filteredReferences =
    refCategoryFilter === 'all'
      ? REFERENCE_ASSETS
      : REFERENCE_ASSETS.filter((a) => a.category === refCategoryFilter);

  return (
    <div className="min-h-screen h-screen flex flex-col bg-[#0d0e11] text-zinc-100 overflow-hidden select-none">
      {/* Top Studio Header */}
      <header className="h-13 border-b border-zinc-800/90 bg-[#12141a] px-4 flex items-center justify-between shrink-0">
        <div className="flex items-center gap-3">
          <div className="w-7 h-7 rounded bg-amber-500/15 border border-amber-500/40 flex items-center justify-center text-amber-400 font-mono font-bold text-xs">
            14
          </div>
          <div>
            <div className="flex items-center gap-2">
              <h1 className="text-sm font-semibold tracking-tight text-white">2014</h1>
              <span className="text-[11px] font-mono uppercase px-1.5 py-0.5 rounded bg-zinc-800 text-zinc-300 border border-zinc-700/70">
                Early iPhone ISP · Filter · Sensor Grain Engine
              </span>
            </div>
          </div>
        </div>

        {/* Pipeline Signal Chain Breadcrumb */}
        <div className="hidden xl:flex items-center gap-1.5 text-[11px] font-mono text-zinc-400 bg-zinc-900/90 px-3 py-1.5 rounded-md border border-zinc-800">
          <span className="text-zinc-300">Input</span>
          <span className="text-zinc-600">→</span>
          <span className="text-amber-400">{cameraProfile.name.split(' ')[0]} {cameraProfile.name.split(' ')[1]} ISP</span>
          <span className="text-zinc-600">→</span>
          <span className="text-sky-400">NR/Ring</span>
          <span className="text-zinc-600">→</span>
          <span className="text-purple-400">{FILTER_PRESETS[filterParams.presetId].name}</span>
          <span className="text-zinc-600">→</span>
          <span className="text-emerald-400">3D Sensor Grain ({grainParams.renderMode})</span>
        </div>

        {/* Action Toolbar */}
        <div className="flex items-center gap-2">
          <input
            ref={fileInputRef}
            type="file"
            accept="image/*"
            onChange={handleUploadFile}
            className="hidden"
          />
          <button
            onClick={() => fileInputRef.current?.click()}
            className="flex items-center gap-1.5 px-2.5 py-1.5 rounded text-xs font-medium bg-zinc-800 hover:bg-zinc-700 text-zinc-200 border border-zinc-700 transition-colors cursor-pointer"
          >
            <Upload className="w-3.5 h-3.5" />
            <span>Load Image</span>
          </button>

          <button
            onClick={() => setShowCodeModal(true)}
            className="flex items-center gap-1.5 px-2.5 py-1.5 rounded text-xs font-medium bg-zinc-800 hover:bg-zinc-700 text-amber-300 border border-amber-500/30 transition-colors cursor-pointer"
          >
            <Code2 className="w-3.5 h-3.5" />
            <span>C++ AE Plugin Source</span>
          </button>

          <button
            onClick={handleDownloadFrame}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded text-xs font-semibold bg-amber-500 hover:bg-amber-400 text-zinc-950 transition-colors cursor-pointer"
          >
            <Download className="w-3.5 h-3.5" />
            <span>Export Frame</span>
          </button>
        </div>
      </header>

      {/* Main Workspace */}
      <div className="flex-1 flex min-h-0">
        {/* Left Effect Controls Dock (After Effects Effect Controls Layout) */}
        <aside className="w-[380px] border-r border-zinc-800/90 bg-[#12141a] flex flex-col shrink-0">
          {/* Subsystem Selector Tabs */}
          <div className="grid grid-cols-4 border-b border-zinc-800 bg-[#0f1015] p-1 gap-1">
            <button
              onClick={() => setActiveTab('camera')}
              className={`flex items-center justify-center gap-1.5 py-2 rounded text-xs font-medium transition-colors cursor-pointer ${
                activeTab === 'camera'
                  ? 'bg-zinc-800 text-amber-400 border border-zinc-700'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
            >
              <Camera className="w-3.5 h-3.5" />
              <span>Camera</span>
            </button>
            <button
              onClick={() => setActiveTab('filter')}
              className={`flex items-center justify-center gap-1.5 py-2 rounded text-xs font-medium transition-colors cursor-pointer ${
                activeTab === 'filter'
                  ? 'bg-zinc-800 text-amber-400 border border-zinc-700'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
            >
              <Sliders className="w-3.5 h-3.5" />
              <span>2014 Filter</span>
            </button>
            <button
              onClick={() => setActiveTab('grain')}
              className={`flex items-center justify-center gap-1.5 py-2 rounded text-xs font-medium transition-colors cursor-pointer ${
                activeTab === 'grain'
                  ? 'bg-zinc-800 text-amber-400 border border-zinc-700'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
            >
              <Sparkles className="w-3.5 h-3.5" />
              <span>Grain</span>
            </button>
            <button
              onClick={() => setActiveTab('references')}
              className={`flex items-center justify-center gap-1.5 py-2 rounded text-xs font-medium transition-colors cursor-pointer ${
                activeTab === 'references'
                  ? 'bg-zinc-800 text-amber-400 border border-zinc-700'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
            >
              <ImageIcon className="w-3.5 h-3.5" />
              <span>Refs (25)</span>
            </button>
          </div>

          {/* Scrollable Parameter Stack */}
          <div className="flex-1 overflow-y-auto p-4 space-y-5">
            {activeTab === 'camera' && (
              <div className="space-y-4">
                <div>
                  <label className="block text-[11px] font-mono uppercase text-zinc-400 mb-1.5">
                    Camera Model Profile (iPhone 5 → 8 Plus)
                  </label>
                  <select
                    value={selectedCameraId}
                    onChange={(e) => handleSelectCamera(e.target.value as CameraModelId)}
                    className="w-full bg-zinc-900 border border-zinc-700 rounded px-3 py-2 text-xs text-white focus:outline-none focus:border-amber-500"
                  >
                    {Object.values(CAMERA_PROFILES).map((p) => (
                      <option key={p.id} value={p.id}>
                        {p.name} ({p.year})
                      </option>
                    ))}
                  </select>
                  <div className="mt-2 p-2.5 rounded bg-zinc-900/90 border border-zinc-800 text-[11px] space-y-1">
                    <div className="font-mono text-amber-400">{cameraProfile.sensorSpec}</div>
                    <p className="text-zinc-400 leading-relaxed">{cameraProfile.evidenceNotes}</p>
                  </div>
                </div>

                <div className="border-t border-zinc-800 pt-3 space-y-1">
                  <div className="text-[11px] font-mono uppercase text-zinc-400 mb-1">
                    Master Profile Response
                  </div>
                  <SliderControl
                    label="Profile Strength"
                    value={cameraProfile.strength}
                    min={0}
                    max={1}
                    onChange={(v) => setCameraProfile({ ...cameraProfile, strength: v })}
                  />
                  <SliderControl
                    label="Sensor Warmth Bias"
                    value={cameraProfile.warmth}
                    min={-0.5}
                    max={0.8}
                    onChange={(v) => setCameraProfile({ ...cameraProfile, warmth: v })}
                  />
                  <SliderControl
                    label="Color Response Saturation"
                    value={cameraProfile.saturation}
                    min={-0.5}
                    max={0.6}
                    onChange={(v) => setCameraProfile({ ...cameraProfile, saturation: v })}
                  />
                </div>

                <div className="border-t border-zinc-800 pt-3 space-y-1">
                  <div className="text-[11px] font-mono uppercase text-zinc-400 mb-1">
                    ISP Local Tone & HDR Character
                  </div>
                  <SliderControl
                    label="HDR Exposure Blending Character"
                    value={cameraProfile.hdrCharacter}
                    min={0}
                    max={1}
                    onChange={(v) => setCameraProfile({ ...cameraProfile, hdrCharacter: v })}
                  />
                  <SliderControl
                    label="Highlight Roll-Off"
                    value={cameraProfile.highlightRollOff}
                    min={0}
                    max={1}
                    onChange={(v) => setCameraProfile({ ...cameraProfile, highlightRollOff: v })}
                  />
                  <SliderControl
                    label="Shadow Lift"
                    value={cameraProfile.shadowLift}
                    min={0}
                    max={0.6}
                    onChange={(v) => setCameraProfile({ ...cameraProfile, shadowLift: v })}
                  />
                </div>

                <div className="border-t border-zinc-800 pt-3 space-y-1">
                  <div className="text-[11px] font-mono uppercase text-zinc-400 mb-1">
                    ISP Denoise, Edge Ringing & Optics
                  </div>
                  <SliderControl
                    label="Low-Light Noise Reduction (Detail Loss)"
                    value={cameraProfile.lowLightNR}
                    min={0}
                    max={1}
                    onChange={(v) => setCameraProfile({ ...cameraProfile, lowLightNR: v })}
                  />
                  <SliderControl
                    label="ISP Edge Sharpening / Ringing"
                    value={cameraProfile.sharpening}
                    min={0}
                    max={1.2}
                    onChange={(v) => setCameraProfile({ ...cameraProfile, sharpening: v })}
                  />
                  <SliderControl
                    label="Corner Lens Softness"
                    value={cameraProfile.lensSoftness}
                    min={0}
                    max={0.8}
                    onChange={(v) => setCameraProfile({ ...cameraProfile, lensSoftness: v })}
                  />
                  <SliderControl
                    label="Lateral Chromatic Aberration"
                    value={cameraProfile.chromaticAberration}
                    min={0}
                    max={0.6}
                    onChange={(v) => setCameraProfile({ ...cameraProfile, chromaticAberration: v })}
                  />
                  <SliderControl
                    label="Sensor Vignette / Shading"
                    value={cameraProfile.vignette}
                    min={0}
                    max={0.8}
                    onChange={(v) => setCameraProfile({ ...cameraProfile, vignette: v })}
                  />
                </div>
              </div>
            )}

            {activeTab === 'filter' && (
              <div className="space-y-4">
                <div>
                  <label className="block text-[11px] font-mono uppercase text-zinc-400 mb-1.5">
                    2014 City Preset (`spec/filter_profiles.json`)
                  </label>
                  <div className="grid grid-cols-2 gap-1.5">
                    {Object.values(FILTER_PRESETS).map((preset) => (
                      <button
                        key={preset.id}
                        onClick={() => handleSelectFilter(preset.id)}
                        className={`text-left px-2.5 py-2 rounded border text-xs transition-all cursor-pointer ${
                          filterParams.presetId === preset.id
                            ? 'bg-amber-500/15 border-amber-500/60 text-amber-300 font-medium'
                            : 'bg-zinc-900/80 border-zinc-800 text-zinc-300 hover:border-zinc-700'
                        }`}
                      >
                        <div className="truncate">{preset.name}</div>
                        <div className="text-[10px] font-mono text-zinc-500 truncate">
                          {preset.family}
                        </div>
                      </button>
                    ))}
                  </div>
                  <p className="mt-2 text-[11px] text-zinc-400 bg-zinc-900/80 p-2 rounded border border-zinc-800">
                    {FILTER_PRESETS[filterParams.presetId].description}
                  </p>
                </div>

                <div className="border-t border-zinc-800 pt-3 space-y-1">
                  <SliderControl
                    label="Filter Mix Strength"
                    value={filterParams.strength}
                    min={0}
                    max={1}
                    onChange={(v) => setFilterParams({ ...filterParams, strength: v })}
                  />
                  <SliderControl
                    label="Exposure"
                    value={filterParams.exposure}
                    min={-100}
                    max={100}
                    step={1}
                    onChange={(v) => setFilterParams({ ...filterParams, exposure: v })}
                  />
                  <SliderControl
                    label="Brilliance"
                    value={filterParams.brilliance}
                    min={-100}
                    max={100}
                    step={1}
                    onChange={(v) => setFilterParams({ ...filterParams, brilliance: v })}
                  />
                  <SliderControl
                    label="Brightness"
                    value={filterParams.brightness}
                    min={-100}
                    max={100}
                    step={1}
                    onChange={(v) => setFilterParams({ ...filterParams, brightness: v })}
                  />
                  <SliderControl
                    label="Contrast"
                    value={filterParams.contrast}
                    min={-100}
                    max={100}
                    step={1}
                    onChange={(v) => setFilterParams({ ...filterParams, contrast: v })}
                  />
                  <SliderControl
                    label="Highlights"
                    value={filterParams.highlights}
                    min={-100}
                    max={100}
                    step={1}
                    onChange={(v) => setFilterParams({ ...filterParams, highlights: v })}
                  />
                  <SliderControl
                    label="Shadows"
                    value={filterParams.shadows}
                    min={-100}
                    max={100}
                    step={1}
                    onChange={(v) => setFilterParams({ ...filterParams, shadows: v })}
                  />
                  <SliderControl
                    label="Black Point / Blacks"
                    value={filterParams.blacks}
                    min={-100}
                    max={100}
                    step={1}
                    onChange={(v) => setFilterParams({ ...filterParams, blacks: v })}
                  />
                  <SliderControl
                    label="Fade (Lifted Floor)"
                    value={filterParams.fade}
                    min={0}
                    max={100}
                    step={1}
                    onChange={(v) => setFilterParams({ ...filterParams, fade: v })}
                  />
                </div>

                <div className="border-t border-zinc-800 pt-3 space-y-1">
                  <div className="text-[11px] font-mono uppercase text-zinc-400 mb-1">
                    Color & Definition
                  </div>
                  <SliderControl
                    label="Warmth / Temperature"
                    value={filterParams.temperature}
                    min={-100}
                    max={100}
                    step={1}
                    onChange={(v) => setFilterParams({ ...filterParams, temperature: v })}
                  />
                  <SliderControl
                    label="Tint (Magenta / Green)"
                    value={filterParams.tint}
                    min={-100}
                    max={100}
                    step={1}
                    onChange={(v) => setFilterParams({ ...filterParams, tint: v })}
                  />
                  <SliderControl
                    label="Saturation"
                    value={filterParams.saturation}
                    min={-100}
                    max={100}
                    step={1}
                    onChange={(v) => setFilterParams({ ...filterParams, saturation: v })}
                  />
                  <SliderControl
                    label="Vibrance"
                    value={filterParams.vibrance}
                    min={-100}
                    max={100}
                    step={1}
                    onChange={(v) => setFilterParams({ ...filterParams, vibrance: v })}
                  />
                  <SliderControl
                    label="Definition / Microcontrast"
                    value={filterParams.microcontrast}
                    min={-100}
                    max={100}
                    step={1}
                    onChange={(v) => setFilterParams({ ...filterParams, microcontrast: v })}
                  />
                  <SliderControl
                    label="Vignette"
                    value={filterParams.vignette}
                    min={0}
                    max={100}
                    step={1}
                    onChange={(v) => setFilterParams({ ...filterParams, vignette: v })}
                  />
                </div>

                <div className="border-t border-zinc-800 pt-3">
                  <button
                    onClick={() => setShowAdvancedFilter(!showAdvancedFilter)}
                    className="w-full flex items-center justify-between text-xs font-mono uppercase text-zinc-400 hover:text-zinc-200 py-1 cursor-pointer"
                  >
                    <span>Split-Tone Casts (Shadow / Highlight)</span>
                    <span>{showAdvancedFilter ? '−' : '+'}</span>
                  </button>
                  {showAdvancedFilter && (
                    <div className="mt-2 space-y-1 pl-2 border-l border-zinc-800">
                      <SliderControl
                        label="Shadow Violet/Blue Cast"
                        value={filterParams.shadowColor[2]}
                        min={-0.5}
                        max={0.5}
                        onChange={(v) =>
                          setFilterParams({
                            ...filterParams,
                            shadowColor: [
                              filterParams.shadowColor[0],
                              filterParams.shadowColor[1],
                              v,
                            ],
                          })
                        }
                      />
                      <SliderControl
                        label="Shadow Red/Cyan Cast"
                        value={filterParams.shadowColor[0]}
                        min={-0.5}
                        max={0.5}
                        onChange={(v) =>
                          setFilterParams({
                            ...filterParams,
                            shadowColor: [
                              v,
                              filterParams.shadowColor[1],
                              filterParams.shadowColor[2],
                            ],
                          })
                        }
                      />
                      <SliderControl
                        label="Highlight Warmth Cast"
                        value={filterParams.highlightColor[0]}
                        min={-0.5}
                        max={0.5}
                        onChange={(v) =>
                          setFilterParams({
                            ...filterParams,
                            highlightColor: [
                              v,
                              filterParams.highlightColor[1],
                              filterParams.highlightColor[2],
                            ],
                          })
                        }
                      />
                    </div>
                  )}
                </div>
              </div>
            )}

            {activeTab === 'grain' && (
              <div className="space-y-4">
                {/* Color Mode & Render Mode */}
                <div className="grid grid-cols-2 gap-2">
                  <div>
                    <label className="block text-[10px] font-mono uppercase text-zinc-400 mb-1">
                      Color Mode
                    </label>
                    <select
                      value={grainParams.colorMode}
                      onChange={(e) =>
                        setGrainParams({
                          ...grainParams,
                          colorMode: e.target.value as GrainColorMode,
                        })
                      }
                      className="w-full bg-zinc-900 border border-zinc-700 rounded px-2 py-1.5 text-xs text-white"
                    >
                      <option value="camera_profile">Camera Profile (Default)</option>
                      <option value="full_chroma">Full YCbCr Chroma</option>
                      <option value="warm_chroma">Warm Chroma</option>
                      <option value="cool_chroma">Cool Chroma</option>
                      <option value="neutral_luma">Neutral Luma</option>
                      <option value="monochrome">Monochrome</option>
                    </select>
                  </div>
                  <div>
                    <label className="block text-[10px] font-mono uppercase text-zinc-400 mb-1">
                      Render Engine Mode
                    </label>
                    <select
                      value={grainParams.renderMode}
                      onChange={(e) =>
                        setGrainParams({
                          ...grainParams,
                          renderMode: e.target.value as GrainRenderMode,
                        })
                      }
                      className="w-full bg-zinc-900 border border-zinc-700 rounded px-2 py-1.5 text-xs text-white"
                    >
                      <option value="accurate">Accurate (Multi-Component)</option>
                      <option value="preview">Preview (3D Fast)</option>
                      <option value="draft">Draft (Stochastic Only)</option>
                    </select>
                  </div>
                </div>

                {/* Main Grain Controls */}
                <div className="border-t border-zinc-800 pt-3 space-y-1">
                  <div className="text-[11px] font-mono uppercase text-amber-400/90 mb-1">
                    Main Grain / Sensor Texture Controls
                  </div>
                  <SliderControl
                    label="Amount (Amplitude)"
                    value={grainParams.amount}
                    min={0}
                    max={1}
                    onChange={(v) => setGrainParams({ ...grainParams, amount: v })}
                  />
                  <SliderControl
                    label="Size (Particle Scale)"
                    value={grainParams.size}
                    min={0.5}
                    max={3.0}
                    onChange={(v) => setGrainParams({ ...grainParams, size: v })}
                  />
                  <SliderControl
                    label="Density (Cluster Frequency)"
                    value={grainParams.density}
                    min={0.2}
                    max={2.0}
                    onChange={(v) => setGrainParams({ ...grainParams, density: v })}
                  />
                  <SliderControl
                    label="Scale / Zoom"
                    value={grainParams.scale}
                    min={0.5}
                    max={2.5}
                    onChange={(v) => setGrainParams({ ...grainParams, scale: v })}
                  />
                  <SliderControl
                    label="Softness (Spatial Filtering)"
                    value={grainParams.softness}
                    min={0}
                    max={1}
                    onChange={(v) => setGrainParams({ ...grainParams, softness: v })}
                  />
                  <SliderControl
                    label="Color / Chroma Balance"
                    value={grainParams.chroma}
                    min={0}
                    max={1}
                    onChange={(v) => setGrainParams({ ...grainParams, chroma: v })}
                  />
                  <SliderControl
                    label="Animation Amount"
                    value={grainParams.animation}
                    min={0}
                    max={1}
                    onChange={(v) => setGrainParams({ ...grainParams, animation: v })}
                  />
                  <SliderControl
                    label="Animation Speed"
                    value={grainParams.speed}
                    min={0.1}
                    max={3.0}
                    onChange={(v) => setGrainParams({ ...grainParams, speed: v })}
                  />
                  <SliderControl
                    label="Deterministic Seed"
                    value={grainParams.seed}
                    min={1}
                    max={9999}
                    step={1}
                    onChange={(v) => setGrainParams({ ...grainParams, seed: Math.round(v) })}
                  />
                </div>

                {/* Advanced Sensor & Tonal Controls */}
                <div className="border-t border-zinc-800 pt-3">
                  <button
                    onClick={() => setShowAdvancedGrain(!showAdvancedGrain)}
                    className="w-full flex items-center justify-between text-xs font-mono uppercase text-zinc-300 hover:text-white py-1 cursor-pointer"
                  >
                    <span>Advanced Tonal & Sensor Physics</span>
                    <span>{showAdvancedGrain ? '−' : '+'}</span>
                  </button>

                  {showAdvancedGrain && (
                    <div className="mt-2 space-y-3 pl-2 border-l border-zinc-800">
                      <div className="space-y-1">
                        <div className="text-[10px] font-mono uppercase text-zinc-500">
                          Tonal Response Curve
                        </div>
                        <SliderControl
                          label="Shadows Response"
                          value={grainParams.shadows}
                          min={0}
                          max={2}
                          onChange={(v) => setGrainParams({ ...grainParams, shadows: v })}
                        />
                        <SliderControl
                          label="Midtones Response"
                          value={grainParams.midtones}
                          min={0}
                          max={2}
                          onChange={(v) => setGrainParams({ ...grainParams, midtones: v })}
                        />
                        <SliderControl
                          label="Highlights Response"
                          value={grainParams.highlights}
                          min={0}
                          max={2}
                          onChange={(v) => setGrainParams({ ...grainParams, highlights: v })}
                        />
                      </div>

                      <div className="space-y-1 pt-2 border-t border-zinc-800/70">
                        <div className="text-[10px] font-mono uppercase text-zinc-500">
                          Structure & Temporal Coherence
                        </div>
                        <SliderControl
                          label="Fine Grain Component"
                          value={grainParams.fine}
                          min={0}
                          max={2}
                          onChange={(v) => setGrainParams({ ...grainParams, fine: v })}
                        />
                        <SliderControl
                          label="Coarse Clustered Component"
                          value={grainParams.coarse}
                          min={0}
                          max={2}
                          onChange={(v) => setGrainParams({ ...grainParams, coarse: v })}
                        />
                        <SliderControl
                          label="Low-Light Clustering"
                          value={grainParams.clustering}
                          min={0}
                          max={1}
                          onChange={(v) => setGrainParams({ ...grainParams, clustering: v })}
                        />
                        <SliderControl
                          label="Spatial Correlation"
                          value={grainParams.spatialCorrelation}
                          min={0}
                          max={1}
                          onChange={(v) =>
                            setGrainParams({ ...grainParams, spatialCorrelation: v })
                          }
                        />
                        <SliderControl
                          label="Temporal Coherence (3D vs Frame)"
                          value={grainParams.temporalCoherence}
                          min={0}
                          max={1}
                          onChange={(v) =>
                            setGrainParams({ ...grainParams, temporalCoherence: v })
                          }
                        />
                      </div>

                      <div className="space-y-1 pt-2 border-t border-zinc-800/70">
                        <div className="text-[10px] font-mono uppercase text-zinc-500">
                          Sensor Noise & ISP Interaction
                        </div>
                        <SliderControl
                          label="Shot Noise (Signal-Dependent)"
                          value={grainParams.shotNoise}
                          min={0}
                          max={1}
                          onChange={(v) => setGrainParams({ ...grainParams, shotNoise: v })}
                        />
                        <SliderControl
                          label="Read Noise (Shadow Floor)"
                          value={grainParams.readNoise}
                          min={0}
                          max={1}
                          onChange={(v) => setGrainParams({ ...grainParams, readNoise: v })}
                        />
                        <SliderControl
                          label="PRNU Sensor Pattern Amount"
                          value={grainParams.patternAmount}
                          min={0}
                          max={0.5}
                          onChange={(v) => setGrainParams({ ...grainParams, patternAmount: v })}
                        />
                        <SliderControl
                          label="ISP Denoise Interaction"
                          value={grainParams.denoiseInteraction}
                          min={0}
                          max={1}
                          onChange={(v) =>
                            setGrainParams({ ...grainParams, denoiseInteraction: v })
                          }
                        />
                        <SliderControl
                          label="Edge Sharpening Interaction"
                          value={grainParams.sharpeningInteraction}
                          min={0}
                          max={1}
                          onChange={(v) =>
                            setGrainParams({ ...grainParams, sharpeningInteraction: v })
                          }
                        />
                      </div>
                    </div>
                  )}
                </div>
              </div>
            )}

            {activeTab === 'references' && (
              <div className="space-y-3">
                <div className="flex items-center justify-between">
                  <span className="text-[11px] font-mono uppercase text-zinc-400">
                    Calibration Archive (25 Files)
                  </span>
                  <select
                    value={refCategoryFilter}
                    onChange={(e) => setRefCategoryFilter(e.target.value)}
                    className="bg-zinc-900 border border-zinc-700 rounded px-2 py-1 text-[11px] text-zinc-200"
                  >
                    <option value="all">All Categories (25)</option>
                    <option value="2014_looks">2014 Looks (8)</option>
                    <option value="ios_5">iOS 5 Unfiltered (6)</option>
                    <option value="ios_5_filtered">iOS 5 Filtered (2)</option>
                    <option value="ios_6">iOS 6 Samples (4)</option>
                    <option value="ios_7">iOS 7 Samples (4)</option>
                  </select>
                </div>

                <div className="grid grid-cols-2 gap-2">
                  {filteredReferences.map((asset) => {
                    const isSelected = !customImageUrl && selectedAssetPath === asset.path;
                    return (
                      <button
                        key={asset.id}
                        onClick={() => {
                          setCustomImageUrl(null);
                          setSelectedAssetPath(asset.path);
                        }}
                        className={`group text-left rounded overflow-hidden border transition-all cursor-pointer ${
                          isSelected
                            ? 'border-amber-500 bg-amber-500/10'
                            : 'border-zinc-800 bg-zinc-900/60 hover:border-zinc-700'
                        }`}
                      >
                        <div className="aspect-4/3 bg-zinc-950 overflow-hidden">
                          <img
                            src={asset.path}
                            alt={asset.title}
                            loading="lazy"
                            className="w-full h-full object-cover group-hover:scale-105 transition-transform"
                          />
                        </div>
                        <div className="p-2">
                          <div className="text-[11px] font-medium text-zinc-200 truncate">
                            {asset.title}
                          </div>
                          <div className="text-[10px] text-zinc-400 line-clamp-2 mt-0.5">
                            {asset.notes}
                          </div>
                        </div>
                      </button>
                    );
                  })}
                </div>
              </div>
            )}
          </div>

          {/* Bottom Reset Bar */}
          <div className="p-3 border-t border-zinc-800 bg-[#0f1015] flex items-center justify-between">
            <button
              onClick={handleResetAll}
              className="flex items-center gap-1.5 text-xs text-zinc-400 hover:text-zinc-200 transition-colors cursor-pointer"
            >
              <RotateCcw className="w-3.5 h-3.5" />
              <span>Reset All Parameters</span>
            </button>
            <span className="text-[11px] font-mono text-zinc-500">32-bit Float Pipeline</span>
          </div>
        </aside>

        {/* Center Composition Viewport */}
        <main className="flex-1 flex flex-col bg-[#090a0c] min-w-0">
          {/* Viewport Transport & Inspection Bar */}
          <div className="h-11 border-b border-zinc-800/80 bg-[#101217] px-4 flex items-center justify-between">
            <div className="flex items-center gap-2">
              <button
                onClick={() => setIsPlaying(!isPlaying)}
                className={`flex items-center gap-1.5 px-2.5 py-1 rounded text-xs font-medium border transition-colors cursor-pointer ${
                  isPlaying
                    ? 'bg-emerald-500/15 border-emerald-500/40 text-emerald-300'
                    : 'bg-zinc-800 border-zinc-700 text-zinc-300'
                }`}
              >
                {isPlaying ? <Pause className="w-3.5 h-3.5" /> : <Play className="w-3.5 h-3.5" />}
                <span>{isPlaying ? 'Temporal Grain Active (24fps)' : 'Paused Still'}</span>
              </button>

              <button
                onClick={() => setSplitCompare(!splitCompare)}
                className={`flex items-center gap-1.5 px-2.5 py-1 rounded text-xs font-medium border transition-colors cursor-pointer ${
                  splitCompare
                    ? 'bg-amber-500/15 border-amber-500/50 text-amber-300'
                    : 'bg-zinc-800 border-zinc-700 text-zinc-300 hover:text-white'
                }`}
              >
                <SplitSquareVertical className="w-3.5 h-3.5" />
                <span>Split Wipe</span>
              </button>

              {splitCompare && (
                <div className="flex items-center gap-2 ml-2">
                  <span className="text-[11px] font-mono text-zinc-400">Wipe:</span>
                  <input
                    type="range"
                    min={0.05}
                    max={0.95}
                    step={0.01}
                    value={splitPos}
                    onChange={(e) => setSplitPos(parseFloat(e.target.value))}
                    className="w-24 h-1 bg-zinc-700 accent-amber-500 cursor-pointer"
                  />
                </div>
              )}
            </div>

            {/* Zoom Controls for Grain Structure Inspection */}
            <div className="flex items-center gap-1.5">
              <ZoomIn className="w-3.5 h-3.5 text-zinc-400 mr-1" />
              {([1, 2, 3] as const).map((z) => (
                <button
                  key={z}
                  onClick={() => setZoomLevel(z)}
                  className={`px-2 py-0.5 rounded text-xs font-mono cursor-pointer ${
                    zoomLevel === z
                      ? 'bg-amber-500 text-zinc-950 font-semibold'
                      : 'bg-zinc-800 text-zinc-400 hover:text-zinc-200'
                  }`}
                >
                  {z * 100}%
                </button>
              ))}
            </div>
          </div>

          {/* Canvas Stage */}
          <div className="flex-1 overflow-auto flex items-center justify-center p-6 relative">
            <div
              className="relative shadow-2xl border border-zinc-800/90 bg-black overflow-hidden transition-transform duration-150"
              style={{
                transform: `scale(${zoomLevel})`,
                transformOrigin: 'center center',
              }}
            >
              <canvas
                ref={mainCanvasRef}
                className="block max-h-[calc(100vh-220px)] max-w-full object-contain"
                style={{ imageRendering: zoomLevel > 1 ? 'pixelated' : 'auto' }}
              />
              {splitCompare && (
                <div
                  className="absolute top-0 bottom-0 w-0.5 bg-amber-400 pointer-events-none shadow"
                  style={{ left: `${splitPos * 100}%` }}
                >
                  <div className="absolute top-2 -left-14 bg-zinc-950/90 text-[10px] font-mono px-1.5 py-0.5 rounded border border-zinc-700 text-zinc-300">
                    Original
                  </div>
                  <div className="absolute top-2 left-2 bg-zinc-950/90 text-[10px] font-mono px-1.5 py-0.5 rounded border border-amber-500/40 text-amber-300">
                    2014 Output
                  </div>
                </div>
              )}
            </div>
          </div>

          {/* Bottom Histogram & Sensor Telemetry Bar */}
          <div className="h-24 border-t border-zinc-800/90 bg-[#111319] px-5 flex items-center justify-between gap-6 shrink-0">
            <div className="flex items-center gap-6">
              <div>
                <div className="text-[10px] font-mono uppercase text-zinc-500">Active Camera ISP</div>
                <div className="text-xs font-semibold text-zinc-200 mt-0.5">
                  {cameraProfile.name}
                </div>
                <div className="text-[11px] font-mono text-amber-400/90">
                  Strength {(cameraProfile.strength * 100).toFixed(0)}% · NR {(cameraProfile.lowLightNR * 100).toFixed(0)}%
                </div>
              </div>

              <div className="h-8 w-px bg-zinc-800" />

              <div>
                <div className="text-[10px] font-mono uppercase text-zinc-500">2014 Look Preset</div>
                <div className="text-xs font-semibold text-zinc-200 mt-0.5">
                  {FILTER_PRESETS[filterParams.presetId].name}
                </div>
                <div className="text-[11px] font-mono text-purple-400">
                  Exp {filterParams.exposure} · Bril {filterParams.brilliance} · Warmth {filterParams.temperature}
                </div>
              </div>

              <div className="h-8 w-px bg-zinc-800" />

              <div>
                <div className="text-[10px] font-mono uppercase text-zinc-500">
                  Sensor Grain Telemetry
                </div>
                <div className="text-xs font-mono text-emerald-400 mt-0.5">
                  Shadow RMS: {metrics ? (metrics.shadowNoiseRms * 100).toFixed(2) : '0.00'}% · Chroma:{' '}
                  {metrics ? (metrics.chromaSpread * 100).toFixed(2) : '0.00'}%
                </div>
                <div className="text-[11px] font-mono text-zinc-400">
                  Coherence {(grainParams.temporalCoherence * 100).toFixed(0)}% · Seed #{grainParams.seed}
                </div>
              </div>
            </div>

            {/* Live Luminance & RGB Histogram */}
            {metrics && (
              <div className="flex items-center gap-3">
                <div className="text-right hidden sm:block">
                  <div className="text-[10px] font-mono uppercase text-zinc-500">
                    Output Luminance Distribution
                  </div>
                  <div className="text-[11px] font-mono text-zinc-400">
                    Mean Y: {(metrics.meanLuma * 100).toFixed(1)}%
                  </div>
                </div>
                <div className="w-48 h-14 bg-zinc-950 border border-zinc-800 rounded p-1 flex items-end gap-[1px]">
                  {metrics.histogramY.map((count, i) => {
                    const maxVal = Math.max(1, ...metrics.histogramY);
                    const hPct = Math.min(100, (count / maxVal) * 100);
                    return (
                      <div
                        key={i}
                        className="flex-1 bg-amber-400/75 rounded-t-[1px]"
                        style={{ height: `${Math.max(4, hPct)}%` }}
                      />
                    );
                  })}
                </div>
              </div>
            )}
          </div>
        </main>
      </div>

      {/* Native After Effects C++ Source Modal */}
      {showCodeModal && (
        <div className="fixed inset-0 z-50 bg-black/75 backdrop-blur-xs flex items-center justify-center p-6">
          <div className="bg-[#13151c] border border-zinc-700 rounded-lg max-w-4xl w-full max-h-[85vh] flex flex-col shadow-2xl">
            <div className="px-5 py-3.5 border-b border-zinc-800 flex items-center justify-between">
              <div>
                <h2 className="text-sm font-semibold text-white">
                  Native After Effects C++ Core & Calibrated Parameter State
                </h2>
                <p className="text-xs text-zinc-400">
                  Live-synced with your current Camera, Filter, and Grain controls for `2014_core` (.aex build).
                </p>
              </div>
              <button
                onClick={() => setShowCodeModal(false)}
                className="px-2.5 py-1 rounded bg-zinc-800 hover:bg-zinc-700 text-xs text-zinc-300 cursor-pointer"
              >
                Close
              </button>
            </div>

            <div className="flex border-b border-zinc-800 bg-[#0e1015] px-4 pt-2 gap-2">
              {cppFiles.map((file, idx) => (
                <button
                  key={file.filename}
                  onClick={() => setActiveCodeFileIndex(idx)}
                  className={`px-3 py-1.5 text-xs font-mono rounded-t border-t border-x cursor-pointer ${
                    activeCodeFileIndex === idx
                      ? 'bg-[#13151c] border-zinc-700 text-amber-400'
                      : 'bg-transparent border-transparent text-zinc-400 hover:text-zinc-200'
                  }`}
                >
                  {file.title}
                </button>
              ))}
            </div>

            <div className="flex-1 overflow-auto p-4 bg-[#0b0c10] font-mono text-xs text-zinc-200 relative">
              <button
                onClick={() => {
                  navigator.clipboard.writeText(cppFiles[activeCodeFileIndex].code);
                  setCopiedCode(true);
                  setTimeout(() => setCopiedCode(false), 1800);
                }}
                className="absolute top-4 right-4 flex items-center gap-1.5 px-2.5 py-1 rounded bg-zinc-800 hover:bg-zinc-700 text-zinc-200 text-xs border border-zinc-700 cursor-pointer"
              >
                {copiedCode ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                <span>{copiedCode ? 'Copied' : 'Copy Code'}</span>
              </button>
              <pre className="leading-relaxed whitespace-pre-wrap">
                {cppFiles[activeCodeFileIndex].code}
              </pre>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
export default App;
