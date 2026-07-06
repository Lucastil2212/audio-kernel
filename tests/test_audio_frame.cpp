#include "test_runner.h"

#include "manticore/core/audio_frame.h"

#include <limits>

void testAudioFrame()
{
    using namespace manticore;

    AudioFrame frame(64, 48000, 1);
    EXPECT_TRUE(frame.sampleCount() == 64);
    EXPECT_TRUE(frame.framesPerChannel() == 64);
    EXPECT_NEAR(frame.durationMs(), 64.0f / 48000.0f * 1000.0f, 1e-4f);
    EXPECT_TRUE(frame.isMono());
    EXPECT_TRUE(!frame.isStereo());

    frame.set(10, 0, 0.5f);
    EXPECT_NEAR(frame.get(10, 0), 0.5f, 1e-6f);

    frame.clear();
    EXPECT_NEAR(frame.get(10, 0), 0.0f, 1e-6f);

    AudioFrame stereo(32, 48000, 2);
    EXPECT_TRUE(stereo.sampleCount() == 64);
    EXPECT_TRUE(stereo.isStereo());
    stereo.set(1, 1, -0.25f);
    EXPECT_NEAR(stereo.get(1, 1), -0.25f, 1e-6f);

    stereo.samples[0] = std::numeric_limits<float>::quiet_NaN();
    EXPECT_TRUE(stereo.hasInvalidSamples());
}
