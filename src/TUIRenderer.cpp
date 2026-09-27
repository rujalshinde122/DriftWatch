#include "TUIRenderer.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace {
// ANSI styling helpers
const char* RESET     = "\033[0m";
const char* BOLD      = "\033[1m";
const char* DIM       = "\033[2m";
const char* RED       = "\033[31m";
const char* GREEN     = "\033[32m";
const char* YELLOW    = "\033[33m";
const char* CYAN      = "\033[36m";

std::string getLevelBadge(const std::string& level) {
    if (level == "ERROR" || level == "FATAL") {
        return std::string(BOLD) + RED + "[" + level + "]" + RESET;
    } else if (level == "WARN") {
        return std::string(BOLD) + YELLOW + "[" + level + " ]" + RESET;
    } else if (level == "INFO") {
        return std::string(GREEN) + "[" + level + " ]" + RESET;
    }
    return std::string(DIM) + "[" + level + " ]" + RESET;
}

std::string getBarChar(int val, int maxVal) {
    if (maxVal <= 0 || val <= 0) return " ";
    const std::vector<std::string> bars = {" ", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    double ratio = static_cast<double>(val) / static_cast<double>(maxVal);
    size_t idx = static_cast<size_t>(ratio * (bars.size() - 1));
    idx = std::min(idx, bars.size() - 1);
    return bars[idx];
}
} // namespace

TUIRenderer::TUIRenderer(std::string watchedTarget, double zThreshold, int windowSeconds)
    : watchedTarget_(std::move(watchedTarget)),
      zThreshold_(zThreshold),
      windowSeconds_(windowSeconds),
      startTime_(std::chrono::steady_clock::now()) {
    errorHistory_.assign(10, 0); // initialize 10 blank slots
}

TUIRenderer::~TUIRenderer() {
    stop();
}

void TUIRenderer::pushLog(const LogEvent& event) {
    totalEventsCount_++;
    std::lock_guard<std::mutex> lock(dataMutex_);
    recentLogs_.push_back(event);
    if (recentLogs_.size() > 14) {
        recentLogs_.pop_front();
    }
}

void TUIRenderer::pushErrorSnapshot(int errorCount, double /*zScore*/) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    errorHistory_.push_back(errorCount);
    if (errorHistory_.size() > 10) {
        errorHistory_.erase(errorHistory_.begin());
    }
}

void TUIRenderer::pushAlert(const Alert& alert) {
    std::lock_guard<std::mutex> lock(dataMutex_);
    recentAlerts_.push_back(alert);
    if (recentAlerts_.size() > 5) {
        recentAlerts_.pop_front();
    }
}

void TUIRenderer::start(int refreshIntervalMs) {
    if (isRunning_) return;
    isRunning_ = true;

    // Switch to alternate screen buffer, clear screen, hide cursor
    std::cout << "\033[?1049h\033[2J\033[?25l" << std::flush;

    renderThread_ = std::thread(&TUIRenderer::renderLoop, this, refreshIntervalMs);
}

void TUIRenderer::stop() {
    if (isRunning_.exchange(false)) {
        if (renderThread_.joinable()) {
            renderThread_.join();
        }
        // Restore normal terminal buffer and show cursor
        std::cout << "\033[?25h\033[?1049l" << std::flush;
    }
}

bool TUIRenderer::isRunning() const {
    return isRunning_.load();
}

void TUIRenderer::renderLoop(int refreshIntervalMs) {
    while (isRunning_) {
        drawScreen();
        std::this_thread::sleep_for(std::chrono::milliseconds(refreshIntervalMs));
    }
}

