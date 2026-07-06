# Manticore Audio Kernel Software Specification

**Project:** Manticore Audio Kernel  
**Repository name:** `manticore-audio-kernel`  
**Primary language:** C++17 initially; C-compatible DSP core discipline where possible  
**Primary target:** Desktop Prototype 0 bench rig  
**Future targets:** embedded DSP/MCU, dual-ear wearable prototype, Relief Mode v0 acoustic renderer  
**Document purpose:** This specification defines the software architecture, directory structure, runtime objects, APIs, DSP modules, safety behavior, test strategy, and implementation sequence needed to build the Manticore Audio Kernel from scratch.

---

## 1. Product and Engineering Thesis

The Manticore Audio Kernel is the foundational software layer for a software-defined auditory interface. It is not a consumer app, not an AI product, and not a tinnitus therapy by itself. It is the deterministic audio engine that all later perception, hearing, therapy, and adaptation features must sit on top of.

The governing rule is:

> Render the world back into the ear safely, believably, and on time before adding intelligence.

For Prototype 0, the kernel must prove that Manticore can:

1. represent audio cleanly in C++;
2. generate and transform audio frames;
3. process audio through deterministic DSP modules;
4. write audio to disk for offline verification;
5. run a live callback-based audio path on a laptop;
6. measure latency, headroom, clipping, CPU load, and buffer health;
7. expose stable data structures that can later map to firmware, app, telemetry, and Relief Mode.

---

## 2. Scope

### 2.1 In Scope for Prototype 0

Prototype 0 is the desktop audio kernel bench rig.

It includes:

- C++ project setup.
- `AudioFrame` representation.
- Mono and stereo frame support.
- Offline sine/noise generation.
- WAV writer.
- Gain.
- Hard limiter.
- Frame analyzer.
- Basic filters.
- Delay line.
- Simple mixer/compositor.
- Therapy stimulus engine stubs.
- Cue health metrics stubs.
- Safety events.
- Runtime configuration.
- PortAudio/RtAudio desktop backend.
- Live mic passthrough.
- Round-trip latency measurement tools.
- Unit tests and golden WAV regression tests.

### 2.2 Out of Scope for Prototype 0

Prototype 0 must not attempt:

- hearing-aid fitting;
- medical claims;
- AI speech separation;
- ear-EEG;
- Bluetooth LE Audio;
- embedded firmware flashing;
- real clinical tinnitus therapy;
- final hardware design;
- cloud services;
- mobile app implementation.

Stub the interfaces now, but do not implement those features yet.

---

## 3. Architectural Doctrine

Manticore uses three planes.

### 3.1 Plane A: Deterministic Audio Kernel

Plane A owns continuous audio rendering. It must not wait on AI inference, network calls, cloud services, or slow policy logic.

Prototype 0 implements the beginning of Plane A.

Plane A responsibilities:

- audio input/output;
- sample framing;
- gain;
- filtering;
- limiting;
- delay;
- mixing;
- safe composition;
- frame analysis;
- latency instrumentation;
- future beamforming, feedback suppression, compression, and cue preservation.

### 3.2 Plane B: Policy and Session Layer

Plane B proposes settings. It never directly renders samples.

Prototype 0 includes only simple local configuration objects and stubs.

Plane B future responsibilities:

- scene mode selection;
- therapy session state;
- personalization;
- parameter envelopes;
- user profile logic;
- companion app policy.

### 3.3 Plane C: Sensing and Telemetry Layer

Plane C measures state. It does not own rendering.

Prototype 0 includes only simulated telemetry and local health metrics.

Plane C future responsibilities:

- fit state;
- leak state;
- own-voice confidence;
- sync confidence;
- IMU state;
- system health;
- optional biosignal quality.

---

## 4. Prototype 0 Success Criteria

Prototype 0 is complete when the codebase can do all of the following:

1. Build with CMake on Linux.
2. Generate a 440 Hz sine wave into an `AudioFrame`.
3. Analyze frame min, max, peak, RMS, clipping, and DC offset.
4. Apply gain and limiting safely.
5. Write valid 16-bit PCM WAV files.
6. Generate shaped noise and ambient procedural beds offline.
7. Process frames through a configurable DSP graph.
8. Run live mic passthrough through the same DSP graph.
9. Log callback time, buffer size, sample rate, overrun/underrun count, and peak level.
10. Measure approximate round-trip latency using click/impulse testing.
11. Demonstrate that all DSP modules work at frame sizes 16, 32, and 64 samples at 48 kHz.
12. Run automated tests without external hardware except for live audio tests.

