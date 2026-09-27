#include "EventParser.h"

EventParser::EventParser()
    : logPattern_(R"(^\[(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\]\s+([A-Z]+)\s+(.*)$)") {}

std::optional<LogEvent> EventParser::parse(const std::string& rawLine, const std::string& source) const {
    std::smatch match;
    if (std::regex_match(rawLine, match, logPattern_)) {
        LogEvent event;
        event.timestamp = match[1].str();
        event.level     = match[2].str();
        event.message   = match[3].str();
        event.source    = source;
        return event;
    }
    return std::nullopt;
}
