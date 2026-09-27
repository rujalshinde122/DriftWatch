#include "AnomalyDetector.h"

#include <cmath>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <iostream>

AnomalyDetector::AnomalyDetector(MetricEngine& metricEngine,
                                 std::string monitoredLevel,
                                 double zThreshold,
                                 int minEventCount,
                                 std::chrono::seconds window,
                                 size_t maxHistory)
    : metricEngine_(metricEngine),
      monitoredLevel_(std::move(monitoredLevel)),
      zThreshold_(zThreshold),
      minEventCount_(minEventCount),
      window_(window),
      maxHistory_(maxHistory) {}

AnomalyDetector::~AnomalyDetector() {
    stop();
}

void AnomalyDetector::setAlertCallback(AlertCallback callback) {
    alertCallback_ = std::move(callback);
}

void AnomalyDetector::addHistorySample(double count) {
    std::lock_guard<std::mutex> lock(historyMutex_);
    history_.push_back(count);
    if (history_.size() > maxHistory_) {
        history_.pop_front();
    }
}

void AnomalyDetector::clearHistory() {
    std::lock_guard<std::mutex> lock(historyMutex_);
    history_.clear();
}

size_t AnomalyDetector::getHistorySize() const {
    std::lock_guard<std::mutex> lock(historyMutex_);
    return history_.size();
}

double AnomalyDetector::getBaselineMean() const {
    std::lock_guard<std::mutex> lock(historyMutex_);
    if (history_.empty()) return 0.0;
    double sum = std::accumulate(history_.begin(), history_.end(), 0.0);
    return sum / static_cast<double>(history_.size());
}

double AnomalyDetector::getBaselineStdDev() const {
    std::lock_guard<std::mutex> lock(historyMutex_);
    if (history_.size() < 2) return 0.0;
    double mean = std::accumulate(history_.begin(), history_.end(), 0.0) / static_cast<double>(history_.size());
    double varianceSum = 0.0;
    for (double val : history_) {
        varianceSum += (val - mean) * (val - mean);
    }
    return std::sqrt(varianceSum / static_cast<double>(history_.size()));
}

bool AnomalyDetector::evaluate() {
    int currentCount = metricEngine_.getCount(monitoredLevel_, window_);

    double mean = 0.0;
    double stddev = 0.0;
    size_t histSize = 0;

    {
        std::lock_guard<std::mutex> lock(historyMutex_);
        histSize = history_.size();
        if (histSize >= 2) {
            double sum = std::accumulate(history_.begin(), history_.end(), 0.0);
            mean = sum / static_cast<double>(histSize);
            double varianceSum = 0.0;
            for (double val : history_) {
                varianceSum += (val - mean) * (val - mean);
            }
            stddev = std::sqrt(varianceSum / static_cast<double>(histSize));
        }
    }

    bool isAnomaly = false;
    double zScore = 0.0;

    // We need at least 2 historical samples to establish a standard deviation
    if (histSize >= 2) {
        if (stddev < 0.0001) {
            // Flat baseline (e.g. constant 0s or 1s)
            if (currentCount >= minEventCount_ && static_cast<double>(currentCount) > mean) {
                zScore = static_cast<double>(currentCount) - mean;
                isAnomaly = (zScore >= zThreshold_);
            }
        } else {
            zScore = (static_cast<double>(currentCount) - mean) / stddev;
            isAnomaly = (zScore >= zThreshold_ && currentCount >= minEventCount_);
        }
    }

    if (isAnomaly) {
        Alert alert;
        alert.triggeredAt = std::chrono::system_clock::now();
        alert.type = "ERROR_SPIKE";
        alert.level = monitoredLevel_;
        alert.currentCount = currentCount;
        alert.zScore = zScore;
        alert.baselineMean = mean;
        alert.baselineStdDev = stddev;

        std::ostringstream msg;
        msg << "Anomaly detected: " << monitoredLevel_ << " spike (observed: "
            << currentCount << ", baseline mean: " << std::fixed << std::setprecision(2)
            << mean << ", stddev: " << stddev << ", z-score: " << zScore << ")";
        alert.message = msg.str();

        if (alertCallback_) {
            alertCallback_(alert);
        }
    }

    // Record the current observed count into history for future baselines
    addHistorySample(static_cast<double>(currentCount));

    return isAnomaly;
}

void AnomalyDetector::start(std::chrono::seconds checkInterval) {
    if (isRunning_) return;
    isRunning_ = true;

    workerThread_ = std::thread([this, checkInterval]() {
        while (isRunning_) {
            evaluate();
            std::this_thread::sleep_for(checkInterval);
        }
    });
}

void AnomalyDetector::stop() {
    if (isRunning_.exchange(false)) {
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }
}

bool AnomalyDetector::isRunning() const {
    return isRunning_.load();
}