---

## 5. Global Technical Defaults

| Parameter | Prototype 0 Default | Rationale |
|---|---:|---|
| Sample rate | 48000 Hz | Standard for audio devices and low-latency work. |
| Frame sizes | 16, 32, 64 samples | Matches low-latency critical-path discipline. |
| Default frame size | 64 samples | Easier first target; 1.333 ms at 48 kHz. |
| Internal sample type | `float` | Simple DSP, normalized range. |
| Normalized range | -1.0 to +1.0 | Standard float audio convention. |
| Offline WAV format | 16-bit PCM | Simple and widely playable. |
| Live backend | PortAudio first | Portable desktop callback API. |
| Build system | CMake | Portable, common in C++/embedded-adjacent projects. |
| C++ standard | C++17 | Modern enough, not excessive. |
| Dynamic allocation in callback | Forbidden after initialization | Required for future real-time discipline. |

---

## 6. Repository Layout

```text
manticore-audio-kernel/
  CMakeLists.txt
  README.md
  docs/
    architecture.md
    latency_measurement.md
    dsp_notes.md
    safety_model.md
  include/
    manticore/
      core/
        audio_frame.h
        audio_types.h
        constants.h
        errors.h
        time.h
      dsp/
        gain.h
        limiter.h
        analyzer.h
        oscillator.h
        noise.h
        filters.h
        delay_line.h
        mixer.h
        envelope.h
        wav_writer.h
      engine/
        audio_engine.h
        dsp_graph.h
        processor.h
        runtime_config.h
      safety/
        cue_health.h
        safety_event.h
        safety_governor.h
      therapy/
        therapy_param_set.h
        stimulus_engine.h
        carrier_family.h
      telemetry/
        fit_state.h
        system_health.h
        outcome_packet.h
      io/
        audio_backend.h
        portaudio_backend.h
        wav_reader.h
  src/
    main.cpp
    dsp/
      filters.cpp
      wav_writer.cpp
    engine/
      audio_engine.cpp
      dsp_graph.cpp
    io/
      portaudio_backend.cpp
    therapy/
      stimulus_engine.cpp
    safety/
      safety_governor.cpp
  tools/
    generate_sine.cpp
    generate_noise.cpp
    live_passthrough.cpp
    latency_click_test.cpp
    inspect_wav.cpp
  tests/
    test_audio_frame.cpp
    test_gain.cpp
    test_limiter.cpp
    test_analyzer.cpp
    test_oscillator.cpp
    test_wav_writer.cpp
    test_filters.cpp
    test_delay_line.cpp
    test_safety_governor.cpp
  assets/
    golden/
      sine_440_1s.wav
      limited_test.wav
  build/
```

Early learning versions may keep headers under `src/`, but the long-term implementation should move public headers into `include/manticore/...`.

---

## 7. Naming Conventions

- Types: `PascalCase`
- Functions: `camelCase`
- Constants: `kCamelCase` or `UPPER_SNAKE_CASE` for compile-time numeric constants
- Files: `snake_case.h`, `snake_case.cpp`
- Namespaces: all public code lives under `manticore`

Example:

```cpp
namespace manticore {
struct AudioFrame {};
void applyGain(AudioFrame& frame, float gainLinear);
}
```

---

## 8. Core Data Model

### 8.1 AudioSample

For Prototype 0:

```cpp
using Sample = float;
```

Rules:

- Normal operating range is `[-1.0f, +1.0f]`.
- Values outside this range are allowed internally before limiting, but must be analyzed and clamped before output when required.
- NaN and infinity are invalid and must be detected in analyzers and safety checks.

### 8.2 Channel Layout

```cpp
enum class ChannelLayout {
    Mono,
    StereoInterleaved,
    BinauralInterleaved
};
```

Prototype 0 begins with mono, then stereo interleaved.

Interleaved stereo layout:

```text
L0, R0, L1, R1, L2, R2, ...
```

### 8.3 AudioFrame

`AudioFrame` is the core unit that moves through the kernel.

Required fields:

