#include "manticore/toneflow/presets.h"

#include <cstring>

namespace manticore::toneflow {
namespace {

/**
 * Soothing pure-sine presets.
 * Quiet carriers, long fades, gentle continuous sub hum (phone-safe ~50 Hz).
 */
ToneParams make(float carrier, float beat, float volume = 0.18f,
                float fadeIn = 5.0f, float subLevel = 0.15f, float subHz = 52.0f,
                BeatMode mode = BeatMode::Binaural)
{
    ToneParams p;
    p.carrierHz = carrier;
    p.beatHz = beat;
    p.mode = mode;
    p.perception = PerceptionMode::Standard;
    p.reverb = ReverbPreset::Off;
    p.harmonicLayers = 1;
    p.kernelNoiseBlend = 0.0f;
    p.masterVolume = volume;
    p.fadeInSec = fadeIn;
    p.comfortModDepth = 0.0f;
    p.filter = FilterType::Off;
    p.filterMix = 0.0f;
    p.noiseMask = NoiseMask::None;
    p.noiseLevelDbfs = -36.0f;
    p.subLevel = subLevel;
    p.subHz = subHz;
    return p;
}

/**
 * Calming / healing journeys — exploratory naming only, not medical.
 * Tuned for soft listening on headphones and phone speakers.
 */
const TonePreset kPresets[] = {
    // --- Core soothe ---
    {"alpha_calm", "Alpha Soothe (10 Hz)",
     "Clean pure-sine alpha. Stable playback path.", "calm",
     make(200.0f, 10.0f, 0.18f, 5.0f, 0.12f, 52.0f), 20},
    {"theta_meditate", "Theta Cradle (6 Hz)",
     "Quiet theta cradle with soft body hum.", "deep",
     make(180.0f, 6.0f, 0.17f, 6.0f, 0.15f, 50.0f), 25},
    {"delta_sleep", "Delta Nest (2.5 Hz)",
     "Soft slow-wave nest for rest.", "deep",
     make(150.0f, 2.5f, 0.15f, 8.0f, 0.18f, 48.0f), 35},
    {"schumann", "Schumann Soft (7.83 Hz)",
     "Earth-band beat, warm and clean.", "heal",
     make(196.0f, 7.83f, 0.18f, 5.0f, 0.15f, 52.0f), 20},
    {"beta_soft", "Beta Soft Focus (14 Hz)",
     "Light alert focus without harshness.", "focus",
     make(220.0f, 14.0f, 0.18f, 4.0f, 0.08f, 55.0f), 15},

    // --- Soft body ---
    {"soft_hum", "Soft Body Hum",
     "Theta tones over a continuous soothing 50 Hz hum.", "calm",
     make(170.0f, 5.0f, 0.17f, 5.0f, 0.22f, 50.0f), 20},
    {"warm_bath", "Warm Tone Bath",
     "Low alpha with a light sub bed.", "calm",
     make(190.0f, 8.0f, 0.17f, 6.0f, 0.18f, 52.0f), 20},
    {"deep_cradle", "Deep Cradle",
     "Delta-theta border; quiet and enveloping.", "deep",
     make(160.0f, 3.5f, 0.15f, 7.0f, 0.20f, 48.0f), 30},

    // --- Healing (quiet) ---
    {"heal_174", "174 Hz — Soft Foundation",
     "Soft 174 Hz sine with gentle hum.", "heal",
     make(174.0f, 4.0f, 0.17f, 5.0f, 0.16f, 48.0f), 20},
    {"heal_285", "285 Hz — Soft Renewal",
     "Quiet renewal field.", "heal",
     make(285.0f, 5.0f, 0.17f, 5.0f, 0.12f, 52.0f), 20},
    {"heal_396", "396 Hz — Soft Release",
     "Soft release field.", "heal",
     make(396.0f, 6.0f, 0.17f, 5.0f, 0.12f, 52.0f), 20},
    {"heal_417", "417 Hz — Soft Change",
     "Gentle change field with Schumann beat.", "heal",
     make(417.0f, 7.83f, 0.17f, 5.0f, 0.12f, 52.0f), 20},
    {"heal_528", "528 Hz — Soft Love",
     "Quiet 528 Hz with feather-light hum.", "heal",
     make(528.0f, 10.0f, 0.16f, 5.0f, 0.10f, 55.0f), 20},
    {"heal_639", "639 Hz — Soft Connection",
     "Soft connection field.", "heal",
     make(639.0f, 10.0f, 0.15f, 5.0f, 0.08f, 55.0f), 20},
    {"heal_741", "741 Hz — Soft Clarity",
     "Light clarity tone.", "heal",
     make(741.0f, 12.0f, 0.15f, 4.0f, 0.06f, 58.0f), 15},
    {"heal_852", "852 Hz — Soft Intuition",
     "Quiet intuition field.", "heal",
     make(852.0f, 8.0f, 0.14f, 5.0f, 0.06f, 55.0f), 20},
    {"heal_963", "963 Hz — Soft Unity",
     "Very soft high unity tone.", "heal",
     make(963.0f, 6.0f, 0.14f, 5.0f, 0.08f, 52.0f), 20},

    // --- Focus ladder (soft) ---
    {"hemisync_focus10", "Focus 10 — Soft Theta",
     "Inspired by Focus 10. Quiet coherent theta.", "hemisync",
     make(180.0f, 6.0f, 0.17f, 6.0f, 0.15f, 50.0f), 25},
    {"hemisync_focus12", "Focus 12 — Soft Expand",
     "Inspired by Focus 12. Soft 7 Hz field.", "hemisync",
     make(180.0f, 7.0f, 0.17f, 6.0f, 0.14f, 50.0f), 25},
    {"hemisync_focus15", "Focus 15 — Soft Deep",
     "Inspired by Focus 15. Deep quiet theta.", "hemisync",
     make(165.0f, 4.0f, 0.15f, 7.0f, 0.18f, 48.0f), 30},
    {"hemisync_focus21", "Focus 21 — Soft Bridge",
     "Inspired by Focus 21. Soft bridge band.", "hemisync",
     make(165.0f, 4.5f, 0.15f, 7.0f, 0.16f, 48.0f), 30},
    {"hemisync_focus27", "Focus 27 — Soft Journey",
     "Inspired by Focus 27. Deep quiet journey.", "hemisync",
     make(160.0f, 3.5f, 0.14f, 8.0f, 0.18f, 48.0f), 35},
    {"gateway_ascent", "Gateway — Soft Ascent",
     "Gentle progressive theta path.", "hemisync",
     make(175.0f, 5.5f, 0.15f, 7.0f, 0.15f, 50.0f), 30},

    {"isochronic_soft", "Isochronic Soft Alpha",
     "Soft-gated isochronic (speakers OK).", "explore",
     make(200.0f, 10.0f, 0.16f, 4.0f, 0.12f, 52.0f, BeatMode::Isochronic), 15},
    {"free_play", "Free Play — Soft Start",
     "Clean start. Keep Soft sub hum low for purity.", "explore",
     make(196.0f, 7.83f, 0.18f, 3.0f, 0.12f, 52.0f), 0},
};

}  // namespace

const TonePreset* allPresets(size_t& count)
{
    count = sizeof(kPresets) / sizeof(kPresets[0]);
    return kPresets;
}

size_t presetCount()
{
    return sizeof(kPresets) / sizeof(kPresets[0]);
}

const TonePreset* presetByIndex(size_t index)
{
    if (index >= presetCount()) {
        return nullptr;
    }
    return &kPresets[index];
}

const TonePreset* presetById(const char* id)
{
    if (id == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < presetCount(); ++i) {
        if (std::strcmp(kPresets[i].id, id) == 0) {
            return &kPresets[i];
        }
    }
    return nullptr;
}

}  // namespace manticore::toneflow
