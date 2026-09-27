#pragma once

#include <string>
#include <functional>
#include <atomic>

class LogWatcher {
public:
    using LineCallback = std::function<void(const std::string&)>;

    explicit LogWatcher(std::string filePath);

    // Register callback for newly appended lines
    void setCallback(LineCallback callback);

    // Start watching and tailing the file (blocking until stop() is called or interrupted)
    void start();

    // Stop the tailing loop
    void stop();

    // Check if the watcher is currently running
    bool isRunning() const;

private:
    std::string filePath_;
    LineCallback callback_;
    std::atomic<bool> isRunning_{false};
};
