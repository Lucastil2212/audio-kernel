#include "test_runner.h"

#include "manticore/core/audio_frame.h"
#include "manticore/dsp/analyzer.h"
#include "manticore/dsp/oscillator.h"

void testOscillator()
{
    using namespace manticore;

    SineOscillator osc;
    osc.frequencyHz = 440.0f;
    osc.amplitude = 0.25f;
    osc.phaseRadians = 0.0f;

    AudioFrame frame(64, 48000, 1);
    osc.render(frame);

    FrameStats stats = analyzeFrame(frame);
    EXPECT_TRUE(stats.peakAbs > 0.0f);
    EXPECT_TRUE(stats.peakAbs <= osc.amplitude + 1e-3f);

    const float phaseBefore = osc.phaseRadians;
    osc.render(frame);
    EXPECT_TRUE(osc.phaseRadians != phaseBefore || osc.frequencyHz == 0.0f);
}
