#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include "dsp/AmpModel.h"

struct AmpParams {
    std::atomic<float> preampVol{5.0f};
    std::atomic<float> bass{5.0f};
    std::atomic<float> mid{5.0f};
    std::atomic<float> treble{5.0f};
    std::atomic<float> presence{5.0f};
    std::atomic<float> master{5.0f};

    std::atomic<float> inputGain{1.0f};   // linear, 0.0–2.0
    std::atomic<float> outputGain{1.0f};
};

struct DeviceInfo {
    std::string name;
    int  index;       // position in the enumerated list
    bool isDefault;
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

    // Device enumeration — call after init()
    std::vector<DeviceInfo> enumerateInputDevices();
    std::vector<DeviceInfo> enumerateOutputDevices();

    // Select device by index from the enumerated list (-1 = system default)
    void setInputDevice(int index);
    void setOutputDevice(int index);

    // Restart audio engine with current device selection
    bool restart();

    // Ring-buffer access for spectrum analysis (lock-free, safe to call from UI thread)
    static constexpr int RING_SIZE = 4096;
    void copyInputSamples(float* dst, int n) const;
    void copyOutputSamples(float* dst, int n) const;

    // Peak levels written by audio thread, read by UI (dBFS)
    float inputPeakDb()  const;
    float outputPeakDb() const;

    // Amp model switching (thread-safe)
    void      setAmpModel(int index);
    int       currentAmpModel() const;
    AmpModel& ampModel(int index);

    void processAudio(float* out, const float* in, unsigned int frameCount);

    AmpParams params;

private:
    bool initDevice();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::unique_ptr<DspProcessor> m_dsp;
    bool         m_running    = false;
    unsigned int m_sampleRate = 44100;
};