```cpp
struct AudioFrame {
    std::vector<float> samples;
    uint32_t sampleRateHz = 48000;
    uint32_t channels = 1;
    uint64_t frameIndex = 0;
    uint64_t timestampNs = 0;
    bool clipFlag = false;
    bool overflowFlag = false;
    bool underflowFlag = false;
};
```

Required methods:

```cpp
size_t sampleCount() const;
size_t framesPerChannel() const;
float durationMs() const;
bool isMono() const;
bool isStereo() const;
float get(size_t frame, size_t channel) const;
void set(size_t frame, size_t channel, float value);
void clear();
bool hasInvalidSamples() const;
```

Constructor requirements:

```cpp
AudioFrame();
AudioFrame(uint32_t framesPerChannel, uint32_t sampleRateHz, uint32_t channels);
```

Validation rules:

- `sampleRateHz > 0`
- `channels >= 1`
- `samples.size() % channels == 0`
- `durationMs()` must be computed as:

```text
framesPerChannel / sampleRateHz * 1000
```

### 8.4 FrameStats

```cpp
struct FrameStats {
    float minSample = 0.0f;
    float maxSample = 0.0f;
    float peakAbs = 0.0f;
    float rms = 0.0f;
    float dcOffset = 0.0f;
    bool clipped = false;
    bool hasNaN = false;
    bool hasInf = false;
};
```

Analyzer function:

```cpp
FrameStats analyzeFrame(const AudioFrame& frame);
```

Required behavior:

- Empty frame returns zero stats.
- Clipped is true if any sample is `>= +1.0f` or `<= -1.0f`.
- RMS is sqrt(mean(sample^2)).
- DC offset is arithmetic mean.

### 8.5 RuntimeConfig

```cpp
struct RuntimeConfig {
    uint32_t sampleRateHz = 48000;
    uint32_t frameSize = 64;
    uint32_t channels = 1;
    bool enableLimiter = true;
    bool enableTelemetry = true;
    bool enableTherapy = false;
    float inputGain = 1.0f;
    float outputGain = 1.0f;
};
```

Rules:

- Therapy disabled by default.
- Limiter enabled by default.
- Invalid config must fail before audio starts.

---

## 9. Safety and Health Data Structures

### 9.1 CueHealth

Prototype 0 implements partial CueHealth.

```cpp
enum class CueState {
    Normal,
    SoftBackoff,
    SafeProfile,
    TherapyOff,
    BaselineOnly
};

struct CueHealth {
    CueState state = CueState::BaselineOnly;
    float itdErrorUs = 0.0f;
    float ildErrorDb = 0.0f;
    float coherenceIndex = 1.0f;
    float colorationIndex = 0.0f;
    float headroomDb = 0.0f;
    float syncConfidence = 1.0f;
    std::string stopReason;
};
```

Prototype 0 metrics:

- headroom dB;
- clipped flag;
- invalid sample flag;
- callback overrun/underrun state;
- future placeholders for ITD/ILD/coherence.

### 9.2 SafetyEvent

```cpp
enum class SafetySeverity {
    Info,
    Warning,
    Error,
    Critical
};

struct SafetyEvent {
    uint64_t timestampNs = 0;
    SafetySeverity severity = SafetySeverity::Info;
    std::string subsystem;
    std::string eventCode;
    std::string message;
    CueState fallbackApplied = CueState::Normal;
};
```

Required event codes:

```text
AUDIO_CLIP_DETECTED
AUDIO_NAN_DETECTED
AUDIO_INF_DETECTED
CALLBACK_OVERRUN
CALLBACK_UNDERRUN
PARAM_STALE
PARAM_OUT_OF_RANGE
THERAPY_DISABLED_BY_GUARD
LIMITER_ENGAGED
CONFIG_INVALID
```

### 9.3 FitState Stub

```cpp
enum class MotionState {
    Unknown,
    Still,
    Moving,
    Speaking,
    HighMotion
};

struct FitState {
    float fitConfidence = 1.0f;
    float leakIndex = 0.0f;
    float ownVoiceConfidence = 0.0f;
    float referenceMicHealth = 1.0f;
    MotionState motionState = MotionState::Unknown;
    uint64_t calibrationAgeMs = 0;
};
```

Prototype 0 uses simulated defaults only.

---

## 10. Therapy Parameter Model

Prototype 0 does not claim therapy efficacy. It builds the parameter interface that future Relief Mode can use.

### 10.1 CarrierFamily

