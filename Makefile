# Makefile for Lossless Data Compression Tool
# TAI Project #1 - 2025/26

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -I./include
LDFLAGS =

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Source files
RANGE_SRC = $(SRC_DIR)/arithmetic/range_coder.cpp
MODEL_SRC = $(SRC_DIR)/model/frequency_model.cpp
CONTEXT_SRC = $(SRC_DIR)/model/context_model.cpp
UTILS_SRC = $(SRC_DIR)/utils/file_io.cpp
ENTROPY_SRC = $(SRC_DIR)/utils/entropy_calculator.cpp
BWT_SRC = $(SRC_DIR)/transform/bwt.cpp
COMPRESSOR_SRC = $(SRC_DIR)/compressor.cpp
DECOMPRESSOR_SRC = $(SRC_DIR)/decompressor.cpp

# Object files
RANGE_OBJ = $(OBJ_DIR)/range_coder.o
MODEL_OBJ = $(OBJ_DIR)/frequency_model.o
CONTEXT_OBJ = $(OBJ_DIR)/context_model.o
UTILS_OBJ = $(OBJ_DIR)/file_io.o
ENTROPY_OBJ = $(OBJ_DIR)/entropy_calculator.o
BWT_OBJ = $(OBJ_DIR)/bwt.o
COMPRESSOR_OBJ = $(OBJ_DIR)/compressor.o
DECOMPRESSOR_OBJ = $(OBJ_DIR)/decompressor.o

# Common objects (used by both compressor and decompressor)
COMMON_OBJS = $(RANGE_OBJ) $(MODEL_OBJ) $(CONTEXT_OBJ) $(UTILS_OBJ) $(ENTROPY_OBJ) $(BWT_OBJ)

# Executables
COMPRESSOR = $(BIN_DIR)/compress
DECOMPRESSOR = $(BIN_DIR)/decompress
TEST_BWT = $(BIN_DIR)/test_bwt

# Targets
.PHONY: all clean test benchmark

all: $(COMPRESSOR) $(DECOMPRESSOR)

# Create directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compile object files
$(RANGE_OBJ): $(RANGE_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MODEL_OBJ): $(MODEL_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(CONTEXT_OBJ): $(CONTEXT_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(UTILS_OBJ): $(UTILS_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(ENTROPY_OBJ): $(ENTROPY_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BWT_OBJ): $(BWT_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(COMPRESSOR_OBJ): $(COMPRESSOR_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(DECOMPRESSOR_OBJ): $(DECOMPRESSOR_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link executables
$(COMPRESSOR): $(COMPRESSOR_OBJ) $(COMMON_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(DECOMPRESSOR): $(DECOMPRESSOR_OBJ) $(COMMON_OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Test executable
$(TEST_BWT): tests/test_bwt.cpp $(BWT_OBJ) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Run tests
test: $(COMPRESSOR) $(DECOMPRESSOR) $(TEST_BWT)
	@echo "Running BWT unit tests..."
	@$(TEST_BWT)
	@echo ""
	@echo "Running integration test..."
	@bash tests/verify_lossless.sh

# Run comprehensive benchmark on all data files
benchmark: $(COMPRESSOR) $(DECOMPRESSOR)
	@echo "Running comprehensive benchmark..."
	@bash benchmarks/benchmark_all.sh
