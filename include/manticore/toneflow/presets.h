#pragma once

#include <cstddef>

#include "manticore/toneflow/types.h"

namespace manticore::toneflow {

const TonePreset* allPresets(size_t& count);
const TonePreset* presetById(const char* id);
const TonePreset* presetByIndex(size_t index);
size_t presetCount();

}  // namespace manticore::toneflow