```cpp
enum class CarrierFamily {
    None,
    Sine,
    WhiteNoise,
    PinkNoise,
    ShapedNoise,
    AmbientBed
};
```

### 10.2 SessionStage

```cpp
enum class SessionStage {
    Disabled,
    RampIn,
    Active,
    RampOut,
    Complete
};
```

### 10.3 TherapyParamSet

```cpp
struct TherapyParamSet {
    uint32_t schemaVersion = 1;
    uint32_t profileId = 0;
    CarrierFamily carrierFamily = CarrierFamily::None;
    float bandLowHz = 200.0f;
    float bandHighHz = 8000.0f;
    float blendGain = 0.0f;
    float envelopeRateHz = 0.05f;
    float envelopeDepth = 0.0f;
    float itdUs = 0.0f;
    float ildDb = 0.0f;
    uint32_t phaseRule = 0;
    SessionStage stage = SessionStage::Disabled;
    uint64_t validUntilTimestampNs = 0;
};
```

Validation rules:

- `bandLowHz >= 20.0f`
- `bandHighHz <= sampleRateHz / 2`
- `bandLowHz < bandHighHz`
- `blendGain >= 0.0f && blendGain <= 1.0f`
- `envelopeDepth >= 0.0f && envelopeDepth <= 1.0f`
- default `itdUs = 0.0f` and `ildDb = 0.0f`
- stale parameter sets are rejected

---

## 11. DSP Module Specifications

All DSP modules must be deterministic and allocation-free after initialization.

### 11.1 Gain

Header:

```cpp
void applyGain(AudioFrame& frame, float gainLinear);
```

Behavior:

```text
sample = sample * gainLinear
```

Rules:

- Gain may be greater than 1.0 internally.
- Limiter must protect output if enabled.
- Negative gain is allowed for inversion but should not be used in default configs.

### 11.2 Hard Limiter

Header:

```cpp
float clampSample(float sample, float minValue = -1.0f, float maxValue = 1.0f);
void applyLimiter(AudioFrame& frame, float threshold = 1.0f);
```

Behavior:

- If sample exceeds threshold, clamp to threshold.
- If sample is below negative threshold, clamp to negative threshold.
- Set frame clip flag if limiting occurs.

### 11.3 Analyzer

Header:

```cpp
FrameStats analyzeFrame(const AudioFrame& frame);
```

Must compute:

- min;
- max;
- peak absolute value;
- RMS;
- DC offset;
- clipped;
- NaN;
- infinity.

### 11.4 SineOscillator

```cpp
struct SineOscillator {
    float frequencyHz = 440.0f;
    float amplitude = 0.25f;
    float phaseRadians = 0.0f;
    void render(AudioFrame& frame);
};
```

Formula:

```text
sample[n] = amplitude * sin(phase)
phase += 2π * frequencyHz / sampleRateHz
```

Rules:

- Phase must wrap at `2π`.
- Frequency must be below Nyquist.
- Amplitude should default to safe headroom.

### 11.5 WhiteNoiseGenerator

```cpp
struct WhiteNoiseGenerator {
    uint32_t seed = 1;
    float amplitude = 0.1f;
    void render(AudioFrame& frame);
};
```

Rules:

- Must be deterministic when seeded.
- Output range before amplitude is approximately `[-1.0, +1.0]`.
- Use fixed seed vectors for regression tests.

### 11.6 OnePoleLowPass

```cpp
class OnePoleLowPass {
public:
    void setCutoff(float cutoffHz, float sampleRateHz);
    void reset();
    float processSample(float x);
    void processFrame(AudioFrame& frame);
private:
    float alpha_ = 0.0f;
    float z1_ = 0.0f;
};
```

Suggested formula:

```text
alpha = dt / (RC + dt)
RC = 1 / (2πfc)
dt = 1 / fs
y[n] = y[n-1] + alpha * (x[n] - y[n-1])
```

Rules:

- cutoff must be > 0 and < Nyquist.
- state must be resettable.

### 11.7 OnePoleHighPass

```cpp
class OnePoleHighPass {
public:
    void setCutoff(float cutoffHz, float sampleRateHz);
    void reset();
    float processSample(float x);
    void processFrame(AudioFrame& frame);
private:
    float alpha_ = 0.0f;
    float previousInput_ = 0.0f;
    float previousOutput_ = 0.0f;
};
```

Suggested formula:

```text
y[n] = alpha * (y[n-1] + x[n] - x[n-1])
```

