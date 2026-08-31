# All of the following are overridable from the command line, so that CI can
# build portably while a local `make` keeps the original tuned defaults:
#
#   make                                  # -march=native, as before
#   make ARCHFLAGS= CXX=g++-14            # portable, reproducible build
#   make WARNFLAGS="-Wall -Wextra -Werror"
#
# ARCHFLAGS must be overridable because -march=native tunes the binary to
# whichever CPU the build happens to run on, which is wrong for CI.
# Respect CXX from the environment or the command line, but override make's
# built-in default (c++) so a plain `make` still uses g++ as it always has.
ifeq ($(origin CXX),default)
    CXX := g++
endif
ARCHFLAGS ?= -march=native
WARNFLAGS ?= -Wall -Wextra -Wpedantic

CXXFLAGS := -std=c++20 -O3 $(ARCHFLAGS) $(WARNFLAGS)
INCLUDES := -Iinclude
DEPFLAGS := -MMD -MP

SRC_DIR      := src
BENCH_DIR    := benchmarks
TEST_DIR     := tests
PRELIM_DIR   := tests/prelim-tests
BUILD_DIR    := build
BIN_DIR      := bin

CORE_SRCS := $(wildcard $(SRC_DIR)/*.cpp)
CORE_OBJS := $(CORE_SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

BENCH_SRCS  := $(wildcard $(BENCH_DIR)/*.cpp)
TEST_SRCS   := $(wildcard $(TEST_DIR)/*.cpp)
PRELIM_SRCS := $(wildcard $(PRELIM_DIR)/*.cpp)

BENCH_BINS  := $(BENCH_SRCS:$(BENCH_DIR)/%.cpp=$(BIN_DIR)/%)
TEST_BINS   := $(TEST_SRCS:$(TEST_DIR)/%.cpp=$(BIN_DIR)/%)
PRELIM_BINS := $(PRELIM_SRCS:$(PRELIM_DIR)/%.cpp=$(BIN_DIR)/%)

all: $(BENCH_BINS) $(TEST_BINS) $(PRELIM_BINS)

$(BIN_DIR)/%: $(BENCH_DIR)/%.cpp $(CORE_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

$(BIN_DIR)/%: $(TEST_DIR)/%.cpp $(CORE_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

$(BIN_DIR)/%: $(PRELIM_DIR)/%.cpp $(CORE_OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

# Without this, make classifies build/%.o as intermediate files in the
# .cpp -> .o -> binary chain and deletes them once linking finishes, so every
# subsequent `make` recompiles all core objects from scratch.
.SECONDARY: $(CORE_OBJS)

-include $(CORE_OBJS:.o=.d)

# Tests that gate CI. The prelim-tests/ binaries are exploratory: they print
# their results but always exit 0, so they are built but never gated on.
# stress_test is excluded here because it requires a data file; see test-stress.
GATING_TESTS := correctness-test \
                adaptive_integrity_test \
                precision_encoder_test \
                precision_decoder_test

test: $(TEST_BINS)
	@set -e; \
	for t in $(GATING_TESTS); do \
	    printf '%-28s ' "$$t"; \
	    if ./$(BIN_DIR)/$$t > /dev/null 2>&1; then \
	        echo "PASS"; \
	    else \
	        echo "FAIL"; exit 1; \
	    fi; \
	done

# Requires a CSV of prices: make test-stress DATA=data/market_data.csv
test-stress: $(BIN_DIR)/stress_test
	@test -n "$(DATA)" || { echo "set DATA=<path to csv>"; exit 1; }
	./$(BIN_DIR)/stress_test $(DATA)

clean:
	@echo "Cleaning up..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean test test-stress