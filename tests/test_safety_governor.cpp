#include "test_runner.h"

#include "manticore/core/audio_frame.h"
#include "manticore/dsp/analyzer.h"
#include "manticore/engine/runtime_config.h"
#include "manticore/safety/safety_governor.h"
#include "manticore/telemetry/system_health.h"

#include <limits>

void testSafetyGovernor()
{
    using namespace manticore;

    SafetyGovernor governor;
    RuntimeConfig config;
    governor.prepare(config);
    SystemHealth health;

    AudioFrame nanFrame(4, 48000, 1);
    nanFrame.set(0, 0, std::numeric_limits<float>::quiet_NaN());
    CueHealth cue = governor.evaluateFrame(nanFrame, health);
    EXPECT_TRUE(cue.state == CueState::BaselineOnly);

    governor.clearEvents();
    governor.prepare(config);

    AudioFrame clipFrame(4, 48000, 1);
    clipFrame.set(0, 0, 1.05f);
    clipFrame.set(1, 0, -1.05f);
    clipFrame.set(2, 0, 0.0f);
    clipFrame.set(3, 0, 0.0f);
    cue = governor.evaluateFrame(clipFrame, health);
    EXPECT_TRUE(cue.state == CueState::SafeProfile ||
                cue.state == CueState::SoftBackoff);

    governor.clearEvents();
    governor.prepare(config);
    for (int i = 0; i < 3; ++i) {
        cue = governor.evaluateFrame(clipFrame, health);
    }
    EXPECT_TRUE(cue.state == CueState::TherapyOff);

    float outputGain = 2.0f;
    float therapyBlend = 0.8f;
    AudioFrame frame(4, 48000, 1);
    governor.applyFallback(frame, cue, outputGain, therapyBlend);
    EXPECT_TRUE(therapyBlend == 0.0f);
}