### 11.8 DelayLine

```cpp
class DelayLine {
public:
    void prepare(size_t maxDelaySamples);
    void setDelaySamples(size_t delaySamples);
    void reset();
    float processSample(float x);
    void processFrame(AudioFrame& frame);
private:
    std::vector<float> buffer_;
    size_t writeIndex_ = 0;
    size_t delaySamples_ = 0;
};
```

Rules:

- Allocate only in `prepare()`.
- No allocation in `processSample()` or `processFrame()`.
- Used for echo tests, latency demonstrations, future ITD simulation.

### 11.9 Mixer

```cpp
void mixFrames(const AudioFrame& a, const AudioFrame& b, AudioFrame& out, float gainA, float gainB);
```

Rules:

- All frames must have matching sample rate, channel count, and sample count.
- Output may exceed range before limiter.

### 11.10 Envelope

```cpp
class SineEnvelope {
public:
    void set(float rateHz, float depth, float sampleRateHz);
    float next();
    void reset();
};
```

Behavior:

```text
envelope = 1.0 - depth + depth * 0.5 * (1 + sin(phase))
```

Rules:

- depth in `[0.0, 1.0]`
- rate usually slow for therapy-like beds

---

## 12. WAV I/O Specification

### 12.1 WAV Writer

Function:

```cpp
bool writeWavFile(
    const std::string& filename,
    const std::vector<float>& samples,
    uint32_t sampleRateHz,
    uint16_t channels);
```

Required format:

- RIFF/WAVE
- PCM integer format
- 16-bit samples
- little endian
- interleaved channels

Conversion:

```text
float [-1.0, +1.0] -> int16 [-32767, +32767]
```

Rules:

- Clamp before conversion.
- Return false on file open/write failure.
- Unit test must verify expected file size and header fields.

### 12.2 WAV Reader

Optional after writer.

Function:

```cpp
bool readWavFile(const std::string& filename, std::vector<float>& samples, uint32_t& sampleRateHz, uint16_t& channels);
```

Prototype 0 may defer reader until live audio works.

---

## 13. Audio Engine Architecture

### 13.1 AudioBackend Interface

```cpp
class AudioBackend {
public:
    virtual ~AudioBackend() = default;
    virtual bool open(const RuntimeConfig& config) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void close() = 0;
};
```

### 13.2 AudioProcessor Interface

```cpp
class AudioProcessor {
public:
    virtual ~AudioProcessor() = default;
    virtual void prepare(const RuntimeConfig& config) = 0;
    virtual void process(AudioFrame& input, AudioFrame& output) = 0;
    virtual void reset() = 0;
};
```

### 13.3 DspGraph

A simple ordered graph.

```cpp
class DspGraph : public AudioProcessor {
public:
    void prepare(const RuntimeConfig& config) override;
    void process(AudioFrame& input, AudioFrame& output) override;
    void reset() override;
    void setInputGain(float gain);
    void setOutputGain(float gain);
    void enableLimiter(bool enabled);
};
```

Default pipeline:

```text
input frame
→ copy to output
→ input gain
→ optional filter
→ optional therapy mix
→ output gain
→ analyzer
→ limiter
→ analyzer
→ health update
```

### 13.4 AudioEngine

```cpp
class AudioEngine {
public:
    bool configure(const RuntimeConfig& config);
    bool start();
    void stop();
    SystemHealth getHealth() const;
private:
    RuntimeConfig config_;
    std::unique_ptr<AudioBackend> backend_;
    std::unique_ptr<AudioProcessor> processor_;
};
```

Rules:

- Engine owns backend and processor.
- Backend owns audio device lifecycle.
- Processor owns DSP state.
- Health must be queryable without blocking audio.

---

## 14. Live Audio Callback Rules

The callback is the most important real-time boundary.

Inside the callback, the code must not:

- allocate memory;
- lock a mutex;
- print to console;
- open files;
- call network services;
- call slow logging;
- throw exceptions;
- wait on another thread.

Inside the callback, the code may:

- copy input samples into preallocated buffers;
- process DSP;
- update atomic counters;
- write output samples;
- set simple flags.

Callback pseudocode:

```cpp
int audioCallback(const float* input, float* output, unsigned long framesPerBuffer) {
    copy input into preallocated AudioFrame inputFrame;
    processor.process(inputFrame, outputFrame);
    copy outputFrame samples to output;
    update atomic health counters;
    return continue;
}
```

