#pragma once
#include "AmpModel.h"
#include "Biquad.h"

// Fender Deluxe Reverb '65 (AB763 circuit)
// Key differences from JCM800:
//   - Single 12AX7 preamp stage → much more headroom, clean to light breakup
//   - Fender tone stack: bass at 100 Hz, mid NOT inverted, treble at 2.5 kHz
//   - ch5 = "Bright" cap (treble boost at low volumes, a Fender signature)
//   - 6V6 push-pull power amp → softer, more "bloom" than EL34
//   - 1×12 Jensen P12R cabinet (more mid-forward, less low-end than 4×12)
class FenderDeluxeModel : public AmpModel {
public:
    void setSampleRate(unsigned int sr) override;
    void resetState() noexcept override;
    float process(float in, float preampVol, float bass, float mid,
                  float treble, float ch5, float master) noexcept override;

    const char* name()     const noexcept override { return "Fender Deluxe Reverb '65"; }
    const char* brand()    const noexcept override { return "Fender"; }
    const char* subtitle() const noexcept override { return "22W  Class A/B"; }

    std::array<const char*, 6> knobLabels() const noexcept override {
        return { "VOLUME", "BASS", "MIDDLE", "TREBLE", "BRIGHT", "MASTER\nVOLUME" };
    }
    std::array<float, 6> knobDefaults() const noexcept override {
        // Fender sounds best with treble up and master high (it's a single-channel amp)
        return { 5.0f, 5.0f, 5.0f, 6.5f, 3.0f, 7.0f };
    }

private:
    void rebuildTone(float bass, float mid, float treble, float bright);

    // 6V6 tweed-style cubic soft clip — smoother, more second-harmonic than JCM800
    static float tubeClip(float x) noexcept {
        if (x >  1.0f) return  2.0f / 3.0f;
        if (x < -1.0f) return -2.0f / 3.0f;
        return x - x * x * x / 3.0f;
    }

    unsigned int m_sr = 44100;
    Biquad m_dc;
    Biquad m_bass, m_mid, m_treble, m_bright;
    Biquad m_cabHP, m_cabLP, m_cabP1, m_cabP2;
    float  m_cBass = -99, m_cMid = -99, m_cTreble = -99, m_cBright = -99;
};
