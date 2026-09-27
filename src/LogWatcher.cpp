#include "LogWatcher.h"

#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <filesystem>
#include <utility>

LogWatcher::LogWatcher(std::string filePath)
    : filePath_(std::move(filePath)) {}

void LogWatcher::setCallback(LineCallback callback) {
    callback_ = std::move(callback);
}

void LogWatcher::stop() {
    isRunning_ = false;
}

bool LogWatcher::isRunning() const {
    return isRunning_.load();
}

void LogWatcher::start() {
    // If file does not exist yet, ensure it is created so we can tail it
    if (!std::filesystem::exists(filePath_)) {
        std::ofstream touch(filePath_, std::ios::app);
    }

    std::ifstream file(filePath_);
    if (!file.is_open()) {
        std::cerr << "[LogWatcher] Error: Could not open file: " << filePath_ << "\n";
        return;
    }

    // Seek to the end of the file to tail only new lines
    file.seekg(0, std::ios::end);
    std::streampos lastPos = file.tellg();

    isRunning_ = true;
    std::cout << "[LogWatcher] Started watching: " << filePath_ << " (Press Ctrl+C to stop)\n";

    while (isRunning_) {
        std::string line;
        bool readAny = false;

        // Read all newly available lines
        while (std::getline(file, line)) {
            readAny = true;
            if (callback_) {
                callback_(line);
            }
        }

        if (file.eof()) {
            file.clear(); // Clear EOF bit to allow reading subsequent writes
        }

        // Handle possible log file truncation / rotation
        std::error_code ec;
        auto currentSize = std::filesystem::file_size(filePath_, ec);
        if (!ec) {
            std::streampos currPos = file.tellg();
            if (currPos > static_cast<std::streampos>(currentSize)) {
                // File shrunk, rewind to start
                file.seekg(0, std::ios::beg);
            }
        }

        // Sleep briefly before polling again
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[LogWatcher] Stopped watching: " << filePath_ << "\n";
}
