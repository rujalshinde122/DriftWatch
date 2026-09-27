#include "EventParser.h"
#include "MockLogGenerator.h"

#include <iostream>
#include <cassert>
#include <fstream>
#include <thread>
#include <chrono>
#include <filesystem>

// Simple, zero-dependency test runner
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

void testParserValidLines() {
    std::cout << "Running testParserValidLines...\n";
    EventParser parser;

    // Test 1: Standard INFO line
    std::string line1 = "[2026-09-26 14:02:31] INFO  Request processed in 42ms [route=/api/users]";
    auto ev1 = parser.parse(line1, "app.log");
    TEST_ASSERT(ev1.has_value(), "Valid INFO line is parsed");
    if (ev1) {
        TEST_ASSERT(ev1->timestamp == "2026-09-26 14:02:31", "Timestamp matches");
        TEST_ASSERT(ev1->level == "INFO", "Level matches INFO");
        TEST_ASSERT(ev1->message == "Request processed in 42ms [route=/api/users]", "Message matches");
        TEST_ASSERT(ev1->source == "app.log", "Source matches");
    }

    // Test 2: Standard ERROR line
    std::string line2 = "[2026-09-26 14:02:45] ERROR Database connection timeout after 5000ms";
    auto ev2 = parser.parse(line2);
    TEST_ASSERT(ev2.has_value(), "Valid ERROR line is parsed");
    if (ev2) {
        TEST_ASSERT(ev2->level == "ERROR", "Level matches ERROR");
        TEST_ASSERT(ev2->message == "Database connection timeout after 5000ms", "Message matches");
    }

    // Test 3: WARN line with multiple spaces
    std::string line3 = "[2026-09-26 09:15:00]   WARN    Cache missed for key: session_123";
    auto ev3 = parser.parse(line3);
    TEST_ASSERT(ev3.has_value(), "Line with flexible whitespace is parsed");
    if (ev3) {
        TEST_ASSERT(ev3->level == "WARN", "Level matches WARN");
    }
}

void testParserInvalidLines() {
    std::cout << "Running testParserInvalidLines...\n";
    EventParser parser;

    // Test: Empty line
    TEST_ASSERT(!parser.parse("").has_value(), "Empty line returns nullopt");

    // Test: Missing timestamp brackets
    TEST_ASSERT(!parser.parse("2026-09-26 14:02:31 INFO Hello").has_value(), "Missing brackets returns nullopt");

    // Test: Invalid timestamp format
    TEST_ASSERT(!parser.parse("[26-09-2026 14:02] INFO Hello").has_value(), "Wrong date format returns nullopt");

    // Test: Random unformatted line
    TEST_ASSERT(!parser.parse("Segmentation fault (core dumped)").has_value(), "Random text returns nullopt");
}

void testMockLogGenerator() {
    std::cout << "Running testMockLogGenerator...\n";
    std::string testFile = "/tmp/driftwatch_test_mock.log";

    // Clean up test file if left over
    std::filesystem::remove(testFile);

    MockLogGenerator generator;
    generator.start(testFile, /*intervalMs=*/50, /*burstIntervalSeconds=*/0);
    TEST_ASSERT(generator.isRunning(), "MockLogGenerator is marked running");

    // Let it generate a few log lines
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    generator.stop();
    TEST_ASSERT(!generator.isRunning(), "MockLogGenerator stopped cleanly");

    // Verify file was created and contains valid parseable log lines
    std::ifstream in(testFile);
    TEST_ASSERT(in.is_open(), "Mock log file was created");

    EventParser parser;
    std::string line;
    int lineCount = 0;
    int parsedCount = 0;

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        lineCount++;
        if (parser.parse(line, testFile)) {
            parsedCount++;
        }
    }

    TEST_ASSERT(lineCount > 0, "At least one log line was generated");
    TEST_ASSERT(lineCount == parsedCount, "All generated mock log lines are validly parsed");

    // Clean up
    std::filesystem::remove(testFile);
}

int main() {
    std::cout << "========================================\n";
    std::cout << "   DriftWatch Module 2 Test Suite\n";
    std::cout << "========================================\n";

    testParserValidLines();
    testParserInvalidLines();
    testMockLogGenerator();

    std::cout << "========================================\n";
    std::cout << "Tests Passed: " << g_testsPassed << "\n";
    std::cout << "Tests Failed: " << g_testsFailed << "\n";
    std::cout << "========================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
