#include "Persistence.h"
#include <fstream>
#include <sstream>
#include <map>
#include <filesystem>
#include <cstdlib>

// INI helpers

using Ini = std::map<std::string, std::map<std::string, std::string>>;

static std::string trim(const std::string& s) {
    auto a = s.find_first_not_of(" \t\r");
    auto b = s.find_last_not_of(" \t\r");
    return (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
}

static Ini parseIni(const std::string& path) {
    Ini result;
    std::ifstream f(path);
    std::string line, section;
    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line[0] == '[') {
            auto end = line.find(']');
            if (end != std::string::npos) section = line.substr(1, end - 1);
        } else {
            auto eq = line.find('=');
            if (eq != std::string::npos) {
                result[section][trim(line.substr(0, eq))] = trim(line.substr(eq + 1));
            }
        }
    }
    return result;
}

static float getF(const Ini& ini, const std::string& sec,
                  const std::string& key, float def) {
    try { return std::stof(ini.at(sec).at(key)); } catch (...) { return def; }
}
static int getI(const Ini& ini, const std::string& sec,
                const std::string& key, int def) {
    try { return std::stoi(ini.at(sec).at(key)); } catch (...) { return def; }
}
static std::string getS(const Ini& ini, const std::string& sec,
                        const std::string& key, const std::string& def = "") {
    try { return ini.at(sec).at(key); } catch (...) { return def; }
}

// Public API

std::string Persistence::defaultPath() {
    std::string base;
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg && *xdg) {
        base = xdg;
    } else {
        const char* home = std::getenv("HOME");
        base = home ? std::string(home) + "/.config" : "/tmp";
    }
    return base + "/wx-guitar-amp/state.ini";
}

AppState Persistence::load(const std::string& path) {
    AppState s;
    Ini ini = parseIni(path);  // returns empty map if file doesn't exist

    s.currentModel     = getI(ini, "state", "model",        0);
    s.inputGain        = getI(ini, "state", "inputGain",    100);
    s.outputGain       = getI(ini, "state", "outputGain",   100);
    s.inputDeviceName  = getS(ini, "state", "inputDevice");
    s.outputDeviceName = getS(ini, "state", "outputDevice");

    if (s.currentModel < 0 || s.currentModel >= AMP_MODEL_COUNT) s.currentModel = 0;

    static const char* keys[] = { "preampVol", "bass", "mid", "treble", "ch5", "master" };

    // Default values per model (curated to sound good out of the box)
    static const float defaults[AMP_MODEL_COUNT][6] = {
        { 7.0f, 5.0f, 4.0f, 6.0f, 5.0f, 4.0f },  // JCM800 - classic rock crunch
        { 5.0f, 6.0f, 5.0f, 7.0f, 4.0f, 7.0f },  // Fender - sparkly clean
        { 6.0f, 5.0f, 7.0f, 5.0f, 4.0f, 5.0f },  // Orange - mid-heavy crunch
        { 5.0f, 4.0f, 6.5f, 2.5f, 4.0f, 6.0f },  // Vox AC30 - bright jangle
    };

    for (int m = 0; m < AMP_MODEL_COUNT; ++m) {
        std::string sec = "model" + std::to_string(m);
        for (int i = 0; i < 6; ++i)
            s.models[m].vals[i] = getF(ini, sec, keys[i], defaults[m][i]);
    }

    return s;
}

void Persistence::save(const AppState& s, const std::string& path) {
    // Create directory if it doesn't exist
    std::error_code ec;
    std::filesystem::create_directories(
        std::filesystem::path(path).parent_path(), ec);

    std::ofstream f(path);
    if (!f) return;

    f << "[state]\n";
    f << "model="        << s.currentModel      << "\n";
    f << "inputGain="    << s.inputGain          << "\n";
    f << "outputGain="   << s.outputGain         << "\n";
    f << "inputDevice="  << s.inputDeviceName    << "\n";
    f << "outputDevice=" << s.outputDeviceName   << "\n\n";

    static const char* keys[] = { "preampVol", "bass", "mid", "treble", "ch5", "master" };
    for (int m = 0; m < AMP_MODEL_COUNT; ++m) {
        f << "[model" << m << "]\n";
        for (int i = 0; i < 6; ++i)
            f << keys[i] << "=" << s.models[m].vals[i] << "\n";
        f << "\n";
    }
}
