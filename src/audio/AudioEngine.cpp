#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "AudioEngine.h"
#include "dsp/DspProcessor.h"
#include <array>
#include <cmath>
#include <cstring>

struct AudioEngine::Impl {
    ma_context context{};
    bool       contextOk = false;

    ma_device  device{};

    // Copied from context after enumeration
    std::vector<ma_device_info> captureInfos;
    std::vector<ma_device_info> playbackInfos;

    int selectedCapture  = -1;   // -1 = system default
    int selectedPlayback = -1;

    // Lock-free ring buffers for spectrum view
    static constexpr unsigned int RING = AudioEngine::RING_SIZE;
    std::array<float, RING> inputRing{};
    std::array<float, RING> outputRing{};
    std::atomic<unsigned int> ringWritePos{0};

    // Peak hold (exponential decay, written by audio thread)
    std::atomic<float> inputPeak{0.0f};
    std::atomic<float> outputPeak{0.0f};
};

// Construction

AudioEngine::AudioEngine()
    : m_impl(std::make_unique<Impl>())
    , m_dsp(std::make_unique<DspProcessor>())
{}

AudioEngine::~AudioEngine() {
    shutdown();
}

// Device helpers 

bool AudioEngine::initDevice() {
    ma_device_config cfg = ma_device_config_init(ma_device_type_duplex);
    cfg.capture.format    = ma_format_f32;
    cfg.capture.channels  = 1;
    cfg.playback.format   = ma_format_f32;
    cfg.playback.channels = 2;
    cfg.sampleRate        = m_sampleRate;
    cfg.performanceProfile = ma_performance_profile_low_latency;
    cfg.pUserData         = this;
    cfg.dataCallback = [](ma_device* dev, void* out, const void* in, ma_uint32 frames) {
        static_cast<AudioEngine*>(dev->pUserData)->processAudio(
            static_cast<float*>(out), static_cast<const float*>(in), frames);
    };

    if (m_impl->selectedCapture >= 0 &&
        m_impl->selectedCapture < (int)m_impl->captureInfos.size())
        cfg.capture.pDeviceID = &m_impl->captureInfos[m_impl->selectedCapture].id;

    if (m_impl->selectedPlayback >= 0 &&
        m_impl->selectedPlayback < (int)m_impl->playbackInfos.size())
        cfg.playback.pDeviceID = &m_impl->playbackInfos[m_impl->selectedPlayback].id;

    if (ma_device_init(&m_impl->context, &cfg, &m_impl->device) != MA_SUCCESS)
        return false;

    m_sampleRate = m_impl->device.sampleRate;
    m_dsp->setSampleRate(m_sampleRate);

    if (ma_device_start(&m_impl->device) != MA_SUCCESS) {
        ma_device_uninit(&m_impl->device);
        return false;
    }
    return true;
}

