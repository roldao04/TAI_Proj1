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
SHARED = $(OBJ)/bwt.o $(OBJ)/mtf.o $(OBJ)/zero_rle.o $(OBJ)/lzp.o $(OBJ)/x86_filter.o \
         $(OBJ)/range_coder.o $(OBJ)/rans_static.o \
         $(OBJ)/bit_arithmetic_coder.o \
         $(OBJ)/file_io.o $(OBJ)/entropy_calculator.o \
         $(OBJ)/stream_header.o $(OBJ)/libsais.o

# ============================================
# VERSION-SPECIFIC MODELS
# ============================================
# Shared statistical models
STAT_MODELS = $(OBJ)/frequency_model.o $(OBJ)/context_model.o

# v6 specific models (per-block multi-pipeline, same as old v9)
V6_MODELS = $(STAT_MODELS)

# v7 specific models (interleaved rANS + bit-level PPM)
V7_MODELS = $(STAT_MODELS) $(OBJ)/multi_order_ppm.o \
            $(OBJ)/context_mixer.o $(OBJ)/prediction_utils.o $(OBJ)/bit_ppm.o

# v8 specific models (minimal - LZ77 only, no PPM)
V8_MODELS =

# v9 specific models (refined v5 production pipeline)
V9_MODELS = $(STAT_MODELS)

# v10 specific models (v6 pipeline + PRNG detector)
V10_MODELS = $(STAT_MODELS) $(OBJ)/prng_model.o

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

$(OBJ)/lzp.o: $(SRC)/transform/lzp.cpp | $(OBJ)
	@echo "Compiling lzp.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/x86_filter.o: $(SRC)/transform/x86_filter.cpp | $(OBJ)
	@echo "Compiling x86_filter.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Shared arithmetic coders
$(OBJ)/range_coder.o: $(SRC)/arithmetic/range_coder.cpp | $(OBJ)
	@echo "Compiling range_coder.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/rans_static.o: $(SRC)/arithmetic/rans_static.cpp | $(OBJ)
	@echo "Compiling rans_static.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/bit_arithmetic_coder.o: $(SRC)/arithmetic/bit_arithmetic_coder.cpp | $(OBJ)
	@echo "Compiling bit_arithmetic_coder.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Shared models
$(OBJ)/frequency_model.o: $(SRC)/model/frequency_model.cpp | $(OBJ)
	@echo "Compiling frequency_model.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/context_model.o: $(SRC)/model/context_model.cpp | $(OBJ)
	@echo "Compiling context_model.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/test_context_model.o: tests/test_context_model.cpp | $(OBJ)
	@echo "Compiling test_context_model.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/test_lzp.o: tests/test_lzp.cpp | $(OBJ)
	@echo "Compiling test_lzp.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/test_x86_filter.o: tests/test_x86_filter.cpp | $(OBJ)
	@echo "Compiling test_x86_filter.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/test_bwt.o: tests/test_bwt.cpp | $(OBJ)
	@echo "Compiling test_bwt.cpp..."
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

$(OBJ)/bit_ppm.o: $(SRC)/model/bit_ppm.cpp | $(OBJ)
	@echo "Compiling bit_ppm.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/prng_model.o: $(SRC)/model/prng_model.cpp | $(OBJ)
	@echo "Compiling prng_model.cpp..."
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

