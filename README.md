# wx-guitar-amp

Real-time guitar amp simulator in C++. No effects libraries, everything DSP is written from scratch. Plug in a guitar, pick an amp, play.

Currently models three amps:

- **Marshall JCM800 2203** - the classic British crunch/high gain. Two cascaded 12AX7 stages, passive tone stack with inverted mid, EL34 push-pull, 4x12 Greenback cab.
- **Fender Deluxe Reverb '65** - single 12AX7, way more headroom, cubic soft clip, Fender tone stack (mid not inverted, bass sits lower at 100Hz), 6V6 power amp, 1x12 Jensen cab.
- **Orange OR120** - 120W, asymmetric cathode-follower clip, mid centred at 900Hz, Vintage 30 4x12 with the characteristic 850Hz honk. Bigger and warmer than the Marshall.

![screenshot](res/screenshot.png)

---

## Signal chain

Each amp model follows the same stages but with different component values, clipping functions and cabinet tuning:

```
guitar in (mono)
    |
    v
DC block (removes cable offset)
    |
    v
preamp stages (1 or 2x 12AX7, gain + soft clip)
    |
    v
tone stack (bass / mid / treble biquads)
    |
    v
ch5 control (presence / bright cap / top boost depending on model)
    |
    v
power amp (tanh saturation, drive scales with master volume)
    |
    v
cab sim (3-5 biquads tuned per speaker type)
    |
    v
stereo out
```

---

## Amp models

### Marshall JCM800 2203

Two gain stages, both using `x / (1 + |x|)` with a small DC bias to get asymmetric clipping -> even harmonics -> warmth. The mid control is **inverted** (noon = max cut), exactly like the real circuit. At high preamp volume the two stages compound hard and you get the classic JCM800 compression.

Cab: 4x12 Marshall closed-back. HP 80Hz, peak 2.2kHz (+4dB), LP 6.5kHz, peak 4.5kHz (+2dB).

### Fender Deluxe Reverb '65

Single gain stage, `x - x^3/3` cubic clipper (smoother, second-harmonic character). Much more headroom - noon on the volume knob is still pretty clean. Mid is not inverted and sits at 400Hz instead of 700Hz. The Bright knob adds a high shelf at 8kHz, which is what the bright cap on the original volume pot does.

Cab: 1x12 Jensen P12R. Rolls off at 8kHz, peak at 1.2kHz (+3.5dB), peak at 3.5kHz (+2.5dB). More mid-forward than the 4x12.

### Orange OR120

Asymmetric clip: positive half `x / (1 + x*0.75)`, negative half `x / (1 - x*1.1)`. The difference between the two sides is what gives Orange amps the warmer, more complex distortion vs the Marshall. Mid sits at 900Hz (not 700) and is not inverted. Top Boost simulates plugging into the high-gain input jack - adds gain and a 6kHz shelf simultaneously.

Cab: 4x12 Vintage 30. The V30 has a notorious dip around 250Hz (low-mid suck-out) and a strong peak at 850Hz - that's the "honk" you hear on Orange recordings. Five biquads total to model it properly.

---

## Architecture

The three amps implement an `AmpModel` interface (strategy pattern):

```
AmpModel (pure virtual)
    process(in, preampVol, bass, mid, treble, ch5, master) -> float
    resetState()
    name() / brand() / subtitle()
    knobLabels() / knobDefaults()

JCM800Model    : AmpModel
FenderDeluxeModel : AmpModel
OrangeOR120Model  : AmpModel
```

`DspProcessor` owns all three instances and dispatches to whichever is active via `std::atomic<int>`. When you switch models the audio thread detects the change, calls `resetState()` on the new model (zeroes all biquad state), and starts using it. No locks, no allocation in the audio path.

UI thread -> writes to `std::atomic<float>` params -> audio thread reads them lock-free each callback. Filter coefficients are rebuilt inside the audio thread when a param changes, state is preserved across rebuilds to avoid clicks.

---

## UI

- **Spectrum analyzer** - 1024-point FFT (Cooley-Tukey, written from scratch), Hanning window, log frequency axis 20Hz-20kHz. Input in blue, output in green. Runs at 20fps via wxTimer.
- **Device selection** - input and output device dropdowns populated from miniaudio's enumeration. Changing device does a hot restart (uninit + reinit the device, context stays alive).
- **Input/output gain** - sliders 0-200%, applied before/after the DSP. Peak meters update at 20fps with exponential decay (~300ms hold).
- **6 knobs** - labels and defaults change when you switch amp model.

---

## Building

Dependencies: C++17, CMake 3.16+, wxWidgets 3.2+. miniaudio is included as a single header.

**Arch:**
```bash
sudo pacman -S wxwidgets-gtk3 cmake base-devel
```

**Ubuntu/Debian:**
```bash
sudo apt install libwxgtk3.2-dev cmake build-essential
```

**Build:**
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/WxGuitarAmp
```

---

## Roadmap

- IR convolution cab sim (load real .wav IR files)
- Noise gate
- Delay / spring reverb
- JACK support
- Preset save/load
- Mesa Boogie Dual Rectifier (different again - silicon diode rectifier, 5 gain stages)

---

## Stack

- UI -> wxWidgets 3.2
- Audio I/O -> miniaudio (single header, no deps)
- DSP -> hand-written C++, no effects libraries
- Build -> CMake
