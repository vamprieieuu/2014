#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../src/pipeline_core.h"

using namespace y2014;

static bool isBufferFiniteFloat(const std::vector<PixelFloat>& buf) {
    for (const auto& px : buf) {
        if (!std::isfinite(px.alpha) || !std::isfinite(px.red) ||
            !std::isfinite(px.green) || !std::isfinite(px.blue)) {
            return false;
        }
        if (px.red < 0.0f || px.red > 1.0f ||
            px.green < 0.0f || px.green > 1.0f ||
            px.blue < 0.0f || px.blue > 1.0f) {
            return false;
        }
    }
    return true;
}

int main() {
    std::printf("[2014_tests] Running core processing & crash-safety verification suite...\n");

    RenderContext defaultCtx{};
    defaultCtx.camera = getCameraPreset(CameraModelId::IPHONE_6);
    defaultCtx.filter = getFilterPreset(FilterPresetId::NEW_YORK);
    defaultCtx.grain = GrainParams{};
    defaultCtx.timeSeconds = 0.5f;

    // 1. Black, White, Mid-Gray, Gradient, RGB & Alpha Preservation (8-bit)
    {
        const int32_t W = 64, H = 64;
        std::vector<Pixel8> src(W * H);
        std::vector<Pixel8> dst(W * H);

        for (int32_t y = 0; y < H; ++y) {
            for (int32_t x = 0; x < W; ++x) {
                Pixel8 p{};
                p.alpha = static_cast<uint8_t>((x * 3 + y * 5) & 0xFF);
                if (y < 16) {
                    p.red = p.green = p.blue = 0; // Black
                } else if (y < 32) {
                    p.red = p.green = p.blue = 255; // White
                } else if (y < 48) {
                    p.red = p.green = p.blue = 128; // Mid-gray
                } else {
                    p.red = static_cast<uint8_t>((x * 255) / (W - 1));
                    p.green = static_cast<uint8_t>((y * 255) / (H - 1));
                    p.blue = 160;
                }
                src[y * W + x] = p;
            }
        }

        renderBuffer8(
            reinterpret_cast<const uint8_t*>(src.data()),
            W * static_cast<int32_t>(sizeof(Pixel8)),
            reinterpret_cast<uint8_t*>(dst.data()),
            W * static_cast<int32_t>(sizeof(Pixel8)),
            W,
            H,
            defaultCtx);

        for (size_t i = 0; i < src.size(); ++i) {
            assert(dst[i].alpha == src[i].alpha && "Alpha channel must be strictly preserved");
        }
        std::printf("  [PASS] 8-bit Black/White/Gray/Gradient & Alpha preservation\n");
    }

    // 2. Deterministic Repeatability & Temporal Animation Difference
    {
        const int32_t W = 32, H = 32;
        std::vector<Pixel8> src(W * H, Pixel8{255, 96, 104, 112});
        std::vector<Pixel8> outRun1(W * H);
        std::vector<Pixel8> outRun2(W * H);
        std::vector<Pixel8> outNextFrame(W * H);

        RenderContext ctxA = defaultCtx;
        ctxA.timeSeconds = 1.25f;
        ctxA.grain.seed = 2014U;

        renderBuffer8(
            reinterpret_cast<const uint8_t*>(src.data()),
            W * 4,
            reinterpret_cast<uint8_t*>(outRun1.data()),
            W * 4,
            W,
            H,
            ctxA);

        renderBuffer8(
            reinterpret_cast<const uint8_t*>(src.data()),
            W * 4,
            reinterpret_cast<uint8_t*>(outRun2.data()),
            W * 4,
            W,
            H,
            ctxA);

        assert(std::memcmp(outRun1.data(), outRun2.data(), W * H * sizeof(Pixel8)) == 0 &&
               "Same seed + frame time must produce identical deterministic output");

        RenderContext ctxB = ctxA;
        ctxB.timeSeconds = 2.00f;
        renderBuffer8(
            reinterpret_cast<const uint8_t*>(src.data()),
            W * 4,
            reinterpret_cast<uint8_t*>(outNextFrame.data()),
            W * 4,
            W,
            H,
            ctxB);

        bool foundDiff = (std::memcmp(outRun1.data(), outNextFrame.data(), W * H * sizeof(Pixel8)) != 0);
        assert(foundDiff && "Animated grain must evolve across different frame times");
        std::printf("  [PASS] Deterministic seed repeatability & temporal grain animation\n");
    }

    // 3. 16-bit Deep Color Buffer Validation (0..32768 range)
    {
        const int32_t W = 32, H = 32;
        std::vector<Pixel16> src16(W * H, Pixel16{32768, 12000, 14000, 16000});
        std::vector<Pixel16> dst16(W * H);

        renderBuffer16(
            reinterpret_cast<const uint8_t*>(src16.data()),
            W * static_cast<int32_t>(sizeof(Pixel16)),
            reinterpret_cast<uint8_t*>(dst16.data()),
            W * static_cast<int32_t>(sizeof(Pixel16)),
            W,
            H,
            defaultCtx);

        for (const auto& px : dst16) {
            assert(px.alpha == 32768);
            assert(px.red <= 32768 && px.green <= 32768 && px.blue <= 32768);
        }
        std::printf("  [PASS] 16-bit AE deep color (0..32768) validation\n");
    }

    // 4. 32-bit Float & Extreme / NaN / Infinity Input Safety
    {
        const int32_t W = 16, H = 16;
        std::vector<PixelFloat> srcF(W * H, PixelFloat{1.0f, 0.35f, 0.42f, 0.50f});
        srcF[0] = PixelFloat{1.0f, NAN, INFINITY, -INFINITY};
        srcF[1] = PixelFloat{1.0f, -50.0f, 999.0f, 0.0f};
        std::vector<PixelFloat> dstF(W * H);

        RenderContext extremeCtx = defaultCtx;
        extremeCtx.camera.exposure = 10.0f;
        extremeCtx.filter.presetId = FilterPresetId::NOSTALGIC_NIGHTS_REF;
        extremeCtx.filter = getFilterPreset(FilterPresetId::NOSTALGIC_NIGHTS_REF);
        extremeCtx.grain.amount = 2.0f;
        extremeCtx.grain.size = 0.001f;

        renderBufferFloat(
            reinterpret_cast<const uint8_t*>(srcF.data()),
            W * static_cast<int32_t>(sizeof(PixelFloat)),
            reinterpret_cast<uint8_t*>(dstF.data()),
            W * static_cast<int32_t>(sizeof(PixelFloat)),
            W,
            H,
            extremeCtx);

        assert(isBufferFiniteFloat(dstF) && "Float output must never contain NaN, Inf, or out-of-range values");
        std::printf("  [PASS] 32-bit float & extreme/NaN/Inf crash-safety validation\n");
    }

    // 5. All Camera Models (iPhone 5..8 Plus) & All 2014 Filter Presets & Tiny 1x1 Image
    {
        Pixel8 srcTiny{255, 110, 120, 130};
        Pixel8 dstTiny{};
        for (int c = 1; c <= 8; ++c) {
            for (int f = 1; f <= 11; ++f) {
                RenderContext ctx = defaultCtx;
                ctx.camera = getCameraPreset(static_cast<CameraModelId>(c));
                ctx.filter = getFilterPreset(static_cast<FilterPresetId>(f));
                renderBuffer8(
                    reinterpret_cast<const uint8_t*>(&srcTiny),
                    sizeof(Pixel8),
                    reinterpret_cast<uint8_t*>(&dstTiny),
                    sizeof(Pixel8),
                    1,
                    1,
                    ctx);
                assert(dstTiny.alpha == 255);
            }
        }
        // Null/zero dimension safety check
        renderBuffer8(nullptr, 0, nullptr, 0, 0, 0, defaultCtx);
        std::printf("  [PASS] All 8 Camera Models x 11 Filter Presets & 1x1 / Null safety\n");
    }

    std::printf("[2014_tests] ALL TESTS PASSED.\n");
    return 0;
}
