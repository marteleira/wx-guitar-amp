#pragma once
#include "AmpModel.h"
#include "Biquad.h"

// Orange OR120 (late 1960s/early 1970s)
// Key differences:
//   - Cathode follower input buffer → different harmonic character (more asymmetric)
//   - 2× 12AX7 preamp + gain boost switch (ch5) → more dynamic, less compressed than JCM800
//   - Orange British tone stack: mid NOT inverted, centred higher (900 Hz)
//   - 4× EL34 at 120W → more headroom, softer power-amp saturation curve
//   - 4×12 with Celestion Vintage 30 → strong 850 Hz "honk", scooped low-mid
class OrangeOR120Model : public AmpModel {
public:
    void setSampleRate(unsigned int sr) override;
    void resetState() noexcept override;
    float process(float in, float preampVol, float bass, float mid,
                  float treble, float ch5, float master) noexcept override;

    const char* name()     const noexcept override { return "Orange OR120"; }
    const char* brand()    const noexcept override { return "Orange"; }
    const char* subtitle() const noexcept override { return "120W  4\xC3\x97 EL34"; }

    std::array<const char*, 6> knobLabels() const noexcept override {
        return { "VOLUME", "BASS", "MIDDLE", "TREBLE", "TOP\nBOOST", "MASTER\nVOLUME" };
    }
    std::array<float, 6> knobDefaults() const noexcept override {
        // Orange likes mids up — that IS the Orange sound
        return { 5.0f, 5.0f, 6.5f, 5.0f, 4.0f, 5.0f };
    }

private:
    void rebuildTone(float bass, float mid, float treble, float topBoost);

    // Cathode-follower-influenced asymmetric clip:
    // positive half clips softer (valve breathes), negative harder
    static float orangeClip(float x) noexcept {
        return x > 0.0f
            ? x / (1.0f + x * 0.75f)   // soft positive (even harmonics)
            : x / (1.0f - x * 1.1f);   // slightly harder negative
    }

    unsigned int m_sr = 44100;
    Biquad m_dc[2];
    Biquad m_bass, m_mid, m_treble, m_topBoost;
    Biquad m_cabHP, m_cabLP, m_cabHonk, m_cabDip, m_cabPres;
    float  m_cBass = -99, m_cMid = -99, m_cTreble = -99, m_cTop = -99;
};
