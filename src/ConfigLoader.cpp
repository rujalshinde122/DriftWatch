#include "ConfigLoader.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

Config ConfigLoader::load(const std::string& filePath) {
    Config config; // defaults
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[ConfigLoader] Warning: Could not open config file: " << filePath
                  << ", using default configuration.\n";
        return config;
    }

    try {
        json j;
        file >> j;

        if (j.contains("watch_files") && j["watch_files"].is_array()) {
            config.watchFiles = j["watch_files"].get<std::vector<std::string>>();
        }
        config.anomalyThresholdZ = j.value("anomaly_threshold_z", config.anomalyThresholdZ);
        config.minEventCount     = j.value("min_event_count", config.minEventCount);
        config.windowSeconds     = j.value("window_seconds", config.windowSeconds);
        config.historyDepth      = j.value("history_depth", config.historyDepth);

        if (j.contains("alert_sinks") && j["alert_sinks"].is_object()) {
            auto sinks = j["alert_sinks"];
            config.alertSinks.console = sinks.value("console", config.alertSinks.console);
            config.alertSinks.file    = sinks.value("file", config.alertSinks.file);
            config.alertSinks.webhook = sinks.value("webhook", config.alertSinks.webhook);
        }
    } catch (const std::exception& e) {
        std::cerr << "[ConfigLoader] Error parsing JSON config: " << e.what()
                  << ", falling back to defaults.\n";
    }

    return config;
}

bool ConfigLoader::save(const Config& config, const std::string& filePath) {
    try {
        json j;
        j["watch_files"] = config.watchFiles;
        j["anomaly_threshold_z"] = config.anomalyThresholdZ;
        j["min_event_count"] = config.minEventCount;
        j["window_seconds"] = config.windowSeconds;
        j["history_depth"] = config.historyDepth;

        json sinks;
        sinks["console"] = config.alertSinks.console;
        sinks["file"] = config.alertSinks.file;
        sinks["webhook"] = config.alertSinks.webhook;
        j["alert_sinks"] = sinks;

        std::ofstream out(filePath);
        if (!out.is_open()) return false;
        out << j.dump(4) << std::endl;
        return true;
    } catch (...) {
        return false;
    }
}
