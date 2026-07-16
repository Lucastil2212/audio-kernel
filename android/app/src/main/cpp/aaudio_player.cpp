#include "aaudio_player.h"

#include <android/log.h>
#include <algorithm>
#include <cstring>

#include "manticore/toneflow/presets.h"

#define LOG_TAG "ManticoreToneFlow"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace manticore::android {
namespace {

bool tryOpenWithSharing(AAudioStreamBuilder* builder, AAudioStream** out,
                        aaudio_sharing_mode_t mode)
{
    AAudioStreamBuilder_setSharingMode(builder, mode);
    const aaudio_result_t result = AAudioStreamBuilder_openStream(builder, out);
    return result == AAUDIO_OK && out != nullptr && *out != nullptr;
}

}  // namespace

AAudioPlayer::AAudioPlayer()
{
    micRing_.assign(kRingSize, 0.0f);
}

AAudioPlayer::~AAudioPlayer() { stop(); }

void AAudioPlayer::stabilizeBuffer(AAudioStream* stream)
{
    if (stream == nullptr) {
        return;
    }
    const int32_t burst = AAudioStream_getFramesPerBurst(stream);
    if (burst <= 0) {
        return;
    }
    // 4 bursts of slack — prevents underrun crackle that sounds like static.
    const int32_t target = burst * 4;
    const aaudio_result_t setResult =
        AAudioStream_setBufferSizeInFrames(stream, target);
    const int32_t actual = AAudioStream_getBufferSizeInFrames(stream);
    LOGI("buffer stabilize: burst=%d target=%d set=%s actual=%d", burst, target,
         AAudio_convertResultToText(setResult), actual);
}

void AAudioPlayer::publishTelemetry()
{
    telemCarrier_.store(engine_.carrierHz(), std::memory_order_relaxed);
    telemBeat_.store(engine_.beatHz(), std::memory_order_relaxed);
    telemRms_.store(engine_.currentRms(), std::memory_order_relaxed);
    telemPhase_.store(engine_.beatPhase(), std::memory_order_relaxed);
    telemMicRms_.store(engine_.micRms(), std::memory_order_relaxed);
    telemMicAgc_.store(engine_.micAgcGain(), std::memory_order_relaxed);
    telemSub_.store(engine_.subLevel(), std::memory_order_relaxed);
    telemMode_.store(static_cast<int>(engine_.params().mode),
                     std::memory_order_relaxed);
    telemPerception_.store(static_cast<int>(engine_.params().perception),
                           std::memory_order_relaxed);
    telemReverb_.store(static_cast<int>(engine_.params().reverb),
                       std::memory_order_relaxed);
    telemFilter_.store(static_cast<int>(engine_.params().filter),
                       std::memory_order_relaxed);
    telemMicEn_.store(engine_.micFeedbackEnabled(), std::memory_order_relaxed);
}

bool AAudioPlayer::openOutputStream()
{
    AAudioStreamBuilder* builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGE("output builder failed: %s", AAudio_convertResultToText(result));
        return false;
    }

    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSampleRate(builder, 48000);
    AAudioStreamBuilder_setChannelCount(builder, 2);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    // Stable music path — exclusive/tiny buffers caused underrun "static".
    AAudioStreamBuilder_setPerformanceMode(builder,
                                           AAUDIO_PERFORMANCE_MODE_NONE);
    AAudioStreamBuilder_setUsage(builder, AAUDIO_USAGE_MEDIA);
    AAudioStreamBuilder_setContentType(builder, AAUDIO_CONTENT_TYPE_MUSIC);
    AAudioStreamBuilder_setDataCallback(builder, outputCallback, this);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, this);
    // Let the device choose a healthy callback size.
    AAudioStreamBuilder_setFramesPerDataCallback(builder, 0);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, 0);

    // Prefer shared mixer for glitch-free sustained sine tones.
    if (!tryOpenWithSharing(builder, &outputStream_,
                            AAUDIO_SHARING_MODE_SHARED) &&
        !tryOpenWithSharing(builder, &outputStream_,
                            AAUDIO_SHARING_MODE_EXCLUSIVE)) {
        LOGE("open output failed");
        AAudioStreamBuilder_delete(builder);
        outputStream_ = nullptr;
        return false;
    }
    AAudioStreamBuilder_delete(builder);

    sampleRate_ = AAudioStream_getSampleRate(outputStream_);
    framesPerBurst_ = AAudioStream_getFramesPerBurst(outputStream_);
    if (framesPerBurst_ <= 0) {
        framesPerBurst_ = 192;
    }
    stabilizeBuffer(outputStream_);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto prior = engine_.params();
        const bool micEn = engine_.micFeedbackEnabled();
        const float micGain = engine_.micFeedbackGain();
        const float toneMix = engine_.toneMix();
        engine_.prepare(static_cast<uint32_t>(sampleRate_),
                        static_cast<uint32_t>(framesPerBurst_ * 2));
        engine_.setParams(prior);
        engine_.setMicFeedbackEnabled(micEn);
        engine_.setMicFeedbackGain(micGain);
        engine_.setToneMix(toneMix);
        engine_.setMicLowLatency(true);
        engine_.setPlaying(true);
        publishTelemetry();
    }

    LOGI("output opened: sr=%d burst=%d sharing=%d buf=%d", sampleRate_,
         framesPerBurst_, AAudioStream_getSharingMode(outputStream_),
         AAudioStream_getBufferSizeInFrames(outputStream_));
    return true;
}

