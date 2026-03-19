# Makefile for Lossless Data Compression Tool
# TAI Project #1 - 2025/26

CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -march=native -flto=auto -I./include
CFLAGS = -O3 -march=native -flto=auto -I./include
LDFLAGS = -lpthread -flto=auto

# Directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Source files
RANGE_SRC = $(SRC_DIR)/arithmetic/range_coder.cpp
RANS_SRC = $(SRC_DIR)/arithmetic/rans_static.cpp
MODEL_SRC = $(SRC_DIR)/model/frequency_model.cpp
CONTEXT_SRC = $(SRC_DIR)/model/context_model.cpp
UTILS_SRC = $(SRC_DIR)/utils/file_io.cpp
ENTROPY_SRC = $(SRC_DIR)/utils/entropy_calculator.cpp
BWT_SRC = $(SRC_DIR)/transform/bwt.cpp
MTF_SRC = $(SRC_DIR)/transform/mtf.cpp
ZRLE_SRC = $(SRC_DIR)/transform/zero_rle.cpp
HEADER_SRC = $(SRC_DIR)/utils/stream_header.cpp
LIBSAIS_SRC = $(SRC_DIR)/libsais.c
COMPRESSOR_SRC = $(SRC_DIR)/compressor.cpp
DECOMPRESSOR_SRC = $(SRC_DIR)/decompressor.cpp

# Object files
RANGE_OBJ = $(OBJ_DIR)/range_coder.o
RANS_OBJ = $(OBJ_DIR)/rans_static.o
LIBSAIS_OBJ = $(OBJ_DIR)/libsais.o
MODEL_OBJ = $(OBJ_DIR)/frequency_model.o
CONTEXT_OBJ = $(OBJ_DIR)/context_model.o
UTILS_OBJ = $(OBJ_DIR)/file_io.o
ENTROPY_OBJ = $(OBJ_DIR)/entropy_calculator.o
BWT_OBJ = $(OBJ_DIR)/bwt.o
MTF_OBJ = $(OBJ_DIR)/mtf.o
ZRLE_OBJ = $(OBJ_DIR)/zero_rle.o
HEADER_OBJ = $(OBJ_DIR)/stream_header.o
COMPRESSOR_OBJ = $(OBJ_DIR)/compressor.o
DECOMPRESSOR_OBJ = $(OBJ_DIR)/decompressor.o

# Common objects (used by both compressor and decompressor)
COMMON_OBJS = $(RANGE_OBJ) $(RANS_OBJ) $(MODEL_OBJ) $(CONTEXT_OBJ) $(UTILS_OBJ) $(ENTROPY_OBJ) $(BWT_OBJ) $(MTF_OBJ) $(ZRLE_OBJ) $(HEADER_OBJ) $(LIBSAIS_OBJ)

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

$(RANS_OBJ): $(RANS_SRC) | $(OBJ_DIR)
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

$(LIBSAIS_OBJ): $(LIBSAIS_SRC) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(MTF_OBJ): $(MTF_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(ZRLE_OBJ): $(ZRLE_SRC) | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(HEADER_OBJ): $(HEADER_SRC) | $(OBJ_DIR)
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
$(TEST_BWT): tests/test_bwt.cpp $(BWT_OBJ) $(LIBSAIS_OBJ) | $(BIN_DIR)
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
