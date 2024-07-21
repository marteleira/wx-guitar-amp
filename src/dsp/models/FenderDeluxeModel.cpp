#include "FenderDeluxeModel.h"
#include <cmath>

void FenderDeluxeModel::setSampleRate(unsigned int sr) {
    m_sr = sr;
    double s = sr;
    m_dc    = Biquad::highPass(10.0, 0.7, s);

    // 1×12 Jensen P12R cabinet: smaller, more mid-focused than a Marshall 4×12
    m_cabHP = Biquad::highPass(65.0,   0.7,  s);    // speaker rolls off below ~65 Hz
    m_cabLP = Biquad::lowPass (8000.0, 0.6,  s);    // smooth Jensen rolloff
    m_cabP1 = Biquad::peaking (1200.0, 3.5, 1.6, s); // Jensen mid character
    m_cabP2 = Biquad::peaking (3500.0, 2.5, 1.2, s); // upper-mid sparkle

    m_cBass = m_cMid = m_cTreble = m_cBright = -99;
}

void FenderDeluxeModel::resetState() noexcept {
    m_dc.resetState();
    m_bass.resetState(); m_mid.resetState(); m_treble.resetState(); m_bright.resetState();
    m_cabHP.resetState(); m_cabLP.resetState(); m_cabP1.resetState(); m_cabP2.resetState();
    m_cBass = m_cMid = m_cTreble = m_cBright = -99;
}

void FenderDeluxeModel::rebuildTone(float bass, float mid, float treble, float bright) {
    double s = m_sr;

    // Fender tone stack: bass lower, mid NOT inverted, bright cap as high shelf
    Biquad nb = Biquad::lowShelf  (100.0,  (bass   - 5.0) * 2.0,       s);  // deeper bass
    Biquad nm = Biquad::peaking   (400.0,  (mid    - 5.0) * 1.5, 0.8,  s);  // not inverted
    Biquad nt = Biquad::highShelf (2500.0, (treble - 5.0) * 2.4,       s);
    Biquad nbr= Biquad::highShelf (8000.0,  bright         * 0.8,       s);  // 0–8 dB sparkle

    nb.preserveStateFrom(m_bass);   m_bass   = nb;
    nm.preserveStateFrom(m_mid);    m_mid    = nm;
    nt.preserveStateFrom(m_treble); m_treble = nt;
    nbr.preserveStateFrom(m_bright); m_bright = nbr;

    m_cBass = bass; m_cMid = mid; m_cTreble = treble; m_cBright = bright;
}

float FenderDeluxeModel::process(float in,
                                 float preampVol, float bass, float mid,
                                 float treble, float ch5, float master) noexcept {
    if (bass != m_cBass || mid != m_cMid || treble != m_cTreble || ch5 != m_cBright)
        rebuildTone(bass, mid, treble, ch5);

    float norm = preampVol / 10.0f;

    // Single 12AX7 — much gentler gain curve, plenty of clean headroom
    // norm^1.5 makes the first half of the knob very clean
    float g  = 1.0f + std::pow(norm, 1.5f) * 40.0f;
    float s1 = m_dc.process(in * g);

    // Cubic soft clip: smooth, Fender "bloom" character
    // Scale to ±1 range, clip, scale back
    float clipIn = s1 * 1.4f;
    float clipped = tubeClip(clipIn) / 1.4f;

    float pre = clipped * 0.5f;  // scale down before tone stack

    // Tone stack (mid is NOT inverted — this alone makes it sound very different)
    float t = m_bright.process(m_treble.process(m_mid.process(m_bass.process(pre))));

    // 6V6 push-pull power amp: softer, "blossoming" saturation
    float masterN = master / 10.0f;
    float drive   = 1.0f + masterN * 1.5f;  // gentler drive range than EL34
    // Slight positive bias for even-harmonic "warmth"
    float biased  = t + 0.04f;
    float pwr     = std::tanh(biased * drive) / std::tanh(drive) - std::tanh(0.04f * drive) / std::tanh(drive);

    // 1×12 Jensen cabinet
    float out = m_cabHP.process(pwr);
    out = m_cabP1.process(out);
    out = m_cabLP.process(out);
    out = m_cabP2.process(out);

    return out * masterN;
}
