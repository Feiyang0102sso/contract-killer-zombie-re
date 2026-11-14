// zombsniper.cfg: the single place that names every key, its section and its default value.
// To add a setting, add a field to Config and one row to ConfigKeys; Config.cpp needs no change.
#pragma once

#include <filesystem>
#include <string>

namespace ckz {

// Read from the folder of the executable; the build copies the repository's copy there.
constexpr wchar_t ConfigFileName[] = L"zombsniper.cfg";

// Values of [graph] Engine.
constexpr const char* EngineM3g = "m3g";       // native m3gcore runtime
constexpr const char* EngineSwerve = "swerve"; // original ARM Swerve core run by Unicorn

/** Settings of zombsniper.cfg. LoadConfig fills every field, from the file or from its default. */
struct Config {
    std::string title;  // [common] Title
    std::string engine; // [graph] Engine: EngineM3g or EngineSwerve
    int windowWidth;    // [graph] WindowWidth
    int windowHeight;   // [graph] WindowHeight
    bool fullScreen;    // [graph] FullScreen
    bool vSync;         // [graph] VSync
    bool drawFps;       // [game] DrawFPS
    bool keyHints;      // [game] KeyHints: key badges on the mission HUD controls (KeyHints.cpp)
    bool debugMode;     // [debug] DebugMode
    bool fpsLog;        // [debug] FpsLog; LoadConfig turns it off without DebugMode
};

/** One key of zombsniper.cfg. Exactly one of text / number / flag is set: the field its value goes to. */
struct ConfigKey {
    const char* section;
    const char* name;
    const char* defaultValue;     // parsed like a value from the file
    std::string Config::* text;   // any text
    int Config::* number;         // a positive whole number
    bool Config::* flag;          // 0 or 1
};

inline const ConfigKey ConfigKeys[] = {
    { "common", "Title",        "Contract Killer: Zombies", &Config::title,  nullptr,              nullptr },
    { "graph",  "Engine",       "m3g",                      &Config::engine, nullptr,              nullptr },
    { "graph",  "WindowWidth",  "2048",                     nullptr,         &Config::windowWidth, nullptr },
    { "graph",  "WindowHeight", "1536",                     nullptr,         &Config::windowHeight, nullptr },
    { "graph",  "FullScreen",   "0",                        nullptr,         nullptr,              &Config::fullScreen },
    { "graph",  "VSync",        "1",                        nullptr,         nullptr,              &Config::vSync },
    { "game",   "DrawFPS",      "1",                        nullptr,         nullptr,              &Config::drawFps },
    { "game",   "KeyHints",     "1",                        nullptr,         nullptr,              &Config::keyHints },
    { "debug",  "DebugMode",    "0",                        nullptr,         nullptr,              &Config::debugMode },
    { "debug",  "FpsLog",       "0",                        nullptr,         nullptr,              &Config::fpsLog },
};

/** The settings in use; filled by LoadConfig at startup. */
extern Config config;

/** Fill `config` with the defaults, then with the keys found in `file` (a missing file is fine). */
void LoadConfig(const std::filesystem::path& file);

} // namespace ckz
