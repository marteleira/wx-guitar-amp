#include "DspProcessor.h"

void DspProcessor::setSampleRate(unsigned int sr) {
    for (auto* m : m_models) m->setSampleRate(sr);
}

void DspProcessor::setModel(int index) {
    if (index >= 0 && index < MODEL_COUNT)
        m_current.store(index, std::memory_order_relaxed);
}

float DspProcessor::process(float in, float preampVol, float bass, float mid,
                            float treble, float ch5, float master) noexcept {
    int idx = m_current.load(std::memory_order_relaxed);

    // On model switch: reset new model's filter state from audio thread (no allocation, safe)
    if (idx != m_last) {
        m_models[idx]->resetState();
        m_last = idx;
    }

    return m_models[idx]->process(in, preampVol, bass, mid, treble, ch5, master);
}
