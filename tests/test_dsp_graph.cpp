#include "test_runner.h"

#include "manticore/core/audio_frame.h"
#include "manticore/engine/dsp_graph.h"
#include "manticore/engine/runtime_config.h"

void testDspGraph()
{
    using namespace manticore;

    for (uint32_t frameSize : {16u, 32u, 64u}) {
        RuntimeConfig config;
        config.frameSize = frameSize;
        config.sampleRateHz = 48000;
        config.channels = 1;
        config.inputGain = 0.5f;
        config.outputGain = 0.5f;
        config.enableLimiter = true;

        DspGraph graph;
        graph.prepare(config);

        AudioFrame input(frameSize, 48000, 1);
        for (size_t i = 0; i < input.framesPerChannel(); ++i) {
            input.set(i, 0, 0.8f);
        }

        AudioFrame output;
        graph.process(input, output);
        EXPECT_TRUE(output.framesPerChannel() == frameSize);
        EXPECT_TRUE(output.sampleCount() == frameSize);
    }
}
