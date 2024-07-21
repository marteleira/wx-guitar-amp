#pragma once
#include "AmpModel.h"
#include "Biquad.h"

// Vox AC30 Top Boost (circa 1962)
// Key differences:
//   - Class A operation (4x EL84) — tubes always conducting, no crossover distortion
//   - Symmetric-ish clipping with subtle even harmonics → the "chime"
//   - Tone stack: Bass + Treble only (no middle), simpler than Marshall
//   - CUT control (ch5): INVERTED passive high-cut — higher value = darker tone
//   - 2x12 Celestion Alnico Blue cabinet: sparkly, less bass, strong 1.5-3kHz
class VoxAC30Model : public AmpModel {
public:
    void setSampleRate(unsigned int sr) override;
    void resetState() noexcept override;
    float process(float in, float preampVol, float bass, float mid,
                  float treble, float ch5, float master) noexcept override;

    const char* name()     const noexcept override { return "Vox AC30 Top Boost"; }
    const char* brand()    const noexcept override { return "Vox"; }
    const char* subtitle() const noexcept override { return "30W  Class A  EL84"; }

    std::array<const char*, 6> knobLabels() const noexcept override {
        // ch5 = CUT (inverted), mid param = TREBLE, treble param = CUT
        return { "VOLUME", "BASS", "TREBLE", "CUT", "BRIGHT", "MASTER\nVOLUME" };
    }
    std::array<float, 6> knobDefaults() const noexcept override {
        // Low CUT (=bright), treble slightly up — classic Vox jangle
        return { 5.0f, 4.0f, 6.5f, 2.5f, 4.0f, 6.0f };
    }

private:
    // param mapping: preampVol=vol, bass=bass, mid=treble, treble=cut, ch5=bright
    void rebuildTone(float bass, float trebleCtrl, float cutCtrl, float bright);

    // Class A EL84 character: slight asymmetric bias, less aggressive than JCM800
    static float voxClip(float x) noexcept {
        float b = x + 0.04f;  // small class-A bias → subtle even harmonics
        return b / (1.0f + (b > 0 ? b : -b)) - 0.04f / 1.04f;
    }

    unsigned int m_sr = 44100;
    Biquad m_dc[2];
    Biquad m_bass, m_treble, m_cut, m_bright;
    Biquad m_cabHP, m_cabLP, m_cabP1, m_cabP2;
    float  m_cBass = -99, m_cTreble = -99, m_cCut = -99, m_cBright = -99;
};
