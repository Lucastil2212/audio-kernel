#include "test_runner.h"

#include "manticore/core/audio_frame.h"
#include "manticore/dsp/filters.h"

#include <cmath>

void testFilters()
{
    using namespace manticore;

    OnePoleLowPass lpf;
    lpf.setCutoff(1000.0f, 48000.0f);
    float y = 0.0f;
    for (int i = 0; i < 100; ++i) {
        y = lpf.processSample(1.0f);
    }
    EXPECT_TRUE(y > 0.9f);

    lpf.reset();
    y = lpf.processSample(0.5f);
    EXPECT_TRUE(y > 0.0f && y <= 0.5f);

    OnePoleHighPass hpf;
    hpf.setCutoff(100.0f, 48000.0f);
    float dcRemoved = 0.0f;
    for (int i = 0; i < 4800; ++i) {
        dcRemoved = hpf.processSample(0.5f);
    }
    EXPECT_TRUE(std::fabs(dcRemoved) < 0.05f);

    hpf.reset();
    EXPECT_NEAR(hpf.processSample(0.0f), 0.0f, 1e-3f);
}