bool AAudioPlayer::openInputStream()
{
    if (inputStream_ != nullptr) {
        return true;
    }

    AAudioStreamBuilder* builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGE("input builder failed: %s", AAudio_convertResultToText(result));
        return false;
    }

    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setSampleRate(builder, sampleRate_);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setPerformanceMode(builder,
                                           AAUDIO_PERFORMANCE_MODE_NONE);
    AAudioStreamBuilder_setInputPreset(builder,
                                       AAUDIO_INPUT_PRESET_VOICE_RECOGNITION);
    AAudioStreamBuilder_setFramesPerDataCallback(builder, 0);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, 0);
    AAudioStreamBuilder_setDataCallback(builder, inputCallback, this);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, this);

    if (!tryOpenWithSharing(builder, &inputStream_,
                            AAUDIO_SHARING_MODE_EXCLUSIVE) &&
        !tryOpenWithSharing(builder, &inputStream_,
                            AAUDIO_SHARING_MODE_SHARED)) {
        LOGE("open input failed");
        AAudioStreamBuilder_delete(builder);
        inputStream_ = nullptr;
        return false;
    }
    AAudioStreamBuilder_delete(builder);

    inputChannels_ = AAudioStream_getChannelCount(inputStream_);
    if (inputChannels_ <= 0) {
        inputChannels_ = 1;
    }
    stabilizeBuffer(inputStream_);

    // Drop any stale samples so feedback starts from "now".
    ringWrite_.store(0, std::memory_order_relaxed);
    ringRead_.store(0, std::memory_order_relaxed);

    result = AAudioStream_requestStart(inputStream_);
    if (result != AAUDIO_OK) {
        LOGE("start input failed: %s", AAudio_convertResultToText(result));
        AAudioStream_close(inputStream_);
        inputStream_ = nullptr;
        return false;
    }

    LOGI("input opened: ch=%d sr=%d burst=%d sharing=%d", inputChannels_,
         AAudioStream_getSampleRate(inputStream_),
         AAudioStream_getFramesPerBurst(inputStream_),
         AAudioStream_getSharingMode(inputStream_));
    return true;
}

void AAudioPlayer::closeStreams()
{
    if (inputStream_ != nullptr) {
        AAudioStream_requestStop(inputStream_);
        AAudioStream_close(inputStream_);
        inputStream_ = nullptr;
    }
    if (outputStream_ != nullptr) {
        AAudioStream_requestStop(outputStream_);
        AAudioStream_close(outputStream_);
        outputStream_ = nullptr;
    }
}

void AAudioPlayer::ensureScratch(int32_t numFrames)
{
    if (static_cast<int32_t>(micScratch_.size()) < numFrames) {
        micScratch_.assign(static_cast<size_t>(numFrames), 0.0f);
    }
}

void AAudioPlayer::pushMicFrames(const float* data, int32_t numFrames,
                                 int32_t channels)
{
    if (data == nullptr || numFrames <= 0 || channels <= 0) {
        return;
    }

    uint32_t w = ringWrite_.load(std::memory_order_relaxed);
    for (int32_t i = 0; i < numFrames; ++i) {
        float mono = data[i * channels];
        if (channels > 1) {
            mono = 0.5f * (data[i * channels] + data[i * channels + 1]);
        }
        micRing_[w & kRingMask] = mono;
        ++w;
    }
    ringWrite_.store(w, std::memory_order_release);

    // Keep only the freshest ~2 bursts so we never accumulate delay.
    const uint32_t r = ringRead_.load(std::memory_order_relaxed);
    const uint32_t avail = w - r;
    const uint32_t maxKeep = static_cast<uint32_t>(std::max(framesPerBurst_ * 2, 64));
    if (avail > maxKeep) {
        ringRead_.store(w - maxKeep, std::memory_order_relaxed);
    }
}

