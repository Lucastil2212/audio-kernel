#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace manticore {

bool writeWavFile(const std::string& filename, const std::vector<float>& samples,
                  uint32_t sampleRateHz, uint16_t channels);

bool readWavFile(const std::string& filename, std::vector<float>& samples,
                 uint32_t& sampleRateHz, uint16_t& channels);

}  // namespace manticore
