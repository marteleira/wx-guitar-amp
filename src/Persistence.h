#pragma once
#include <string>
#include <array>

static constexpr int AMP_MODEL_COUNT = 4;

struct ModelPreset {
    std::array<float, 6> vals = { 5, 5, 5, 5, 5, 5 };
    // order: preampVol, bass, mid, treble, ch5, master
};

struct AppState {
    int currentModel = 0;
    ModelPreset models[AMP_MODEL_COUNT];
    std::string inputDeviceName;
    std::string outputDeviceName;
    int inputGain  = 100;   // 0-200 slider value
    int outputGain = 100;
};

class Persistence {
public:
    static std::string defaultPath();
    static AppState    load(const std::string& path);
    static void        save(const AppState& state, const std::string& path);
};
