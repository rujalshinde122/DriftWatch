#pragma once

#include "LogEvent.h"
#include "Alert.h"

#include <string>
#include <deque>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>

class TUIRenderer {
public:
    TUIRenderer(std::string watchedTarget, double zThreshold, int windowSeconds);
    ~TUIRenderer();

    // Push new events to TUI feed (thread-safe)
    void pushLog(const LogEvent& event);
    void pushErrorSnapshot(int errorCount, double zScore);
    void pushAlert(const Alert& alert);

    // Lifecycle
    void start(int refreshIntervalMs = 400);
    void stop();
    bool isRunning() const;

private:
    void renderLoop(int refreshIntervalMs);
    void drawScreen();

    std::string watchedTarget_;
    double zThreshold_;
    int windowSeconds_;

    std::chrono::steady_clock::time_point startTime_;
    std::atomic<size_t> totalEventsCount_{0};

    // Shared thread-safe display buffers
    mutable std::mutex dataMutex_;
    std::deque<LogEvent> recentLogs_;          // max 16
    std::vector<int> errorHistory_;            // max 12
    std::deque<Alert> recentAlerts_;           // max 5

    std::thread renderThread_;
    std::atomic<bool> isRunning_{false};
};
