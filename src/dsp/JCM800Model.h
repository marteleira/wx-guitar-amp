#pragma once
#include "AmpModel.h"
#include "Biquad.h"

class JCM800Model : public AmpModel {
public:
    void setSampleRate(unsigned int sr) override;
    void resetState() noexcept override;
    float process(float in, float preampVol, float bass, float mid,
                  float treble, float ch5, float master) noexcept override;

    const char* name()     const noexcept override { return "Marshall JCM800 2203"; }
    const char* brand()    const noexcept override { return "Marshall"; }
    const char* subtitle() const noexcept override { return "100W  Super Lead"; }

    std::array<const char*, 6> knobLabels() const noexcept override {
        return { "PREAMP\nVOLUME", "BASS", "MIDDLE", "TREBLE", "PRESENCE", "MASTER\nVOLUME" };
    }
    std::array<float, 6> knobDefaults() const noexcept override {
        return { 5.0f, 5.0f, 5.0f, 5.0f, 5.0f, 4.0f };
    }

private:
    void rebuildTone(float bass, float mid, float treble, float presence);

    static float tubeClip(float x) noexcept {
        float b = x + 0.07f;
        return b / (1.0f + (b > 0 ? b : -b)) - 0.07f / 1.07f;
    }

    unsigned int m_sr = 44100;
    Biquad m_dc[3];
    Biquad m_bass, m_mid, m_treble, m_presence;
    Biquad m_cabHP, m_cabLP, m_cabP1, m_cabP2;
    float  m_cBass = -99, m_cMid = -99, m_cTreble = -99, m_cPres = -99;
};
