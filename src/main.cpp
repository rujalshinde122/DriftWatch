#include "LogWatcher.h"
#include "EventParser.h"
#include "MockLogGenerator.h"
#include "MetricEngine.h"
#include "AnomalyDetector.h"
#include "ConfigLoader.h"
#include "AlertEngine.h"
#include "TUIRenderer.h"

#include <iostream>
#include <csignal>
#include <memory>
#include <string>
#include <vector>
#include <filesystem>

namespace {
std::unique_ptr<LogWatcher> g_watcher;
std::unique_ptr<MockLogGenerator> g_mockGenerator;
std::unique_ptr<AnomalyDetector> g_detector;
std::unique_ptr<TUIRenderer> g_tui;

void signalHandler(int /*signum*/) {
    if (g_tui) {
        g_tui->stop();
    }
    if (g_detector) {
        g_detector->stop();
    }
    if (g_mockGenerator) {
        g_mockGenerator->stop();
    }
    if (g_watcher) {
        g_watcher->stop();
    }
}

void printHelp() {
    std::cout << "DriftWatch - Real-time Log Observability & Anomaly Daemon (v1.0.0)\n\n"
              << "Usage:\n"
              << "  driftwatch [options] [log_file]\n\n"
              << "Options:\n"
              << "  --mock             Run built-in log generator and watch simulated traffic\n"
              << "  --daemon           Run headless in daemon mode (disable interactive TUI)\n"
              << "  --config <path>    Path to JSON configuration file (default: ./config.json)\n"
              << "  -v, --version      Display version information\n"
              << "  -h, --help         Display this help message\n\n"
              << "Examples:\n"
              << "  driftwatch --mock\n"
              << "  driftwatch --mock --daemon\n"
              << "  driftwatch /var/log/system.log\n"
              << "  driftwatch --config custom_config.json\n";
}
} // namespace

int main(int argc, char* argv[]) {
    std::string configPath = "config.json";
    std::string targetFile = "";
    bool isMockMode = false;
    bool isDaemonMode = false;

    // Parse CLI arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "DriftWatch version 1.0.0\n";
            return 0;
        } else if (arg == "--mock") {
            isMockMode = true;
        } else if (arg == "--daemon") {
            isDaemonMode = true;
        } else if (arg == "--config" && i + 1 < argc) {
            configPath = argv[++i];
        } else if (arg[0] != '-') {
            targetFile = arg;
        }
    }

    // Load configuration
    Config config;
    if (std::filesystem::exists(configPath)) {
        config = ConfigLoader::load(configPath);
    }

    // Determine target file
    if (isMockMode) {
        targetFile = "/tmp/driftwatch_mock.log";
    } else if (targetFile.empty()) {
        if (!config.watchFiles.empty()) {
            targetFile = config.watchFiles[0];
        } else {
            std::cerr << "[DriftWatch] Error: No log file specified. Use --mock or provide a file path.\n";
            std::cerr << "Run 'driftwatch --help' for usage.\n";
            return 1;
        }
    }

    // Initialize subsystems
    MetricEngine metricEngine;
    AlertEngine alertEngine(config);

    g_detector = std::make_unique<AnomalyDetector>(
        metricEngine,
        "ERROR",
        config.anomalyThresholdZ,
        config.minEventCount,
        std::chrono::seconds(config.windowSeconds),
        config.historyDepth
    );

    // If mock mode is requested, start mock generator
    if (isMockMode) {
        g_mockGenerator = std::make_unique<MockLogGenerator>();
        g_mockGenerator->start(targetFile, /*intervalMs=*/150, /*burstIntervalSeconds=*/18);
    }

    // Initialize TUI if not in daemon mode
    if (!isDaemonMode) {
        g_tui = std::make_unique<TUIRenderer>(targetFile, config.anomalyThresholdZ, config.windowSeconds);
    }

    // Register signal handlers for clean exit
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Wire up Anomaly Alerts
    g_detector->setAlertCallback([&alertEngine, isDaemonMode](const Alert& alert) {
        // Dispatch to configured sinks (file, console, webhook)
        alertEngine.dispatch(alert);

        if (!isDaemonMode && g_tui) {
            g_tui->pushAlert(alert);
        }
    });

    // Start background detector evaluation
    g_detector->start(std::chrono::seconds(2));

    // Initialize watcher
    g_watcher = std::make_unique<LogWatcher>(targetFile);
    EventParser parser;

    // Connect LogWatcher to Parser and MetricEngine
    g_watcher->setCallback([&parser, &metricEngine, &targetFile, isDaemonMode](const std::string& line) {
        auto parsedEvent = parser.parse(line, targetFile);
        if (parsedEvent) {
            metricEngine.record(*parsedEvent);
            if (!isDaemonMode && g_tui) {
                g_tui->pushLog(*parsedEvent);
            } else if (isDaemonMode) {
                std::cout << "[" << parsedEvent->timestamp << "] ["
                          << parsedEvent->level << "] " << parsedEvent->message << "\n";
            }
        } else if (isDaemonMode) {
            std::cout << "[RAW] " << line << "\n";
        }
    });

    // If TUI mode, run background sync of error snapshots to TUI and start TUI
    std::thread tuiSyncThread;
    std::atomic<bool> tuiSyncRunning{true};
    if (!isDaemonMode && g_tui) {
        tuiSyncThread = std::thread([&tuiSyncRunning, &metricEngine, &config]() {
            while (tuiSyncRunning) {
                if (g_tui && g_detector) {
                    int errCount = metricEngine.getCount("ERROR", std::chrono::seconds(config.windowSeconds));
                    g_tui->pushErrorSnapshot(errCount, g_detector->getBaselineMean());
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
        g_tui->start(350);
    }

    if (isDaemonMode) {
        std::cout << "[DriftWatch] Daemon running on " << targetFile << " (Press Ctrl+C to stop)\n";
    }

    // Start watching (blocking)
    g_watcher->start();

    // Cleanup upon exit
    tuiSyncRunning = false;
    if (tuiSyncThread.joinable()) {
        tuiSyncThread.join();
    }
    if (g_tui) {
        g_tui->stop();
    }
    if (g_detector) {
        g_detector->stop();
    }
    if (g_mockGenerator) {
        g_mockGenerator->stop();
    }

    std::cout << "[DriftWatch] Shutdown complete. Goodbye!\n";
    return 0;
}
