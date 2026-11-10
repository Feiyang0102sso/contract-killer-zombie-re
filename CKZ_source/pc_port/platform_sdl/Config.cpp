// Read zombsniper.cfg into ckz::config; the keys and defaults are in Config.h.
//   [section]
//   Key=Value      ; comment
// Key and section names ignore case. A missing or empty key keeps its default.
#include "Config.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

namespace ckz {

Config config;

namespace {

/** Text without the spaces and tabs at both ends. */
std::string Trim(const std::string& text)
{
    const char* blanks = " \t\r\n";
    const size_t first = text.find_first_not_of(blanks);
    if (first == std::string::npos) {
        return std::string();
    }
    const size_t last = text.find_last_not_of(blanks);
    return text.substr(first, last - first + 1);
}

const ConfigKey* FindKey(const std::string& section, const std::string& name)
{
    for (const ConfigKey& key : ConfigKeys) {
        if (SDL_strcasecmp(key.section, section.c_str()) == 0 && SDL_strcasecmp(key.name, name.c_str()) == 0) {
            return &key;
        }
    }
    return nullptr;
}

/** Store `value` in the field of `key`. Returns false (field unchanged) when the value is not valid. */
bool ApplyValue(const ConfigKey& key, const std::string& value)
{
    if (key.text != nullptr) {
        config.*key.text = value;
        return true;
    }
    if (key.number != nullptr) {
        char* end = nullptr;
        const long number = strtol(value.c_str(), &end, 10);
        if (end == value.c_str() || *end != '\0' || number < 1) {
            return false;
        }
        config.*key.number = static_cast<int>(number);
        return true;
    }
    if (value == "0") {
        config.*key.flag = false;
        return true;
    }
    if (value == "1") {
        config.*key.flag = true;
        return true;
    }
    return false;
}

/** Apply one line of the file; `section` is the section the line is in and changes on a [section] line. */
void ApplyLine(const std::string& rawLine, int lineNumber, std::string& section)
{
    std::string line = rawLine;
    const size_t comment = line.find(';');
    if (comment != std::string::npos) {
        line.erase(comment);
    }
    line = Trim(line);
    if (line.empty()) {
        return;
    }
    if (line.front() == '[' && line.back() == ']') {
        section = Trim(line.substr(1, line.size() - 2));
        return;
    }
    const size_t equals = line.find('=');
    if (equals == std::string::npos) {
        fprintf(stderr, "Config line %d ignored: no '=' in \"%s\"\n", lineNumber, line.c_str());
        return;
    }
    const std::string name = Trim(line.substr(0, equals));
    const std::string value = Trim(line.substr(equals + 1));
    const ConfigKey* key = FindKey(section, name);
    if (key == nullptr) {
        fprintf(stderr, "Config line %d ignored: unknown key [%s] %s\n", lineNumber, section.c_str(), name.c_str());
        return;
    }
    if (value.empty()) {
        return;
    }
    if (!ApplyValue(*key, value)) {
        fprintf(stderr, "Config line %d ignored: bad value \"%s\" for %s, default %s used\n", lineNumber,
                value.c_str(), key->name, key->defaultValue);
    }
}

} // namespace

void LoadConfig(const std::filesystem::path& file)
{
    for (const ConfigKey& key : ConfigKeys) {
        ApplyValue(key, key.defaultValue);
    }

    std::ifstream stream(file);
    if (!stream) {
        fwprintf(stderr, L"Config %ls not found, defaults used\n", file.c_str());
    } else {
        std::string section;
        std::string line;
        int lineNumber = 0;
        while (std::getline(stream, line)) {
            lineNumber++;
            // Skip the UTF-8 byte order mark an editor may write.
            if (lineNumber == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
                line.erase(0, 3);
            }
            ApplyLine(line, lineNumber, section);
        }
        fwprintf(stderr, L"Config %ls loaded\n", file.c_str());
    }

    if (config.engine != EngineM3g && config.engine != EngineSwerve) {
        fprintf(stderr, "Config: unknown Engine \"%s\", %s used\n", config.engine.c_str(), EngineM3g);
        config.engine = EngineM3g;
    }
    // The frame rate log goes with the debug console.
    if (!config.debugMode) {
        config.fpsLog = false;
    }
    fprintf(stderr, "Config: engine %s, window %dx%d, fullscreen %d, vsync %d, draw fps %d, debug %d, fps log %d\n",
            config.engine.c_str(), config.windowWidth, config.windowHeight, config.fullScreen, config.vSync,
            config.drawFps, config.debugMode, config.fpsLog);
}

} // namespace ckz
