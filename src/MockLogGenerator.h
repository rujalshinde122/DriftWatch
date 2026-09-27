#pragma once

#include <string>
#include <thread>
#include <atomic>

class MockLogGenerator {
public:
    MockLogGenerator();
    ~MockLogGenerator();

    // Start generating simulated log events to the specified file in a background thread
    void start(const std::string& outputFile, int intervalMs = 200, int burstIntervalSeconds = 20);

    // Stop log generation and join background thread
    void stop();

    // Check if the generator is actively running
    bool isRunning() const;

private:
    void runLoop(std::string outputFile, int intervalMs, int burstIntervalSeconds);

    std::thread workerThread_;
    std::atomic<bool> isRunning_{false};
};
