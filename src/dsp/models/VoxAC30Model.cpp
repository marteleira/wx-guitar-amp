#include "VoxAC30Model.h"
#include <cmath>

void VoxAC30Model::setSampleRate(unsigned int sr) {
    m_sr = sr;
    double s = sr;
    for (auto& f : m_dc) f = Biquad::highPass(10.0, 0.7, s);

    // Celestion Alnico Blue 2x12 cabinet
    // The Blue is the "chime" speaker — sparkly mids, rolls off cleanly, less bass than V30
    m_cabHP = Biquad::highPass(80.0,   0.7,  s);
    m_cabLP = Biquad::lowPass (6000.0, 0.6,  s);    // Blue rolls off earlier than V30
    m_cabP1 = Biquad::peaking (1500.0, 3.5, 1.8, s); // the Celestion Blue "chime" band
    m_cabP2 = Biquad::peaking (3000.0, 2.5, 1.4, s); // upper-mid sparkle

    m_cBass = m_cTreble = m_cCut = m_cBright = -99;
}

void VoxAC30Model::resetState() noexcept {
    for (auto& f : m_dc) f.resetState();
    m_bass.resetState(); m_treble.resetState(); m_cut.resetState(); m_bright.resetState();
    m_cabHP.resetState(); m_cabLP.resetState(); m_cabP1.resetState(); m_cabP2.resetState();
    m_cBass = m_cTreble = m_cCut = m_cBright = -99;
}

void VoxAC30Model::rebuildTone(float bass, float trebleCtrl, float cutCtrl, float bright) {
    double s = m_sr;

    // Bass: low shelf, sits a bit higher than Fender (Vox bass is warmer, not as deep)
    Biquad nb = Biquad::lowShelf (300.0,  (bass       - 5.0) * 2.0,       s);

    // Treble: high shelf — Vox Top Boost treble has a lot of presence
    Biquad nt = Biquad::highShelf(2000.0, (trebleCtrl - 5.0) * 2.4,       s);

    // CUT: inverted passive high-cut — Vox signature. Higher value = more cut = darker.
    // Cutoff range: ~12kHz (cut=0, wide open) down to ~600Hz (cut=10, very dark)
    double cutNorm = cutCtrl / 10.0;
    double cutFc   = 12000.0 * std::pow(600.0 / 12000.0, cutNorm);
    Biquad nc = Biquad::lowPass(cutFc, 0.65, s);

    // Bright: high shelf simulating the Top Boost circuit character
    Biquad nbr = Biquad::highShelf(5000.0, bright * 0.7, s);  // 0 → +7 dB

    nb.preserveStateFrom(m_bass);    m_bass   = nb;
    nt.preserveStateFrom(m_treble);  m_treble = nt;
    nc.preserveStateFrom(m_cut);     m_cut    = nc;
    nbr.preserveStateFrom(m_bright); m_bright = nbr;

    m_cBass = bass; m_cTreble = trebleCtrl; m_cCut = cutCtrl; m_cBright = bright;
}

float VoxAC30Model::process(float in,
                            float preampVol, float bass, float mid,
                            float treble, float ch5, float master) noexcept {
    // param mapping: mid → treble control, treble → cut control
    float trebleCtrl = mid;
    float cutCtrl    = treble;
    float bright     = ch5;

    if (bass != m_cBass || trebleCtrl != m_cTreble || cutCtrl != m_cCut || bright != m_cBright)
        rebuildTone(bass, trebleCtrl, cutCtrl, bright);

    float norm = preampVol / 10.0f;

    // Two 12AX7 stages (Top Boost channel has more gain than Normal channel)
    // Gain is moderate — AC30 is known for dynamic breakup, not extreme gain
    float g1 = 1.0f + norm * norm * 60.0f;
    float s1 = m_dc[0].process(in * g1);
    s1 = voxClip(s1);

    float g2 = 1.0f + norm * norm * 25.0f;
    float s2 = m_dc[1].process(s1 * g2);
    s2 = voxClip(s2);

    float pre = s2 * 0.07f;

    // Vox Top Boost tone stack: bass → treble → cut → bright
    float t = m_bright.process(m_cut.process(m_treble.process(m_bass.process(pre))));

    // 4x EL84 Class A power amp
    // Class A = no crossover, very soft clipping onset, rich harmonic content
    // Lower drive range than JCM800 (30W vs 100W, and class A clips earlier)
    float masterN = master / 10.0f;
    float drive   = 1.4f + masterN * 1.6f;
    float pwr     = std::tanh(t * drive) / std::tanh(drive);

    // Celestion Alnico Blue 2x12
    float out = m_cabHP.process(pwr);
    out = m_cabP1.process(out);   // chime
    out = m_cabP2.process(out);   // sparkle
    out = m_cabLP.process(out);

    return out * masterN;
}
