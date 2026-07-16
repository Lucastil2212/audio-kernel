#include "manticore/toneflow/tone_engine.h"

#include <cmath>

namespace manticore::toneflow {
namespace {
constexpr float kTwoPi = 6.28318530717958647692f;
}

void ToneEngine::prepare(uint32_t sampleRateHz, uint32_t framesPerCallback)
{
    sampleRateHz_ = sampleRateHz == 0 ? 48000 : sampleRateHz;
    framesPerCallback_ =
        framesPerCallback == 0 ? 256 : framesPerCallback;

    const float chunkSec =
        static_cast<float>(framesPerCallback_) / static_cast<float>(sampleRateHz_);
    smoothAlpha_ = 1.0f - std::exp(-chunkSec / 0.220f);

    toneFrame_ = AudioFrame(framesPerCallback_, sampleRateHz_, 2);
    noiseFrame_ = AudioFrame(framesPerCallback_, sampleRateHz_, 2);
    dryFrame_ = AudioFrame(framesPerCallback_, sampleRateHz_, 2);
    micFrame_ = AudioFrame(framesPerCallback_, sampleRateHz_, 2);
    reverb_.prepare(sampleRateHz_);
    micReverb_.prepare(sampleRateHz_);
    micReverb_.setPreset(ReverbPreset::Room);
    micReverb_.setWet(0.10f);
    filters_.prepare(sampleRateHz_);
    micFilters_.prepare(sampleRateHz_);
    micFilters_.setType(FilterType::Warm);
    noiseHp_.setCutoff(100.0f, static_cast<float>(sampleRateHz_));
    noiseLp_.setCutoff(900.0f, static_cast<float>(sampleRateHz_));
    // Speech / monitoring band — cuts rumble and hiss with almost no delay.
    micHp_.setCutoff(120.0f, static_cast<float>(sampleRateHz_));
    micLp_.setCutoff(4500.0f, static_cast<float>(sampleRateHz_));
    micHpTight_.setCutoff(180.0f, static_cast<float>(sampleRateHz_));
    micLpTight_.setCutoff(3800.0f, static_cast<float>(sampleRateHz_));
    // Soothing velvet — removes grit / speaker hash without killing the carrier.
    velvetLpL_.setCutoff(3200.0f, static_cast<float>(sampleRateHz_));
    velvetLpR_.setCutoff(3200.0f, static_cast<float>(sampleRateHz_));
    subSmooth_.setCutoff(120.0f, static_cast<float>(sampleRateHz_));
    pink_.seed = 0xC0FFEEu;
    pink_.amplitude = 1.0f;
    pink_.reset();
    white_.seed = 0xA11CEu;
    white_.amplitude = 1.0f;
    white_.reset();
    reset();
}

void ToneEngine::configureTherapeuticReverb()
{
    reverb_.setPreset(params_.reverb);
    if (!reverb_.enabled()) {
        return;
    }
    // Soft spatial halo only — preserve binaural beat coherence.
    if (params_.mode == BeatMode::Binaural) {
        float wet = reverb_.wet();
        switch (params_.reverb) {
            case ReverbPreset::Room:
                wet = std::min(wet, 0.10f);
                break;
            case ReverbPreset::Chamber:
                wet = std::min(wet, 0.13f);
                break;
            case ReverbPreset::Cave:
                wet = std::min(wet, 0.16f);
                break;
            default:
                break;
        }
        reverb_.setWet(wet);
    }
}

void ToneEngine::setParams(const ToneParams& params)
{
    target_ = params;
    target_.carrierHz = clamp(params.carrierHz, 100.0f, 1000.0f);
    target_.beatHz = clamp(params.beatHz, 0.0f, 40.0f);
    target_.masterVolume = clamp(params.masterVolume, 0.0f, 1.0f);
    target_.kernelNoiseBlend = clamp(params.kernelNoiseBlend, 0.0f, 1.0f);
    target_.noiseLevelDbfs = clamp(params.noiseLevelDbfs, -36.0f, -6.0f);
    target_.fadeInSec = clamp(params.fadeInSec, 0.0f, 60.0f);
    target_.comfortModDepth = clamp(params.comfortModDepth, 0.0f, 0.15f);
    target_.filterMix = clamp(params.filterMix, 0.0f, 1.0f);
    target_.subLevel = clamp(params.subLevel, 0.0f, 1.0f);
    target_.subHz = clamp(params.subHz, 40.0f, 80.0f);
    target_.harmonicLayers =
        params.harmonicLayers < 1 ? 1
                                  : (params.harmonicLayers > 3 ? 3 : params.harmonicLayers);

    params_.mode = target_.mode;
    params_.perception = target_.perception;
    params_.reverb = target_.reverb;
    params_.noiseMask = target_.noiseMask;
    params_.filter = target_.filter;
    params_.harmonicLayers = target_.harmonicLayers;
    params_.masterVolume = target_.masterVolume;
    params_.kernelNoiseBlend = target_.kernelNoiseBlend;
    params_.noiseLevelDbfs = target_.noiseLevelDbfs;
    params_.fadeInSec = target_.fadeInSec;
    params_.comfortModDepth = target_.comfortModDepth;
    params_.filterMix = target_.filterMix;
    params_.subLevel = target_.subLevel;
    params_.subHz = target_.subHz;
    filters_.setType(params_.filter);
    filters_.setToneHz(target_.carrierHz);
    configureTherapeuticReverb();
}

void ToneEngine::setPreset(const TonePreset& preset)
{
    setParams(preset.params);
    carrierHz_ = target_.carrierHz;
    beatHz_ = target_.beatHz;
    params_.carrierHz = carrierHz_;
    params_.beatHz = beatHz_;
    samplesRendered_ = 0;
    comfortPhase_ = 0.0f;
    beatPhase_ = 0.0f;
}

void ToneEngine::setCarrierHz(float hz)
{
    target_.carrierHz = clamp(hz, 100.0f, 1000.0f);
    filters_.setToneHz(target_.carrierHz);
}

void ToneEngine::setBeatHz(float hz) { target_.beatHz = clamp(hz, 0.0f, 40.0f); }

void ToneEngine::setMode(BeatMode mode)
{
    params_.mode = mode;
    target_.mode = mode;
    configureTherapeuticReverb();
}

void ToneEngine::setPerception(PerceptionMode mode)
{
    params_.perception = mode;
    target_.perception = mode;
}

void ToneEngine::setReverb(ReverbPreset preset)
{
    params_.reverb = preset;
    target_.reverb = preset;
    configureTherapeuticReverb();
}

void ToneEngine::setFilter(FilterType type)
{
    params_.filter = type;
    target_.filter = type;
    filters_.setType(type);
}

void ToneEngine::setHarmonicLayers(int layers)
{
    params_.harmonicLayers = layers < 1 ? 1 : (layers > 3 ? 3 : layers);
    target_.harmonicLayers = params_.harmonicLayers;
}

void ToneEngine::setKernelNoiseBlend(float blend)
{
    params_.kernelNoiseBlend = clamp(blend, 0.0f, 1.0f);
    target_.kernelNoiseBlend = params_.kernelNoiseBlend;
}

void ToneEngine::setVolume(float volume)
{
    params_.masterVolume = clamp(volume, 0.0f, 1.0f);
    target_.masterVolume = params_.masterVolume;
}

void ToneEngine::setSubLevel(float level)
{
    params_.subLevel = clamp(level, 0.0f, 1.0f);
    target_.subLevel = params_.subLevel;
}

void ToneEngine::setSubHz(float hz)
{
    // Floor at 40 Hz — lower values rattle phone speakers into staticy distortion.
    params_.subHz = clamp(hz, 40.0f, 80.0f);
    target_.subHz = params_.subHz;
}

void ToneEngine::setMicFeedbackEnabled(bool enabled)
{
    micFeedbackEnabled_ = enabled;
    if (!enabled) {
        micRms_ = 0.0f;
        micAgcGain_ = 1.0f;
        micGateOpen_ = 0.0f;
        micFrame_.clear();
    }
}

void ToneEngine::setMicFeedbackGain(float gain)
{
    micFeedbackGain_ = clamp(gain, 0.0f, 1.5f);
}

void ToneEngine::setToneMix(float mix) { toneMix_ = clamp(mix, 0.0f, 1.0f); }

void ToneEngine::setPlaying(bool playing)
{
    if (playing && !playing_) {
        samplesRendered_ = 0;
        comfortPhase_ = 0.0f;
    }
    playing_ = playing;
}

void ToneEngine::reset()
{
    osc_.reset();
    perception_ = PerceptionState{};
    reverb_.reset();
    micReverb_.reset();
    filters_.reset();
    micFilters_.reset();
    pink_.reset();
    white_.reset();
    noiseHp_.reset();
    noiseLp_.reset();
    micHp_.reset();
    micLp_.reset();
    carrierHz_ = target_.carrierHz;
    beatHz_ = target_.beatHz;
    currentRms_ = 0.0f;
    samplesRendered_ = 0;
    comfortPhase_ = 0.0f;
    beatPhase_ = 0.0f;
    subPhase_ = 0.0;
    rumblePhase_ = 0.0;
    subPulsePhase_ = 0.0;
    micRms_ = 0.0f;
    micAgcGain_ = 1.0f;
    micGateOpen_ = 0.0f;
    micHpTight_.reset();
    micLpTight_.reset();
    velvetLpL_.reset();
    velvetLpR_.reset();
    subSmooth_.reset();
}

void ToneEngine::smoothTowardTargets()
{
    carrierHz_ += smoothAlpha_ * (target_.carrierHz - carrierHz_);
    beatHz_ += smoothAlpha_ * (target_.beatHz - beatHz_);
    params_.carrierHz = carrierHz_;
    params_.beatHz = beatHz_;
    if (params_.filter != FilterType::Off) {
        filters_.setToneHz(carrierHz_);
    }
}

void ToneEngine::advanceBeatPhase(uint32_t frames)
{
    if (sampleRateHz_ == 0) {
        return;
    }
    beatPhase_ +=
        kTwoPi * beatHz_ * (static_cast<float>(frames) / static_cast<float>(sampleRateHz_));
    while (beatPhase_ >= kTwoPi) {
        beatPhase_ -= kTwoPi;
    }
}

void ToneEngine::ensureWorkBuffers(uint32_t frames, uint32_t sampleRateHz)
{
    if (noiseFrame_.framesPerChannel() != frames ||
        noiseFrame_.sampleRateHz != sampleRateHz) {
        noiseFrame_ = AudioFrame(frames, sampleRateHz, 2);
    }
    if (dryFrame_.framesPerChannel() != frames ||
        dryFrame_.sampleRateHz != sampleRateHz) {
        dryFrame_ = AudioFrame(frames, sampleRateHz, 2);
    }
}

void ToneEngine::blendSubBass(AudioFrame& stereo)
{
    const float level = params_.subLevel;
    if (level <= 0.0f || stereo.sampleRateHz == 0 || !stereo.isStereo()) {
        return;
    }

    constexpr double kTwoPi = 6.283185307179586476925286766559;
    const double sr = static_cast<double>(stereo.sampleRateHz);
    // Phone-safe band — deep subs (<40 Hz) distort tiny speakers into static.
    const float subHz = clamp(params_.subHz, 40.0f, 80.0f);
    const double subInc = kTwoPi * static_cast<double>(subHz) / sr;
    // Very slow breath swell (~5/min) — soothing, never choppy.
    constexpr double kBreathHz = 0.0833;
    const double breathInc = kTwoPi * kBreathHz / sr;

    // Modest gain — headroom prevents clipping hash.
    const float gain = level * 0.28f;

    for (size_t i = 0; i < stereo.framesPerChannel(); ++i) {
        float body = static_cast<float>(std::sin(subPhase_)) * gain;
        body = subSmooth_.processSample(body);
        const float breath =
            0.88f + 0.12f * (0.5f - 0.5f * static_cast<float>(std::cos(subPulsePhase_)));
        body *= breath;

        stereo.set(i, 0, stereo.get(i, 0) + body);
        stereo.set(i, 1, stereo.get(i, 1) + body);

        subPhase_ += subInc;
        subPulsePhase_ += breathInc;
    }

    while (subPhase_ >= kTwoPi) {
        subPhase_ -= kTwoPi;
    }
    while (subPulsePhase_ >= kTwoPi) {
        subPulsePhase_ -= kTwoPi;
    }
}

void ToneEngine::applyVelvetSmooth(AudioFrame& stereo)
{
    if (!stereo.isStereo()) {
        return;
    }
    for (size_t i = 0; i < stereo.framesPerChannel(); ++i) {
        stereo.set(i, 0, velvetLpL_.processSample(stereo.get(i, 0)));
        stereo.set(i, 1, velvetLpR_.processSample(stereo.get(i, 1)));
    }
}

void ToneEngine::blendKernelNoise(AudioFrame& stereo)
{
    const float blend = params_.kernelNoiseBlend;
    if (blend <= 0.0f) {
        return;
    }

    ensureWorkBuffers(static_cast<uint32_t>(stereo.framesPerChannel()),
                      stereo.sampleRateHz);
    // Extremely quiet optional bed — never the default path.
    pink_.amplitude = 0.06f;
    pink_.render(noiseFrame_);
    noiseHp_.processFrame(noiseFrame_);
    noiseLp_.processFrame(noiseFrame_);

    const float toneGain = 1.0f - blend * 0.04f;
    const float noiseGain = blend * 0.06f;
    for (size_t i = 0; i < stereo.framesPerChannel(); ++i) {
        for (uint32_t ch = 0; ch < 2; ++ch) {
            stereo.set(i, ch,
                       stereo.get(i, ch) * toneGain +
                           noiseFrame_.get(i, ch) * noiseGain);
        }
    }
}

void ToneEngine::applyNoiseMask(AudioFrame& stereo)
{
    if (params_.noiseMask == NoiseMask::None) {
        return;
    }

    ensureWorkBuffers(static_cast<uint32_t>(stereo.framesPerChannel()),
                      stereo.sampleRateHz);
    const float amp = std::pow(10.0f, params_.noiseLevelDbfs / 20.0f);
    if (params_.noiseMask == NoiseMask::Pink) {
        pink_.amplitude = amp;
        pink_.render(noiseFrame_);
    } else {
        white_.amplitude = amp;
        white_.render(noiseFrame_);
    }

    for (size_t i = 0; i < stereo.framesPerChannel(); ++i) {
        stereo.set(i, 0, stereo.get(i, 0) + noiseFrame_.get(i, 0));
        stereo.set(i, 1, stereo.get(i, 1) + noiseFrame_.get(i, 1));
    }
}

void ToneEngine::applyColorFilter(AudioFrame& stereo)
{
    if (params_.filter == FilterType::Off || params_.filterMix <= 0.0f) {
        return;
    }

    ensureWorkBuffers(static_cast<uint32_t>(stereo.framesPerChannel()),
                      stereo.sampleRateHz);
    for (size_t i = 0; i < stereo.framesPerChannel(); ++i) {
        dryFrame_.set(i, 0, stereo.get(i, 0));
        dryFrame_.set(i, 1, stereo.get(i, 1));
    }

    filters_.process(stereo);

    const float wet = params_.filterMix;
    const float dry = 1.0f - wet;
    for (size_t i = 0; i < stereo.framesPerChannel(); ++i) {
        stereo.set(i, 0, dry * dryFrame_.get(i, 0) + wet * stereo.get(i, 0));
        stereo.set(i, 1, dry * dryFrame_.get(i, 1) + wet * stereo.get(i, 1));
    }
}

void ToneEngine::applyComfortMod(AudioFrame& stereo)
{
    const float depth = params_.comfortModDepth;
    if (depth <= 0.0f || stereo.sampleRateHz == 0) {
        return;
    }

    constexpr float modHz = 0.18f;
    const float dt = 1.0f / static_cast<float>(stereo.sampleRateHz);
    float phase = comfortPhase_;
    for (size_t i = 0; i < stereo.framesPerChannel(); ++i) {
        const float g = 1.0f - depth * 0.5f + depth * 0.5f * std::sin(phase);
        stereo.set(i, 0, stereo.get(i, 0) * g);
        stereo.set(i, 1, stereo.get(i, 1) * g);
        phase += kTwoPi * modHz * dt;
    }
    while (phase >= kTwoPi) {
        phase -= kTwoPi;
    }
    comfortPhase_ = phase;
}

void ToneEngine::applyFadeIn(AudioFrame& stereo)
{
    const float fadeSec = params_.fadeInSec;
    if (fadeSec <= 0.0f || stereo.sampleRateHz == 0) {
        samplesRendered_ += stereo.framesPerChannel();
        return;
    }

    const uint64_t fadeSamples =
        static_cast<uint64_t>(fadeSec * static_cast<float>(stereo.sampleRateHz));
    if (samplesRendered_ >= fadeSamples) {
        samplesRendered_ += stereo.framesPerChannel();
        return;
    }

    for (size_t i = 0; i < stereo.framesPerChannel(); ++i) {
        const uint64_t s = samplesRendered_ + i;
        float env = 1.0f;
        if (s < fadeSamples) {
            const float t = static_cast<float>(s) / static_cast<float>(fadeSamples);
            env = 0.5f - 0.5f * std::cos(kTwoPi * 0.5f * t);
        }
        stereo.set(i, 0, stereo.get(i, 0) * env);
        stereo.set(i, 1, stereo.get(i, 1) * env);
    }
    samplesRendered_ += stereo.framesPerChannel();
}

void ToneEngine::applySoftLimiter(AudioFrame& stereo)
{
    // High threshold — only safety. Aggressive limiting was adding harsh hash.
    constexpr float kThreshold = 0.90f;
    for (float& s : stereo.samples) {
        const float a = std::fabs(s);
        if (a <= kThreshold) {
            continue;
        }
        const float sign = s >= 0.0f ? 1.0f : -1.0f;
        const float over = (a - kThreshold) / (1.0f - kThreshold + 1e-6f);
        const float shaped = kThreshold + (1.0f - kThreshold) * std::tanh(over * 0.8f);
        s = sign * shaped;
    }
}

void ToneEngine::render(AudioFrame& stereoOut)
{
    if (!stereoOut.isStereo()) {
        stereoOut.clear();
        return;
    }

    if (!playing_) {
        stereoOut.clear();
        currentRms_ = 0.0f;
        return;
    }

    const float c0 = carrierHz_;
    const float b0 = beatHz_;
    smoothTowardTargets();
    const float c1 = carrierHz_;
    const float b1 = beatHz_;

    if (stereoOut.framesPerChannel() != toneFrame_.framesPerChannel() ||
        stereoOut.sampleRateHz != toneFrame_.sampleRateHz) {
        toneFrame_ =
            AudioFrame(static_cast<uint32_t>(stereoOut.framesPerChannel()),
                       stereoOut.sampleRateHz, 2);
    }

    // Pure sine path first. FX are opt-in only — defaults stay glass-clean.
    osc_.render(toneFrame_, c0, c1, b0, b1, params_.mode,
                params_.harmonicLayers);

    for (size_t i = 0; i < stereoOut.framesPerChannel(); ++i) {
        stereoOut.set(i, 0, toneFrame_.get(i, 0));
        stereoOut.set(i, 1, toneFrame_.get(i, 1));
    }

    // Soft continuous sub hum (no beat chopping — that sounded staticy).
    blendSubBass(stereoOut);

    if (params_.kernelNoiseBlend > 0.0f) {
        blendKernelNoise(stereoOut);
    }
    if (params_.perception != PerceptionMode::Standard) {
        applyPerception(stereoOut, params_.perception, beatHz_, perception_);
    }
    if (params_.filter != FilterType::Off && params_.filterMix > 0.0f) {
        filters_.setToneHz(carrierHz_);
        applyColorFilter(stereoOut);
    }
    if (params_.noiseMask != NoiseMask::None) {
        applyNoiseMask(stereoOut);
    }
    if (params_.reverb != ReverbPreset::Off) {
        reverb_.process(stereoOut);
    }
    if (params_.comfortModDepth > 0.0f) {
        applyComfortMod(stereoOut);
    }

    // Velvet smooth the whole bed — soothing, removes speaker grit.
    applyVelvetSmooth(stereoOut);
    applyFadeIn(stereoOut);

    const float vol = params_.masterVolume;
    double sumSq = 0.0;
    for (float& s : stereoOut.samples) {
        s *= vol;
        sumSq += static_cast<double>(s) * static_cast<double>(s);
    }
    applySoftLimiter(stereoOut);

    if (!stereoOut.samples.empty()) {
        currentRms_ = static_cast<float>(
            std::sqrt(sumSq / static_cast<double>(stereoOut.samples.size())));
    }

    advanceBeatPhase(static_cast<uint32_t>(stereoOut.framesPerChannel()));
}

void ToneEngine::processMicFrame(const float* micInterleaved, uint32_t frames,
                                 uint32_t micChannels)
{
    if (micInterleaved == nullptr || frames == 0 || micChannels == 0) {
        micFrame_.clear();
        micRms_ = 0.0f;
        micGateOpen_ *= 0.85f;
        return;
    }

    if (micFrame_.framesPerChannel() != frames ||
        micFrame_.sampleRateHz != sampleRateHz_) {
        micFrame_ = AudioFrame(frames, sampleRateHz_, 2);
    }

    for (uint32_t i = 0; i < frames; ++i) {
        float mono = 0.0f;
        if (micChannels == 1) {
            mono = micInterleaved[i];
        } else {
            mono = 0.5f * (micInterleaved[i * micChannels] +
                           micInterleaved[i * micChannels + 1]);
        }
        micFrame_.set(i, 0, mono);
        micFrame_.set(i, 1, mono);
    }

    // Band-limit first so RMS/AGC see a cleaner signal.
    micHp_.processFrame(micFrame_);
    micLp_.processFrame(micFrame_);
    if (micLowLatency_) {
        // Extra gentle speech shaping without reverb (still ~zero delay).
        micHpTight_.processFrame(micFrame_);
        micLpTight_.processFrame(micFrame_);
    } else {
        micFilters_.setType(params_.filter == FilterType::Off ? FilterType::Warm
                                                              : params_.filter);
        micFilters_.setToneHz(carrierHz_);
        micFilters_.process(micFrame_);
        if (params_.reverb != ReverbPreset::Off) {
            micReverb_.setPreset(params_.reverb);
            micReverb_.setWet(std::min(0.10f, reverb_.wet()));
            micReverb_.process(micFrame_);
        }
    }

    double filteredSq = 0.0;
    for (float s : micFrame_.samples) {
        filteredSq += static_cast<double>(s) * static_cast<double>(s);
    }
    const float rawRms = static_cast<float>(
        std::sqrt(filteredSq / static_cast<double>(micFrame_.samples.size())));
    micRms_ = rawRms;

    // Aggressive noise gate — room hiss / speaker bleed stays muted.
    constexpr float kGateOpen = 0.022f;   // ~-33 dBFS
    constexpr float kGateClose = 0.014f;  // hysteresis
    const float gateTarget =
        (rawRms > (micGateOpen_ > 0.5f ? kGateClose : kGateOpen)) ? 1.0f : 0.0f;
    // Fast open, slower close to avoid chatter.
    const float gateAlpha = gateTarget > micGateOpen_ ? 0.55f : 0.12f;
    micGateOpen_ += gateAlpha * (gateTarget - micGateOpen_);

    // Hard mute once the gate is nearly closed — no residual hiss under tones.
    if (micGateOpen_ < 0.05f) {
        micGateOpen_ = 0.0f;
        micFrame_.clear();
        micAgcGain_ += 0.08f * (1.0f - micAgcGain_);
        return;
    }

    // AGC only when the gate is open — never pump up the noise floor.
    if (micGateOpen_ > 0.25f && rawRms > kGateClose) {
        const float desired = clamp(micAgcTarget_ / rawRms, 0.6f, 1.6f);
        micAgcGain_ += 0.05f * (desired - micAgcGain_);
    } else {
        micAgcGain_ += 0.06f * (1.0f - micAgcGain_);
    }
    micAgcGain_ = clamp(micAgcGain_, 0.6f, 1.6f);

    const float g = micAgcGain_ * micFeedbackGain_ * micGateOpen_;
    for (float& s : micFrame_.samples) {
        s *= g;
        // Soft clip
        if (s > 0.75f) {
            s = 0.75f + 0.25f * std::tanh((s - 0.75f) * 2.5f);
        } else if (s < -0.75f) {
            s = -0.75f + 0.25f * std::tanh((s + 0.75f) * 2.5f);
        }
    }
}

void ToneEngine::renderInterleaved(float* interleavedStereo, uint32_t frames)
{
    renderInterleavedWithMic(interleavedStereo, nullptr, frames, 0);
}

void ToneEngine::renderInterleavedWithMic(float* interleavedStereoOut,
                                          const float* micInterleaved,
                                          uint32_t frames, uint32_t micChannels)
{
    if (interleavedStereoOut == nullptr || frames == 0) {
        return;
    }

    if (toneFrame_.framesPerChannel() != frames ||
        toneFrame_.sampleRateHz != sampleRateHz_) {
        toneFrame_ = AudioFrame(frames, sampleRateHz_, 2);
    }

    render(toneFrame_);

    const bool useMic = micFeedbackEnabled_ && micInterleaved != nullptr &&
                        micChannels > 0;
    if (useMic) {
        processMicFrame(micInterleaved, frames, micChannels);
    } else {
        micRms_ = 0.0f;
        micGateOpen_ = 0.0f;
    }

    // When the gate is closed, leave tones at full level — no ducking, no hiss.
    const float gate = useMic ? micGateOpen_ : 0.0f;
    const float micLayer = useMic ? (1.0f - toneMix_) * gate : 0.0f;
    const float toneGain = 1.0f - micLayer * 0.35f;

    auto softCeil = [](float x) {
        const float a = std::fabs(x);
        if (a <= 0.85f) {
            return x;
        }
        const float sign = x >= 0.0f ? 1.0f : -1.0f;
        return sign * (0.85f + 0.12f * std::tanh((a - 0.85f) * 3.0f));
    };

    for (uint32_t i = 0; i < frames; ++i) {
        float l = toneFrame_.get(i, 0) * toneGain;
        float r = toneFrame_.get(i, 1) * toneGain;
        if (micLayer > 0.0f) {
            l += micFrame_.get(i, 0) * micLayer;
            r += micFrame_.get(i, 1) * micLayer;
        }
        interleavedStereoOut[i * 2 + 0] = softCeil(l);
        interleavedStereoOut[i * 2 + 1] = softCeil(r);
    }
}

}  // namespace manticore::toneflow