void TUIRenderer::drawScreen() {
    auto now = std::chrono::steady_clock::now();
    auto uptimeSeconds = std::chrono::duration_cast<std::chrono::seconds>(now - startTime_).count();

    std::deque<LogEvent> logsCopy;
    std::vector<int> errorHistCopy;
    std::deque<Alert> alertsCopy;

    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        logsCopy = recentLogs_;
        errorHistCopy = errorHistory_;
        alertsCopy = recentAlerts_;
    }

    std::ostringstream buf;

    // Move cursor to top-left home position (no flicker)
    buf << "\033[H";

    // Header Banner
    buf << BOLD << CYAN << "╔══════════════════════════════════════════════════════════════════════════════════════════╗\n";
    buf << "║ " << BOLD << "DRIFTWATCH v1.0.0" << RESET << CYAN
        << " │ Real-Time Observability & Statistical Anomaly Daemon           ║\n";
    buf << "╚══════════════════════════════════════════════════════════════════════════════════════════╝" << RESET << "\n";

    // Status bar
    buf << BOLD << " Target: " << RESET << CYAN << watchedTarget_ << RESET
        << "  │ " << BOLD << "Uptime: " << RESET << uptimeSeconds << "s"
        << "  │ " << BOLD << "Events: " << RESET << totalEventsCount_.load()
        << "  │ " << BOLD << "Z-Threshold: " << RESET << std::fixed << std::setprecision(1) << zThreshold_
        << "  │ " << BOLD << "Window: " << RESET << windowSeconds_ << "s"
        << "  │ " << BOLD << "Alerts: " << RESET << (alertsCopy.empty() ? "\033[32m0\033[0m" : (std::string(RED) + std::to_string(alertsCopy.size()) + RESET))
        << "\n\n";

    auto repeatStr = [](const std::string& str, size_t count) {
        std::string res;
        for (size_t i = 0; i < count; ++i) res += str;
        return res;
    };

    // Section 1: Live Log Stream
    buf << BOLD << "─── [ LIVE LOG STREAM ] " << repeatStr("─", 64) << RESET << "\n";
    if (logsCopy.empty()) {
        buf << DIM << "  (Awaiting incoming log events...)" << RESET << "\n";
        for (int i = 0; i < 9; ++i) buf << "\n";
    } else {
        int printed = 0;
        for (const auto& log : logsCopy) {
            std::string msg = log.message;
            if (msg.length() > 62) {
                msg = msg.substr(0, 59) + "...";
            }
            buf << DIM << log.timestamp << RESET << " "
                << getLevelBadge(log.level) << " "
                << msg << "\n";
            printed++;
        }
        for (int i = printed; i < 10; ++i) buf << "\n";
    }

    // Section 2: Error Frequency Chart
    buf << "\n" << BOLD << "─── [ ERROR FREQUENCY & ANOMALY TRACKER ] " << repeatStr("─", 46) << RESET << "\n";
    buf << " History (last 10 windows):  ";

    int maxCount = 1;
    for (int count : errorHistCopy) {
        if (count > maxCount) maxCount = count;
    }

    for (int count : errorHistCopy) {
        if (count > 5) {
            buf << BOLD << RED << getBarChar(count, maxCount) << " " << RESET;
        } else if (count > 0) {
            buf << YELLOW << getBarChar(count, maxCount) << " " << RESET;
        } else {
            buf << DIM << "_" << " " << RESET;
        }
    }
    buf << "  [Current Window Max: " << maxCount << "]\n";

    // Section 3: Recent Incident Alerts
    buf << "\n" << BOLD << "─── [ RECENT INCIDENT ALERTS ] " << repeatStr("─", 57) << RESET << "\n";
    if (alertsCopy.empty()) {
        buf << GREEN << "  ✔ No anomalies detected. System operating within normal baseline." << RESET << "\n\n";
    } else {
        for (auto it = alertsCopy.rbegin(); it != alertsCopy.rend(); ++it) {
            buf << BOLD << RED << "  🚨 " << it->type << RESET
                << " | observed=" << it->currentCount
                << " | z=" << std::fixed << std::setprecision(2) << it->zScore
                << " | μ=" << it->baselineMean << " σ=" << it->baselineStdDev
                << " (" << it->message << ")\n";
        }
        if (alertsCopy.size() < 2) buf << "\n";
    }

    // Bottom Navigation Bar
    buf << DIM << "────────────────────────────────────────────────────────────────────────────────────────────\n"
        << " [Ctrl+C] Graceful Exit  │  Mode: Live TUI Monitor  │  DriftWatch Active" << RESET << "\n";

    std::cout << buf.str() << std::flush;
}
