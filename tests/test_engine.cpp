#include "2014_engine.h"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>

namespace {

int gFailures = 0;

void check(bool condition, const char* expr, const char* file, int line) {
    if (!condition) {
        std::fprintf(stderr, "[FAIL] %s:%d: %s\n", file, line, expr);
        ++gFailures;
    }
}

#define TEST_ASSERT(cond) check((cond), #cond, __FILE__, __LINE__)

bool approxEqual(float a, float b, float eps = 1e-4f) {
    return std::fabs(a - b) <= eps;
}

void testCameraProfiles() {
    std::printf("[TEST] Verifying 8 iPhone Camera Profiles (iPhone 5 -> iPhone 8 Plus)...\n");

    const size_t count = static_cast<size_t>(y2014::CameraModel::Count);
    TEST_ASSERT(count == 8);

    for (size_t i = 0; i < count; ++i) {
        auto model = static_cast<y2014::CameraModel>(i);
        const y2014::CameraSpec& spec = y2014::getCameraSpec(model);
        TEST_ASSERT(spec.model == model);
        TEST_ASSERT(spec.name != nullptr && spec.name[0] != '\0');
        TEST_ASSERT(spec.sensorSummary != nullptr && spec.sensorSummary[0] != '\0');
        TEST_ASSERT(spec.baseSensorNoise > 0.0f && spec.baseSensorNoise < 1.0f);
    }

    // Verify iPhone 5 vs iPhone 5s vs iPhone 8 have distinct sensor noise and tone characteristics
    const auto& ip5  = y2014::getCameraSpec(y2014::CameraModel::IPhone5);
    const auto& ip5s = y2014::getCameraSpec(y2014::CameraModel::IPhone5s);
    const auto& ip6  = y2014::getCameraSpec(y2014::CameraModel::IPhone6);
    const auto& ip8  = y2014::getCameraSpec(y2014::CameraModel::IPhone8);

    TEST_ASSERT(ip5.baseSensorNoise > ip8.baseSensorNoise);
    TEST_ASSERT(ip6.lowLightDenoise > ip5.lowLightDenoise);
    TEST_ASSERT(ip5s.shotReadBalance != ip8.shotReadBalance);

    y2014::CameraParams p5{};
    p5.model = y2014::CameraModel::IPhone5;
    p5.profileStrength = 100.0f;

    y2014::CameraParams p8{};
    p8.model = y2014::CameraModel::IPhone8;
    p8.profileStrength = 100.0f;

    float r5 = 0.0f, g5 = 0.0f, b5 = 0.0f;
    float r8 = 0.0f, g8 = 0.0f, b8 = 0.0f;
    y2014::evaluateCameraPixel(0.42f, 0.36f, 0.31f, 0.75f, 0.75f, ip5, p5, 0.35f, 0.02f, r5, g5, b5);
    y2014::evaluateCameraPixel(0.42f, 0.36f, 0.31f, 0.75f, 0.75f, ip8, p8, 0.35f, 0.02f, r8, g8, b8);

    TEST_ASSERT(!approxEqual(r5, r8, 1e-4f) || !approxEqual(b5, b8, 1e-4f));

    // Profile strength 0 should pass through unmodified
    p5.profileStrength = 0.0f;
    float rb = 0.0f, gb = 0.0f, bb = 0.0f;
    y2014::evaluateCameraPixel(0.42f, 0.36f, 0.31f, 0.5f, 0.5f, ip5, p5, 0.35f, 0.02f, rb, gb, bb);
    TEST_ASSERT(approxEqual(rb, 0.42f));
    TEST_ASSERT(approxEqual(gb, 0.36f));
    TEST_ASSERT(approxEqual(bb, 0.31f));
}

void testFilterPresets() {
    std::printf("[TEST] Verifying 2014 City Filter Engine & Presets...\n");

    const size_t count = static_cast<size_t>(y2014::FilterPreset::Count);
    TEST_ASSERT(count == 11);

    const float inR = 0.48f, inG = 0.44f, inB = 0.39f;
    const float blR = 0.45f, blG = 0.42f, blB = 0.37f;

    // None preset should be neutral identity
    y2014::FilterParams noneP = y2014::getFilterPresetParams(y2014::FilterPreset::None);
    float nr = 0.0f, ng = 0.0f, nb = 0.0f;
    y2014::evaluateFilterPixel(inR, inG, inB, 0.5f, 0.5f, blR, blG, blB, noneP, nr, ng, nb);
    TEST_ASSERT(approxEqual(nr, inR, 1e-3f));
    TEST_ASSERT(approxEqual(ng, inG, 1e-3f));
    TEST_ASSERT(approxEqual(nb, inB, 1e-3f));

    // Verify New York, Miami, Tokyo, Nostalgic Nights produce distinct looks
    y2014::FilterParams nyP = y2014::getFilterPresetParams(y2014::FilterPreset::NewYork);
    y2014::FilterParams miP = y2014::getFilterPresetParams(y2014::FilterPreset::Miami);
    y2014::FilterParams tkP = y2014::getFilterPresetParams(y2014::FilterPreset::Tokyo);
    y2014::FilterParams nnP = y2014::getFilterPresetParams(y2014::FilterPreset::NostalgicNights);

    float nyr = 0.0f, nyg = 0.0f, nyb = 0.0f;
    float mir = 0.0f, mig = 0.0f, mib = 0.0f;
    float tkr = 0.0f, tkg = 0.0f, tkb = 0.0f;
    float nnr = 0.0f, nng = 0.0f, nnb = 0.0f;

    y2014::evaluateFilterPixel(inR, inG, inB, 0.6f, 0.6f, blR, blG, blB, nyP, nyr, nyg, nyb);
    y2014::evaluateFilterPixel(inR, inG, inB, 0.6f, 0.6f, blR, blG, blB, miP, mir, mig, mib);
    y2014::evaluateFilterPixel(inR, inG, inB, 0.6f, 0.6f, blR, blG, blB, tkP, tkr, tkg, tkb);
    y2014::evaluateFilterPixel(inR, inG, inB, 0.6f, 0.6f, blR, blG, blB, nnP, nnr, nng, nnb);

    TEST_ASSERT(!approxEqual(nyr, mir, 1e-3f));
    TEST_ASSERT(!approxEqual(mib, tkb, 1e-3f));
    TEST_ASSERT(!approxEqual(nyg, nng, 1e-3f));

    // Miami is warmer (higher R - B) than New York
    TEST_ASSERT((mir - mib) > (nyr - nyb));
}

void testSensorGrainEngine() {
    std::printf("[TEST] Verifying Procedural 2014 Sensor Grain Engine...\n");

    const y2014::CameraSpec& cam = y2014::getCameraSpec(y2014::CameraModel::IPhone5s);
    y2014::GrainParams gp{};
    y2014::applyCameraDefaultsToGrain(y2014::CameraModel::IPhone5s, gp);

    // 1. Zero amount must produce zero delta
    gp.amount = 0.0f;
    y2014::GrainSampleResult zeroRes = y2014::evaluateGrainAtPixel(32, 32, 128, 128, 0, 0.0f, 0.12f, 0.12f, 0.12f, 0.01f, cam, gp);
    TEST_ASSERT(approxEqual(zeroRes.deltaR, 0.0f));
    TEST_ASSERT(approxEqual(zeroRes.deltaG, 0.0f));
    TEST_ASSERT(approxEqual(zeroRes.deltaB, 0.0f));

    // 2. Shadow vs Highlight energy across a patch (shadows must carry higher sensor noise energy than bright highlights)
    gp.amount = 65.0f;
    double shadowEnergy = 0.0;
    double highlightEnergy = 0.0;
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            y2014::GrainSampleResult sOut = y2014::evaluateGrainAtPixel(x, y, 64, 64, 1, 0.04f, 0.10f, 0.10f, 0.10f, 0.0f, cam, gp);
            y2014::GrainSampleResult hOut = y2014::evaluateGrainAtPixel(x, y, 64, 64, 1, 0.04f, 0.92f, 0.92f, 0.92f, 0.0f, cam, gp);
            float ds = std::fabs(sOut.deltaR) + std::fabs(sOut.deltaG) + std::fabs(sOut.deltaB);
            float dh = std::fabs(hOut.deltaR) + std::fabs(hOut.deltaG) + std::fabs(hOut.deltaB);
            shadowEnergy += ds;
            highlightEnergy += dh;
        }
    }
    TEST_ASSERT(shadowEnergy > highlightEnergy * 1.35);

    // 3. Monochrome mode must produce equal R, G, B deltas on neutral input
    gp.colorMode = y2014::GrainColorMode::Monochrome;
    y2014::GrainSampleResult monoOut = y2014::evaluateGrainAtPixel(19, 27, 64, 64, 2, 0.08f, 0.25f, 0.25f, 0.25f, 0.0f, cam, gp);
    TEST_ASSERT(approxEqual(monoOut.deltaR, monoOut.deltaG, 1e-5f));
    TEST_ASSERT(approxEqual(monoOut.deltaG, monoOut.deltaB, 1e-5f));

    // 4. Temporal evolution: frame 0 vs frame 5 with speed > 0 must differ
    gp.colorMode = y2014::GrainColorMode::CameraProfile;
    gp.speed = 60.0f;
    gp.animation = 80.0f;
    y2014::GrainSampleResult f0 = y2014::evaluateGrainAtPixel(25, 41, 64, 64, 0, 0.0f, 0.22f, 0.22f, 0.22f, 0.01f, cam, gp);
    y2014::GrainSampleResult f5 = y2014::evaluateGrainAtPixel(25, 41, 64, 64, 5, 5.0f / 24.0f, 0.22f, 0.22f, 0.22f, 0.01f, cam, gp);
    TEST_ASSERT(!approxEqual(f0.deltaR, f5.deltaR, 1e-5f) || !approxEqual(f0.deltaG, f5.deltaG, 1e-5f));
}

