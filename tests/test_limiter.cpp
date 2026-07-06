#include "test_runner.h"

#include "manticore/core/audio_frame.h"
#include "manticore/dsp/limiter.h"

void testLimiter()
{
    using namespace manticore;

    AudioFrame frame(3, 48000, 1);
    frame.set(0, 0, 1.5f);
    frame.set(1, 0, -2.0f);
    frame.set(2, 0, 0.5f);

    applyLimiter(frame);
    EXPECT_NEAR(frame.get(0, 0), 1.0f, 1e-6f);
    EXPECT_NEAR(frame.get(1, 0), -1.0f, 1e-6f);
    EXPECT_NEAR(frame.get(2, 0), 0.5f, 1e-6f);
    EXPECT_TRUE(frame.clipFlag);

    AudioFrame safe(2, 48000, 1);
    safe.set(0, 0, 0.1f);
    safe.set(1, 0, -0.1f);
    applyLimiter(safe);
    EXPECT_TRUE(!safe.clipFlag);
}
