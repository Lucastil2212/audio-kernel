#pragma once

#include <aaudio/AAudio.h>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#include "manticore/toneflow/tone_engine.h"

namespace manticore::android {

/**
 * Stable music-path duplex player.
 * Therapeutic tones need glitch-free buffers — not ultra-low-latency exclusive.
 * UI telemetry is lock-free so the meter cannot starve the audio callback.
 */
class AAudioPlayer {
public:
    AAudioPlayer();
    ~AAudioPlayer();

    bool start();
    void stop();
    bool isRunning() const { return running_.load(); }

    void setPresetIndex(int index);
    void setVolume(float volume);
    void setPlaying(bool playing);
    void setCarrierHz(float hz);
    void setBeatHz(float hz);
    void setMode(int mode);
    void setPerception(int mode);
    void setReverb(int preset);
    void setFilter(int type);
    void setHarmonicLayers(int layers);
    void setKernelNoiseBlend(float blend);
    void setSubLevel(float level);
    void setSubHz(float hz);
    float subLevel() const;
    void setMicFeedbackEnabled(bool enabled);
    void setMicFeedbackGain(float gain);
    void setToneMix(float mix);
    void cycleMode();
    void cyclePerception();
    void cycleReverb();
    void cycleFilter();

    int presetCount() const;
    const char* presetLabel(int index) const;
    const char* presetDescription(int index) const;
    const char* modeLabel() const;
    const char* perceptionLabel() const;
    const char* reverbLabel() const;
    const char* filterLabel() const;
    float carrierHz() const;
    float beatHz() const;
    float currentRms() const;
    float beatPhase() const;
    float micRms() const;
    float micAgcGain() const;
    bool micFeedbackEnabled() const;
    bool micStreamOpen() const { return inputStream_ != nullptr; }

private:
    static aaudio_data_callback_result_t outputCallback(
        AAudioStream* stream, void* userData, void* audioData,
        int32_t numFrames);
    static aaudio_data_callback_result_t inputCallback(
        AAudioStream* stream, void* userData, void* audioData,
        int32_t numFrames);
    static void errorCallback(AAudioStream* stream, void* userData,
                              aaudio_result_t error);

    bool openOutputStream();
    bool openInputStream();
    void closeStreams();
    void stabilizeBuffer(AAudioStream* stream);
    void publishTelemetry();
    void ensureScratch(int32_t numFrames);
    void pushMicFrames(const float* data, int32_t numFrames, int32_t channels);
    int32_t popMicFrames(float* dstMono, int32_t numFrames);

    AAudioStream* outputStream_ = nullptr;
    AAudioStream* inputStream_ = nullptr;
    toneflow::ToneEngine engine_;
    mutable std::mutex mutex_;
    std::atomic<bool> running_{false};
    std::atomic<bool> micWanted_{false};
    int32_t sampleRate_ = 48000;
    int32_t framesPerBurst_ = 192;
    int32_t inputChannels_ = 1;

    // Lock-free SPSC ring of mono float samples (power-of-two).
    static constexpr size_t kRingPow2 = 13;  // 8192 frames
    static constexpr size_t kRingSize = 1u << kRingPow2;
    static constexpr size_t kRingMask = kRingSize - 1;
    std::vector<float> micRing_;
    std::atomic<uint32_t> ringWrite_{0};
    std::atomic<uint32_t> ringRead_{0};
    std::vector<float> micScratch_;

    // Lock-free telemetry for UI (never block the audio callback from UI reads).
    std::atomic<float> telemCarrier_{210.0f};
    std::atomic<float> telemBeat_{10.0f};
    std::atomic<float> telemRms_{0.0f};
    std::atomic<float> telemPhase_{0.0f};
    std::atomic<float> telemMicRms_{0.0f};
    std::atomic<float> telemMicAgc_{1.0f};
    std::atomic<float> telemSub_{0.2f};
    std::atomic<int> telemMode_{0};
    std::atomic<int> telemPerception_{0};
    std::atomic<int> telemReverb_{0};
    std::atomic<int> telemFilter_{0};
    std::atomic<bool> telemMicEn_{false};
};

}  // namespace manticore::android
