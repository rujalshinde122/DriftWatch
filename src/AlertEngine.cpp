#include "AlertEngine.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
std::string formatTime(const std::chrono::system_clock::time_point& tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
} // namespace

AlertEngine::AlertEngine(Config config)
    : config_(std::move(config)) {}

void AlertEngine::setConfig(Config config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = std::move(config);
}

const Config& AlertEngine::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

void AlertEngine::dispatch(const Alert& alert) {
    std::lock_guard<std::mutex> lock(mutex_);
    totalAlertsCount_++;
    history_.push_back(alert);
    if (history_.size() > 50) {
        history_.pop_front();
    }

    if (config_.alertSinks.console) {
        writeToConsole(alert);
    }

    if (!config_.alertSinks.file.empty()) {
        writeToFile(alert, config_.alertSinks.file);
    }

    if (!config_.alertSinks.webhook.empty()) {
        dispatchWebhook(alert, config_.alertSinks.webhook);
    }
}

void AlertEngine::writeToConsole(const Alert& alert) {
    std::cout << "\n\033[1;31m======================================================\033[0m\n";
    std::cout << "\033[1;31m🚨 [ALERT TRIGGERED]\033[0m " << alert.type << " (" << alert.level << ")\n";
    std::cout << "   " << alert.message << "\n";
    std::cout << "   Triggered at: " << formatTime(alert.triggeredAt) << "\n";
    std::cout << "\033[1;31m======================================================\033[0m\n\n";
}

void AlertEngine::writeToFile(const Alert& alert, const std::string& filePath) {
    std::ofstream out(filePath, std::ios::app);
    if (!out.is_open()) {
        std::cerr << "[AlertEngine] Warning: Could not write alert to " << filePath << "\n";
        return;
    }

    out << "[" << formatTime(alert.triggeredAt) << "] "
        << "[ALERT] type=" << alert.type
        << " level=" << alert.level
        << " observed=" << alert.currentCount
        << " z_score=" << std::fixed << std::setprecision(2) << alert.zScore
        << " mean=" << alert.baselineMean
        << " stddev=" << alert.baselineStdDev
        << " msg=\"" << alert.message << "\""
        << std::endl;
}

void AlertEngine::dispatchWebhook(const Alert& alert, const std::string& webhookUrl) {
    // Webhook dispatch simulation / log hook
    std::cout << "[AlertEngine] Dispatched alert to webhook endpoint: " << webhookUrl
              << " (z=" << std::fixed << std::setprecision(2) << alert.zScore << ")\n";
}

std::vector<Alert> AlertEngine::getRecentAlerts(size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Alert> result;
    size_t start = (history_.size() > limit) ? (history_.size() - limit) : 0;
    for (size_t i = start; i < history_.size(); ++i) {
        result.push_back(history_[i]);
    }
    return result;
}

size_t AlertEngine::getTotalAlertsCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return totalAlertsCount_;
}
