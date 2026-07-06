# Safety Model

## CueHealth states

- **Normal** — full processing allowed
- **SoftBackoff** — reduce output gain and therapy blend
- **SafeProfile** — passthrough + limiter, therapy off
- **TherapyOff** — stimulus disabled
- **BaselineOnly** — invalid samples; limiter-only path

## Event codes

`AUDIO_CLIP_DETECTED`, `AUDIO_NAN_DETECTED`, `AUDIO_INF_DETECTED`, `CALLBACK_OVERRUN`, `CALLBACK_UNDERRUN`, `PARAM_STALE`, `PARAM_OUT_OF_RANGE`, `THERAPY_DISABLED_BY_GUARD`, `LIMITER_ENGAGED`, `CONFIG_INVALID`

## Headroom

```text
headroomDb = 20 * log10(1.0 / peakAbs)
```

## Prototype thresholds

| Condition | Action |
|-----------|--------|
| NaN/Inf | BaselineOnly |
| peak > 1.2 | SafeProfile |
| peak > 1.0 or clipped | SoftBackoff |
| repeated clipping | TherapyOff |
| callback overruns | SafeProfile |

Therapy and adaptive features fail toward baseline hearing.
