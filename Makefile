CXX = g++
CXXFLAGS = -g -std=c++20 -I../dep/databento-cpp/include -I../dep/databento-cpp/src -I../dep/gtest/include -Isrc/core -O3 -Wall -Wno-unused-function
GTEST_LDFLAGS = -L../dep/gtest/build/lib -lgtest -lgtest_main -pthread -lssl -lcrypto -lzstd

MAKEFLAGS += -j32

BUILD_DIR = build

# Recursively find all databento source files
DATABENTO_SRC_DIR = ../dep/databento-cpp/src
DATABENTO_SRC = $(shell find $(DATABENTO_SRC_DIR) -name '*.cpp')
DATABENTO_OBJ = $(patsubst $(DATABENTO_SRC_DIR)/%.cpp,$(BUILD_DIR)/databento_obj/%.o,$(DATABENTO_SRC))

# Source files for our project
CORE_SOURCES = src/core/OrderBook.cpp src/core/FlatMapOrderBook.cpp
APP_GENERATE_STATS_SOURCE = src/apps/generate_stats.cpp src/apps/cli.cpp
APP_JSON_GEN_SOURCE = src/apps/json_generator.cpp src/apps/cli.cpp
APP_BENCHMARK_SOURCE = src/apps/benchmark.cpp
TEST_SOURCE = src/tests/tests.cpp
TEST_DATA_GEN = generate_test_data
TEST_DATA_GEN_SOURCE = src/apps/generate_test_data.cpp
TEST_DATA_GEN_OBJECTS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(TEST_DATA_GEN_SOURCE))

GENERATE_STATS_SOURCES = $(CORE_SOURCES) $(APP_GENERATE_STATS_SOURCE)
JSON_GEN_SOURCES = $(CORE_SOURCES) $(APP_JSON_GEN_SOURCE)
BENCHMARK_SOURCES = $(CORE_SOURCES) $(APP_BENCHMARK_SOURCE)
TEST_SOURCES = $(CORE_SOURCES) $(TEST_SOURCE)

# Object files
GENERATE_STATS_OBJECTS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(GENERATE_STATS_SOURCES))
JSON_GEN_OBJECTS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(JSON_GEN_SOURCES))
BENCHMARK_OBJECTS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(BENCHMARK_SOURCES))
TEST_OBJECTS = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(TEST_SOURCES))

# Executable names
GENERATE_STATS_EXECUTABLE = generate_stats
JSON_GEN_EXECUTABLE = json_generator
BENCHMARK_EXECUTABLE = benchmark
TEST_EXECUTABLE = tests

.PHONY: all clean test

all: $(GENERATE_STATS_EXECUTABLE) $(JSON_GEN_EXECUTABLE) $(BENCHMARK_EXECUTABLE) $(TEST_EXECUTABLE) $(TEST_DATA_GEN)

# Rule to build the extreme test cases generator executable
$(TEST_DATA_GEN): $(TEST_DATA_GEN_OBJECTS) $(DATABENTO_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lssl -lcrypto -lzstd

# Rule to build the json generator executable
$(JSON_GEN_EXECUTABLE): $(JSON_GEN_OBJECTS) $(DATABENTO_OBJ)
	$(CXX) $^ -o $@ -lssl -lcrypto -lzstd

# Rule to build the generate_stats executable
$(GENERATE_STATS_EXECUTABLE): $(GENERATE_STATS_OBJECTS) $(DATABENTO_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lssl -lcrypto -lzstd

# Rule to build the benchmark executable
$(BENCHMARK_EXECUTABLE): $(BENCHMARK_OBJECTS) $(DATABENTO_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lssl -lcrypto -lzstd -L../dep/benchmark/build/lib -lbenchmark

# Rule to build the test executable
test: $(TEST_EXECUTABLE)

$(TEST_EXECUTABLE): $(TEST_OBJECTS) $(DATABENTO_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(GTEST_LDFLAGS)

# Generic rule to build object files from our project
$(BUILD_DIR)/src/core/%.o: src/core/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/src/apps/%.o: src/apps/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/src/tests/%.o: src/tests/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Specific rule to build databento object files
$(BUILD_DIR)/databento_obj/%.o: $(DATABENTO_SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(GENERATE_STATS_EXECUTABLE) $(JSON_GEN_EXECUTABLE) $(BENCHMARK_EXECUTABLE) $(TEST_EXECUTABLE) $(TEST_DATA_GEN)
	rm -rf $(BUILD_DIR)
