#pragma once
#include "JCM800Model.h"
#include "FenderDeluxeModel.h"
#include <atomic>

class DspProcessor {
public:
    static constexpr int MODEL_COUNT = 2;

    void setSampleRate(unsigned int sr);

    // Thread-safe: called from UI thread
    void setModel(int index);
    int  currentModel() const { return m_current.load(std::memory_order_relaxed); }

    // Called from audio thread only
    float process(float in, float preampVol, float bass, float mid,
                  float treble, float ch5, float master) noexcept;

    AmpModel& model(int index) { return *m_models[index]; }

private:
    JCM800Model       m_jcm800;
    FenderDeluxeModel m_fender;
    AmpModel* m_models[MODEL_COUNT] = { &m_jcm800, &m_fender };

    std::atomic<int> m_current{0};
    int m_last = 0;  // audio thread only — no atomic needed
};
