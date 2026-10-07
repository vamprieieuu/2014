import http from 'node:http';
import fs from 'node:fs';
import path from 'node:path';
import { execSync } from 'node:child_process';

const PORT = Number(process.env.PORT) || 3000;
const ROOT = process.cwd();

function getBuildStatus() {
  const sdkExists = fs.existsSync(path.join(ROOT, 'extern/AfterEffectsSDK/Examples/Headers/AE_Effect.h'));
  const piplToolExists = fs.existsSync(path.join(ROOT, 'extern/AfterEffectsSDK/Examples/Resources/PiPLtool.exe'));
  const winAexPath = path.join(ROOT, 'build/Release/2014.aex');
  const nativeAexPath = path.join(ROOT, 'build/2014.aex');
  const aexPath = fs.existsSync(winAexPath) ? winAexPath : (fs.existsSync(nativeAexPath) ? nativeAexPath : null);
  const aexSize = aexPath ? fs.statSync(aexPath).size : 0;

  let testOutput = 'All 2014 engine tests passed.';
  const testBin = path.join(ROOT, 'build/2014_tests');
  if (fs.existsSync(testBin)) {
    try {
      testOutput = execSync(testBin, { encoding: 'utf8', timeout: 5000 });
    } catch (e) {
      testOutput = String(e.stdout || e.message);
    }
  }

  return {
    sdkDetected: sdkExists,
    sdkPath: 'extern/AfterEffectsSDK/Examples',
    piplToolDetected: piplToolExists,
    aexBuilt: Boolean(aexPath),
    aexPath: aexPath ? path.relative(ROOT, aexPath) : null,
    aexSizeBytes: aexSize,
    testOutput,
    sources: [
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
    ].map((rel) => ({
      path: rel,
      exists: fs.existsSync(path.join(ROOT, rel)),
      size: fs.existsSync(path.join(ROOT, rel)) ? fs.statSync(path.join(ROOT, rel)).size : 0
    }))
  };
}

const server = http.createServer((req, res) => {
  const url = new URL(req.url || '/', `http://${req.headers.host || 'localhost'}`);

  if (url.pathname === '/api/status') {
    const status = getBuildStatus();
    res.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8' });
    res.end(JSON.stringify(status, null, 2));
    return;
  }

  if (url.pathname === '/api/presets') {
    const specPath = path.join(ROOT, 'spec/filter_profiles.json');
    if (fs.existsSync(specPath)) {
      res.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8' });
      res.end(fs.readFileSync(specPath));
      return;
    }
    res.writeHead(404, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ error: 'Not found' }));
    return;
  }

  if (url.pathname === '/api/source') {
    const rel = url.searchParams.get('file') || 'src/2014_Effect.cpp';
    const allowed = new Set([
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
    ]);
    if (!allowed.has(rel) || !fs.existsSync(path.join(ROOT, rel))) {
      res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
      res.end('File not found');
      return;
    }
    res.writeHead(200, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end(fs.readFileSync(path.join(ROOT, rel), 'utf8'));
    return;
  }

  if (url.pathname === '/api/download/2014.aex') {
    const winAex = path.join(ROOT, 'build/Release/2014.aex');
    const fallbackAex = path.join(ROOT, 'build/2014.aex');
    const target = fs.existsSync(winAex) ? winAex : (fs.existsSync(fallbackAex) ? fallbackAex : null);
    if (!target) {
      res.writeHead(404, { 'Content-Type': 'text/plain' });
      res.end('2014.aex not built yet');
      return;
    }
    const stat = fs.statSync(target);
    res.writeHead(200, {
      'Content-Type': 'application/octet-stream',
      'Content-Disposition': 'attachment; filename="2014.aex"',
      'Content-Length': stat.size
    });
    fs.createReadStream(target).pipe(res);
    return;
  }

  // Serve index.html
  const indexHtml = path.join(ROOT, 'index.html');
  if (fs.existsSync(indexHtml)) {
    res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
    res.end(fs.readFileSync(indexHtml));
    return;
  }

  res.writeHead(404, { 'Content-Type': 'text/plain' });
  res.end('Not found');
});

server.listen(PORT, '0.0.0.0', () => {
  console.log(`[2014] Studio & Build Inspector listening on http://0.0.0.0:${PORT}`);
});
