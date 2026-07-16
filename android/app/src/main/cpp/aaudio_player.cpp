#include "aaudio_player.h"

#include <android/log.h>
#include <algorithm>
#include <cstring>

#include "manticore/toneflow/presets.h"

#define LOG_TAG "ManticoreToneFlow"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace manticore::android {

AAudioPlayer::AAudioPlayer() = default;

AAudioPlayer::~AAudioPlayer() { stop(); }

bool AAudioPlayer::openOutputStream()
{
    AAudioStreamBuilder* builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGE("output builder failed: %s", AAudio_convertResultToText(result));
        return false;
    }

    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setSampleRate(builder, 48000);
    AAudioStreamBuilder_setChannelCount(builder, 2);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setPerformanceMode(builder,
                                           AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    // Prefer media usage so Bluetooth / headphones route correctly.
    AAudioStreamBuilder_setUsage(builder, AAUDIO_USAGE_MEDIA);
    AAudioStreamBuilder_setContentType(builder, AAUDIO_CONTENT_TYPE_MUSIC);
    AAudioStreamBuilder_setDataCallback(builder, dataCallback, this);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, this);

    result = AAudioStreamBuilder_openStream(builder, &outputStream_);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK || outputStream_ == nullptr) {
        LOGE("open output failed: %s", AAudio_convertResultToText(result));
        outputStream_ = nullptr;
        return false;
    }

    sampleRate_ = AAudioStream_getSampleRate(outputStream_);
    framesPerBurst_ = AAudioStream_getFramesPerBurst(outputStream_);
    if (framesPerBurst_ <= 0) {
        framesPerBurst_ = 192;
    }

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
        engine_.setPlaying(true);
    }

    LOGI("output opened: sr=%d burst=%d", sampleRate_, framesPerBurst_);
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
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setSampleRate(builder, sampleRate_);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setPerformanceMode(builder,
                                           AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setInputPreset(builder,
                                       AAUDIO_INPUT_PRESET_VOICE_PERFORMANCE);

    result = AAudioStreamBuilder_openStream(builder, &inputStream_);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK || inputStream_ == nullptr) {
        LOGE("open input failed: %s", AAudio_convertResultToText(result));
        inputStream_ = nullptr;
        return false;
    }

    inputChannels_ = AAudioStream_getChannelCount(inputStream_);
    if (inputChannels_ <= 0) {
        inputChannels_ = 1;
    }

    result = AAudioStream_requestStart(inputStream_);
    if (result != AAUDIO_OK) {
        LOGE("start input failed: %s", AAudio_convertResultToText(result));
        AAudioStream_close(inputStream_);
        inputStream_ = nullptr;
        return false;
    }

    LOGI("input opened: ch=%d sr=%d", inputChannels_,
         AAudioStream_getSampleRate(inputStream_));
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

void AAudioPlayer::ensureMicBuffer(int32_t numFrames)
{
    const size_t need =
        static_cast<size_t>(numFrames) * static_cast<size_t>(inputChannels_);
    if (micBuffer_.size() < need) {
        micBuffer_.assign(need, 0.0f);
    }
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

aaudio_data_callback_result_t AAudioPlayer::dataCallback(AAudioStream*,
                                                         void* userData,
                                                         void* audioData,
                                                         int32_t numFrames)
{
    auto* self = static_cast<AAudioPlayer*>(userData);
    auto* out = static_cast<float*>(audioData);

    const float* micPtr = nullptr;
    uint32_t micCh = 0;

    if (self->micWanted_.load() && self->inputStream_ != nullptr) {
        self->ensureMicBuffer(numFrames);
        const aaudio_result_t readResult = AAudioStream_read(
            self->inputStream_, self->micBuffer_.data(), numFrames, 0);
        if (readResult >= 0) {
            const int32_t got = readResult;
            if (got < numFrames) {
                // Pad underrun with silence.
                const size_t start =
                    static_cast<size_t>(got) *
                    static_cast<size_t>(self->inputChannels_);
                const size_t end =
                    static_cast<size_t>(numFrames) *
                    static_cast<size_t>(self->inputChannels_);
                std::fill(self->micBuffer_.begin() +
                              static_cast<std::ptrdiff_t>(start),
                          self->micBuffer_.begin() +
                              static_cast<std::ptrdiff_t>(end),
                          0.0f);
            }
            micPtr = self->micBuffer_.data();
            micCh = static_cast<uint32_t>(self->inputChannels_);
        }
    }

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
