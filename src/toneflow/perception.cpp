#include "manticore/toneflow/perception.h"

#include <cmath>

namespace manticore::toneflow {
namespace {
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr float kPi = 3.14159265358979323846f;
}

void applyPerception(AudioFrame& stereo, PerceptionMode mode, float beatHz,
                     PerceptionState& state)
{
    if (!stereo.isStereo() || mode == PerceptionMode::Standard ||
        stereo.sampleRateHz == 0) {
        return;
    }

    const size_t n = stereo.framesPerChannel();
    const float sr = static_cast<float>(stereo.sampleRateHz);
    const float dt = 1.0f / sr;

    if (mode == PerceptionMode::Rotate) {
        const float rotHz = clamp(beatHz * 0.008f, 0.03f, 0.10f);
        float phase = state.rotationPhase;
        for (size_t i = 0; i < n; ++i) {
            const float pan = std::sin(phase);
            const float l = stereo.get(i, 0);
            const float r = stereo.get(i, 1);
            stereo.set(i, 0, l * (0.94f + 0.06f * pan) + r * (0.02f * (1.0f - pan)));
            stereo.set(i, 1, r * (0.94f - 0.06f * pan) + l * (0.02f * (1.0f + pan)));
            phase += kTwoPi * rotHz * dt;
        }
        while (phase >= kTwoPi) {
            phase -= kTwoPi;
        }
        state.rotationPhase = phase;
        return;
    }

    if (mode == PerceptionMode::Alternate) {
        const float b = std::max(beatHz, 0.5f);
        float phase = state.altPhase;
        for (size_t i = 0; i < n; ++i) {
            const float gate = 0.5f - 0.5f * std::cos(phase);
            const float lGate = 0.25f + 0.75f * gate;
            const float rGate = 0.25f + 0.75f * (1.0f - gate);
            const float l = stereo.get(i, 0);
            const float r = stereo.get(i, 1);
            const float mono = 0.5f * (l + r);
            stereo.set(i, 0, l * 0.55f * lGate + mono * 0.45f * lGate);
            stereo.set(i, 1, r * 0.55f * rGate + mono * 0.45f * rGate);
            phase += kTwoPi * b * dt;
        }
        while (phase >= kTwoPi) {
            phase -= kTwoPi;
        }
        state.altPhase = phase;
        return;
    }

    if (mode == PerceptionMode::Shimmer) {
        const float b = std::max(beatHz, 0.5f);
        float phase = state.shimmerPhase;
        for (size_t i = 0; i < n; ++i) {
            const float am = 0.90f + 0.10f * std::sin(phase);
            stereo.set(i, 0, stereo.get(i, 0) * am);
            stereo.set(i, 1, stereo.get(i, 1) * am);
            phase += kPi * b * dt;
        }
        state.shimmerPhase = phase;
        return;
    }

    if (mode == PerceptionMode::Breath) {
        constexpr float breathHz = 0.0833f;
        float phase = state.breathPhase;
        for (size_t i = 0; i < n; ++i) {
            const float swell =
                0.72f + 0.28f * (0.5f - 0.5f * std::cos(phase));
            stereo.set(i, 0, stereo.get(i, 0) * swell);
            stereo.set(i, 1, stereo.get(i, 1) * swell);
            phase += kTwoPi * breathHz * dt;
        }
        while (phase >= kTwoPi) {
            phase -= kTwoPi;
        }
        state.breathPhase = phase;
        return;
    }

    if (mode == PerceptionMode::Expand) {
        // Slow width pulse + faint orbit — "opening" spatial feel.
        float phase = state.expandPhase;
        const float expandHz = clamp(beatHz * 0.015f, 0.04f, 0.12f);
        for (size_t i = 0; i < n; ++i) {
            const float open = 0.5f - 0.5f * std::cos(phase);
            const float width = 0.85f + 0.25f * open;
            const float orbit = std::sin(phase * 0.5f) * 0.08f;
            float l = stereo.get(i, 0);
            float r = stereo.get(i, 1);
            const float mid = 0.5f * (l + r);
            const float side = 0.5f * (l - r) * width;
            l = mid + side;
            r = mid - side;
            stereo.set(i, 0, l * (1.0f + orbit));
            stereo.set(i, 1, r * (1.0f - orbit));
            phase += kTwoPi * expandHz * dt;
        }
        while (phase >= kTwoPi) {
            phase -= kTwoPi;
        }
        state.expandPhase = phase;
        return;
    }

    if (mode == PerceptionMode::Lattice) {
        // Dual-rate amplitude lattice inspired by multi-layer Hemi-Sync beds.
        const float b = std::max(beatHz, 0.5f);
        float pa = state.latticePhaseA;
        float pb = state.latticePhaseB;
        for (size_t i = 0; i < n; ++i) {
            const float a = 0.88f + 0.12f * std::sin(pa);
            const float c = 0.92f + 0.08f * std::sin(pb);
            stereo.set(i, 0, stereo.get(i, 0) * a * c);
            stereo.set(i, 1, stereo.get(i, 1) * a * (1.04f - (c - 0.92f)));
            pa += kTwoPi * b * dt;
            pb += kTwoPi * (b * 0.5f) * dt;
        }
        while (pa >= kTwoPi) {
            pa -= kTwoPi;
        }
        while (pb >= kTwoPi) {
            pb -= kTwoPi;
        }
        state.latticePhaseA = pa;
        state.latticePhaseB = pb;
        return;
    }

    if (mode == PerceptionMode::Void) {
        // Deep, slow swells — sparse peaks for "void" journeys.
        float phase = state.voidPhase;
        constexpr float voidHz = 0.05f;
        for (size_t i = 0; i < n; ++i) {
            const float swell = 0.35f + 0.65f * std::pow(0.5f - 0.5f * std::cos(phase), 2.0f);
            stereo.set(i, 0, stereo.get(i, 0) * swell);
            stereo.set(i, 1, stereo.get(i, 1) * swell);
            phase += kTwoPi * voidHz * dt;
        }
        while (phase >= kTwoPi) {
            phase -= kTwoPi;
        }
        state.voidPhase = phase;
    }
}

}  // namespace manticore::toneflow
