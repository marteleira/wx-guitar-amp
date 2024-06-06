#pragma once
#include <atomic>
#include <memory>

struct AmpParams {
    std::atomic<float> preampVol{5.0f};
    std::atomic<float> bass{5.0f};
    std::atomic<float> mid{5.0f};
    std::atomic<float> treble{5.0f};
    std::atomic<float> presence{5.0f};
    std::atomic<float> master{5.0f};
};

class DspProcessor;

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool init();
    void shutdown();
    bool isRunning() const { return m_running; }
    unsigned int getSampleRate() const { return m_sampleRate; }

    void processAudio(float* out, const float* in, unsigned int frameCount);

    AmpParams params;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::unique_ptr<DspProcessor> m_dsp;
    bool m_running = false;
    unsigned int m_sampleRate = 44100;
};
