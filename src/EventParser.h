#pragma once

#include "LogEvent.h"
#include <optional>
#include <string>
#include <regex>

class EventParser {
public:
    EventParser();

    // Parses a raw log line in the standard format:
    // [YYYY-MM-DD HH:MM:SS] LEVEL  Message text
    // Returns std::nullopt if the line does not match the expected pattern.
    std::optional<LogEvent> parse(const std::string& rawLine, const std::string& source = "") const;

private:
    std::regex logPattern_;
};
