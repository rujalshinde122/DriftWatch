#include "MockLogGenerator.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <random>
#include <vector>

namespace {
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &now_c);
#else
    localtime_r(&now_c, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
} // namespace

MockLogGenerator::MockLogGenerator() = default;

MockLogGenerator::~MockLogGenerator() {
    stop();
}

void MockLogGenerator::start(const std::string& outputFile, int intervalMs, int burstIntervalSeconds) {
    if (isRunning_) {
        return;
    }
    isRunning_ = true;
    workerThread_ = std::thread(&MockLogGenerator::runLoop, this, outputFile, intervalMs, burstIntervalSeconds);
}

void MockLogGenerator::stop() {
    if (isRunning_.exchange(false)) {
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }
}

bool MockLogGenerator::isRunning() const {
    return isRunning_.load();
}

void MockLogGenerator::runLoop(std::string outputFile, int intervalMs, int burstIntervalSeconds) {
    std::ofstream out(outputFile, std::ios::app);
    if (!out.is_open()) {
        std::cerr << "[MockLogGenerator] Error: Could not open file for writing: " << outputFile << "\n";
        isRunning_ = false;
        return;
    }

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist100(1, 100);
    std::uniform_int_distribution<int> latencyDist(15, 120);

    const std::vector<std::string> routes = {
        "/api/v1/users", "/api/v1/auth/login", "/api/v1/orders",
        "/api/v1/products", "/healthz", "/metrics"
    };
    std::uniform_int_distribution<size_t> routeDist(0, routes.size() - 1);

    auto lastBurstTime = std::chrono::steady_clock::now();

    while (isRunning_) {
        auto nowSteady = std::chrono::steady_clock::now();
        auto elapsedSeconds = std::chrono::duration_cast<std::chrono::seconds>(nowSteady - lastBurstTime).count();

        // Check if it's time to inject an anomaly error burst
        if (burstIntervalSeconds > 0 && elapsedSeconds >= burstIntervalSeconds) {
            lastBurstTime = nowSteady;
            for (int i = 0; i < 12 && isRunning_; ++i) {
                out << "[" << getCurrentTimestamp() << "] ERROR "
                    << "Database connection pool exhausted: timeout after 5000ms [pool=primary, worker=" << i << "]"
                    << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
            continue;
        }

        int roll = dist100(rng);
        std::string level;
        std::string msg;

        if (roll <= 80) {
            level = "INFO";
            msg = "Request processed in " + std::to_string(latencyDist(rng)) + "ms [route=" + routes[routeDist(rng)] + "]";
        } else if (roll <= 92) {
            level = "WARN";
            msg = "High cache miss ratio detected on cluster node-" + std::to_string(roll % 3 + 1);
        } else {
            level = "ERROR";
            msg = "Upstream service timeout while querying payment-gateway";
        }

        out << "[" << getCurrentTimestamp() << "] " << level << "  " << msg << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
}
