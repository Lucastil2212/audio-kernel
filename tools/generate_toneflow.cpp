#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "manticore/dsp/wav_writer.h"
#include "manticore/toneflow/presets.h"
#include "manticore/toneflow/tone_engine.h"

int main(int argc, char** argv)
{
    const char* presetId = "alpha_calm";
    const char* outPath = "toneflow_out.wav";
    float durationSec = 3.0f;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--preset") == 0 && i + 1 < argc) {
            presetId = argv[++i];
        } else if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            outPath = argv[++i];
        } else if (std::strcmp(argv[i], "--duration") == 0 && i + 1 < argc) {
            durationSec = static_cast<float>(std::atof(argv[++i]));
        }
    }

    const auto* preset = manticore::toneflow::presetById(presetId);
    if (preset == nullptr) {
        std::fprintf(stderr, "Unknown preset: %s\n", presetId);
        return 1;
    }

    constexpr uint32_t kSampleRate = 48000;
    constexpr uint32_t kBlock = 256;
    manticore::toneflow::ToneEngine engine;
    engine.prepare(kSampleRate, kBlock);
    engine.setPreset(*preset);
    engine.setPlaying(true);

    const uint32_t totalFrames =
        static_cast<uint32_t>(durationSec * static_cast<float>(kSampleRate));
    manticore::AudioFrame out(totalFrames, kSampleRate, 2);
    manticore::AudioFrame block(kBlock, kSampleRate, 2);

    uint32_t written = 0;
    while (written < totalFrames) {
        const uint32_t n =
            std::min(kBlock, totalFrames - written);
        if (n != kBlock) {
            block = manticore::AudioFrame(n, kSampleRate, 2);
        }
        engine.render(block);
        for (uint32_t i = 0; i < n; ++i) {
            out.set(written + i, 0, block.get(i, 0));
            out.set(written + i, 1, block.get(i, 1));
        }
        written += n;
    }

    if (!manticore::writeWavFile(outPath, out.samples, out.sampleRateHz,
                                 static_cast<uint16_t>(out.channels))) {
        std::fprintf(stderr, "Failed to write %s\n", outPath);
        return 1;
    }

    std::printf("Wrote %s (%s, %.1fs)\n", outPath, preset->label, durationSec);
    return 0;
}
