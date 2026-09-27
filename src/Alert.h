#pragma once

#include <string>
#include <chrono>

// Represents an anomaly detection alert
struct Alert {
    std::chrono::system_clock::time_point triggeredAt;
    std::string type;       // e.g. "ERROR_SPIKE"
    std::string level;      // e.g. "ERROR"
    std::string message;    // Human-readable summary
    double zScore;          // Statistical z-score
    int currentCount;       // Observed count in the active window
    double baselineMean;    // Historical average count
    double baselineStdDev;  // Historical standard deviation
};
