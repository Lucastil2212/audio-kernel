#include "manticore/therapy/stimulus_engine.h"

#include <algorithm>
#include <cmath>

#include "manticore/core/time.h"
#include "manticore/dsp/envelope.h"
#include "manticore/dsp/filters.h"
#include "manticore/dsp/mixer.h"
#include "manticore/dsp/noise.h"
#include "manticore/dsp/oscillator.h"

namespace manticore {

class StimulusEngine::Impl {
public:
    SineOscillator sine;
    WhiteNoiseGenerator whiteNoise;
    PinkNoiseGenerator pinkNoise;
    OnePoleLowPass lowPass;
    OnePoleHighPass highPass;
    SineEnvelope envelope;
};

void StimulusEngine::ensureImpl()
{
    if (!impl_) {
        impl_ = new Impl();
    }
}

void StimulusEngine::destroyImpl()
{
    delete impl_;
    impl_ = nullptr;
}

void StimulusEngine::prepare(const RuntimeConfig& config)
{
    config_ = config;
    ensureImpl();
    impl_->lowPass.setCutoff(8000.0f, static_cast<float>(config.sampleRateHz));
    impl_->highPass.setCutoff(200.0f, static_cast<float>(config.sampleRateHz));
    impl_->envelope.set(0.05f, 0.0f, static_cast<float>(config.sampleRateHz));
    reset();
}

void StimulusEngine::setParams(const TherapyParamSet& params)
{
    std::string error;
    if (!validateTherapyParams(params, config_.sampleRateHz, monotonicTimeNs(),
                               error)) {
        active_ = false;
        return;
    }
    params_ = params;
    active_ = params.stage != SessionStage::Disabled &&
              params.stage != SessionStage::Complete &&
              params.carrierFamily != CarrierFamily::None;

    ensureImpl();
    impl_->sine.frequencyHz = 440.0f;
    impl_->sine.amplitude = 0.1f;
    impl_->whiteNoise.seed = params.seed;
    impl_->whiteNoise.amplitude = 0.1f;
    impl_->pinkNoise.seed = params.seed;
    impl_->pinkNoise.amplitude = 0.1f;
    impl_->lowPass.setCutoff(params.bandHighHz,
                            static_cast<float>(config_.sampleRateHz));
    impl_->highPass.setCutoff(params.bandLowHz,
                              static_cast<float>(config_.sampleRateHz));
    impl_->envelope.set(params.envelopeRateHz, params.envelopeDepth,
                        static_cast<float>(config_.sampleRateHz));
}

float StimulusEngine::computeRampGain() const
{
    const uint32_t rampSamples =
        static_cast<uint32_t>(config_.sampleRateHz * 5);
    if (params_.stage == SessionStage::RampIn) {
        return std::min(1.0f, static_cast<float>(samplesRendered_) /
                                   static_cast<float>(rampSamples));
    }
    if (params_.stage == SessionStage::RampOut) {
        return std::max(0.0f, 1.0f - static_cast<float>(samplesRendered_) /
                                       static_cast<float>(rampSamples));
    }
    if (params_.stage == SessionStage::Active) {
        return 1.0f;
    }
    return 0.0f;
}

void StimulusEngine::render(AudioFrame& therapyFrame)
{
    if (!active_ || !impl_) {
        therapyFrame.clear();
        currentBlend_ = 0.0f;
        return;
    }

    therapyFrame.clear();
    AudioFrame carrier(therapyFrame.framesPerChannel(), therapyFrame.sampleRateHz,
                       therapyFrame.channels);

    switch (params_.carrierFamily) {
        case CarrierFamily::Sine:
            impl_->sine.render(carrier);
            break;
        case CarrierFamily::WhiteNoise:
            impl_->whiteNoise.render(carrier);
            break;
        case CarrierFamily::PinkNoise:
            impl_->pinkNoise.render(carrier);
            break;
        case CarrierFamily::ShapedNoise:
            impl_->whiteNoise.render(carrier);
            impl_->highPass.processFrame(carrier);
            impl_->lowPass.processFrame(carrier);
            break;
        case CarrierFamily::AmbientBed:
            impl_->pinkNoise.render(carrier);
            for (size_t i = 0; i < carrier.framesPerChannel(); ++i) {
                const float env = impl_->envelope.next();
                for (uint32_t ch = 0; ch < carrier.channels; ++ch) {
                    carrier.set(i, ch, carrier.get(i, ch) * env);
                }
            }
            break;
        default:
            break;
    }

    const float ramp = computeRampGain();
    currentBlend_ = params_.blendGain * ramp;

    mixFrames(carrier, therapyFrame, therapyFrame, currentBlend_, 0.0f);
    samplesRendered_ += therapyFrame.framesPerChannel();
}

void StimulusEngine::reset()
{
    samplesRendered_ = 0;
    rampProgress_ = 0.0f;
    currentBlend_ = 0.0f;
    if (impl_) {
        impl_->sine.phaseRadians = 0.0f;
        impl_->whiteNoise.reset();
        impl_->pinkNoise.reset();
        impl_->lowPass.reset();
        impl_->highPass.reset();
        impl_->envelope.reset();
    }
}

StimulusEngine::StimulusEngine() = default;

StimulusEngine::~StimulusEngine() { destroyImpl(); }

}  // namespace manticore