int32_t AAudioPlayer::popMicFrames(float* dstMono, int32_t numFrames)
{
    if (dstMono == nullptr || numFrames <= 0) {
        return 0;
    }

    const uint32_t w = ringWrite_.load(std::memory_order_acquire);
    uint32_t r = ringRead_.load(std::memory_order_relaxed);
    uint32_t avail = w - r;

    // Prefer latest: if we have more than needed, skip older frames.
    if (avail > static_cast<uint32_t>(numFrames)) {
        r = w - static_cast<uint32_t>(numFrames);
        avail = static_cast<uint32_t>(numFrames);
    }

    const int32_t got = static_cast<int32_t>(avail);
    for (int32_t i = 0; i < got; ++i) {
        dstMono[i] = micRing_[(r + static_cast<uint32_t>(i)) & kRingMask];
    }
    for (int32_t i = got; i < numFrames; ++i) {
        dstMono[i] = 0.0f;
    }
    ringRead_.store(r + static_cast<uint32_t>(got), std::memory_order_relaxed);
    return got;
}

bool AAudioPlayer::start()
{
    if (running_.load()) {
        return true;
    }
    if (!openOutputStream()) {
        return false;
    }

    if (micWanted_.load()) {
        if (!openInputStream()) {
            LOGE("mic requested but input open failed; continuing tones-only");
        }
    }

    const aaudio_result_t result = AAudioStream_requestStart(outputStream_);
    if (result != AAUDIO_OK) {
        LOGE("requestStart output failed: %s",
             AAudio_convertResultToText(result));
        closeStreams();
        return false;
    }
    running_.store(true);
    return true;
}

void AAudioPlayer::stop()
{
    running_.store(false);
    closeStreams();
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setPlaying(false);
}

aaudio_data_callback_result_t AAudioPlayer::inputCallback(AAudioStream*,
                                                          void* userData,
                                                          void* audioData,
                                                          int32_t numFrames)
{
    auto* self = static_cast<AAudioPlayer*>(userData);
    if (!self->micWanted_.load(std::memory_order_relaxed)) {
        return AAUDIO_CALLBACK_RESULT_CONTINUE;
    }
    self->pushMicFrames(static_cast<const float*>(audioData), numFrames,
                        self->inputChannels_);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

aaudio_data_callback_result_t AAudioPlayer::outputCallback(AAudioStream*,
                                                           void* userData,
                                                           void* audioData,
                                                           int32_t numFrames)
{
    auto* self = static_cast<AAudioPlayer*>(userData);
    auto* out = static_cast<float*>(audioData);

    const float* micPtr = nullptr;
    uint32_t micCh = 0;
    if (self->micWanted_.load(std::memory_order_relaxed)) {
        self->ensureScratch(numFrames);
        self->popMicFrames(self->micScratch_.data(), numFrames);
        micPtr = self->micScratch_.data();
        micCh = 1;
    }

    // Render under lock; UI telemetry reads atomics and never contends here.
    {
        std::lock_guard<std::mutex> lock(self->mutex_);
        self->engine_.renderInterleavedWithMic(out, micPtr,
                                               static_cast<uint32_t>(numFrames),
                                               micCh);
        self->publishTelemetry();
    }
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

void AAudioPlayer::errorCallback(AAudioStream*, void* userData,
                                 aaudio_result_t error)
{
    auto* self = static_cast<AAudioPlayer*>(userData);
    LOGE("AAudio error: %s", AAudio_convertResultToText(error));
    self->running_.store(false);
}

void AAudioPlayer::setPresetIndex(int index)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto* preset = toneflow::presetByIndex(static_cast<size_t>(index));
    if (preset != nullptr) {
        engine_.setPreset(*preset);
        engine_.reset();
        engine_.setMicLowLatency(true);
        engine_.setPlaying(true);
        publishTelemetry();
    }
}

void AAudioPlayer::setVolume(float volume)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setVolume(volume);
}

void AAudioPlayer::setPlaying(bool playing)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setPlaying(playing);
}

void AAudioPlayer::setCarrierHz(float hz)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setCarrierHz(hz);
    publishTelemetry();
}

void AAudioPlayer::setBeatHz(float hz)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setBeatHz(hz);
    publishTelemetry();
}

void AAudioPlayer::setMode(int mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setMode(static_cast<toneflow::BeatMode>(mode));
    publishTelemetry();
}

void AAudioPlayer::setPerception(int mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setPerception(static_cast<toneflow::PerceptionMode>(mode));
    publishTelemetry();
}

