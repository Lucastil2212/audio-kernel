# DSP Notes

## Sample format

Internal samples are `float` in normalized range `[-1.0, +1.0]`.

## One-pole filters

Low-pass:

```text
alpha = dt / (RC + dt),  RC = 1 / (2πfc)
y[n] = y[n-1] + alpha * (x[n] - y[n-1])
```

High-pass:

```text
y[n] = alpha * (y[n-1] + x[n] - x[n-1])
```

## Delay line

Allocation occurs only in `prepare()`. Used for echo tests and future ITD simulation.

## Therapy stimulus

`StimulusEngine` supports sine, white/pink noise, band-shaped noise, and ambient beds with ramp-in/out envelopes. Therapy is disabled by default.
