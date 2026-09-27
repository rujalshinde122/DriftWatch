CXX ?= clang++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -Isrc -I/opt/homebrew/include -I/usr/local/include

SRCS = src/main.cpp src/LogWatcher.cpp src/EventParser.cpp src/MockLogGenerator.cpp \
       src/MetricEngine.cpp src/AnomalyDetector.cpp src/ConfigLoader.cpp \
       src/AlertEngine.cpp src/TUIRenderer.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = driftwatch

TEST2_SRCS = tests/test_module2.cpp src/EventParser.cpp src/MockLogGenerator.cpp
TEST2_OBJS = $(TEST2_SRCS:.cpp=.o)
TEST2_TARGET = test_module2

TEST3_SRCS = tests/test_module3.cpp src/MetricEngine.cpp src/AnomalyDetector.cpp
TEST3_OBJS = $(TEST3_SRCS:.cpp=.o)
TEST3_TARGET = test_module3

TEST4_SRCS = tests/test_module4_5.cpp src/ConfigLoader.cpp src/AlertEngine.cpp src/TUIRenderer.cpp
TEST4_OBJS = $(TEST4_SRCS:.cpp=.o)
TEST4_TARGET = test_module4_5

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

test: $(TEST2_TARGET) $(TEST3_TARGET) $(TEST4_TARGET)
	./$(TEST2_TARGET)
	./$(TEST3_TARGET)
	./$(TEST4_TARGET)

$(TEST2_TARGET): $(TEST2_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST2_OBJS)

$(TEST3_TARGET): $(TEST3_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST3_OBJS)

$(TEST4_TARGET): $(TEST4_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST4_OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TEST2_OBJS) $(TEST3_OBJS) $(TEST4_OBJS) $(TARGET) $(TEST2_TARGET) $(TEST3_TARGET) $(TEST4_TARGET)

.PHONY: all clean test
