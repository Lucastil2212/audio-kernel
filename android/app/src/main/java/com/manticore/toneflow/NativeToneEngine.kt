package com.manticore.toneflow

class NativeToneEngine {
    private var handle: Long = 0L

    init {
        System.loadLibrary("manticore_toneflow")
        handle = nativeCreate()
    }

    fun start(): Boolean = nativeStart(handle)
    fun stop() = nativeStop(handle)
    fun setPreset(index: Int) = nativeSetPreset(handle, index)
    fun setVolume(volume: Float) = nativeSetVolume(handle, volume)
    fun setPlaying(playing: Boolean) = nativeSetPlaying(handle, playing)
    fun setCarrier(hz: Float) = nativeSetCarrier(handle, hz)
    fun setBeat(hz: Float) = nativeSetBeat(handle, hz)
    fun setHarmonics(layers: Int) = nativeSetHarmonics(handle, layers)
    fun setNoiseBlend(blend: Float) = nativeSetNoiseBlend(handle, blend)
    fun setSubLevel(level: Float) = nativeSetSubLevel(handle, level)
    fun subLevel(): Float = nativeSubLevel(handle)
    fun setMicFeedback(enabled: Boolean) = nativeSetMicFeedback(handle, enabled)
    fun setMicGain(gain: Float) = nativeSetMicGain(handle, gain)
    fun setToneMix(mix: Float) = nativeSetToneMix(handle, mix)
    fun cycleMode() = nativeCycleMode(handle)
    fun cyclePerception() = nativeCyclePerception(handle)
    fun cycleReverb() = nativeCycleReverb(handle)
    fun cycleFilter() = nativeCycleFilter(handle)

    fun presetCount(): Int = nativePresetCount(handle)
    fun presetLabel(index: Int): String = nativePresetLabel(handle, index)
    fun presetDescription(index: Int): String = nativePresetDescription(handle, index)
    fun modeLabel(): String = nativeModeLabel(handle)
    fun perceptionLabel(): String = nativePerceptionLabel(handle)
    fun reverbLabel(): String = nativeReverbLabel(handle)
    fun filterLabel(): String = nativeFilterLabel(handle)
    fun carrierHz(): Float = nativeCarrierHz(handle)
    fun beatHz(): Float = nativeBeatHz(handle)
    fun rms(): Float = nativeRms(handle)
    fun beatPhase(): Float = nativeBeatPhase(handle)
    fun micRms(): Float = nativeMicRms(handle)
    fun micAgc(): Float = nativeMicAgc(handle)
    fun micEnabled(): Boolean = nativeMicEnabled(handle)

    fun release() {
        if (handle != 0L) {
            nativeStop(handle)
            nativeDestroy(handle)
            handle = 0L
        }
    }

    private external fun nativeCreate(): Long
    private external fun nativeDestroy(handle: Long)
    private external fun nativeStart(handle: Long): Boolean
    private external fun nativeStop(handle: Long)
    private external fun nativeSetPreset(handle: Long, index: Int)
    private external fun nativeSetVolume(handle: Long, volume: Float)
    private external fun nativeSetPlaying(handle: Long, playing: Boolean)
    private external fun nativeSetCarrier(handle: Long, hz: Float)
    private external fun nativeSetBeat(handle: Long, hz: Float)
    private external fun nativeSetHarmonics(handle: Long, layers: Int)
    private external fun nativeSetNoiseBlend(handle: Long, blend: Float)
    private external fun nativeSetSubLevel(handle: Long, level: Float)
    private external fun nativeSubLevel(handle: Long): Float
    private external fun nativeSetMicFeedback(handle: Long, enabled: Boolean)
    private external fun nativeSetMicGain(handle: Long, gain: Float)
    private external fun nativeSetToneMix(handle: Long, mix: Float)
    private external fun nativeCycleMode(handle: Long)
    private external fun nativeCyclePerception(handle: Long)
    private external fun nativeCycleReverb(handle: Long)
    private external fun nativeCycleFilter(handle: Long)
    private external fun nativePresetCount(handle: Long): Int
    private external fun nativePresetLabel(handle: Long, index: Int): String
    private external fun nativePresetDescription(handle: Long, index: Int): String
    private external fun nativeModeLabel(handle: Long): String
    private external fun nativePerceptionLabel(handle: Long): String
    private external fun nativeReverbLabel(handle: Long): String
    private external fun nativeFilterLabel(handle: Long): String
    private external fun nativeCarrierHz(handle: Long): Float
    private external fun nativeBeatHz(handle: Long): Float
    private external fun nativeRms(handle: Long): Float
    private external fun nativeBeatPhase(handle: Long): Float
    private external fun nativeMicRms(handle: Long): Float
    private external fun nativeMicAgc(handle: Long): Float
    private external fun nativeMicEnabled(handle: Long): Boolean
}
