# ============================================
# G07 Compression - Version-Specific Binaries
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

# v5 specific models
V5_MODELS = $(OBJ)/frequency_model.o $(OBJ)/context_model.o

# v7 specific models (v5 + multi-order)
V7_MODELS = $(V5_MODELS) $(OBJ)/multi_order_ppm.o \
            $(OBJ)/context_mixer.o $(OBJ)/prediction_utils.o

# v8 specific models (v7 + bit-level) - placeholder for future
V8_MODELS = $(V7_MODELS)

# ============================================
# BUILD ALL VERSIONS (default)
# ============================================
all: v5
	@echo ""
	@echo "═══════════════════════════════════════════════════════════"
	@echo "✅ Build complete!"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""
	@echo "Available binaries:"
	@echo "  bin/g07-v5-c  <input> <output>   → v5.0 Compressor (54.73%)"
	@echo "  bin/g07-v5-d  <input> <output>   → v5.0 Decompressor"
	@echo ""
	@echo "No flags needed - optimal settings hardcoded!"
	@echo "═══════════════════════════════════════════════════════════"
	@echo ""

# ============================================
# VERSION-SPECIFIC TARGETS
# ============================================

v5: $(BIN)/g07-v5-c $(BIN)/g07-v5-d
	@echo "✅ v5.0 built successfully"

v7: $(BIN)/g07-v7-c $(BIN)/g07-v7-d
	@echo "✅ v7.0 built successfully"

v8: $(BIN)/g07-v8-c $(BIN)/g07-v8-d
	@echo "✅ v8.0 built successfully"

# ============================================
# v5.0 BINARIES
# ============================================

$(BIN)/g07-v5-c: $(OBJ)/compressor_v5.o $(SHARED) $(V5_MODELS) | $(BIN)
	@echo "Linking g07-v5-c..."
	@$(CXX) $^ -o $@ $(LDFLAGS)
	@echo "  Binary size: $$(du -h $@ | cut -f1)"

$(BIN)/g07-v5-d: $(OBJ)/decompressor_v5.o $(SHARED) $(V5_MODELS) | $(BIN)
	@echo "Linking g07-v5-d..."
	@$(CXX) $^ -o $@ $(LDFLAGS)
	@echo "  Binary size: $$(du -h $@ | cut -f1)"

# ============================================
# v7.0 BINARIES (placeholder - not implemented yet)
# ============================================

$(BIN)/g07-v7-c: $(OBJ)/compressor_v7.o $(SHARED) $(V7_MODELS) | $(BIN)
	@echo "Linking g07-v7-c..."
	@$(CXX) $^ -o $@ $(LDFLAGS)
	@echo "  Binary size: $$(du -h $@ | cut -f1)"

$(BIN)/g07-v7-d: $(OBJ)/decompressor_v7.o $(SHARED) $(V7_MODELS) | $(BIN)
	@echo "Linking g07-v7-d..."
	@$(CXX) $^ -o $@ $(LDFLAGS)
	@echo "  Binary size: $$(du -h $@ | cut -f1)"

# ============================================
# v8.0 BINARIES (placeholder - not implemented yet)
# ============================================

$(BIN)/g07-v8-c: $(OBJ)/compressor_v8.o $(SHARED) $(V8_MODELS) | $(BIN)
	@echo "Linking g07-v8-c..."
	@$(CXX) $^ -o $@ $(LDFLAGS)
	@echo "  Binary size: $$(du -h $@ | cut -f1)"

$(BIN)/g07-v8-d: $(OBJ)/decompressor_v8.o $(SHARED) $(V8_MODELS) | $(BIN)
	@echo "Linking g07-v8-d..."
	@$(CXX) $^ -o $@ $(LDFLAGS)
	@echo "  Binary size: $$(du -h $@ | cut -f1)"

# ============================================
# COMPILE RULES
# ============================================

# Compressors
$(OBJ)/compressor_v5.o: $(SRC)/compressor_v5.cpp | $(OBJ)
	@echo "Compiling compressor_v5.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/compressor_v7.o: $(SRC)/compressor_v7.cpp | $(OBJ)
	@echo "Compiling compressor_v7.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/compressor_v8.o: $(SRC)/compressor_v8.cpp | $(OBJ)
	@echo "Compiling compressor_v8.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Decompressors
$(OBJ)/decompressor_v5.o: $(SRC)/decompressor_v5.cpp | $(OBJ)
	@echo "Compiling decompressor_v5.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/decompressor_v7.o: $(SRC)/decompressor_v7.cpp | $(OBJ)
	@echo "Compiling decompressor_v7.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ)/decompressor_v8.o: $(SRC)/decompressor_v8.cpp | $(OBJ)
	@echo "Compiling decompressor_v8.cpp..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

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
	@echo "✅ Clean complete"

# Check binary sizes (must be < 1MB per teacher requirement)
check-sizes: all
	@echo ""
	@echo "Checking binary sizes (teacher requirement: < 1MB)..."
	@echo "───────────────────────────────────────────────────────────"
	@for bin in $(BIN)/g07-*; do \
		if [ -f "$$bin" ]; then \
			size=$$(stat -f%z "$$bin" 2>/dev/null || stat -c%s "$$bin" 2>/dev/null); \
			size_kb=$$(($$size / 1024)); \
			size_mb=$$(($$size / 1048576)); \
			if [ $$size -gt 1048576 ]; then \
				echo "❌ $$bin: $$(du -h $$bin | cut -f1) (TOO BIG!)"; \
			else \
				echo "✅ $$bin: $$(du -h $$bin | cut -f1) ($${size_kb} KB)"; \
			fi; \
		fi; \
	done
	@echo "───────────────────────────────────────────────────────────"
	@echo ""

test: v5
	@echo "Running quick lossless verification..."
	@echo "Testing on file A (lossless verification built into benchmark)"
	@bash benchmarks/compare.sh data/A

benchmark: v5
	@echo "Running benchmarks..."
	@bash benchmarks/benchmark_all.sh

.PHONY: all v5 v7 v8 clean check-sizes test benchmark
