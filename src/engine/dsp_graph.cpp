#include "manticore/engine/dsp_graph.h"

#include "manticore/dsp/analyzer.h"
#include "manticore/dsp/gain.h"
#include "manticore/dsp/limiter.h"
#include "manticore/dsp/mixer.h"

namespace manticore {

class DspGraph::Impl {
public:
    AudioFrame therapyFrame;
};

DspGraph::DspGraph() = default;

DspGraph::~DspGraph() { destroyImpl(); }

void DspGraph::ensureImpl()
{
    if (!impl_) {
        impl_ = new Impl();
    }
}

void DspGraph::destroyImpl()
{
    delete impl_;
    impl_ = nullptr;
}

void DspGraph::prepare(const RuntimeConfig& config)
{
    config_ = config;
    inputGain_ = config.inputGain;
    outputGain_ = config.outputGain;
    limiterEnabled_ = config.enableLimiter;
    therapyEnabled_ = config.enableTherapy;

    ensureImpl();
    impl_->therapyFrame =
        AudioFrame(config.frameSize, config.sampleRateHz, config.channels);

    stimulusEngine_.prepare(config);
    safetyGovernor_.prepare(config);
    health_ = SystemHealth{};
}

void DspGraph::reset()
{
    stimulusEngine_.reset();
    safetyGovernor_.clearEvents();
    health_ = SystemHealth{};
}

void DspGraph::setInputGain(float gain) { inputGain_ = gain; }

void DspGraph::setOutputGain(float gain) { outputGain_ = gain; }

void DspGraph::enableLimiter(bool enabled) { limiterEnabled_ = enabled; }

void DspGraph::enableTherapy(bool enabled) { therapyEnabled_ = enabled; }

void DspGraph::setTherapyParams(const TherapyParamSet& params)
{
    stimulusEngine_.setParams(params);
}

void DspGraph::process(AudioFrame& input, AudioFrame& output)
{
    output = input;
    output.clipFlag = false;

    applyGain(output, inputGain_);

    if (therapyEnabled_ && stimulusEngine_.isActive()) {
        ensureImpl();
        impl_->therapyFrame.sampleRateHz = output.sampleRateHz;
        impl_->therapyFrame.channels = output.channels;
        if (impl_->therapyFrame.framesPerChannel() != output.framesPerChannel()) {
            impl_->therapyFrame = AudioFrame(output.framesPerChannel(),
                                             output.sampleRateHz, output.channels);
        }
        stimulusEngine_.render(impl_->therapyFrame);
        therapyBlend_ = stimulusEngine_.currentBlend();
        mixFrames(output, impl_->therapyFrame, output, 1.0f, therapyBlend_);
    }

    applyGain(output, outputGain_);

    FrameStats preLimiterStats = analyzeFrame(output);
    health_.lastPeakAbs = preLimiterStats.peakAbs;
    health_.lastRms = preLimiterStats.rms;

    CueHealth cueHealth = safetyGovernor_.evaluateFrame(output, health_);
    float adjustedOutputGain = outputGain_;
    float adjustedTherapyBlend = therapyBlend_;
    safetyGovernor_.applyFallback(output, cueHealth, adjustedOutputGain,
                                  adjustedTherapyBlend);
    health_.cueHealth = cueHealth;

    if (limiterEnabled_) {
        applyLimiter(output);
    }

    if (output.clipFlag) {
        ++health_.clipCount;
    }

    ++health_.callbackCount;
}

}  // namespace manticore
