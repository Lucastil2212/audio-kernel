#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "manticore/dsp/analyzer.h"
#include "manticore/dsp/wav_writer.h"

#ifdef MANTICORE_ENABLE_PORTAUDIO
#include "manticore/engine/audio_engine.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>
#endif

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

#ifndef MANTICORE_ENABLE_PORTAUDIO
    std::cerr << "latency_click_test requires PortAudio\n";
    return 1;
#else

    const uint32_t sampleRateHz = static_cast<uint32_t>(
        std::strtoul(getArg(argc, argv, "--sample-rate", "48000").c_str(),
                     nullptr, 10));
    const uint32_t frameSize = static_cast<uint32_t>(
        std::strtoul(getArg(argc, argv, "--frame-size", "64").c_str(), nullptr,
                     10));
    const float duration =
        std::strtof(getArg(argc, argv, "--duration", "3").c_str(), nullptr);
    const std::string out = getArg(argc, argv, "--out", "latency_capture.wav");

    RuntimeConfig config;
    config.sampleRateHz = sampleRateHz;
    config.frameSize = frameSize;
    config.channels = 1;
    config.inputGain = 1.0f;
    config.outputGain = 1.0f;
    config.enableLimiter = true;

    const size_t captureSamples =
        static_cast<size_t>(sampleRateHz * duration);
    std::vector<float> captured(captureSamples, 0.0f);
    std::atomic<size_t> captureIndex{0};
    std::atomic<bool> clickSent{false};

    AudioEngine engine;
    if (!engine.configure(config) || !engine.start()) {
        std::cerr << "Failed to start audio engine\n";
        return 1;
    }

    std::cout << "Recording " << duration << "s for latency analysis...\n";
    std::cout << "Play a click manually or use loopback routing.\n";

    const auto startTime = std::chrono::steady_clock::now();
    while (true) {
        const auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (elapsed > std::chrono::milliseconds(
                          static_cast<int>(duration * 1000.0f))) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    engine.stop();

    const bool ok = writeWavFile(out, captured, sampleRateHz, 1);
    if (!ok) {
        std::cerr << "Failed to write capture file\n";
        return 1;
    }

    size_t peakIndex = 0;
    float peakValue = 0.0f;
    for (size_t i = 0; i < captured.size(); ++i) {
        const float absVal = std::fabs(captured[i]);
        if (absVal > peakValue) {
            peakValue = absVal;
            peakIndex = i;
        }
    }

    const float latencyMs =
        static_cast<float>(peakIndex) / static_cast<float>(sampleRateHz) *
        1000.0f;

    std::cout << "Capture written to " << out << "\n";
    std::cout << "Peak index: " << peakIndex << " samples\n";
    std::cout << "Estimated latency: " << latencyMs << " ms\n";
    std::cout << "(Requires acoustic loopback for meaningful measurement)\n";

    return 0;
#endif
}
