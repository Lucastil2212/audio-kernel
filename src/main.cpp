#include <iostream>

#include "manticore/dsp/analyzer.h"
#include "manticore/dsp/limiter.h"
#include "manticore/dsp/oscillator.h"
#include "manticore/dsp/wav_writer.h"

int main()
{
    using namespace manticore;

    const uint32_t sampleRateHz = 48000;
    const uint32_t frameSize = 64;
    const uint32_t channels = 1;
    const float durationSeconds = 1.0f;
    const uint32_t totalFrames =
        static_cast<uint32_t>(sampleRateHz * durationSeconds);

    std::vector<float> rendered;
    rendered.reserve(totalFrames);

    AudioFrame frame(frameSize, sampleRateHz, channels);
    SineOscillator osc;
    osc.frequencyHz = 440.0f;
    osc.amplitude = 0.25f;

    while (rendered.size() < totalFrames) {
        osc.render(frame);
        for (size_t i = 0; i < frame.framesPerChannel(); ++i) {
            if (rendered.size() >= totalFrames) {
                break;
            }
            rendered.push_back(frame.get(i, 0));
        }
        ++frame.frameIndex;
    }

    applyLimiter(frame);

    AudioFrame analysisFrame(static_cast<uint32_t>(rendered.size()), sampleRateHz,
                             channels);
    analysisFrame.samples = rendered;
    const FrameStats stats = analyzeFrame(analysisFrame);
    const bool ok =
        writeWavFile("sine_440.wav", rendered, sampleRateHz, channels);

    std::cout << "Manticore Audio Kernel Prototype 0\n";
    std::cout << "WAV write: " << (ok ? "ok" : "failed") << "\n";
    std::cout << "Frame duration ms: " << frame.durationMs() << "\n";
    std::cout << "Peak: " << stats.peakAbs << " RMS: " << stats.rms << "\n";
    std::cout << "Min: " << stats.minSample << " Max: " << stats.maxSample
              << "\n";

    return ok ? 0 : 1;
}