---

## 15. PortAudio Backend Specification

### 15.1 Dependency

Use PortAudio for first live audio implementation.

Linux install example:

```bash
sudo apt install portaudio19-dev
```

### 15.2 Backend Responsibilities

`PortAudioBackend` must:

- initialize PortAudio;
- enumerate devices optionally;
- open input/output stream;
- set sample rate and frame size;
- register callback;
- start stream;
- stop stream;
- terminate PortAudio safely.

### 15.3 Initial Mode

Start with full-duplex mono passthrough:

```text
mic input channel 0 → DSP graph → output channel 0
```

Then stereo:

```text
input L/R → DSP graph → output L/R
```

---

## 16. SystemHealth

```cpp
struct SystemHealth {
    uint64_t callbackCount = 0;
    uint64_t overrunCount = 0;
    uint64_t underflowCount = 0;
    uint64_t clipCount = 0;
    float lastPeakAbs = 0.0f;
    float lastRms = 0.0f;
    float cpuLoad = 0.0f;
    float callbackDurationMs = 0.0f;
    CueHealth cueHealth;
};
```

Rules:

- Updated in lightweight form during callback.
- Full logging happens outside callback.
- Health snapshots must not block audio.

---

## 17. Safety Governor

### 17.1 Purpose

The safety governor turns health metrics into fallback decisions.

### 17.2 States

```text
Normal
SoftBackoff
SafeProfile
TherapyOff
BaselineOnly
```

### 17.3 Initial Prototype Logic

```text
if invalid samples:
    limiter on, therapy off, log critical event
else if repeated clipping:
    reduce output gain or therapy blend
else if callback overruns:
    disable expensive modules
else:
    normal
```

### 17.4 Required Fallback Ladder

```text
hold-safe
→ taper-risk
→ safe-profile
→ therapy-off
→ baseline-only
```

Prototype 0 maps these as:

- hold-safe: keep current safe settings;
- taper-risk: reduce gain/blend;
- safe-profile: simple passthrough + limiter;
- therapy-off: disable stimulus engine;
- baseline-only: input to output through limiter only.

---

## 18. Stimulus Engine Specification

Prototype 0 begins the Relief Mode-compatible synthesis engine without making therapy claims.

### 18.1 Interface

```cpp
class StimulusEngine {
public:
    void prepare(const RuntimeConfig& config);
    void setParams(const TherapyParamSet& params);
    void render(AudioFrame& therapyFrame);
    void reset();
};
```

### 18.2 Supported v0 Carriers

1. Sine tone for education and test only.
2. White noise.
3. Shaped noise using low-pass/high-pass band shaping.
4. Ambient procedural bed using low-rate modulation.

### 18.3 Required Controls

- carrier family;
- band low/high;
- blend gain;
- envelope rate;
- envelope depth;
- session stage;
- seed.

### 18.4 Ramp Behavior

Ramp time defaults:

```text
ramp in: 5 s
ramp out: 5 s
```

Prototype 0 may simulate shorter ramp times for testing.

Rules:

- No abrupt stimulus onset.
- No abrupt stimulus stop except emergency safety fallback.
- Emergency stop should use short fade if possible.

---

## 19. Cue Guard Prototype Specification

Prototype 0 Cue Guard is not full binaural cue preservation yet. It must still enforce the architectural habit.

### 19.1 Interface

```cpp
class CueGuard {
public:
    void prepare(const RuntimeConfig& config);
    CueHealth evaluate(const AudioFrame& baseFrame, const AudioFrame& candidateFrame, const FitState& fitState);
    void applyFallback(AudioFrame& candidateFrame, const CueHealth& health);
};
```

### 19.2 Prototype Metrics

- candidate peak;
- headroom;
- clipping risk;
- invalid sample risk;
- stereo imbalance placeholder;
- sync confidence placeholder.

### 19.3 Headroom Calculation

```text
headroomDb = 20 * log10(1.0 / peakAbs)
```

If `peakAbs == 0`, headroom is very high or capped at a constant.

### 19.4 Prototype Thresholds

| Condition | Action |
|---|---|
| NaN/Inf | BaselineOnly |
| peakAbs > 1.2 before limiter | SafeProfile |
| peakAbs > 1.0 | SoftBackoff + limiter |
| repeated clipping | TherapyOff |
| callback overrun | Disable optional modules |

