#include "MetricEngine.h"
#include "AnomalyDetector.h"
#include "LogEvent.h"
#include "Alert.h"

#include <iostream>
#include <cmath>
#include <vector>
#include <chrono>

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

void testMetricEngineWindowAndCounts() {
    std::cout << "Running testMetricEngineWindowAndCounts...\n";
    MetricEngine engine;

    auto now = std::chrono::system_clock::now();

    // Insert events at known past timestamps
    engine.record("INFO", now - std::chrono::seconds(10));
    engine.record("INFO", now - std::chrono::seconds(5));
    engine.record("ERROR", now - std::chrono::seconds(2));
    engine.record("ERROR", now - std::chrono::seconds(1));
    engine.record("WARN", now - std::chrono::seconds(1));

    // Test counts within 15 seconds (should include all 5)
    TEST_ASSERT(engine.getCount("INFO", std::chrono::seconds(15)) == 2, "INFO count is 2 in 15s window");
    TEST_ASSERT(engine.getCount("ERROR", std::chrono::seconds(15)) == 2, "ERROR count is 2 in 15s window");
    TEST_ASSERT(engine.getCount("WARN", std::chrono::seconds(15)) == 1, "WARN count is 1 in 15s window");
    TEST_ASSERT(engine.getTotalCount(std::chrono::seconds(15)) == 5, "Total count is 5 in 15s window");

    // Test counts within 3 seconds (should only include the 2 ERRORs and 1 WARN)
    TEST_ASSERT(engine.getCount("INFO", std::chrono::seconds(3)) == 0, "Older INFO events excluded from 3s window");
    TEST_ASSERT(engine.getCount("ERROR", std::chrono::seconds(3)) == 2, "Recent ERROR events included in 3s window");
    TEST_ASSERT(engine.getTotalCount(std::chrono::seconds(3)) == 3, "Total count is 3 in 3s window");
}

void testAnomalyDetectorStatistics() {
    std::cout << "Running testAnomalyDetectorStatistics...\n";
    MetricEngine engine;
    AnomalyDetector detector(engine, "ERROR", 2.5, 5, std::chrono::seconds(60), 10);

    // Baseline with numbers: 2, 4, 4, 4, 5, 5, 7, 9
    // Sum = 40, Count = 8 -> Mean = 5.0
    // Variance = ((9 + 1 + 1 + 1 + 0 + 0 + 4 + 16) / 8) = 32 / 8 = 4.0
    // StdDev = sqrt(4.0) = 2.0
    std::vector<double> samples = {2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0};
    for (double s : samples) {
        detector.addHistorySample(s);
    }

    TEST_ASSERT(std::abs(detector.getBaselineMean() - 5.0) < 0.001, "Baseline mean accurately matches 5.0");
    TEST_ASSERT(std::abs(detector.getBaselineStdDev() - 2.0) < 0.001, "Baseline stddev accurately matches 2.0");
    TEST_ASSERT(detector.getHistorySize() == 8, "History size is 8");
}

void testAnomalyDetectorAlertSpike() {
    std::cout << "Running testAnomalyDetectorAlertSpike...\n";
    MetricEngine engine;
    AnomalyDetector detector(engine, "ERROR", /*zThreshold=*/2.5, /*minEventCount=*/5, std::chrono::seconds(30), 10);

    // Seed historical baseline with low error counts (mean ~2.0, stddev ~0.5)
    for (int i = 0; i < 8; ++i) {
        detector.addHistorySample(2.0);
    }

    bool alertFired = false;
    Alert capturedAlert;
    detector.setAlertCallback([&](const Alert& a) {
        alertFired = true;
        capturedAlert = a;
    });

    // 1. Normal traffic: record 2 ERRORs, evaluate -> should NOT alert
    auto now = std::chrono::system_clock::now();
    engine.record("ERROR", now);
    engine.record("ERROR", now);

    bool resNormal = detector.evaluate();
    TEST_ASSERT(!resNormal, "Normal error count does not trigger anomaly");
    TEST_ASSERT(!alertFired, "Alert callback not invoked for normal count");

    // 2. Incident traffic: inject spike of 15 ERRORs
    for (int i = 0; i < 15; ++i) {
        engine.record("ERROR", now);
    }

    bool resSpike = detector.evaluate();
    TEST_ASSERT(resSpike, "Sudden ERROR spike triggers anomaly detection");
    TEST_ASSERT(alertFired, "Alert callback invoked upon spike");
    TEST_ASSERT(capturedAlert.type == "ERROR_SPIKE", "Alert type is ERROR_SPIKE");
    TEST_ASSERT(capturedAlert.currentCount >= 15, "Observed count matches spike count");
    TEST_ASSERT(capturedAlert.zScore >= 2.5, "Z-score exceeds threshold 2.5");
}

void testAnomalyDetectorMinEventCountGuard() {
    std::cout << "Running testAnomalyDetectorMinEventCountGuard...\n";
    MetricEngine engine;
    // minEventCount = 5
    AnomalyDetector detector(engine, "ERROR", /*zThreshold=*/2.0, /*minEventCount=*/5, std::chrono::seconds(30), 10);

    // Zero-error baseline
    for (int i = 0; i < 5; ++i) {
        detector.addHistorySample(0.0);
    }

    bool alertFired = false;
    detector.setAlertCallback([&](const Alert&) { alertFired = true; });

    // Only 2 errors occurred. Even though 2 is infinitely higher than 0,
    // minEventCount is 5, so it must NOT trigger false positive alert.
    engine.record("ERROR", std::chrono::system_clock::now());
    engine.record("ERROR", std::chrono::system_clock::now());

    bool res = detector.evaluate();
    TEST_ASSERT(!res, "Low count below minEventCount does not trigger alert");
    TEST_ASSERT(!alertFired, "Alert guard correctly suppressed alert");
}

int main() {
    std::cout << "========================================\n";
    std::cout << "   DriftWatch Module 3 Test Suite\n";
    std::cout << "========================================\n";

    testMetricEngineWindowAndCounts();
    testAnomalyDetectorStatistics();
    testAnomalyDetectorAlertSpike();
    testAnomalyDetectorMinEventCountGuard();

    std::cout << "========================================\n";
    std::cout << "Tests Passed: " << g_testsPassed << "\n";
    std::cout << "Tests Failed: " << g_testsFailed << "\n";
    std::cout << "========================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
