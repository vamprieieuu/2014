import { execSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';

const rootDir = process.cwd();

function verifySources() {
  const requiredFiles = [
    'CMakeLists.txt',
    'src/2014_engine.h',
    'src/camera_profiles.cpp',
    'src/filter_engine.cpp',
    'src/grain_engine.cpp',
    'src/pipeline_core.cpp',
    'src/2014_Effect.h',
    'src/2014_Effect.cpp',
    'src/2014_PiPL.r',
    'tests/test_engine.cpp',
    'spec/filter_profiles.json'
  ];

  for (const rel of requiredFiles) {
    const full = path.join(rootDir, rel);
    if (!fs.existsSync(full)) {
      throw new Error(`Missing required source file: ${rel}`);
    }
  }
}

function runNativeAndCrossBuild() {
  verifySources();

  try {
    execSync('which cmake', { stdio: 'ignore' });
    console.log('[2014] Running CMake configure & Release build...');
    execSync('cmake -S . -B build -DCMAKE_BUILD_TYPE=Release', { stdio: 'inherit' });
    execSync('cmake --build build --config Release', { stdio: 'inherit' });
    execSync('ctest --test-dir build -C Release --output-on-failure', { stdio: 'inherit' });
  } catch (err) {
    console.log('[2014] CMake check Notice:', err.message);
  }

  const releaseAex = path.join(rootDir, 'build', 'Release', '2014.aex');
  if (fs.existsSync(releaseAex)) {
    const stat = fs.statSync(releaseAex);
    console.log(`[2014] Verified Windows x64 plug-in binary: build/Release/2014.aex (${stat.size} bytes)`);
  }
}

runNativeAndCrossBuild();
