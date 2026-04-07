# ============================================
# G07 Compression - Auto-Detecting Versions
# ============================================

CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -O3 -march=native -flto=auto -Wall -Wextra -I./include
CFLAGS = -O3 -march=native -flto=auto -I./include
LDFLAGS = -lpthread -flto=auto

SRC = src
OBJ = obj
BIN = bin

# ============================================
# SHARED OBJECTS (all versions)
# ============================================
SHARED = $(OBJ)/bwt.o $(OBJ)/mtf.o $(OBJ)/zero_rle.o \
         $(OBJ)/range_coder.o $(OBJ)/rans_static.o \
         $(OBJ)/file_io.o $(OBJ)/entropy_calculator.o \
         $(OBJ)/stream_header.o $(OBJ)/libsais.o

# ============================================
# VERSION-SPECIFIC MODELS
# ============================================
# v5 specific models
V5_MODELS = $(OBJ)/frequency_model.o $(OBJ)/context_model.o

# v6 specific models (v5 + multi-order)
V6_MODELS = $(V5_MODELS) $(OBJ)/multi_order_ppm.o \
            $(OBJ)/context_mixer.o $(OBJ)/prediction_utils.o

# v7+ models (when implemented)
V7_MODELS = $(V6_MODELS)

# ============================================
# AUTO-DETECT AVAILABLE VERSIONS
# ============================================
# Find all compressor_v*.cpp files and extract version numbers
COMPRESSOR_SOURCES := $(wildcard $(SRC)/compressor_v*.cpp)
VERSIONS := $(sort $(patsubst $(SRC)/compressor_v%.cpp,%,$(COMPRESSOR_SOURCES)))

# ============================================
# BUILD ALL AVAILABLE VERSIONS (default)
# ============================================
all: $(foreach v,$(VERSIONS),v$(v))
	@echo ""
	@echo "Build complete!"
	@echo ""
	@echo "Available binaries:"
	@$(foreach v,$(VERSIONS), \
		echo "  bin/g07-v$(v)-c  <input> <output>   -> v$(v) Compressor";)
	@$(foreach v,$(VERSIONS), \
		echo "  bin/g07-v$(v)-d  <input> <output>   -> v$(v) Decompressor";)
	@echo ""
	@echo "No flags needed - optimal settings hardcoded!"
	@echo ""

# ============================================
# DYNAMIC VERSION RULES
# ============================================
# This macro generates build rules for each version automatically
define VERSION_RULES

# Version target
v$(1): $$(BIN)/g07-v$(1)-c $$(BIN)/g07-v$(1)-d
	@echo "v$(1) built successfully"

# Compressor binary
$$(BIN)/g07-v$(1)-c: $$(OBJ)/compressor_v$(1).o $$(SHARED) $$(V$(1)_MODELS) | $$(BIN)
	@echo "Linking g07-v$(1)-c..."
	@$$(CXX) $$^ -o $$@ $$(LDFLAGS)
	@echo "  Binary size: $$$$(du -h $$@ | cut -f1)"

# Decompressor binary
$$(BIN)/g07-v$(1)-d: $$(OBJ)/decompressor_v$(1).o $$(SHARED) $$(V$(1)_MODELS) | $$(BIN)
	@echo "Linking g07-v$(1)-d..."
	@$$(CXX) $$^ -o $$@ $$(LDFLAGS)
	@echo "  Binary size: $$$$(du -h $$@ | cut -f1)"

# Compressor object
$$(OBJ)/compressor_v$(1).o: $$(SRC)/compressor_v$(1).cpp | $$(OBJ)
	@echo "Compiling compressor_v$(1).cpp..."
	@$$(CXX) $$(CXXFLAGS) -c $$< -o $$@

# Decompressor object
$$(OBJ)/decompressor_v$(1).o: $$(SRC)/decompressor_v$(1).cpp | $$(OBJ)
	@echo "Compiling decompressor_v$(1).cpp..."
	@$$(CXX) $$(CXXFLAGS) -c $$< -o $$@

