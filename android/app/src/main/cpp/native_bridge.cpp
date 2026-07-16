#include <jni.h>

#include "aaudio_player.h"

namespace {

manticore::android::AAudioPlayer* fromHandle(jlong handle)
{
    return reinterpret_cast<manticore::android::AAudioPlayer*>(handle);
}

jstring toJString(JNIEnv* env, const char* text)
{
    return env->NewStringUTF(text != nullptr ? text : "");
}

}  // namespace

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeCreate(JNIEnv*, jobject)
{
    return reinterpret_cast<jlong>(new manticore::android::AAudioPlayer());
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeDestroy(JNIEnv*, jobject,
                                                           jlong handle)
{
    delete fromHandle(handle);
}

JNIEXPORT jboolean JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeStart(JNIEnv*, jobject,
                                                         jlong handle)
{
    auto* player = fromHandle(handle);
    return player != nullptr && player->start() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeStop(JNIEnv*, jobject,
                                                        jlong handle)
{
    if (auto* player = fromHandle(handle)) {
        player->stop();
    }
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeSetPreset(JNIEnv*, jobject,
                                                             jlong handle,
                                                             jint index)
{
    if (auto* player = fromHandle(handle)) {
        player->setPresetIndex(index);
    }
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeSetVolume(JNIEnv*, jobject,
                                                             jlong handle,
                                                             jfloat volume)
{
    if (auto* player = fromHandle(handle)) {
        player->setVolume(volume);
    }
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeSetPlaying(JNIEnv*, jobject,
                                                              jlong handle,
                                                              jboolean playing)
{
    if (auto* player = fromHandle(handle)) {
        player->setPlaying(playing == JNI_TRUE);
    }
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeSetCarrier(JNIEnv*, jobject,
                                                              jlong handle,
                                                              jfloat hz)
{
    if (auto* player = fromHandle(handle)) {
        player->setCarrierHz(hz);
    }
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeSetBeat(JNIEnv*, jobject,
                                                           jlong handle,
                                                           jfloat hz)
{
    if (auto* player = fromHandle(handle)) {
        player->setBeatHz(hz);
    }
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeSetHarmonics(JNIEnv*, jobject,
                                                                jlong handle,
                                                                jint layers)
{
    if (auto* player = fromHandle(handle)) {
        player->setHarmonicLayers(layers);
    }
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeSetNoiseBlend(JNIEnv*, jobject,
                                                                 jlong handle,
                                                                 jfloat blend)
{
    if (auto* player = fromHandle(handle)) {
        player->setKernelNoiseBlend(blend);
    }
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeCycleMode(JNIEnv*, jobject,
                                                             jlong handle)
{
    if (auto* player = fromHandle(handle)) {
        player->cycleMode();
    }
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeCyclePerception(JNIEnv*,
                                                                   jobject,
                                                                   jlong handle)
{
    if (auto* player = fromHandle(handle)) {
        player->cyclePerception();
    }
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeCycleReverb(JNIEnv*, jobject,
                                                               jlong handle)
{
    if (auto* player = fromHandle(handle)) {
        player->cycleReverb();
    }
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeCycleFilter(JNIEnv*, jobject,
                                                               jlong handle)
{
    if (auto* player = fromHandle(handle)) {
        player->cycleFilter();
    }
}

JNIEXPORT jint JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativePresetCount(JNIEnv*, jobject,
                                                               jlong handle)
{
    auto* player = fromHandle(handle);
    return player != nullptr ? player->presetCount() : 0;
}

JNIEXPORT jstring JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativePresetLabel(JNIEnv* env,
                                                               jobject,
                                                               jlong handle,
                                                               jint index)
{
    auto* player = fromHandle(handle);
    return toJString(env, player != nullptr ? player->presetLabel(index) : "");
}

JNIEXPORT jstring JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativePresetDescription(
    JNIEnv* env, jobject, jlong handle, jint index)
{
    auto* player = fromHandle(handle);
    return toJString(
        env, player != nullptr ? player->presetDescription(index) : "");
}

JNIEXPORT jstring JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeModeLabel(JNIEnv* env,
                                                             jobject,
                                                             jlong handle)
{
    auto* player = fromHandle(handle);
    return toJString(env, player != nullptr ? player->modeLabel() : "");
}

JNIEXPORT jstring JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativePerceptionLabel(JNIEnv* env,
                                                                   jobject,
                                                                   jlong handle)
{
    auto* player = fromHandle(handle);
    return toJString(env, player != nullptr ? player->perceptionLabel() : "");
}

JNIEXPORT jstring JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeReverbLabel(JNIEnv* env,
                                                               jobject,
                                                               jlong handle)
{
    auto* player = fromHandle(handle);
    return toJString(env, player != nullptr ? player->reverbLabel() : "");
}

JNIEXPORT jstring JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeFilterLabel(JNIEnv* env,
                                                               jobject,
                                                               jlong handle)
{
    auto* player = fromHandle(handle);
    return toJString(env, player != nullptr ? player->filterLabel() : "");
}

JNIEXPORT jfloat JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeCarrierHz(JNIEnv*, jobject,
                                                             jlong handle)
{
    auto* player = fromHandle(handle);
    return player != nullptr ? player->carrierHz() : 0.0f;
}

JNIEXPORT jfloat JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeBeatHz(JNIEnv*, jobject,
                                                          jlong handle)
{
    auto* player = fromHandle(handle);
    return player != nullptr ? player->beatHz() : 0.0f;
}

JNIEXPORT jfloat JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeRms(JNIEnv*, jobject,
                                                       jlong handle)
{
    auto* player = fromHandle(handle);
    return player != nullptr ? player->currentRms() : 0.0f;
}

JNIEXPORT jfloat JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeBeatPhase(JNIEnv*, jobject,
                                                             jlong handle)
{
    auto* player = fromHandle(handle);
    return player != nullptr ? player->beatPhase() : 0.0f;
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeSetMicFeedback(
    JNIEnv*, jobject, jlong handle, jboolean enabled)
{
    if (auto* player = fromHandle(handle)) {
        player->setMicFeedbackEnabled(enabled == JNI_TRUE);
    }
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeSetMicGain(JNIEnv*, jobject,
                                                              jlong handle,
                                                              jfloat gain)
{
    if (auto* player = fromHandle(handle)) {
        player->setMicFeedbackGain(gain);
    }
}

JNIEXPORT void JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeSetToneMix(JNIEnv*, jobject,
                                                              jlong handle,
                                                              jfloat mix)
{
    if (auto* player = fromHandle(handle)) {
        player->setToneMix(mix);
    }
}

JNIEXPORT jfloat JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeMicRms(JNIEnv*, jobject,
                                                          jlong handle)
{
    auto* player = fromHandle(handle);
    return player != nullptr ? player->micRms() : 0.0f;
}

JNIEXPORT jfloat JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeMicAgc(JNIEnv*, jobject,
                                                          jlong handle)
{
    auto* player = fromHandle(handle);
    return player != nullptr ? player->micAgcGain() : 1.0f;
}

JNIEXPORT jboolean JNICALL
Java_com_manticore_toneflow_NativeToneEngine_nativeMicEnabled(JNIEnv*, jobject,
                                                              jlong handle)
{
    auto* player = fromHandle(handle);
    return player != nullptr && player->micFeedbackEnabled() ? JNI_TRUE
                                                             : JNI_FALSE;
}

}  // extern "C"
