cmake_minimum_required(VERSION 3.15)
project(DriftWatch VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find nlohmann_json or include homebrew path
find_package(nlohmann_json QUIET)
if (NOT nlohmann_json_FOUND)
    include_directories(/opt/homebrew/include /usr/local/include)
endif()

# Core library sources
set(DRIFTWATCH_SRCS
    src/LogWatcher.cpp
    src/EventParser.cpp
    src/MockLogGenerator.cpp
    src/MetricEngine.cpp
    src/AnomalyDetector.cpp
    src/ConfigLoader.cpp
    src/AlertEngine.cpp
    src/TUIRenderer.cpp
)

# Main executable
add_executable(driftwatch
    src/main.cpp
    ${DRIFTWATCH_SRCS}
)
target_include_directories(driftwatch PRIVATE src /opt/homebrew/include /usr/local/include)

if (TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(driftwatch PRIVATE nlohmann_json::nlohmann_json)
endif()

# Testing targets
enable_testing()

add_executable(test_module2
    tests/test_module2.cpp
    src/EventParser.cpp
    src/MockLogGenerator.cpp
)
target_include_directories(test_module2 PRIVATE src)
add_test(NAME Module2Tests COMMAND test_module2)

add_executable(test_module3
    tests/test_module3.cpp
    src/MetricEngine.cpp
    src/AnomalyDetector.cpp
)
target_include_directories(test_module3 PRIVATE src)
add_test(NAME Module3Tests COMMAND test_module3)

add_executable(test_module4_5
    tests/test_module4_5.cpp
    src/ConfigLoader.cpp
    src/AlertEngine.cpp
    src/TUIRenderer.cpp
)
target_include_directories(test_module4_5 PRIVATE src /opt/homebrew/include /usr/local/include)
if (TARGET nlohmann_json::nlohmann_json)
    target_link_libraries(test_module4_5 PRIVATE nlohmann_json::nlohmann_json)
endif()
add_test(NAME Module4And5Tests COMMAND test_module4_5)
