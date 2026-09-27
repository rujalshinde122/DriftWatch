#include "ConfigLoader.h"
#include "AlertEngine.h"
#include "TUIRenderer.h"
#include "Alert.h"
#include "LogEvent.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <cassert>

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            std::cout << "  [PASS] " << msg << "\n"; \
            g_testsPassed++; \
        } else { \
            std::cerr << "  [FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            g_testsFailed++; \
        } \
    } while (0)

void testConfigLoader() {
    std::cout << "Running testConfigLoader...\n";

    // 1. Non-existent file should safely fallback to defaults
    Config def = ConfigLoader::load("/tmp/non_existent_config_12345.json");
    TEST_ASSERT(def.anomalyThresholdZ == 2.5, "Default anomalyThresholdZ is 2.5");
    TEST_ASSERT(def.windowSeconds == 30, "Default windowSeconds is 30");

    // 2. Round-trip save and load
    std::string testPath = "/tmp/test_dw_config.json";
    Config custom;
    custom.watchFiles = {"/var/log/test.log"};
    custom.anomalyThresholdZ = 3.2;
    custom.minEventCount = 10;
    custom.windowSeconds = 45;
    custom.alertSinks.console = false;
    custom.alertSinks.file = "/tmp/test_alerts.log";

    bool saved = ConfigLoader::save(custom, testPath);
    TEST_ASSERT(saved, "Config saved successfully to JSON");

    Config loaded = ConfigLoader::load(testPath);
    TEST_ASSERT(loaded.watchFiles.size() == 1 && loaded.watchFiles[0] == "/var/log/test.log", "Loaded watchFiles match");
    TEST_ASSERT(loaded.anomalyThresholdZ == 3.2, "Loaded anomalyThresholdZ is 3.2");
    TEST_ASSERT(loaded.minEventCount == 10, "Loaded minEventCount is 10");
    TEST_ASSERT(loaded.windowSeconds == 45, "Loaded windowSeconds is 45");
    TEST_ASSERT(!loaded.alertSinks.console, "Loaded alertSinks.console is false");
    TEST_ASSERT(loaded.alertSinks.file == "/tmp/test_alerts.log", "Loaded alertSinks.file matches");

    std::filesystem::remove(testPath);
}

void testAlertEngineDispatch() {
    std::cout << "Running testAlertEngineDispatch...\n";

    std::string testAlertFile = "/tmp/test_engine_alerts.log";
    std::filesystem::remove(testAlertFile);

    Config cfg;
    cfg.alertSinks.console = false; // silence console in test
    cfg.alertSinks.file = testAlertFile;

    AlertEngine engine(cfg);
    TEST_ASSERT(engine.getTotalAlertsCount() == 0, "Initial alert count is 0");

    Alert alert;
    alert.triggeredAt = std::chrono::system_clock::now();
    alert.type = "ERROR_SPIKE";
    alert.level = "ERROR";
    alert.message = "Test spike detected";
    alert.zScore = 4.8;
    alert.currentCount = 18;
    alert.baselineMean = 2.0;
    alert.baselineStdDev = 1.0;

    engine.dispatch(alert);

    TEST_ASSERT(engine.getTotalAlertsCount() == 1, "Alert count incremented to 1");
    auto recent = engine.getRecentAlerts(5);
    TEST_ASSERT(recent.size() == 1, "Recent alerts contains 1 entry");
    TEST_ASSERT(recent[0].zScore == 4.8, "Recent alert zScore matches 4.8");

    // Check file output
    std::ifstream file(testAlertFile);
    TEST_ASSERT(file.is_open(), "Alert sink file was created");
    std::string line;
    std::getline(file, line);
    TEST_ASSERT(line.find("[ALERT]") != std::string::npos, "Alert log contains [ALERT] tag");
    TEST_ASSERT(line.find("ERROR_SPIKE") != std::string::npos, "Alert log contains ERROR_SPIKE");
    TEST_ASSERT(line.find("4.80") != std::string::npos, "Alert log contains formatted z-score");

    file.close();
    std::filesystem::remove(testAlertFile);
}

void testTUIRendererPushBuffers() {
    std::cout << "Running testTUIRendererPushBuffers...\n";

    TUIRenderer tui("/tmp/test.log", 2.5, 30);
    // Push events to verify thread-safe buffers without throwing
    LogEvent ev;
    ev.timestamp = "2026-09-26 12:00:00";
    ev.level = "INFO";
    ev.message = "TUI test message";

    tui.pushLog(ev);
    tui.pushErrorSnapshot(5, 1.2);

    Alert alert;
    alert.triggeredAt = std::chrono::system_clock::now();
    alert.type = "SPIKE";
    alert.level = "ERROR";
    alert.message = "TUI test alert";
    alert.zScore = 3.0;
    alert.currentCount = 10;
    alert.baselineMean = 2.0;
    alert.baselineStdDev = 1.0;
    tui.pushAlert(alert);

    TEST_ASSERT(!tui.isRunning(), "TUI is not started by default");
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  DriftWatch Modules 4 & 5 Test Suite\n";
    std::cout << "========================================\n";

    testConfigLoader();
    testAlertEngineDispatch();
    testTUIRendererPushBuffers();

    std::cout << "========================================\n";
    std::cout << "Tests Passed: " << g_testsPassed << "\n";
    std::cout << "Tests Failed: " << g_testsFailed << "\n";
    std::cout << "========================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
