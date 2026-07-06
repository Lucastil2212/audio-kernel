#pragma once

#include <cstdint>
#include <string>

namespace manticore {

struct OutcomePacket {
    uint64_t timestampNs = 0;
    uint32_t sessionId = 0;
    float comfortScore = 0.0f;
    float adherenceScore = 0.0f;
    std::string notes;
};

}  // namespace manticore
