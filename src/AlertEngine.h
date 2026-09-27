#pragma once

#include "Alert.h"
#include "Config.h"

#include <string>
#include <deque>
#include <mutex>
#include <vector>

class AlertEngine {
public:
    explicit AlertEngine(Config config);

    // Update active config
    void setConfig(Config config);
    const Config& getConfig() const;

    // Dispatch an alert across all enabled sinks
    void dispatch(const Alert& alert);

    // Retrieve recent alerts
    std::vector<Alert> getRecentAlerts(size_t limit = 10) const;

    // Total count of dispatched alerts
    size_t getTotalAlertsCount() const;

private:
    void writeToFile(const Alert& alert, const std::string& filePath);
    void writeToConsole(const Alert& alert);
    void dispatchWebhook(const Alert& alert, const std::string& webhookUrl);

    Config config_;
    mutable std::mutex mutex_;
    std::deque<Alert> history_;
    size_t totalAlertsCount_{0};
};
