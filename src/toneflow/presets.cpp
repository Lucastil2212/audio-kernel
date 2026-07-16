#include "manticore/toneflow/presets.h"

#include <cstring>

namespace manticore::toneflow {
namespace {

ToneParams make(float carrier, float beat, BeatMode mode = BeatMode::Binaural,
                PerceptionMode perception = PerceptionMode::Standard,
                ReverbPreset reverb = ReverbPreset::Off, int layers = 1,
                float noiseBlend = 0.10f, float volume = 0.12f,
                float fadeIn = 12.0f, FilterType filter = FilterType::Off)
{
    ToneParams p;
    p.carrierHz = carrier;
    p.beatHz = beat;
    p.mode = mode;
    p.perception = perception;
    p.reverb = reverb;
    p.harmonicLayers = layers;
    p.kernelNoiseBlend = noiseBlend;
    p.masterVolume = volume;
    p.fadeInSec = fadeIn;
    p.comfortModDepth = 0.04f;
    p.filter = filter;
    p.filterMix = filter == FilterType::Off ? 0.0f : 0.85f;
    p.noiseMask = NoiseMask::None;
    p.noiseLevelDbfs = -30.0f;
    return p;
}

/**
 * Inspired by Monroe Institute Focus / Gateway naming — not Hemi-Sync®.
 * Frequencies are exploratory approximations for personal perceptual practice.
 */
const TonePreset kPresets[] = {
    // --- Core bands ---
    {"alpha_calm", "Alpha — Calm (10 Hz)",
     "Relaxed alertness; warm carrier, soft band focus.", "calm",
     make(210.0f, 10.0f, BeatMode::Binaural, PerceptionMode::Standard,
          ReverbPreset::Off, 1, 0.08f, 0.11f, 14.0f, FilterType::SoftBand),
     15},
    {"theta_meditate", "Theta — Meditate (6 Hz)",
     "Quieted attention; deep-well filter, long fade.", "deep",
     make(185.0f, 6.0f, BeatMode::Binaural, PerceptionMode::Standard,
          ReverbPreset::Off, 1, 0.12f, 0.11f, 16.0f, FilterType::DeepWell),
     20},
    {"delta_sleep", "Delta — Deep Rest (2.5 Hz)",
     "Slow-wave band with breath pacing and velvet warmth.", "deep",
     make(160.0f, 2.5f, BeatMode::Binaural, PerceptionMode::Breath,
          ReverbPreset::Room, 1, 0.16f, 0.09f, 20.0f, FilterType::Warm),
     30},
    {"beta_deepwork", "Beta — Deep Work (16 Hz)",
     "Alert focus; presence lift, minimal bed.", "focus",
     make(230.0f, 16.0f, BeatMode::Binaural, PerceptionMode::Standard,
          ReverbPreset::Off, 1, 0.05f, 0.12f, 8.0f, FilterType::Presence),
     15},
    {"gamma_focus", "Gamma — Peak Focus (40 Hz)",
     "40 Hz isochronic pulses with crystal air.", "focus",
     make(220.0f, 40.0f, BeatMode::Isochronic, PerceptionMode::Standard,
          ReverbPreset::Off, 1, 0.04f, 0.10f, 8.0f, FilterType::Crystal),
     10},
    {"schumann", "Schumann (7.83 Hz)",
     "Earth resonance beat; soft shimmer + warm room.", "explore",
     make(200.0f, 7.83f, BeatMode::Binaural, PerceptionMode::Shimmer,
          ReverbPreset::Room, 1, 0.10f, 0.11f, 14.0f, FilterType::Warm),
     20},

    // --- Hemi-Sync inspired Focus ladder ---
    {"hemisync_focus10", "Focus 10 — Mind Awake / Body Asleep",
     "Inspired by Focus 10 (not Hemi-Sync®). Theta + harmonic bed.", "hemisync",
     make(185.0f, 6.0f, BeatMode::Binaural, PerceptionMode::Standard,
          ReverbPreset::Off, 2, 0.11f, 0.11f, 16.0f, FilterType::Warm),
     20},
    {"hemisync_focus12", "Focus 12 — Expanded Awareness",
     "Inspired by Focus 12. High-theta with expand perception.", "hemisync",
     make(185.0f, 7.0f, BeatMode::Binaural, PerceptionMode::Expand,
          ReverbPreset::Off, 2, 0.11f, 0.11f, 16.0f, FilterType::SoftBand),
     25},
    {"hemisync_focus15", "Focus 15 — No Time",
     "Inspired by Focus 15. Deep theta lattice; sparse time feel.", "hemisync",
     make(170.0f, 4.0f, BeatMode::Binaural, PerceptionMode::Lattice,
          ReverbPreset::Chamber, 3, 0.14f, 0.10f, 18.0f, FilterType::DeepWell),
     30},
    {"hemisync_focus21", "Focus 21 — Bridge",
     "Inspired by Focus 21. Deep theta shimmer into restrained cave.", "hemisync",
     make(170.0f, 4.5f, BeatMode::Binaural, PerceptionMode::Shimmer,
          ReverbPreset::Cave, 2, 0.14f, 0.10f, 18.0f, FilterType::Warm),
     30},
    {"hemisync_focus27", "Focus 27 — Other Energy",
     "Inspired by Focus 27. Lattice + void swell; experimental journey.",
     "hemisync",
     make(165.0f, 3.5f, BeatMode::Binaural, PerceptionMode::Void, ReverbPreset::Cave,
          3, 0.16f, 0.09f, 22.0f, FilterType::DeepWell),
     35},
    {"gateway_ascent", "Gateway Ascent",
     "Progressive theta path with consciousness lattice (inspired, not official).",
     "hemisync",
     make(180.0f, 5.5f, BeatMode::Binaural, PerceptionMode::Lattice,
          ReverbPreset::Chamber, 3, 0.13f, 0.10f, 20.0f, FilterType::SoftBand),
     30},
    {"hemi_bridge", "Hemispheric Bridge",
     "Soft ear alternate with 3-layer harmonic bed — bridge L/R attention.",
     "hemisync",
     make(200.0f, 8.0f, BeatMode::Binaural, PerceptionMode::Alternate,
          ReverbPreset::Room, 3, 0.10f, 0.11f, 14.0f, FilterType::Presence),
     20},

    // --- Higher-consciousness / perceptual ---
    {"higher_self", "Higher Self Field (5 Hz)",
     "Low-theta expand field; warm filter, long ceremonial fade.", "conscious",
     make(175.0f, 5.0f, BeatMode::Binaural, PerceptionMode::Expand,
          ReverbPreset::Chamber, 2, 0.12f, 0.10f, 20.0f, FilterType::Warm),
     25},
    {"unity_pulse", "Unity Pulse (7 Hz)",
     "High-theta lattice for coherent whole-field listening.", "conscious",
     make(190.0f, 7.0f, BeatMode::Binaural, PerceptionMode::Lattice,
          ReverbPreset::Room, 2, 0.10f, 0.11f, 16.0f, FilterType::SoftBand),
     20},
    {"void_journey", "Void Journey (3 Hz)",
     "Deep delta void swells — sparse peaks, deep-well sculpting.", "conscious",
     make(150.0f, 3.0f, BeatMode::Binaural, PerceptionMode::Void, ReverbPreset::Cave,
          1, 0.18f, 0.08f, 24.0f, FilterType::DeepWell),
     30},
    {"crystal_mind", "Crystal Mind (12 Hz)",
     "High-alpha / low-beta crystal air for lucid clarity.", "conscious",
     make(240.0f, 12.0f, BeatMode::Binaural, PerceptionMode::Shimmer,
          ReverbPreset::Off, 1, 0.06f, 0.11f, 10.0f, FilterType::Crystal),
     15},
    {"rotate_theta", "Rotating Theta (6 Hz)",
     "Mid-theta with slow spatial rotation.", "explore",
     make(180.0f, 6.0f, BeatMode::Binaural, PerceptionMode::Rotate,
          ReverbPreset::Off, 1, 0.10f, 0.11f, 14.0f, FilterType::Warm),
     20},
    {"shimmer_meditation", "Shimmer Alpha–Theta (8 Hz)",
     "Border band with ethereal shimmer.", "calm",
     make(195.0f, 8.0f, BeatMode::Binaural, PerceptionMode::Shimmer,
          ReverbPreset::Chamber, 1, 0.10f, 0.11f, 14.0f, FilterType::SoftBand),
     20},
    {"breath_alpha", "Breath Sync Alpha (10 Hz)",
     "Mid-alpha with ~5/min breath swells.", "calm",
     make(210.0f, 10.0f, BeatMode::Binaural, PerceptionMode::Breath,
          ReverbPreset::Off, 1, 0.08f, 0.11f, 14.0f, FilterType::Warm),
     15},
    {"float_cave", "Float — Cave (5 Hz)",
     "Low-theta float in a restrained cave.", "deep",
     make(175.0f, 5.0f, BeatMode::Binaural, PerceptionMode::Breath,
          ReverbPreset::Cave, 1, 0.15f, 0.10f, 18.0f, FilterType::DeepWell),
     25},
    {"isochronic_alpha", "Isochronic Alpha (10 Hz)",
     "Soft-gated isochronic alpha for speakers.", "explore",
     make(210.0f, 10.0f, BeatMode::Isochronic, PerceptionMode::Standard,
          ReverbPreset::Off, 1, 0.06f, 0.11f, 10.0f, FilterType::Presence),
     15},
    {"noise_bath_calm", "Noise Bath — Calm",
     "Pink kernel bed under warm alpha.", "calm",
     make(210.0f, 10.0f, BeatMode::Binaural, PerceptionMode::Breath,
          ReverbPreset::Room, 1, 0.24f, 0.10f, 16.0f, FilterType::Warm),
     15},
    {"free_play", "Free Play — Custom",
     "Start here and sculpt carrier, beat, filter, and perception live.",
     "explore",
     make(200.0f, 7.83f, BeatMode::Binaural, PerceptionMode::Expand,
          ReverbPreset::Room, 2, 0.10f, 0.11f, 8.0f, FilterType::SoftBand),
     0},
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
