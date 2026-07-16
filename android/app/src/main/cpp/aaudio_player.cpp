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

void AAudioPlayer::minimizeBuffer(AAudioStream* stream)
{
    if (stream == nullptr) {
        return;
    }
    const int32_t burst = AAudioStream_getFramesPerBurst(stream);
    if (burst <= 0) {
        return;
    }
    // One burst is the lowest stable size on most devices.
    const aaudio_result_t setResult =
        AAudioStream_setBufferSizeInFrames(stream, burst);
    const int32_t actual = AAudioStream_getBufferSizeInFrames(stream);
    LOGI("buffer minimize: burst=%d set=%s actual=%d", burst,
         AAudio_convertResultToText(setResult), actual);
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
    AAudioStreamBuilder_setPerformanceMode(builder,
                                           AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    // Game usage tends to prefer the fast mixer path on many OEMs.
    AAudioStreamBuilder_setUsage(builder, AAUDIO_USAGE_GAME);
    AAudioStreamBuilder_setContentType(builder, AAUDIO_CONTENT_TYPE_MUSIC);
    // Keep callbacks small and regular.
    AAudioStreamBuilder_setDataCallback(builder, outputCallback, this);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, this);
    // Ask for small callbacks; device may round to its burst.
    AAudioStreamBuilder_setFramesPerDataCallback(builder, 64);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, 256);

    if (!tryOpenWithSharing(builder, &outputStream_,
                            AAUDIO_SHARING_MODE_EXCLUSIVE) &&
        !tryOpenWithSharing(builder, &outputStream_,
                            AAUDIO_SHARING_MODE_SHARED)) {
        // Retry without capacity hints if the OEM rejects them.
        AAudioStreamBuilder_setFramesPerDataCallback(builder, 0);
        AAudioStreamBuilder_setBufferCapacityInFrames(builder, 0);
        if (!tryOpenWithSharing(builder, &outputStream_,
                                AAUDIO_SHARING_MODE_SHARED)) {
            LOGE("open output failed");
            AAudioStreamBuilder_delete(builder);
            outputStream_ = nullptr;
            return false;
        }
    }
    AAudioStreamBuilder_delete(builder);

    sampleRate_ = AAudioStream_getSampleRate(outputStream_);
    framesPerBurst_ = AAudioStream_getFramesPerBurst(outputStream_);
    if (framesPerBurst_ <= 0) {
        framesPerBurst_ = 96;
    }
    minimizeBuffer(outputStream_);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto prior = engine_.params();
        const bool micEn = engine_.micFeedbackEnabled();
        const float micGain = engine_.micFeedbackGain();
        const float toneMix = engine_.toneMix();
        engine_.prepare(static_cast<uint32_t>(sampleRate_),
                        static_cast<uint32_t>(framesPerBurst_));
        engine_.setParams(prior);
        engine_.setMicFeedbackEnabled(micEn);
        engine_.setMicFeedbackGain(micGain);
        engine_.setToneMix(toneMix);
        engine_.setMicLowLatency(true);
        engine_.setPlaying(true);
    }

    LOGI("output opened: sr=%d burst=%d sharing=%d", sampleRate_,
         framesPerBurst_, AAudioStream_getSharingMode(outputStream_));
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
                                           AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    // Unprocessed / recognition paths avoid telephony AEC/NS delay.
    AAudioStreamBuilder_setInputPreset(builder,
                                       AAUDIO_INPUT_PRESET_UNPROCESSED);
    AAudioStreamBuilder_setFramesPerDataCallback(builder, framesPerBurst_);
    AAudioStreamBuilder_setBufferCapacityInFrames(builder, framesPerBurst_ * 2);
    AAudioStreamBuilder_setDataCallback(builder, inputCallback, this);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, this);

    if (!tryOpenWithSharing(builder, &inputStream_,
                            AAUDIO_SHARING_MODE_EXCLUSIVE)) {
        LOGI("exclusive input unavailable, falling back to shared");
        AAudioStreamBuilder_setInputPreset(builder,
                                           AAUDIO_INPUT_PRESET_VOICE_RECOGNITION);
        if (!tryOpenWithSharing(builder, &inputStream_,
                                AAUDIO_SHARING_MODE_SHARED)) {
            LOGE("open input failed");
            AAudioStreamBuilder_delete(builder);
            inputStream_ = nullptr;
            return false;
        }
    }
    AAudioStreamBuilder_delete(builder);

    inputChannels_ = AAudioStream_getChannelCount(inputStream_);
    if (inputChannels_ <= 0) {
        inputChannels_ = 1;
    }
    minimizeBuffer(inputStream_);

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

    // Keep lock hold short: render under lock (engine not thread-safe yet).
    std::lock_guard<std::mutex> lock(self->mutex_);
    self->engine_.renderInterleavedWithMic(out, micPtr,
                                           static_cast<uint32_t>(numFrames),
                                           micCh);
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
}

void AAudioPlayer::setBeatHz(float hz)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setBeatHz(hz);
}

void AAudioPlayer::setMode(int mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setMode(static_cast<toneflow::BeatMode>(mode));
}

void AAudioPlayer::setPerception(int mode)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setPerception(static_cast<toneflow::PerceptionMode>(mode));
}

void AAudioPlayer::setReverb(int preset)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setReverb(static_cast<toneflow::ReverbPreset>(preset));
}

void AAudioPlayer::setFilter(int type)
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setFilter(static_cast<toneflow::FilterType>(type));
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
            // Re-minimize buffers when enabling live path.
            minimizeBuffer(outputStream_);
            minimizeBuffer(inputStream_);
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
}

void AAudioPlayer::cyclePerception()
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setPerception(toneflow::cyclePerception(engine_.params().perception));
}

void AAudioPlayer::cycleReverb()
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setReverb(toneflow::cycleReverb(engine_.params().reverb));
}

void AAudioPlayer::cycleFilter()
{
    std::lock_guard<std::mutex> lock(mutex_);
    engine_.setFilter(toneflow::cycleFilter(engine_.params().filter));
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
    std::lock_guard<std::mutex> lock(mutex_);
    return toneflow::beatModeLabel(engine_.params().mode);
}

const char* AAudioPlayer::perceptionLabel() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return toneflow::perceptionLabel(engine_.params().perception);
}

const char* AAudioPlayer::reverbLabel() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return toneflow::reverbLabel(engine_.params().reverb);
}

const char* AAudioPlayer::filterLabel() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return toneflow::filterLabel(engine_.params().filter);
}

float AAudioPlayer::carrierHz() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return engine_.carrierHz();
}

float AAudioPlayer::beatHz() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return engine_.beatHz();
}

float AAudioPlayer::currentRms() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return engine_.currentRms();
}

float AAudioPlayer::beatPhase() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return engine_.beatPhase();
}

float AAudioPlayer::micRms() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return engine_.micRms();
}

float AAudioPlayer::micAgcGain() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return engine_.micAgcGain();
}

bool AAudioPlayer::micFeedbackEnabled() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return engine_.micFeedbackEnabled();
}

}  // namespace manticore::android
