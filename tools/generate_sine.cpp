#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "manticore/dsp/analyzer.h"
#include "manticore/dsp/limiter.h"
#include "manticore/dsp/oscillator.h"
#include "manticore/dsp/wav_writer.h"

namespace {

float parseFloat(const char* value, float fallback)
{
    if (value == nullptr) {
        return fallback;
    }
    return std::strtof(value, nullptr);
}

std::string getArg(int argc, char** argv, const std::string& flag,
                   const std::string& defaultValue = "")
{
    for (int i = 1; i < argc - 1; ++i) {
        if (flag == argv[i]) {
            return argv[i + 1];
        }
    }
    return defaultValue;
}

}  // namespace

int main(int argc, char** argv)
{
    using namespace manticore;

    const float freq =
        parseFloat(getArg(argc, argv, "--freq", "440").c_str(), 440.0f);
    const float duration =
        parseFloat(getArg(argc, argv, "--duration", "2").c_str(), 2.0f);
    const float amp =
        parseFloat(getArg(argc, argv, "--amp", "0.25").c_str(), 0.25f);
    const std::string out =
        getArg(argc, argv, "--out", "sine.wav");

    const uint32_t sampleRateHz = 48000;
    const uint32_t frameSize = 64;
    const uint32_t channels = 1;
    const uint32_t totalFrames =
        static_cast<uint32_t>(sampleRateHz * duration);

    std::vector<float> rendered;
    rendered.reserve(totalFrames);

    AudioFrame frame(frameSize, sampleRateHz, channels);
    SineOscillator osc;
    osc.frequencyHz = freq;
    osc.amplitude = amp;

    while (rendered.size() < totalFrames) {
        osc.render(frame);
        for (size_t i = 0; i < frame.framesPerChannel(); ++i) {
            if (rendered.size() >= totalFrames) {
                break;
            }
            rendered.push_back(frame.get(i, 0));
        }
    }

    applyLimiter(frame);
    const FrameStats stats = analyzeFrame(frame);
    const bool ok = writeWavFile(out, rendered, sampleRateHz, channels);

    std::cout << "Generated " << out << " (" << duration << "s @ " << freq
              << " Hz)\n";
    std::cout << "Peak=" << stats.peakAbs << " RMS=" << stats.rms << "\n";

    return ok ? 0 : 1;
}