---

## 20. Offline Tools

### 20.1 `generate_sine`

Generates a sine WAV.

CLI:

```bash
./generate_sine --freq 440 --duration 2 --amp 0.25 --out sine.wav
```

### 20.2 `generate_noise`

Generates noise WAV.

```bash
./generate_noise --type white --duration 5 --amp 0.1 --out white.wav
```

### 20.3 `inspect_wav`

Prints:

- sample rate;
- channels;
- duration;
- min/max;
- peak;
- RMS;
- clipping.

### 20.4 `live_passthrough`

Runs microphone passthrough.

```bash
./live_passthrough --sample-rate 48000 --frame-size 64 --gain 1.0 --limiter true
```

### 20.5 `latency_click_test`

Generates output clicks and records input to estimate round-trip latency.

Initial version can be manual/semi-automatic.

---

## 21. Latency Measurement Specification

### 21.1 Frame Duration

```text
frameDurationMs = frameSize / sampleRateHz * 1000
```

Examples at 48 kHz:

| Frame Size | Duration |
|---:|---:|
| 16 | 0.333 ms |
| 32 | 0.667 ms |
| 64 | 1.333 ms |
| 128 | 2.667 ms |

### 21.2 Round-Trip Latency Test

Test setup:

```text
speaker/headphone output → physical acoustic path → microphone input
```

Procedure:

1. Generate impulse/click on output.
2. Record microphone input.
3. Find impulse peak in recorded signal.
4. Compute sample offset.
5. Convert sample offset to ms.

Formula:

```text
latencyMs = detectedSampleOffset / sampleRateHz * 1000
```

### 21.3 Required Logs

- timestamp;
- sample rate;
- frame size;
- backend device names;
- measured offset samples;
- measured latency ms;
- callback load;
- overrun/underrun counts.

---

## 22. Testing Specification

### 22.1 Unit Tests

Use Catch2, GoogleTest, or simple assert-based tests initially.

Required tests:

#### AudioFrame

- constructor creates correct sample count;
- duration calculation correct;
- channel get/set works;
- invalid channel access handled;
- clear sets all samples to zero.

#### Gain

- gain 2 doubles values;
- gain 0 silences;
- negative gain inverts.

#### Limiter

- values above threshold clamp;
- values below negative threshold clamp;
- safe values unchanged;
- clip flag set.

#### Analyzer

- min/max correct;
- peak correct;
- RMS correct;
- DC offset correct;
- NaN/Inf detected.

#### Oscillator

- output nonzero;
- peak below amplitude + tolerance;
- phase continuity across frames.

#### WAV Writer

- file created;
- RIFF header valid;
- sample rate written;
- expected file size.

#### Filters

- low-pass smooths step input;
- high-pass removes DC over time;
- reset clears state.

#### DelayLine

- zero delay returns immediate input;
- N delay returns sample after N samples;
- no allocation during process.

#### SafetyGovernor

- NaN triggers baseline-only;
- clipping triggers soft backoff;
- repeated clipping disables therapy.

### 22.2 Regression Tests

Golden files:

- sine 440 Hz 1 second;
- white noise with fixed seed;
- limited clipping test;
- filtered step response.

Regression tolerance:

- exact match for deterministic integer outputs where possible;
- floating tolerance `1e-5` for sample comparisons.

### 22.3 Live Tests

Manual live tests:

1. mic passthrough with gain 1.0;
2. passthrough with gain 0.5;
3. passthrough with limiter;
4. low-pass audibility test;
5. high-pass audibility test;
6. click latency test.

---

## 23. Build System Specification

### 23.1 CMake

Minimum `CMakeLists.txt` behavior:

- require C++17;
- build core library `manticore_audio`;
- build tools;
- optionally build tests;
- optionally enable PortAudio.

Example targets:

```text
manticore_audio
generate_sine
generate_noise
live_passthrough
latency_click_test
tests
```

### 23.2 Compiler Warnings

Recommended flags:

```text
-Wall -Wextra -Wpedantic -Wconversion
```

Treat warnings as errors later, not on day one.

---

## 24. Implementation Sequence

### Phase 1: Offline Core

1. `AudioFrame`
2. `FrameStats`
3. `applyGain`
4. `applyLimiter`
5. `SineOscillator`
6. `WavWriter`
7. offline sine tool
8. tests

