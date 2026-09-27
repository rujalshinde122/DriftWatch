#include "MetricEngine.h"

void MetricEngine::record(const LogEvent& event) {
    record(event.level, std::chrono::system_clock::now());
}

void MetricEngine::record(const std::string& level, std::chrono::system_clock::time_point timestamp) {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.push_back({timestamp, level});

    // Automatically prune events older than 10 minutes to bound memory usage
    auto cutoff = timestamp - std::chrono::minutes(10);
    pruneExpired(cutoff);
}

int MetricEngine::getCount(const std::string& level, std::chrono::seconds window) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - window;

    int count = 0;
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
        if (it->timestamp < cutoff) {
            break; // Since entries are ordered by time, we can stop early
        }
        if (it->level == level) {
            count++;
        }
    }
    return count;
}

int MetricEngine::getTotalCount(std::chrono::seconds window) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::system_clock::now();
    auto cutoff = now - window;

    int count = 0;
    for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
        if (it->timestamp < cutoff) {
            break;
        }
        count++;
    }
    return count;
}

void MetricEngine::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.clear();
}

void MetricEngine::pruneExpired(std::chrono::system_clock::time_point cutoff) {
    while (!entries_.empty() && entries_.front().timestamp < cutoff) {
        entries_.pop_front();
    }
}
