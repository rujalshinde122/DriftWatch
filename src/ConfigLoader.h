#pragma once

#include "Config.h"
#include <string>

class ConfigLoader {
public:
    // Load config from a JSON file. If loading fails, returns default Config.
    static Config load(const std::string& filePath);

    // Save current config to a JSON file
    static bool save(const Config& config, const std::string& filePath);
};
