#include "test_runner.h"

#include "manticore/core/audio_frame.h"
#include "manticore/dsp/gain.h"

void testGain()
{
    using namespace manticore;

    AudioFrame frame(4, 48000, 1);
    frame.set(0, 0, 0.5f);
    frame.set(1, 0, -0.25f);
    frame.set(2, 0, 1.0f);
    frame.set(3, 0, 0.0f);

    applyGain(frame, 2.0f);
    EXPECT_NEAR(frame.get(0, 0), 1.0f, 1e-6f);
    EXPECT_NEAR(frame.get(1, 0), -0.5f, 1e-6f);

    applyGain(frame, 0.0f);
    EXPECT_NEAR(frame.get(0, 0), 0.0f, 1e-6f);

    frame.set(0, 0, 0.5f);
    applyGain(frame, -1.0f);
    EXPECT_NEAR(frame.get(0, 0), -0.5f, 1e-6f);
}
