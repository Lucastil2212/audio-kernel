#include <chrono>
#include <csignal>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include "manticore/engine/audio_engine.h"

namespace {
std::atomic<bool> gRunning{true};

void handleSignal(int) { gRunning = false; }

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

bool parseBool(const std::string& value, bool fallback)
{
    if (value == "true" || value == "1") {
        return true;
    }
    if (value == "false" || value == "0") {
        return false;
    }
    return fallback;
}

}  // namespace

int main(int argc, char** argv)
{
    using namespace manticore;

#ifndef MANTICORE_ENABLE_PORTAUDIO
    std::cerr << "live_passthrough requires PortAudio (build with "
                 "MANTICORE_ENABLE_PORTAUDIO=ON)\n";
    return 1;
#else

    std::signal(SIGINT, handleSignal);

    RuntimeConfig config;
    config.sampleRateHz = static_cast<uint32_t>(
        std::strtoul(getArg(argc, argv, "--sample-rate", "48000").c_str(),
                     nullptr, 10));
    config.frameSize = static_cast<uint32_t>(
        std::strtoul(getArg(argc, argv, "--frame-size", "64").c_str(), nullptr,
                     10));
    config.channels = static_cast<uint32_t>(
        std::strtoul(getArg(argc, argv, "--channels", "1").c_str(), nullptr,
                     10));
    config.inputGain =
        std::strtof(getArg(argc, argv, "--gain", "1.0").c_str(), nullptr);
    config.outputGain = config.inputGain;
    config.enableLimiter =
        parseBool(getArg(argc, argv, "--limiter", "true"), true);

    AudioEngine engine;
    if (!engine.configure(config)) {
        std::cerr << "Failed to configure audio engine\n";
        return 1;
    }
    if (!engine.start()) {
        std::cerr << "Failed to start audio stream\n";
        return 1;
    }

    std::cout << "Live passthrough running (Ctrl+C to stop)\n";
    std::cout << "Sample rate: " << config.sampleRateHz
              << " Frame size: " << config.frameSize << "\n";

    while (gRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        const SystemHealth health = engine.getHealth();
        std::cout << "callbacks=" << health.callbackCount
                  << " peak=" << health.lastPeakAbs
                  << " rms=" << health.lastRms
                  << " overruns=" << health.overrunCount
                  << " underruns=" << health.underflowCount
                  << " clip=" << health.clipCount
                  << " cb_ms=" << health.callbackDurationMs << "\n";
    }

    engine.stop();
    return 0;
#endif
}
