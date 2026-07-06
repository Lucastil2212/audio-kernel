# Latency Measurement

## Frame duration

```text
frameDurationMs = frameSize / sampleRateHz * 1000
```

At 48 kHz:

| Frame size | Duration |
|-----------:|---------:|
| 16 | 0.333 ms |
| 32 | 0.667 ms |
| 64 | 1.333 ms |

## Round-trip test

1. Emit impulse on output.
2. Capture microphone input.
3. Find peak index in captured buffer.
4. Convert to milliseconds:

```text
latencyMs = peakIndex / sampleRateHz * 1000
```

Use `latency_click_test` with acoustic loopback (speaker → mic) for meaningful results.

Log sample rate, frame size, device names, peak index, latency ms, callback load, and overrun/underrun counts.