clean-results:
	@echo "Cleaning benchmark results..."
	@rm -f benchmarks/results.csv benchmarks/results.md
	@rm -rf benchmarks/tmp/*
	@echo "Results cleaned"

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

# Quick test: test single file with single version
# Usage: make test FILE=data/A VERSION=5
test:
ifdef FILE
ifdef VERSION
	@echo "Testing g07-v$(VERSION) on $(FILE)..."
	@bash benchmarks/test.sh $(FILE) $(VERSION)
else
	@echo "Usage: make test FILE=<file> VERSION=<version>"
	@echo "Example: make test FILE=data/A VERSION=9"
	@exit 1
endif
else
	@echo "Usage: make test FILE=<file> VERSION=<version>"
	@echo "Example: make test FILE=data/A VERSION=9"
	@exit 1
endif

# Full benchmark: all versions on all files + external tools
benchmark: all
	@echo "Running comprehensive benchmark (G07 versions + gzip/bzip2/xz/zstd)..."
	@bash benchmarks/benchmark_all.sh

$(BIN)/test_context_model: $(OBJ)/test_context_model.o $(OBJ)/context_model.o $(OBJ)/range_coder.o | $(BIN)
	@echo "Linking test_context_model..."
	@$(CXX) $^ -o $@ $(LDFLAGS)

$(BIN)/test_lzp: $(OBJ)/test_lzp.o $(OBJ)/lzp.o $(OBJ)/bwt.o $(OBJ)/mtf.o $(OBJ)/zero_rle.o $(OBJ)/libsais.o | $(BIN)
	@echo "Linking test_lzp..."
	@$(CXX) $^ -o $@ $(LDFLAGS)

$(BIN)/test_x86_filter: $(OBJ)/test_x86_filter.o $(OBJ)/x86_filter.o | $(BIN)
	@echo "Linking test_x86_filter..."
	@$(CXX) $^ -o $@ $(LDFLAGS)

$(BIN)/test_bwt: $(OBJ)/test_bwt.o $(OBJ)/bwt.o $(OBJ)/libsais.o | $(BIN)
	@echo "Linking test_bwt..."
	@$(CXX) $^ -o $@ $(LDFLAGS)

# Build and run the full unit-test suite (BWT, LZP, context model, x86 filter)
check: $(BIN)/test_bwt $(BIN)/test_lzp $(BIN)/test_context_model $(BIN)/test_x86_filter
	@echo ""
	@echo "=== Running unit tests ==="
	@./$(BIN)/test_bwt
	@./$(BIN)/test_lzp
	@./$(BIN)/test_context_model
	@./$(BIN)/test_x86_filter
	@echo ""
	@echo "All unit tests passed."

# ============================================
# PROFILE-GUIDED OPTIMIZATION
# ============================================
PGO_DIR = pgo_profiles

pgo: clean
	@echo "=== PGO Phase 1: Instrumented build ==="
	@mkdir -p $(OBJ) $(BIN) $(PGO_DIR)
	@$(MAKE) --no-print-directory CXXFLAGS="$(CXXFLAGS) -fprofile-generate=$(PGO_DIR)" \
	         CFLAGS="$(CFLAGS) -fprofile-generate=$(PGO_DIR)" \
	         LDFLAGS="$(LDFLAGS) -fprofile-generate=$(PGO_DIR)" v9
	@echo "=== PGO Phase 2: Training run ==="
	@for f in data/*; do \
		echo "  Training on $$f..."; \
		./bin/g07-v9-c "$$f" /tmp/pgo_compressed.tmp 2>/dev/null; \
		./bin/g07-v9-d /tmp/pgo_compressed.tmp /tmp/pgo_decompressed.tmp 2>/dev/null; \
		rm -f /tmp/pgo_compressed.tmp /tmp/pgo_decompressed.tmp; \
	done
	@echo "=== PGO Phase 3: Optimized rebuild ==="
	@rm -rf $(OBJ) $(BIN)
	@mkdir -p $(OBJ) $(BIN)
	@$(MAKE) --no-print-directory CXXFLAGS="$(CXXFLAGS) -fprofile-use=$(PGO_DIR) -fprofile-correction" \
	         CFLAGS="$(CFLAGS) -fprofile-use=$(PGO_DIR) -fprofile-correction" \
	         LDFLAGS="$(LDFLAGS) -fprofile-use=$(PGO_DIR)" v9
	@rm -rf $(PGO_DIR)
	@echo "=== PGO build complete ==="

.PHONY: all clean check check-sizes test benchmark pgo $(foreach v,$(VERSIONS),v$(v))
