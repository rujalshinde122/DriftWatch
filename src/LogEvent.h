#pragma once

#include <string>

// Represents a single parsed log event
struct LogEvent {
    std::string timestamp;
    std::string level;    // e.g. INFO, WARN, ERROR, FATAL
    std::string message;
    std::string source;   // File path or source identifier
};
