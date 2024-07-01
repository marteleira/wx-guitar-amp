#pragma once
#include <array>

// Strategy interface. Each amp model owns its DSP state (filters, etc.).
// Only the audio thread calls process() and resetState().
class AmpModel {
public:
    virtual ~AmpModel() = default;

    virtual void setSampleRate(unsigned int sr) = 0;

    // Called by audio thread when this model becomes active (state must be zeroed)
    virtual void resetState() noexcept = 0;

    // Main processing. The 6 knobs map as:
    //   preampVol — input gain/drive (0-10)
    //   bass/mid/treble — tone stack (0-10)
    //   ch5 — model-specific 5th control (presence, bright cap, etc.) (0-10)
    //   master — output level (0-10)
    virtual float process(float in,
                          float preampVol, float bass, float mid,
                          float treble, float ch5, float master) noexcept = 0;

    virtual const char* name()     const noexcept = 0;  // full display name
    virtual const char* brand()    const noexcept = 0;  // short brand for header logo
    virtual const char* subtitle() const noexcept = 0;  // e.g. "100W  Super Lead"

    // Labels for the 6 knobs in order: preampVol, bass, mid, treble, ch5, master
    virtual std::array<const char*, 6> knobLabels()   const noexcept = 0;
    virtual std::array<float, 6>       knobDefaults() const noexcept = 0;
};
