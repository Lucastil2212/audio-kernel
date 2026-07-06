# Manticore Audio Kernel

Prototype 0 desktop bench rig for the Manticore software-defined auditory interface.

## Build

```bash
cmake -S . -B build -DMANTICORE_BUILD_TESTS=ON -DMANTICORE_ENABLE_PORTAUDIO=ON
cmake --build build
ctest --test-dir build
```

PortAudio on Linux:

```bash
sudo apt install portaudio19-dev
```

## Quick start

```bash
./build/generate_sine --freq 440 --duration 1 --out sine_440.wav
./build/inspect_wav sine_440.wav
./build/manticore_main
```

Live passthrough (requires audio hardware):

```bash
./build/live_passthrough --sample-rate 48000 --frame-size 64 --gain 1.0 --limiter true
```

## Architecture

Three planes:

- **Plane A** — deterministic audio kernel (DSP, I/O, limiting)
- **Plane B** — policy/session configuration stubs
- **Plane C** — telemetry and health metrics stubs

See `docs/architecture.md` for details.

## Tools

| Tool | Purpose |
|------|---------|
| `generate_sine` | Offline sine WAV generation |
| `generate_noise` | White/pink noise WAV generation |
| `inspect_wav` | WAV analysis (peak, RMS, clipping) |
| `live_passthrough` | Mic passthrough through DSP graph |
| `latency_click_test` | Round-trip latency capture helper |

## Tests

```bash
./build/manticore_tests
```

Covers AudioFrame, gain, limiter, analyzer, oscillator, WAV I/O, filters, delay line, safety governor, and DSP graph at frame sizes 16/32/64.
