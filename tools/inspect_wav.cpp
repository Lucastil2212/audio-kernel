#include <iostream>
#include <string>

#include "manticore/core/audio_frame.h"
#include "manticore/dsp/analyzer.h"
#include "manticore/dsp/wav_writer.h"

int main(int argc, char** argv)
{
    using namespace manticore;

    if (argc < 2) {
        std::cerr << "Usage: inspect_wav <file.wav>\n";
        return 1;
    }

    std::vector<float> samples;
    uint32_t sampleRateHz = 0;
    uint16_t channels = 0;

    if (!readWavFile(argv[1], samples, sampleRateHz, channels)) {
        std::cerr << "Failed to read " << argv[1] << "\n";
        return 1;
    }

    AudioFrame frame(static_cast<uint32_t>(samples.size() / channels),
                     sampleRateHz, channels);
    frame.samples = samples;

    const FrameStats stats = analyzeFrame(frame);
    const float durationSeconds =
        static_cast<float>(samples.size()) /
        static_cast<float>(sampleRateHz * channels);

    std::cout << "File: " << argv[1] << "\n";
    std::cout << "Sample rate: " << sampleRateHz << " Hz\n";
    std::cout << "Channels: " << channels << "\n";
    std::cout << "Duration: " << durationSeconds << " s\n";
    std::cout << "Min: " << stats.minSample << " Max: " << stats.maxSample
              << "\n";
    std::cout << "Peak: " << stats.peakAbs << " RMS: " << stats.rms << "\n";
    std::cout << "DC offset: " << stats.dcOffset << "\n";
    std::cout << "Clipped: " << (stats.clipped ? "yes" : "no") << "\n";

    return 0;
}
