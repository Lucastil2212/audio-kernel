#pragma once

#include <cstdint>

namespace manticore::toneflow {

enum class BeatMode {
    Binaural,
    Monaural,
    Isochronic
};

enum class PerceptionMode {
    Standard,
    Rotate,
    Alternate,
    Shimmer,
    Breath,
    Expand,   // gentle widening + slow orbit
    Lattice,  // multi-rate amplitude lattice (Hemi-Sync inspired)
    Void      // deep attenuation swells
};

enum class ReverbPreset {
    Off,
    Room,
    Chamber,
    Cave
};

enum class NoiseMask {
    None,
    Pink,
    White
};

enum class FilterType {
    Off,
    Warm,      // low-pass velvet
    Presence,  // soft presence peak
    SoftBand,  // bandpass around carrier
    Crystal,   // airy high-pass
    DeepWell,  // strong low-pass for deep states
    NotchHum   // notch 60-ish + warm LP
};

struct ToneParams {
    float carrierHz = 210.0f;
    float beatHz = 10.0f;
    BeatMode mode = BeatMode::Binaural;
    PerceptionMode perception = PerceptionMode::Standard;
    ReverbPreset reverb = ReverbPreset::Off;
    NoiseMask noiseMask = NoiseMask::None;
    FilterType filter = FilterType::Off;
    float noiseLevelDbfs = -30.0f;
    float masterVolume = 0.14f;
    int harmonicLayers = 1;
    float kernelNoiseBlend = 0.0f;
    float fadeInSec = 14.0f;
    float comfortModDepth = 0.0f;
    float filterMix = 0.0f;  // 0 dry .. 1 fully filtered
    /** Soft continuous sub hum (pure sine, mono). 0..1 — keep modest to avoid distortion. */
    float subLevel = 0.22f;
    /** Sub hum Hz. Prefer 48–60 on phones; deeper needs a real woofer. */
    float subHz = 52.0f;
};

struct TonePreset {
    const char* id;
    const char* label;
    const char* description;
    const char* category;
    ToneParams params;
    int durationMin;
};

inline float clamp(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float equalLoudnessGain(float carrierHz)
{
    const float c = clamp(carrierHz, 100.0f, 1000.0f);
    if (c < 250.0f) {
        return 1.0f + (250.0f - c) / 250.0f * 0.18f;
    }
    if (c > 400.0f) {
        return 1.0f - (c - 400.0f) / 600.0f * 0.12f;
    }
    return 1.0f;
}

inline const char* perceptionLabel(PerceptionMode mode)
{
    switch (mode) {
        case PerceptionMode::Rotate:
            return "Rotating field";
        case PerceptionMode::Alternate:
            return "Ear alternate";
        case PerceptionMode::Shimmer:
            return "Shimmer";
        case PerceptionMode::Breath:
            return "Breath sync";
        case PerceptionMode::Expand:
            return "Expand";
        case PerceptionMode::Lattice:
            return "Consciousness lattice";
        case PerceptionMode::Void:
            return "Void swell";
        default:
            return "Standard";
    }
}

inline const char* beatModeLabel(BeatMode mode)
{
    switch (mode) {
        case BeatMode::Monaural:
            return "Monaural";
        case BeatMode::Isochronic:
            return "Isochronic";
        default:
            return "Binaural";
    }
}

inline const char* reverbLabel(ReverbPreset preset)
{
    switch (preset) {
        case ReverbPreset::Room:
            return "Room";
        case ReverbPreset::Chamber:
            return "Chamber";
        case ReverbPreset::Cave:
            return "Cave";
        default:
            return "Off";
    }
}

inline const char* filterLabel(FilterType type)
{
    switch (type) {
        case FilterType::Warm:
            return "Warm";
        case FilterType::Presence:
            return "Presence";
        case FilterType::SoftBand:
            return "Soft band";
        case FilterType::Crystal:
            return "Crystal";
        case FilterType::DeepWell:
            return "Deep well";
        case FilterType::NotchHum:
            return "Notch hum";
        default:
            return "Off";
    }
}

inline PerceptionMode cyclePerception(PerceptionMode mode)
{
    const int n = 8;
    return static_cast<PerceptionMode>((static_cast<int>(mode) + 1) % n);
}

inline BeatMode cycleBeatMode(BeatMode mode)
{
    const int n = 3;
    return static_cast<BeatMode>((static_cast<int>(mode) + 1) % n);
}

inline ReverbPreset cycleReverb(ReverbPreset preset)
{
    const int n = 4;
    return static_cast<ReverbPreset>((static_cast<int>(preset) + 1) % n);
}

inline FilterType cycleFilter(FilterType type)
{
    const int n = 7;
    return static_cast<FilterType>((static_cast<int>(type) + 1) % n);
}

}  // namespace manticore::toneflow