endef

# Generate rules for each detected version
$(foreach v,$(VERSIONS),$(eval $(call VERSION_RULES,$(v))))

# ============================================
# COMPILE RULES FOR SHARED COMPONENTS
# ============================================

# Shared transform components
$(OBJ)/bwt.o: $(SRC)/transform/bwt.cpp | $(OBJ)
	@echo "Compiling bwt.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/mtf.o: $(SRC)/transform/mtf.cpp | $(OBJ)
	@echo "Compiling mtf.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/zero_rle.o: $(SRC)/transform/zero_rle.cpp | $(OBJ)
	@echo "Compiling zero_rle.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Shared arithmetic coders
$(OBJ)/range_coder.o: $(SRC)/arithmetic/range_coder.cpp | $(OBJ)
	@echo "Compiling range_coder.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/rans_static.o: $(SRC)/arithmetic/rans_static.cpp | $(OBJ)
	@echo "Compiling rans_static.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Shared models
$(OBJ)/frequency_model.o: $(SRC)/model/frequency_model.cpp | $(OBJ)
	@echo "Compiling frequency_model.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/context_model.o: $(SRC)/model/context_model.cpp | $(OBJ)
	@echo "Compiling context_model.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/multi_order_ppm.o: $(SRC)/model/multi_order_ppm.cpp | $(OBJ)
	@echo "Compiling multi_order_ppm.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/context_mixer.o: $(SRC)/model/context_mixer.cpp | $(OBJ)
	@echo "Compiling context_mixer.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/prediction_utils.o: $(SRC)/model/prediction_utils.cpp | $(OBJ)
	@echo "Compiling prediction_utils.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Shared utilities
$(OBJ)/file_io.o: $(SRC)/utils/file_io.cpp | $(OBJ)
	@echo "Compiling file_io.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/entropy_calculator.o: $(SRC)/utils/entropy_calculator.cpp | $(OBJ)
	@echo "Compiling entropy_calculator.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/stream_header.o: $(SRC)/utils/stream_header.cpp | $(OBJ)
	@echo "Compiling stream_header.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# libsais (C library)
$(OBJ)/libsais.o: $(SRC)/libsais.c | $(OBJ)
	@echo "Compiling libsais.c..."
	@$(CC) $(CFLAGS) -c $< -o $@

# ============================================
# UTILITIES
# ============================================

$(OBJ) $(BIN):
	mkdir -p $@

clean:
	rm -rf $(OBJ) $(BIN)
	@echo "Clean complete"

# Check binary sizes (must be < 1MB per teacher requirement)
check-sizes: all
	@echo ""
	@echo "Checking binary sizes (teacher requirement: < 1MB)..."
	@echo ""
	@for bin in $(BIN)/g07-*; do \
		if [ -f "$$bin" ]; then \
			size=$$(stat -f%z "$$bin" 2>/dev/null || stat -c%s "$$bin" 2>/dev/null); \
			size_kb=$$(($$size / 1024)); \
			size_mb=$$(($$size / 1048576)); \
			if [ $$size -gt 1048576 ]; then \
				echo "ERROR: $$bin: $$(du -h $$bin | cut -f1) (TOO BIG!)"; \
			else \
				echo "OK: $$bin: $$(du -h $$bin | cut -f1) ($${size_kb} KB)"; \
			fi; \
		fi; \
	done
	@echo ""

# Test first available version (usually v5)
test: all
	@echo "Running quick lossless verification..."
	@VERSION=$$(echo $(VERSIONS) | cut -d' ' -f1); \
	echo "Testing g07-v$$VERSION on file A (lossless verification built into benchmark)"; \
	bash benchmarks/compare.sh data/A

benchmark: all
	@echo "Running benchmarks..."
	@bash benchmarks/benchmark_all.sh

.PHONY: all clean check-sizes test benchmark $(foreach v,$(VERSIONS),v$(v))