void AAudioPlayer::setReverb(int preset)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setReverb(static_cast<toneflow::ReverbPreset>(preset));
    publishTelemetry();
}

void AAudioPlayer::setFilter(int type)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setFilter(static_cast<toneflow::FilterType>(type));
    publishTelemetry();
}

void AAudioPlayer::setHarmonicLayers(int layers)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setHarmonicLayers(layers);
}

void AAudioPlayer::setKernelNoiseBlend(float blend)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setKernelNoiseBlend(blend);
}

void AAudioPlayer::setSubLevel(float level)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setSubLevel(level);
    publishTelemetry();
}

void AAudioPlayer::setSubHz(float hz)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setSubHz(hz);
}

float AAudioPlayer::subLevel() const
{
    return telemSub_.load(std::memory_order_relaxed);
}

void AAudioPlayer::setMicFeedbackEnabled(bool enabled)
{
    micWanted_.store(enabled);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        engine_.setMicFeedbackEnabled(enabled);
        engine_.setMicLowLatency(true);
    }

    if (!running_.load()) {
        return;
    }

    if (enabled) {
        if (inputStream_ == nullptr && !openInputStream()) {
            LOGE("failed to open mic while running");
            micWanted_.store(false);
            std::lock_guard<std::mutex> lock(mutex_);
            engine_.setMicFeedbackEnabled(false);
        } else {
            stabilizeBuffer(outputStream_);
            stabilizeBuffer(inputStream_);
        }
    } else if (inputStream_ != nullptr) {
        AAudioStream_requestStop(inputStream_);
        AAudioStream_close(inputStream_);
        inputStream_ = nullptr;
    }
}

void AAudioPlayer::setMicFeedbackGain(float gain)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setMicFeedbackGain(gain);
}

void AAudioPlayer::setToneMix(float mix)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setToneMix(mix);
}

void AAudioPlayer::cycleMode()
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setMode(toneflow::cycleBeatMode(engine_.params().mode));
    publishTelemetry();
}

void AAudioPlayer::cyclePerception()
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setPerception(toneflow::cyclePerception(engine_.params().perception));
    publishTelemetry();
}

void AAudioPlayer::cycleReverb()
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setReverb(toneflow::cycleReverb(engine_.params().reverb));
    publishTelemetry();
}

void AAudioPlayer::cycleFilter()
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setFilter(toneflow::cycleFilter(engine_.params().filter));
    publishTelemetry();
}

int AAudioPlayer::presetCount() const
{
    return static_cast<int>(toneflow::presetCount());
}

const char* AAudioPlayer::presetLabel(int index) const
{
    const auto* preset = toneflow::presetByIndex(static_cast<size_t>(index));
    return preset != nullptr ? preset->label : "";
}

const char* AAudioPlayer::presetDescription(int index) const
{
    const auto* preset = toneflow::presetByIndex(static_cast<size_t>(index));
    return preset != nullptr ? preset->description : "";
}

const char* AAudioPlayer::modeLabel() const
{
    return toneflow::beatModeLabel(
        static_cast<toneflow::BeatMode>(telemMode_.load(std::memory_order_relaxed)));
}

const char* AAudioPlayer::perceptionLabel() const
{
    return toneflow::perceptionLabel(static_cast<toneflow::PerceptionMode>(
        telemPerception_.load(std::memory_order_relaxed)));
}

const char* AAudioPlayer::reverbLabel() const
{
    return toneflow::reverbLabel(static_cast<toneflow::ReverbPreset>(
        telemReverb_.load(std::memory_order_relaxed)));
}

const char* AAudioPlayer::filterLabel() const
{
    return toneflow::filterLabel(static_cast<toneflow::FilterType>(
        telemFilter_.load(std::memory_order_relaxed)));
}

float AAudioPlayer::carrierHz() const
{
    return telemCarrier_.load(std::memory_order_relaxed);
}

float AAudioPlayer::beatHz() const
{
    return telemBeat_.load(std::memory_order_relaxed);
}

float AAudioPlayer::currentRms() const
{
    return telemRms_.load(std::memory_order_relaxed);
}

float AAudioPlayer::beatPhase() const
{
    return telemPhase_.load(std::memory_order_relaxed);
}

float AAudioPlayer::micRms() const
{
    return telemMicRms_.load(std::memory_order_relaxed);
}

float AAudioPlayer::micAgcGain() const
{
    return telemMicAgc_.load(std::memory_order_relaxed);
}

bool AAudioPlayer::micFeedbackEnabled() const
{
    return telemMicEn_.load(std::memory_order_relaxed);
}

}  // namespace manticore::android
