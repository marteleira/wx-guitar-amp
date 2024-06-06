#include "DspProcessor.h"
#include <cmath>

void DspProcessor::setSampleRate(unsigned int sr) {
    m_sr = sr;
    double s = sr;

    for (auto& f : m_dcBlock)
        f = Biquad::highPass(10.0, 0.7, s);

    // Cabinet simulation (approximate 4x12 Marshall closed-back response)
    m_cabHP    = Biquad::highPass(80.0,  0.7,  s);
    m_cabLP    = Biquad::lowPass(6500.0, 0.65, s);
    m_cabPeak1 = Biquad::peaking(2200.0,  4.0, 1.8, s);  // classic 2kHz presence peak
    m_cabPeak2 = Biquad::peaking(4500.0,  2.0, 1.2, s);  // bite/attack region

    // Force tone filter rebuild on next process() call
    m_cBass = m_cMid = m_cTreble = m_cPresence = -99;
}

void DspProcessor::rebuildToneFilters(float bass, float mid, float treble, float presence) {
    double s = m_sr;

    // Marshall passive tone stack approximation via parametric EQ.
    // The mid control is physically inverted on the JCM800: noon = max cut.
    double bassGain     = (bass     - 5.0) * 2.4;   // +-12 dB
    double midGain      = (5.0 - mid)      * 2.0;   // inverted, +-10 dB
    double trebleGain   = (treble   - 5.0) * 2.4;   // +-12 dB
    double presenceGain = presence * 1.2;            // 0 -> +12 dB high shelf

    // Preserve filter state across coefficient changes to avoid clicks
    Biquad newBass     = Biquad::lowShelf(200.0,  bassGain,   s);
    Biquad newMid      = Biquad::peaking (700.0,  midGain,  0.75, s);
    Biquad newTreble   = Biquad::highShelf(2200.0, trebleGain, s);
    Biquad newPresence = Biquad::highShelf(5000.0, presenceGain, s);

    newBass.preserveStateFrom(m_bass);
    newMid.preserveStateFrom(m_mid);
    newTreble.preserveStateFrom(m_treble);
    newPresence.preserveStateFrom(m_presence);

    m_bass     = newBass;
    m_mid      = newMid;
    m_treble   = newTreble;
    m_presence = newPresence;

    m_cBass = bass; m_cMid = mid; m_cTreble = treble; m_cPresence = presence;
}

float DspProcessor::process(float in,
                            float preampVol, float bass, float mid,
                            float treble, float presence, float master) noexcept {
    if (bass != m_cBass || mid != m_cMid || treble != m_cTreble || presence != m_cPresence)
        rebuildToneFilters(bass, mid, treble, presence);

    float norm = preampVol / 10.0f;

    // Preamp stage 1
    // JCM800 first triode: input sensitivity control drives gain hard
    float g1 = 1.0f + norm * norm * 85.0f;  // 1x (clean) -> 86x (saturated)
    float s1 = m_dcBlock[0].process(in * g1);
    s1 = tubeClip(s1);

    // Preamp stage 2 
    float g2 = 1.0f + norm * norm * 35.0f;
    float s2 = m_dcBlock[1].process(s1 * g2);
    s2 = tubeClip(s2);

    // Scale preamp output to a sane level before the passive tone stack
    float preOut = m_dcBlock[2].process(s2 * 0.08f);

    //  Passive tone stack 
    float toned = m_bass.process(preOut);
    toned = m_mid.process(toned);
    toned = m_treble.process(toned);

    //  Presence (power amp feedback path) 
    float sig = m_presence.process(toned);

    //  Power amp: push-pull EL34 simulation (mild even-order saturation) 
    float masterN = master / 10.0f;
    float drive   = 1.5f + masterN * 2.0f;
    float powerOut = std::tanh(sig * drive) / std::tanh(drive);

    //  Cabinet simulation 
    float out = m_cabHP.process(powerOut);
    out = m_cabPeak1.process(out);
    out = m_cabLP.process(out);
    out = m_cabPeak2.process(out);

    return out * masterN;
}