Exit: generate and inspect sine WAV.

### Phase 2: Basic DSP

1. white noise generator;
2. low-pass filter;
3. high-pass filter;
4. delay line;
5. mixer;
6. envelope;
7. tests.

Exit: generate filtered/noisy WAVs.

### Phase 3: Engine and Graph

1. `RuntimeConfig`;
2. `AudioProcessor` interface;
3. `DspGraph`;
4. `AudioEngine` skeleton;
5. system health;
6. offline engine render.

Exit: offline render flows through same graph live audio will use.

### Phase 4: Live Audio

1. `AudioBackend` interface;
2. `PortAudioBackend`;
3. live passthrough;
4. callback health counters;
5. CLI config;
6. live limiter.

Exit: stable mic passthrough at 64 samples / 48 kHz.

### Phase 5: Latency and Safety

1. click generator;
2. round-trip recorder;
3. latency estimator;
4. `CueHealth`;
5. `SafetyEvent`;
6. `SafetyGovernor`;
7. fallback ladder.

Exit: latency and safety behavior logged.

### Phase 6: Therapy-Compatible Stubs

1. `TherapyParamSet`;
2. `StimulusEngine`;
3. shaped noise;
4. ramp-in/ramp-out;
5. safe-profile;
6. cue guard evaluation;
7. therapy enable/disable.

Exit: therapy-like procedural audio can be mixed safely into baseline passthrough without breaking limiter/safety behavior.

---

## 25. Definition of Done

A feature is not done until:

- it compiles;
- it has a small example;
- it has at least one test;
- it does not allocate in the audio callback path;
- it exposes health or failure behavior if safety-relevant;
- it is documented in `docs/` or header comments;
- it does not violate baseline hearing fallback behavior.

---

## 26. First Concrete Milestone Checklist

The first milestone is intentionally small.

Deliverable:

```text
A C++ program that generates sine_440.wav through AudioFrame → SineOscillator → Analyzer → WAV writer.
```

Files:

```text
include/manticore/core/audio_frame.h
include/manticore/dsp/analyzer.h
include/manticore/dsp/oscillator.h
include/manticore/dsp/wav_writer.h
src/tools/generate_sine.cpp
CMakeLists.txt
```

Acceptance:

- builds successfully;
- writes `sine_440.wav`;
- prints frame duration;
- prints min/max/peak/RMS;
- audio plays without clipping.

---

## 27. Non-Negotiable Engineering Rules

1. The audio path must remain simple before it becomes clever.
2. Plane B never writes raw output samples.
3. Therapy and AI are disabled by default.
4. Limiter is enabled by default.
5. All future adaptive behavior must fail toward baseline hearing.
6. Every new DSP feature must be testable offline first.
7. Live audio comes only after offline correctness.
8. No dynamic allocation inside the callback.
9. No blocking calls inside the callback.
10. No speculative feature enters the kernel unless it helps Prototype 0 or a declared next prototype.

---

## 28. Future Roadmap After Prototype 0

### Prototype 1

Dual-channel / dual-ear wired development rig.

Adds:

- stereo/binaural routing;
- inter-channel delay tests;
- ITD/ILD simulation;
- basic directionality;
- reference mic simulation.

### Prototype 2

Wearable dev kit.

Adds:

- embedded target;
- I2S/ADC/DAC;
- DMA buffers;
- RTOS scheduling;
- IMU/fit telemetry;
- power and thermal instrumentation.

### Prototype 3

Adaptive policy engine.

Adds:

- bounded Plane B policy updates;
- environment-aware profiles;
- personalization data;
- safe rollback.

### Prototype 4

Research sensing mode.

Adds:

- optional biosignal hooks;
- attention/workload telemetry;
- research-only gated adaptation.

---

## 29. Summary

The Manticore Audio Kernel is the root of the entire platform. Its first job is not therapy, AI, or commercial polish. Its first job is to prove that Manticore can move sound through deterministic software safely, measurably, and with low latency.

Build order:

```text
AudioFrame
→ Analyzer
→ Gain
→ Limiter
→ Oscillator
→ WAV Writer
→ Filters
→ Delay
→ DSP Graph
→ Live Passthrough
→ Latency Measurement
→ Safety Governor
→ Therapy-Compatible Stimulus Stub
```

Once this kernel is real, everything else becomes a controlled extension rather than a fantasy architecture.
