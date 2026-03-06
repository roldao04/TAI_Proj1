#!/bin/bash

# Benchmark script to compare compression tools
# Compares: g07, gzip, bzip2, xz (lzma), zstd

set -e

if [ $# -ne 1 ]; then
    echo "Usage: $0 <input_file>"
    exit 1
fi

INPUT_FILE="$1"
BASENAME=$(basename "$INPUT_FILE")
TEMP_DIR="benchmarks/tmp"

mkdir -p "$TEMP_DIR"

# Timeout configuration (seconds)
COMPRESS_TIMEOUT=60    # Maximum time for compression
DECOMPRESS_TIMEOUT=30  # Maximum time for decompression

# Colors
BOLD='\033[1m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${BOLD}=== Compression Benchmark ===${NC}"
echo "Input file: $INPUT_FILE"
ORIGINAL_SIZE=$(stat -c%s "$INPUT_FILE")
echo "Original size: $ORIGINAL_SIZE bytes"
echo

# Function to benchmark a compressor
benchmark() {
    local name="$1"
    local compress_cmd="$2"
    local decompress_cmd="$3"
    local compressed_file="$4"

    echo -e "${BLUE}--- $name ---${NC}"

    # Compression with timeout
    local start=$(date +%s%3N)
    timeout $COMPRESS_TIMEOUT bash -c "$compress_cmd" > /dev/null 2>&1
    local exit_code=$?
    local end=$(date +%s%3N)
    local compress_time=$((end - start))

    # Check for timeout or error
    if [ $exit_code -eq 124 ]; then
        echo -e "${YELLOW}⚠ TIMEOUT: Compression exceeded ${COMPRESS_TIMEOUT}s${NC}"
        return
    elif [ $exit_code -ne 0 ] || [ ! -f "$compressed_file" ]; then
        echo "ERROR: Compression failed"
        return
    fi

    local compressed_size=$(stat -c%s "$compressed_file")
    local ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $compressed_size / $ORIGINAL_SIZE}")
    local bits_per_symbol=$(awk "BEGIN {printf \"%.4f\", 8.0 * $compressed_size / $ORIGINAL_SIZE}")

    # Decompression with timeout
    local decompressed_file="$TEMP_DIR/${BASENAME}.decompressed"
    start=$(date +%s%3N)
    timeout $DECOMPRESS_TIMEOUT bash -c "$decompress_cmd" > /dev/null 2>&1
    exit_code=$?
    end=$(date +%s%3N)
    local decompress_time=$((end - start))

    # Check for timeout
    if [ $exit_code -eq 124 ]; then
        echo "Compressed size:   $compressed_size bytes"
        echo -e "Compression ratio: $ratio%"
        echo "Bits per symbol:   $bits_per_symbol"
        echo "Compress time:     ${compress_time} ms"
        echo -e "Decompress time:   ${YELLOW}⚠ TIMEOUT (>${DECOMPRESS_TIMEOUT}s)${NC}"
        echo
        rm -f "$compressed_file" "$decompressed_file"
        return
    fi

    # Verify lossless
    if cmp -s "$INPUT_FILE" "$decompressed_file"; then
        local lossless="${GREEN}✓ LOSSLESS${NC}"
    else
        local lossless="✗ ERROR: Not lossless!"
    fi

    echo "Compressed size:   $compressed_size bytes"
    echo -e "Compression ratio: $ratio%"
    echo "Bits per symbol:   $bits_per_symbol"
    echo "Compress time:     ${compress_time} ms"
    echo "Decompress time:   ${decompress_time} ms"
    echo -e "Verification:      $lossless"
    echo

    # Clean up
    rm -f "$compressed_file" "$decompressed_file"
}

# Our compressor
benchmark "G07 Compressor" \
    "./bin/compress '$INPUT_FILE' '$TEMP_DIR/${BASENAME}.g07' --yes" \
    "./bin/decompress '$TEMP_DIR/${BASENAME}.g07' '$TEMP_DIR/${BASENAME}.decompressed'" \
    "$TEMP_DIR/${BASENAME}.g07"

# gzip
benchmark "gzip" \
    "gzip -c '$INPUT_FILE' > '$TEMP_DIR/${BASENAME}.gz'" \
    "gzip -dc '$TEMP_DIR/${BASENAME}.gz' > '$TEMP_DIR/${BASENAME}.decompressed'" \
    "$TEMP_DIR/${BASENAME}.gz"

# bzip2
benchmark "bzip2" \
    "bzip2 -c '$INPUT_FILE' > '$TEMP_DIR/${BASENAME}.bz2'" \
    "bzip2 -dc '$TEMP_DIR/${BASENAME}.bz2' > '$TEMP_DIR/${BASENAME}.decompressed'" \
    "$TEMP_DIR/${BASENAME}.bz2"

# xz (lzma)
benchmark "xz (lzma)" \
    "xz -c '$INPUT_FILE' > '$TEMP_DIR/${BASENAME}.xz'" \
    "xz -dc '$TEMP_DIR/${BASENAME}.xz' > '$TEMP_DIR/${BASENAME}.decompressed'" \
    "$TEMP_DIR/${BASENAME}.xz"

# zstd
if command -v zstd &> /dev/null; then
    benchmark "zstd" \
        "zstd -q '$INPUT_FILE' -o '$TEMP_DIR/${BASENAME}.zst'" \
        "zstd -qd '$TEMP_DIR/${BASENAME}.zst' -o '$TEMP_DIR/${BASENAME}.decompressed'" \
        "$TEMP_DIR/${BASENAME}.zst"
else
    echo "zstd not installed, skipping..."
    echo
fi

# Clean up temp directory
rm -rf "$TEMP_DIR"

echo -e "${BOLD}=== Benchmark Complete ===${NC}"
