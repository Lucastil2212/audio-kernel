#include "test_runner.h"

#include "manticore/dsp/wav_writer.h"

#include <fstream>
#include <vector>

void testWavWriter()
{
    using namespace manticore;

    std::vector<float> samples = {0.0f, 0.5f, -0.5f, 1.0f, -1.0f};
    const std::string path = "test_output.wav";
    const bool ok = writeWavFile(path, samples, 48000, 1);
    EXPECT_TRUE(ok);

    std::ifstream file(path, std::ios::binary);
    EXPECT_TRUE(file.good());

    char riff[4];
    file.read(riff, 4);
    EXPECT_TRUE(std::string(riff, 4) == "RIFF");

    int32_t chunkSize = 0;
    file.read(reinterpret_cast<char*>(&chunkSize), sizeof(chunkSize));
    const int32_t expectedSize =
        36 + static_cast<int32_t>(samples.size() * 2);
    EXPECT_TRUE(chunkSize == expectedSize);

    std::vector<float> readBack;
    uint32_t sampleRate = 0;
    uint16_t channels = 0;
    EXPECT_TRUE(readWavFile(path, readBack, sampleRate, channels));
    EXPECT_TRUE(sampleRate == 48000);
    EXPECT_TRUE(channels == 1);
    EXPECT_TRUE(readBack.size() == samples.size());
}
