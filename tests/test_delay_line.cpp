#include "test_runner.h"

#include "manticore/core/audio_frame.h"
#include "manticore/dsp/delay_line.h"

void testDelayLine()
{
    using namespace manticore;

    DelayLine delay;
    delay.prepare(16);
    delay.setDelaySamples(0);
    EXPECT_NEAR(delay.processSample(0.75f), 0.75f, 1e-6f);

    delay.reset();
    delay.setDelaySamples(4);
    delay.processSample(0.1f);
    delay.processSample(0.2f);
    delay.processSample(0.3f);
    delay.processSample(0.4f);
    EXPECT_NEAR(delay.processSample(0.5f), 0.1f, 1e-6f);

    AudioFrame frame(8, 48000, 1);
    for (size_t i = 0; i < frame.framesPerChannel(); ++i) {
        frame.set(i, 0, static_cast<float>(i) * 0.1f);
    }
    const size_t capacityBefore = delay.processSample(0.0f);
    (void)capacityBefore;
    delay.processFrame(frame);
    EXPECT_TRUE(frame.sampleCount() == 8);
}
