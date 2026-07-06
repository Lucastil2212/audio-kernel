# Architecture

Manticore Audio Kernel uses three planes.

## Plane A: Deterministic Audio Kernel

Owns continuous audio rendering. Prototype 0 implements:

- `AudioFrame` sample transport
- DSP modules (gain, limiter, filters, delay, mixer, oscillator, noise)
- `DspGraph` ordered processing pipeline
- `AudioEngine` lifecycle
- PortAudio live callback path

The callback path forbids allocation, locking, I/O, and exceptions.

## Plane B: Policy and Session Layer

Prototype 0 provides `RuntimeConfig` and `TherapyParamSet` validation. Plane B never writes output samples directly.

## Plane C: Sensing and Telemetry

Prototype 0 provides `SystemHealth`, `FitState` stubs, and `CueHealth` metrics updated from frame analysis.

## Default DSP Graph Pipeline

```text
input → input gain → optional therapy mix → output gain → safety evaluate
      → limiter → health update
```

## Safety Fallback Ladder

```text
Normal → SoftBackoff → SafeProfile → TherapyOff → BaselineOnly
```

Triggered by invalid samples, clipping, callback overruns, and stale parameters.
