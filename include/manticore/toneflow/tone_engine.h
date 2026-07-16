#pragma once

#include "manticore/core/audio_frame.h"
#include "manticore/dsp/filters.h"
#include "manticore/dsp/noise.h"
#include "manticore/toneflow/color_filters.h"
#include "manticore/toneflow/perception.h"
#include "manticore/toneflow/stereo_reverb.h"
#include "manticore/toneflow/tone_oscillator.h"
#include "manticore/toneflow/types.h"

namespace manticore::toneflow {

/**
 * Perceptual / therapeutic tone engine (ToneFlow + Hemi-Sync inspired).
 * Not a medical device — for personal exploration only.
 */
class ToneEngine {
public:
    void prepare(uint32_t sampleRateHz, uint32_t framesPerCallback);
    void setParams(const ToneParams& params);
    void setPreset(const TonePreset& preset);
    void setPlaying(bool playing);
    bool isPlaying() const { return playing_; }

    /** Live customizability helpers. */
    void setCarrierHz(float hz);
    void setBeatHz(float hz);
    void setMode(BeatMode mode);
    void setPerception(PerceptionMode mode);
    void setReverb(ReverbPreset preset);
    void setFilter(FilterType type);
    void setHarmonicLayers(int layers);
    void setKernelNoiseBlend(float blend);
    void setVolume(float volume);
    /** Subwoofer / tactile vibration layer (pure low sine + beat pulse). */
    void setSubLevel(float level);
    void setSubHz(float hz);
    float subLevel() const { return params_.subLevel; }
    float subHz() const { return params_.subHz; }

    /** Optional realtime mic feedback into the output mix. */
    void setMicFeedbackEnabled(bool enabled);
    void setMicFeedbackGain(float gain);
    void setToneMix(float mix);  // 0 = mic only, 1 = tones only
    /** Skip reverb / heavy mic filters for minimal monitoring latency. */
    void setMicLowLatency(bool enabled) { micLowLatency_ = enabled; }
    bool micFeedbackEnabled() const { return micFeedbackEnabled_; }
    float micFeedbackGain() const { return micFeedbackGain_; }
    float toneMix() const { return toneMix_; }
    float micRms() const { return micRms_; }
    float micAgcGain() const { return micAgcGain_; }

    void renderInterleaved(float* interleavedStereo, uint32_t frames);
    /** Render tones and optionally mix processed mic (mono or stereo interleaved). */
    void renderInterleavedWithMic(float* interleavedStereoOut,
                                  const float* micInterleaved, uint32_t frames,
                                  uint32_t micChannels);
    void render(AudioFrame& stereoOut);

    const ToneParams& params() const { return params_; }
    float currentRms() const { return currentRms_; }
    float carrierHz() const { return carrierHz_; }
    float beatHz() const { return beatHz_; }
    /** Beat phase in radians for visual sync (0..2π). */
    float beatPhase() const { return beatPhase_; }

    void reset();

private:
    uint32_t sampleRateHz_ = 48000;
    uint32_t framesPerCallback_ = 256;
    ToneParams params_{};
    ToneParams target_{};

    float carrierHz_ = 210.0f;
    float beatHz_ = 10.0f;
    float smoothAlpha_ = 0.1f;
    bool playing_ = false;
    float currentRms_ = 0.0f;
    uint64_t samplesRendered_ = 0;
    float comfortPhase_ = 0.0f;
    float beatPhase_ = 0.0f;
    double subPhase_ = 0.0;
    double rumblePhase_ = 0.0;
    double subPulsePhase_ = 0.0;

    bool micFeedbackEnabled_ = false;
    bool micLowLatency_ = true;
    float micFeedbackGain_ = 0.18f;
    float toneMix_ = 0.92f;  // default: tones nearly pure; mic is a light layer
    float micRms_ = 0.0f;
    float micAgcGain_ = 1.0f;
    float micAgcTarget_ = 0.05f;
    float micGateOpen_ = 0.0f;  // smoothed gate [0..1]

    ToneOscillator osc_;
    PerceptionState perception_{};
    StereoReverb reverb_;
    StereoReverb micReverb_;
    ColorFilterBank filters_;
    ColorFilterBank micFilters_;
    PinkNoiseGenerator pink_;
    WhiteNoiseGenerator white_;
    OnePoleHighPass noiseHp_;
    OnePoleLowPass noiseLp_;
    OnePoleHighPass micHp_;
    OnePoleLowPass micLp_;
    OnePoleHighPass micHpTight_;
    OnePoleLowPass micLpTight_;
    OnePoleLowPass velvetLpL_;
    OnePoleLowPass velvetLpR_;
    OnePoleLowPass subSmooth_;
    AudioFrame toneFrame_;
    AudioFrame noiseFrame_;
    AudioFrame dryFrame_;
    AudioFrame micFrame_;

    void ensureWorkBuffers(uint32_t frames, uint32_t sampleRateHz);
    void blendSubBass(AudioFrame& stereo);
    void applyVelvetSmooth(AudioFrame& stereo);
    void applyNoiseMask(AudioFrame& stereo);
    void blendKernelNoise(AudioFrame& stereo);
    void applyColorFilter(AudioFrame& stereo);
    void applyComfortMod(AudioFrame& stereo);
    void applyFadeIn(AudioFrame& stereo);
    void applySoftLimiter(AudioFrame& stereo);
    void processMicFrame(const float* micInterleaved, uint32_t frames,
                         uint32_t micChannels);
    void configureTherapeuticReverb();
    void smoothTowardTargets();
    void advanceBeatPhase(uint32_t frames);
};

}  // namespace manticore::toneflow
