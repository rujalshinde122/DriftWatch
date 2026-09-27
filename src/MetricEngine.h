#pragma once

#include "LogEvent.h"
#include <string>
#include <deque>
#include <chrono>
#include <mutex>

class MetricEngine {
public:
    MetricEngine() = default;

    // Record an incoming parsed log event using the current system time
    void record(const LogEvent& event);

    // Record an event with an explicit timestamp (useful for deterministic tests)
    void record(const std::string& level, std::chrono::system_clock::time_point timestamp);

    // Get count of events matching level within the last `window`
    int getCount(const std::string& level, std::chrono::seconds window);

    // Get total events of all levels within the last `window`
    int getTotalCount(std::chrono::seconds window);

    // Clear and reset all metrics
    void reset();

private:
    struct TimestampedEntry {
        std::chrono::system_clock::time_point timestamp;
        std::string level;
    };

    void pruneExpired(std::chrono::system_clock::time_point cutoff);

    std::deque<TimestampedEntry> entries_;
    std::mutex mutex_;
};
