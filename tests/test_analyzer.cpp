#include "test_runner.h"

#include "manticore/core/audio_frame.h"
#include "manticore/dsp/analyzer.h"

#include <cmath>
#include <limits>

void testAnalyzer()
{
    using namespace manticore;

    AudioFrame frame(4, 48000, 1);
    frame.set(0, 0, -0.5f);
    frame.set(1, 0, 0.5f);
    frame.set(2, 0, 1.0f);
    frame.set(3, 0, 0.0f);

    FrameStats stats = analyzeFrame(frame);
    EXPECT_NEAR(stats.minSample, -0.5f, 1e-6f);
    EXPECT_NEAR(stats.maxSample, 1.0f, 1e-6f);
    EXPECT_NEAR(stats.peakAbs, 1.0f, 1e-6f);
    EXPECT_TRUE(stats.clipped);

    AudioFrame dc(4, 48000, 1);
    for (size_t i = 0; i < 4; ++i) {
        dc.set(i, 0, 0.25f);
    }
    stats = analyzeFrame(dc);
    EXPECT_NEAR(stats.dcOffset, 0.25f, 1e-6f);
    EXPECT_NEAR(stats.rms, 0.25f, 1e-6f);

    AudioFrame empty;
    stats = analyzeFrame(empty);
    EXPECT_NEAR(stats.peakAbs, 0.0f, 1e-6f);

    AudioFrame invalid(1, 48000, 1);
    invalid.set(0, 0, std::numeric_limits<float>::quiet_NaN());
    stats = analyzeFrame(invalid);
    EXPECT_TRUE(stats.hasNaN);

    invalid.set(0, 0, std::numeric_limits<float>::infinity());
    stats = analyzeFrame(invalid);
    EXPECT_TRUE(stats.hasInf);
}
