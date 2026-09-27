#pragma once

#include <string>
#include <vector>

struct AlertSinksConfig {
    bool console = true;
    std::string file = "alerts.log";
    std::string webhook = "";
};

struct Config {
    std::vector<std::string> watchFiles = {"/tmp/driftwatch_mock.log"};
    double anomalyThresholdZ = 2.5;
    int minEventCount = 5;
    int windowSeconds = 30;
    int historyDepth = 10;
    AlertSinksConfig alertSinks;
};
