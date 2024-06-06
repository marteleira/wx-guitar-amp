#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "AudioEngine.h"
#include "dsp/DspProcessor.h"
#include <cstring>

struct AudioEngine::Impl {
    ma_device device{};
};

AudioEngine::AudioEngine()
    : m_impl(std::make_unique<Impl>())
    , m_dsp(std::make_unique<DspProcessor>())
{}

AudioEngine::~AudioEngine() {
    shutdown();
}

void AudioEngine::processAudio(float* out, const float* in, unsigned int frameCount) {
    float preampVol = params.preampVol.load(std::memory_order_relaxed);
    float bass      = params.bass.load(std::memory_order_relaxed);
    float mid       = params.mid.load(std::memory_order_relaxed);
    float treble    = params.treble.load(std::memory_order_relaxed);
    float presence  = params.presence.load(std::memory_order_relaxed);
    float master    = params.master.load(std::memory_order_relaxed);

    for (unsigned int i = 0; i < frameCount; ++i) {
        float sample = in ? in[i] : 0.0f;
        float processed = m_dsp->process(sample, preampVol, bass, mid, treble, presence, master);
        out[i * 2 + 0] = processed;
        out[i * 2 + 1] = processed;
    }
}

bool AudioEngine::init() {
    ma_device_config config = ma_device_config_init(ma_device_type_duplex);
    config.capture.format    = ma_format_f32;
    config.capture.channels  = 1;
    config.playback.format   = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate        = m_sampleRate;
    config.performanceProfile = ma_performance_profile_low_latency;
    config.pUserData         = this;
    config.dataCallback = [](ma_device* dev, void* out, const void* in, ma_uint32 frames) {
        static_cast<AudioEngine*>(dev->pUserData)->processAudio(
            static_cast<float*>(out),
            static_cast<const float*>(in),
            frames
        );
    };

    if (ma_device_init(nullptr, &config, &m_impl->device) != MA_SUCCESS)
        return false;

    m_sampleRate = m_impl->device.sampleRate;
    m_dsp->setSampleRate(m_sampleRate);

    if (ma_device_start(&m_impl->device) != MA_SUCCESS) {
        ma_device_uninit(&m_impl->device);
        return false;
    }

    m_running = true;
    return true;
}

void AudioEngine::shutdown() {
    if (m_running) {
        ma_device_uninit(&m_impl->device);
        m_running = false;
    }
}