bool AudioEngine::init() {
    if (ma_context_init(nullptr, 0, nullptr, &m_impl->context) != MA_SUCCESS)
        return false;
    m_impl->contextOk = true;

    // Enumerate once at startup so we have the lists ready
    enumerateInputDevices();
    enumerateOutputDevices();

    if (!initDevice()) {
        ma_context_uninit(&m_impl->context);
        m_impl->contextOk = false;
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
    if (m_impl->contextOk) {
        ma_context_uninit(&m_impl->context);
        m_impl->contextOk = false;
    }
}

// Device enumeration

std::vector<DeviceInfo> AudioEngine::enumerateInputDevices() {
    if (!m_impl->contextOk) return {};

    ma_device_info* pInfos = nullptr;
    ma_uint32 count = 0;
    if (ma_context_get_devices(&m_impl->context, nullptr, nullptr, &pInfos, &count) != MA_SUCCESS)
        return {};

    m_impl->captureInfos.assign(pInfos, pInfos + count);

    std::vector<DeviceInfo> result;
    result.reserve(count);
    for (ma_uint32 i = 0; i < count; ++i)
        result.push_back({ m_impl->captureInfos[i].name, (int)i, m_impl->captureInfos[i].isDefault != 0 });
    return result;
}

std::vector<DeviceInfo> AudioEngine::enumerateOutputDevices() {
    if (!m_impl->contextOk) return {};

    ma_device_info* pInfos = nullptr;
    ma_uint32 count = 0;
    if (ma_context_get_devices(&m_impl->context, &pInfos, &count, nullptr, nullptr) != MA_SUCCESS)
        return {};

    m_impl->playbackInfos.assign(pInfos, pInfos + count);

    std::vector<DeviceInfo> result;
    result.reserve(count);
    for (ma_uint32 i = 0; i < count; ++i)
        result.push_back({ m_impl->playbackInfos[i].name, (int)i, m_impl->playbackInfos[i].isDefault != 0 });
    return result;
}

void AudioEngine::setInputDevice(int index)  { m_impl->selectedCapture  = index; }
void AudioEngine::setOutputDevice(int index) { m_impl->selectedPlayback = index; }

bool AudioEngine::restart() {
    if (m_running) {
        ma_device_uninit(&m_impl->device);
        m_running = false;
    }
    if (!initDevice()) return false;
    m_running = true;
    return true;
}

//  Spectrum / metering access

void AudioEngine::copyInputSamples(float* dst, int n) const {
    unsigned int wpos = m_impl->ringWritePos.load(std::memory_order_acquire);
    unsigned int rpos = wpos - (unsigned int)n;
    for (int i = 0; i < n; ++i)
        dst[i] = m_impl->inputRing[(rpos + i) & (Impl::RING - 1)];
}

void AudioEngine::copyOutputSamples(float* dst, int n) const {
    unsigned int wpos = m_impl->ringWritePos.load(std::memory_order_acquire);
    unsigned int rpos = wpos - (unsigned int)n;
    for (int i = 0; i < n; ++i)
        dst[i] = m_impl->outputRing[(rpos + i) & (Impl::RING - 1)];
}

float AudioEngine::inputPeakDb() const {
    float v = m_impl->inputPeak.load(std::memory_order_relaxed);
    return (v > 1e-7f) ? 20.0f * std::log10(v) : -120.0f;
}

float AudioEngine::outputPeakDb() const {
    float v = m_impl->outputPeak.load(std::memory_order_relaxed);
    return (v > 1e-7f) ? 20.0f * std::log10(v) : -120.0f;
}

// Amp model

void AudioEngine::setAmpModel(int index) { m_dsp->setModel(index); }
int  AudioEngine::currentAmpModel() const { return m_dsp->currentModel(); }
AmpModel& AudioEngine::ampModel(int index) { return m_dsp->model(index); }

// Audio processing (runs in real-time thread)

void AudioEngine::processAudio(float* out, const float* in, unsigned int frameCount) {
    float preampVol = params.preampVol.load(std::memory_order_relaxed);
    float bass      = params.bass.load(std::memory_order_relaxed);
    float mid       = params.mid.load(std::memory_order_relaxed);
    float treble    = params.treble.load(std::memory_order_relaxed);
    float presence  = params.presence.load(std::memory_order_relaxed);
    float master    = params.master.load(std::memory_order_relaxed);
    float inGain    = params.inputGain.load(std::memory_order_relaxed);
    float outGain   = params.outputGain.load(std::memory_order_relaxed);

    unsigned int basePos = m_impl->ringWritePos.load(std::memory_order_relaxed);
    float peakIn = 0.0f, peakOut = 0.0f;

    for (unsigned int i = 0; i < frameCount; ++i) {
        float inSample  = (in ? in[i] : 0.0f) * inGain;
        float outSample = m_dsp->process(inSample, preampVol, bass, mid, treble, presence, master)
                          * outGain;

        // Ring buffers for spectrum
        unsigned int pos = (basePos + i) & (Impl::RING - 1);
        m_impl->inputRing[pos]  = inSample;
        m_impl->outputRing[pos] = outSample;

        // Peak detection
        float ai = inSample  > 0 ? inSample  : -inSample;
        float ao = outSample > 0 ? outSample : -outSample;
        if (ai > peakIn)  peakIn  = ai;
        if (ao > peakOut) peakOut = ao;

        out[i * 2 + 0] = outSample;
        out[i * 2 + 1] = outSample;
    }

    m_impl->ringWritePos.store(basePos + frameCount, std::memory_order_release);

    // Peak hold with exponential decay (~300ms half-life at 60fps)
    constexpr float DECAY = 0.997f;
    auto updatePeak = [DECAY](std::atomic<float>& atom, float newPeak) {
        float prev = atom.load(std::memory_order_relaxed);
        atom.store(newPeak > prev ? newPeak : prev * DECAY, std::memory_order_relaxed);
    };
    updatePeak(m_impl->inputPeak,  peakIn);
    updatePeak(m_impl->outputPeak, peakOut);
}
