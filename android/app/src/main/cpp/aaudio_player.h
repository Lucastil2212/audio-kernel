#pragma once

#include <aaudio/AAudio.h>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

#include "manticore/toneflow/tone_engine.h"

namespace manticore::android {

/**
 * Low-latency duplex player:
 * - Output callback drives playback
 * - Input callback pushes mic into a lock-free ring (latest samples preferred)
 * - Buffer sizes forced down to one burst when the device allows it
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
    void minimizeBuffer(AAudioStream* stream);
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
    int32_t framesPerBurst_ = 96;
    int32_t inputChannels_ = 1;

    // Lock-free SPSC ring of mono float samples (power-of-two).
    static constexpr size_t kRingPow2 = 13;  // 8192 frames ~170ms @48k
    static constexpr size_t kRingSize = 1u << kRingPow2;
    static constexpr size_t kRingMask = kRingSize - 1;
    std::vector<float> micRing_;
    std::atomic<uint32_t> ringWrite_{0};
    std::atomic<uint32_t> ringRead_{0};
    std::vector<float> micScratch_;
};

}  // namespace manticore::android
