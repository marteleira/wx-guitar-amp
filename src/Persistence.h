#pragma once
#include <string>
#include <array>

struct ModelPreset {
    std::array<float, 6> vals = { 5, 5, 5, 5, 5, 5 };
    // order: preampVol, bass, mid, treble, ch5, master
};

struct AppState {
    int currentModel = 0;
    ModelPreset models[3];
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
