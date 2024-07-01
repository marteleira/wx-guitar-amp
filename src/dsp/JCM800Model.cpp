#include "JCM800Model.h"
#include <cmath>

void JCM800Model::setSampleRate(unsigned int sr) {
    m_sr = sr;
    double s = sr;
    for (auto& f : m_dc) f = Biquad::highPass(10.0, 0.7, s);
    m_cabHP = Biquad::highPass(80.0,   0.7,  s);
    m_cabLP = Biquad::lowPass (6500.0, 0.65, s);
    m_cabP1 = Biquad::peaking (2200.0, 4.0, 1.8, s);
    m_cabP2 = Biquad::peaking (4500.0, 2.0, 1.2, s);
    m_cBass = m_cMid = m_cTreble = m_cPres = -99;
}

void JCM800Model::resetState() noexcept {
    for (auto& f : m_dc) f.resetState();
    m_bass.resetState(); m_mid.resetState(); m_treble.resetState(); m_presence.resetState();
    m_cabHP.resetState(); m_cabLP.resetState(); m_cabP1.resetState(); m_cabP2.resetState();
    m_cBass = m_cMid = m_cTreble = m_cPres = -99;
}

void JCM800Model::rebuildTone(float bass, float mid, float treble, float presence) {
    double s = m_sr;
    Biquad nb = Biquad::lowShelf  (200.0,  (bass     - 5.0) * 2.4,       s);
    Biquad nm = Biquad::peaking   (700.0,  (5.0 - mid)      * 2.0, 0.75, s);  // inverted
    Biquad nt = Biquad::highShelf (2200.0, (treble   - 5.0) * 2.4,       s);
    Biquad np = Biquad::highShelf (5000.0,  presence         * 1.2,       s);

    nb.preserveStateFrom(m_bass);     m_bass     = nb;
    nm.preserveStateFrom(m_mid);      m_mid      = nm;
    nt.preserveStateFrom(m_treble);   m_treble   = nt;
    np.preserveStateFrom(m_presence); m_presence = np;

    m_cBass = bass; m_cMid = mid; m_cTreble = treble; m_cPres = presence;
}

float JCM800Model::process(float in,
                           float preampVol, float bass, float mid,
                           float treble, float ch5, float master) noexcept {
    if (bass != m_cBass || mid != m_cMid || treble != m_cTreble || ch5 != m_cPres)
        rebuildTone(bass, mid, treble, ch5);

    float norm = preampVol / 10.0f;

    // Two cascaded 12AX7 stages
    float s1 = m_dc[0].process(in * (1.0f + norm * norm * 85.0f));
    s1 = tubeClip(s1);

    float s2 = m_dc[1].process(s1 * (1.0f + norm * norm * 35.0f));
    s2 = tubeClip(s2);

    float pre = m_dc[2].process(s2 * 0.08f);

    // Tone stack (mid is inverted, rebuilt when params change)
    float t = m_treble.process(m_mid.process(m_bass.process(pre)));
    float sig = m_presence.process(t);

    // EL34 push-pull power amp
    float masterN = master / 10.0f;
    float drive   = 1.5f + masterN * 2.0f;
    float pwr     = std::tanh(sig * drive) / std::tanh(drive);

    // 4×12 Marshall closed-back cabinet
    float out = m_cabHP.process(pwr);
    out = m_cabP1.process(out);
    out = m_cabLP.process(out);
    out = m_cabP2.process(out);

    return out * masterN;
}
