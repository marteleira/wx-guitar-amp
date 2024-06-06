#pragma once
#include "Biquad.h"

// JCM800 2203 signal chain:
//   input → DC block → preamp stage 1 (gain+clip) → DC block
//          → preamp stage 2 (gain+clip) → DC block
//          → tone stack (bass/mid/treble) → presence shelf
//          → master vol → power amp sim → cab sim → output
class DspProcessor {
public:
    void setSampleRate(unsigned int sr);

    float process(float in,
                  float preampVol, float bass, float mid,
                  float treble, float presence, float master) noexcept;

private:
    void rebuildToneFilters(float bass, float mid, float treble, float presence);

    // Tube-style asymmetric soft clipper (slight bias → even harmonics)
    static inline float tubeClip(float x) noexcept {
        float biased = x + 0.07f;
        // x / (1+|x|) is cheap and smooth; preserves dynamics better than tanh at low levels
        return biased / (1.0f + (biased > 0 ? biased : -biased)) - 0.07f / 1.07f;
    }

    unsigned int m_sr = 44100;

    Biquad m_dcBlock[3];   // one per stage boundary
    Biquad m_bass, m_mid, m_treble, m_presence;
    Biquad m_cabHP, m_cabLP, m_cabPeak1, m_cabPeak2;

    // Cached param values so we only rebuild filters when something changes
    float m_cBass = -99, m_cMid = -99, m_cTreble = -99, m_cPresence = -99;
};
