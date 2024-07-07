#include "OrangeOR120Model.h"
#include <cmath>

void OrangeOR120Model::setSampleRate(unsigned int sr) {
    m_sr = sr;
    double s = sr;
    for (auto& f : m_dc) f = Biquad::highPass(10.0, 0.7, s);

    // Celestion Vintage 30 4×12 cabinet response
    m_cabHP   = Biquad::highPass(80.0,   0.7,  s);
    m_cabLP   = Biquad::lowPass (7000.0, 0.6,  s);   // V30 rolls off around 7kHz
    m_cabHonk = Biquad::peaking (850.0,  4.5, 2.0, s); // the V30 "honk" — very Orange
    m_cabDip  = Biquad::peaking (250.0, -2.0, 1.2, s); // V30 low-mid suck-out
    m_cabPres = Biquad::peaking (3500.0, 3.0, 1.4, s); // V30 upper-mid cut-through

    m_cBass = m_cMid = m_cTreble = m_cTop = -99;
}

void OrangeOR120Model::resetState() noexcept {
    for (auto& f : m_dc) f.resetState();
    m_bass.resetState(); m_mid.resetState(); m_treble.resetState(); m_topBoost.resetState();
    m_cabHP.resetState(); m_cabLP.resetState(); m_cabHonk.resetState();
    m_cabDip.resetState(); m_cabPres.resetState();
    m_cBass = m_cMid = m_cTreble = m_cTop = -99;
}

void OrangeOR120Model::rebuildTone(float bass, float mid, float treble, float topBoost) {
    double s = m_sr;

    // Orange British tone stack — mid sits higher and is NOT inverted
    Biquad nb  = Biquad::lowShelf  (150.0,  (bass    - 5.0) * 2.4,       s);
    Biquad nm  = Biquad::peaking   (900.0,  (mid     - 5.0) * 2.0, 0.8,  s); // not inverted
    Biquad nt  = Biquad::highShelf (2500.0, (treble  - 5.0) * 2.4,       s);
    Biquad ntb = Biquad::highShelf (6000.0,  topBoost        * 0.9,       s); // 0 → +9 dB air

    nb.preserveStateFrom(m_bass);       m_bass     = nb;
    nm.preserveStateFrom(m_mid);        m_mid      = nm;
    nt.preserveStateFrom(m_treble);     m_treble   = nt;
    ntb.preserveStateFrom(m_topBoost);  m_topBoost = ntb;

    m_cBass = bass; m_cMid = mid; m_cTreble = treble; m_cTop = topBoost;
}

float OrangeOR120Model::process(float in,
                                float preampVol, float bass, float mid,
                                float treble, float ch5, float master) noexcept {
    if (bass != m_cBass || mid != m_cMid || treble != m_cTreble || ch5 != m_cTop)
        rebuildTone(bass, mid, treble, ch5);

    float norm = preampVol / 10.0f;

    // ch5 (Top Boost) adds an extra gain stage simulation ("High" input on the real amp)
    float boostGain = 1.0f + (ch5 / 10.0f) * 3.5f;

    // Stage 1: cathode follower into first triode
    // Lower base gain than JCM800 — this amp relies more on power amp breakup
    float g1 = (1.0f + norm * norm * 55.0f) * boostGain;
    float s1 = m_dc[0].process(in * g1);
    s1 = orangeClip(s1);

    // Stage 2: second triode — also asymmetric
    float g2 = 1.0f + norm * norm * 22.0f;
    float s2 = m_dc[1].process(s1 * g2);
    s2 = orangeClip(s2);

    float pre = s2 * 0.09f;

    // Tone stack (mid NOT inverted — the Orange character lives here)
    float t = m_topBoost.process(m_treble.process(m_mid.process(m_bass.process(pre))));

    // 4× EL34 @ 120W — more headroom, softer knee, more complex saturation
    float masterN = master / 10.0f;
    float drive   = 1.1f + masterN * 1.7f;  // gentler start than JCM800
    float pwr     = std::tanh(t * drive) / std::tanh(drive);

    // Subtle class-AB crossover character (4 tubes, slight mismatch between push/pull)
    pwr += 0.015f * (pwr > 0 ? 1.0f : -1.0f) * (1.0f - std::abs(pwr));

    // Vintage 30 4×12 cabinet
    float out = m_cabHP.process(pwr);
    out = m_cabDip.process(out);   // low-mid suck-out first
    out = m_cabHonk.process(out);  // then the 850 Hz honk
    out = m_cabLP.process(out);
    out = m_cabPres.process(out);

    return out * masterN;
}
