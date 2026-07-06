#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "manticore/dsp/analyzer.h"
#include "manticore/dsp/limiter.h"
#include "manticore/dsp/noise.h"
#include "manticore/dsp/wav_writer.h"

namespace {

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

    const std::string type = getArg(argc, argv, "--type", "white");
    const float duration = std::strtof(getArg(argc, argv, "--duration", "5").c_str(), nullptr);
    const float amp = std::strtof(getArg(argc, argv, "--amp", "0.1").c_str(), nullptr);
    const std::string out = getArg(argc, argv, "--out", "noise.wav");
    const uint32_t seed =
        static_cast<uint32_t>(std::strtoul(getArg(argc, argv, "--seed", "1").c_str(), nullptr, 10));

    const uint32_t sampleRateHz = 48000;
    const uint32_t frameSize = 64;
    const uint32_t channels = 1;
    const uint32_t totalFrames = static_cast<uint32_t>(sampleRateHz * duration);

    std::vector<float> rendered;
    rendered.reserve(totalFrames);

    AudioFrame frame(frameSize, sampleRateHz, channels);
    WhiteNoiseGenerator white;
    white.seed = seed;
    white.amplitude = amp;
    PinkNoiseGenerator pink;
    pink.seed = seed;
    pink.amplitude = amp;

    while (rendered.size() < totalFrames) {
        if (type == "pink") {
            pink.render(frame);
        } else {
            white.render(frame);
        }
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

    std::cout << "Generated " << out << " type=" << type << "\n";
    std::cout << "Peak=" << stats.peakAbs << " RMS=" << stats.rms << "\n";

    return ok ? 0 : 1;
}
