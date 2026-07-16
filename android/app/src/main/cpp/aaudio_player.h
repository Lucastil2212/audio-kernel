#pragma once

#include <aaudio/AAudio.h>
#include <atomic>
#include <mutex>
#include <vector>

#include "manticore/toneflow/tone_engine.h"

namespace manticore::android {

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
    static aaudio_data_callback_result_t dataCallback(
        AAudioStream* stream, void* userData, void* audioData,
        int32_t numFrames);
    static void errorCallback(AAudioStream* stream, void* userData,
                              aaudio_result_t error);

    bool openOutputStream();
    bool openInputStream();
    void closeStreams();
    void ensureMicBuffer(int32_t numFrames);

    AAudioStream* outputStream_ = nullptr;
    AAudioStream* inputStream_ = nullptr;
    toneflow::ToneEngine engine_;
    mutable std::mutex mutex_;
    std::atomic<bool> running_{false};
    std::atomic<bool> micWanted_{false};
    int32_t sampleRate_ = 48000;
    int32_t framesPerBurst_ = 192;
    int32_t inputChannels_ = 1;
    std::vector<float> micBuffer_;
};

}  // namespace manticore::android
