#include "manticore/dsp/wav_writer.h"

#include <algorithm>
#include <fstream>

namespace manticore {

namespace {

void writeString(std::ofstream& file, const std::string& text)
{
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
}

void writeInt16(std::ofstream& file, int16_t value)
{
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void writeInt32(std::ofstream& file, int32_t value)
{
    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

}  // namespace

bool writeWavFile(const std::string& filename, const std::vector<float>& samples,
                  uint32_t sampleRateHz, uint16_t channels)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }

    const uint16_t bitsPerSample = 16;
    const uint16_t bytesPerSample = bitsPerSample / 8;
    const uint32_t dataSize =
        static_cast<uint32_t>(samples.size()) * bytesPerSample;
    const uint32_t byteRate = sampleRateHz * channels * bytesPerSample;
    const uint16_t blockAlign = static_cast<uint16_t>(channels * bytesPerSample);
    const uint32_t chunkSize = 36 + dataSize;

    writeString(file, "RIFF");
    writeInt32(file, static_cast<int32_t>(chunkSize));
    writeString(file, "WAVE");

    writeString(file, "fmt ");
    writeInt32(file, 16);
    writeInt16(file, 1);
    writeInt16(file, static_cast<int16_t>(channels));
    writeInt32(file, static_cast<int32_t>(sampleRateHz));
    writeInt32(file, static_cast<int32_t>(byteRate));
    writeInt16(file, blockAlign);
    writeInt16(file, bitsPerSample);

    writeString(file, "data");
    writeInt32(file, static_cast<int32_t>(dataSize));

    for (float sample : samples) {
        const float clamped = std::clamp(sample, -1.0f, 1.0f);
        const int16_t pcm = static_cast<int16_t>(clamped * 32767.0f);
        writeInt16(file, pcm);
    }

    return file.good();
}

bool readWavFile(const std::string& filename, std::vector<float>& samples,
                 uint32_t& sampleRateHz, uint16_t& channels)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }

    char riff[4];
    file.read(riff, 4);
    if (std::string(riff, 4) != "RIFF") {
        return false;
    }

    int32_t chunkSize = 0;
    file.read(reinterpret_cast<char*>(&chunkSize), sizeof(chunkSize));

    char wave[4];
    file.read(wave, 4);
    if (std::string(wave, 4) != "WAVE") {
        return false;
    }

    uint16_t audioFormat = 0;
    uint16_t bitsPerSample = 16;
    uint32_t dataSize = 0;

    while (file && !file.eof()) {
        char chunkId[4];
        file.read(chunkId, 4);
        if (!file) {
            break;
        }

        int32_t subchunkSize = 0;
        file.read(reinterpret_cast<char*>(&subchunkSize), sizeof(subchunkSize));

        if (std::string(chunkId, 4) == "fmt ") {
            file.read(reinterpret_cast<char*>(&audioFormat), sizeof(audioFormat));
            file.read(reinterpret_cast<char*>(&channels), sizeof(channels));
            file.read(reinterpret_cast<char*>(&sampleRateHz), sizeof(sampleRateHz));
            int32_t byteRate = 0;
            uint16_t blockAlign = 0;
            file.read(reinterpret_cast<char*>(&byteRate), sizeof(byteRate));
            file.read(reinterpret_cast<char*>(&blockAlign), sizeof(blockAlign));
            file.read(reinterpret_cast<char*>(&bitsPerSample), sizeof(bitsPerSample));
            if (subchunkSize > 16) {
                file.seekg(subchunkSize - 16, std::ios::cur);
            }
        } else if (std::string(chunkId, 4) == "data") {
            dataSize = static_cast<uint32_t>(subchunkSize);
            break;
        } else {
            file.seekg(subchunkSize, std::ios::cur);
        }
    }

    if (audioFormat != 1 || bitsPerSample != 16 || dataSize == 0) {
        return false;
    }

    const size_t sampleCount = dataSize / (bitsPerSample / 8);
    samples.resize(sampleCount);

    for (size_t i = 0; i < sampleCount; ++i) {
        int16_t pcm = 0;
        file.read(reinterpret_cast<char*>(&pcm), sizeof(pcm));
        samples[i] = static_cast<float>(pcm) / 32767.0f;
    }

    return file.good();
}

}  // namespace manticore
