#pragma once

#include "MetricEngine.h"
#include "Alert.h"

#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

class AnomalyDetector {
public:
    using AlertCallback = std::function<void(const Alert&)>;

    explicit AnomalyDetector(MetricEngine& metricEngine,
                             std::string monitoredLevel = "ERROR",
                             double zThreshold = 2.5,
                             int minEventCount = 5,
                             std::chrono::seconds window = std::chrono::seconds(60),
                             size_t maxHistory = 10);
    ~AnomalyDetector();

    // Register callback for when an anomaly is detected
    void setAlertCallback(AlertCallback callback);

    // Run a single evaluation step against the current metrics
    // Returns true if an anomaly was detected and alerted
    bool evaluate();

    // Start background evaluation loop
    void start(std::chrono::seconds checkInterval = std::chrono::seconds(5));

    // Stop background evaluation loop
    void stop();

    bool isRunning() const;

    // Direct history manipulation (helpful for testing and priming baselines)
    void addHistorySample(double count);
    void clearHistory();
    size_t getHistorySize() const;

    // Get current computed baseline parameters from history
    double getBaselineMean() const;
    double getBaselineStdDev() const;

private:
    MetricEngine& metricEngine_;
    std::string monitoredLevel_;
    double zThreshold_;
    int minEventCount_;
    std::chrono::seconds window_;
    size_t maxHistory_;

    std::deque<double> history_;
    mutable std::mutex historyMutex_;
    AlertCallback alertCallback_;

    std::thread workerThread_;
    std::atomic<bool> isRunning_{false};
};