void testFullPipelineBuffer() {
    std::printf("[TEST] Verifying Full 2014 Pipeline (renderFrameRGBAf)...\n");

    const int W = 48;
    const int H = 32;
    std::vector<y2014::RGBAf> src(W * H);
    std::vector<y2014::RGBAf> dst(W * H);

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            size_t idx = static_cast<size_t>(y * W + x);
            src[idx].r = static_cast<float>(x) / static_cast<float>(W - 1);
            src[idx].g = static_cast<float>(y) / static_cast<float>(H - 1);
            src[idx].b = 0.45f;
            src[idx].a = 1.0f;
        }
    }

    y2014::PipelineConfig cfg{};
    cfg.camera.model = y2014::CameraModel::IPhone5s;
    cfg.filter = y2014::getFilterPresetParams(y2014::FilterPreset::NewYork);
    y2014::applyCameraDefaultsToGrain(cfg.camera.model, cfg.grain);
    cfg.grain.amount = 45.0f;
    cfg.frameIndex = 3;
    cfg.frameTimeSeconds = 3.0f / 24.0f;

    y2014::renderFrameRGBAf(
        src.data(),
        dst.data(),
        W,
        H,
        W,
        W,
        cfg
    );

    bool anyDiff = false;
    for (size_t i = 0; i < dst.size(); ++i) {
        TEST_ASSERT(dst[i].r >= 0.0f && dst[i].r <= 1.0f);
        TEST_ASSERT(dst[i].g >= 0.0f && dst[i].g <= 1.0f);
        TEST_ASSERT(dst[i].b >= 0.0f && dst[i].b <= 1.0f);
        TEST_ASSERT(approxEqual(dst[i].a, 1.0f));
        if (!approxEqual(dst[i].r, src[i].r, 1e-3f)) {
            anyDiff = true;
        }
    }
    TEST_ASSERT(anyDiff);
}

} // namespace

int main() {
    std::printf("=== 2014 Core & After Effects Pipeline Verification ===\n");
    testCameraProfiles();
    testFilterPresets();
    testSensorGrainEngine();
    testFullPipelineBuffer();

    if (gFailures == 0) {
        std::printf("[PASS] All 2014 engine tests passed.\n");
        return EXIT_SUCCESS;
    } else {
        std::fprintf(stderr, "[FAIL] %d test assertion(s) failed.\n", gFailures);
        return EXIT_FAILURE;
    }
}
