#!/bin/bash

# Benchmark script to compare compression tools on macOS-compatible shells.
# Compares: g07, gzip, bzip2, xz (lzma), zstd

set -e

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
. "$SCRIPT_DIR/common.sh"

if [ $# -ne 1 ]; then
    echo "Usage: $0 <input_file>"
    exit 1
fi

INPUT_FILE="$1"
BASENAME=$(basename "$INPUT_FILE")
TEMP_DIR="$SCRIPT_DIR/tmp_compare_$$"

mkdir -p "$TEMP_DIR"

# Timeout configuration (seconds)
COMPRESS_TIMEOUT=60
DECOMPRESS_TIMEOUT=30
G07_COMPRESS_CMD="${G07_COMPRESS_CMD:-./bin/g07-v9-c}"
G07_DECOMPRESS_CMD="${G07_DECOMPRESS_CMD:-./bin/g07-v9-d}"
G07_COMPRESS_EXTRA_ARGS="${G07_COMPRESS_EXTRA_ARGS-}"

# Colors
BOLD='\033[1m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BOLD}=== Compression Benchmark ===${NC}"
echo "Input file: $INPUT_FILE"
ORIGINAL_SIZE=$(file_size "$INPUT_FILE")
echo "Original size: $ORIGINAL_SIZE bytes"
echo

benchmark() {
    local name="$1"
    local compress_cmd="$2"
    local decompress_cmd="$3"
    local compressed_file="$4"
    local start
    local end
    local exit_code
    local compress_time
    local compressed_size
    local ratio
    local bits_per_symbol
    local decompressed_file
    local decompress_time
    local lossless

    echo -e "${BLUE}--- $name ---${NC}"

    start=$(now_ms)
    if run_with_timeout "$COMPRESS_TIMEOUT" "$compress_cmd"; then
        exit_code=0
    else
        exit_code=$?
    fi
    end=$(now_ms)
    compress_time=$((end - start))

    if [ $exit_code -eq 124 ]; then
        echo -e "${YELLOW}TIMEOUT: Compression exceeded ${COMPRESS_TIMEOUT}s${NC}"
        return
    elif [ $exit_code -ne 0 ] || [ ! -f "$compressed_file" ]; then
        echo "ERROR: Compression failed"
        return
    fi

    compressed_size=$(file_size "$compressed_file")
    ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $compressed_size / $ORIGINAL_SIZE}")
    bits_per_symbol=$(awk "BEGIN {printf \"%.4f\", 8.0 * $compressed_size / $ORIGINAL_SIZE}")

    decompressed_file="$TEMP_DIR/${BASENAME}.decompressed"
    start=$(now_ms)
    if run_with_timeout "$DECOMPRESS_TIMEOUT" "$decompress_cmd"; then
        exit_code=0
    else
        exit_code=$?
    fi
    end=$(now_ms)
    decompress_time=$((end - start))

    if [ $exit_code -eq 124 ]; then
        echo "Compressed size:   $compressed_size bytes"
        echo -e "Compression ratio: $ratio%"
        echo "Bits per symbol:   $bits_per_symbol"
        echo "Compress time:     ${compress_time} ms"
        echo -e "Decompress time:   ${YELLOW}TIMEOUT (> ${DECOMPRESS_TIMEOUT}s)${NC}"
        echo
        rm -f "$compressed_file" "$decompressed_file"
        return
    elif [ $exit_code -ne 0 ]; then
        echo "ERROR: Decompression failed"
        echo
        rm -f "$compressed_file" "$decompressed_file"
        return
    fi

    if cmp -s "$INPUT_FILE" "$decompressed_file"; then
        lossless="${GREEN}LOSSLESS${NC}"
    else
        lossless="ERROR: Not lossless!"
    fi

    echo "Compressed size:   $compressed_size bytes"
    echo -e "Compression ratio: $ratio%"
    echo "Bits per symbol:   $bits_per_symbol"
    echo "Compress time:     ${compress_time} ms"
    echo "Decompress time:   ${decompress_time} ms"
    echo -e "Verification:      $lossless"
    echo

    rm -f "$compressed_file" "$decompressed_file"
}

benchmark "G07 Compressor" \
    "$G07_COMPRESS_CMD '$INPUT_FILE' '$TEMP_DIR/${BASENAME}.g07'${G07_COMPRESS_EXTRA_ARGS:+ $G07_COMPRESS_EXTRA_ARGS}" \
    "$G07_DECOMPRESS_CMD '$TEMP_DIR/${BASENAME}.g07' '$TEMP_DIR/${BASENAME}.decompressed'" \
    "$TEMP_DIR/${BASENAME}.g07"

benchmark "gzip" \
    "gzip -c '$INPUT_FILE' > '$TEMP_DIR/${BASENAME}.gz'" \
    "gzip -dc '$TEMP_DIR/${BASENAME}.gz' > '$TEMP_DIR/${BASENAME}.decompressed'" \
    "$TEMP_DIR/${BASENAME}.gz"

benchmark "bzip2" \
    "bzip2 -c '$INPUT_FILE' > '$TEMP_DIR/${BASENAME}.bz2'" \
    "bzip2 -dc '$TEMP_DIR/${BASENAME}.bz2' > '$TEMP_DIR/${BASENAME}.decompressed'" \
    "$TEMP_DIR/${BASENAME}.bz2"

benchmark "xz (lzma)" \
    "xz -c '$INPUT_FILE' > '$TEMP_DIR/${BASENAME}.xz'" \
    "xz -dc '$TEMP_DIR/${BASENAME}.xz' > '$TEMP_DIR/${BASENAME}.decompressed'" \
    "$TEMP_DIR/${BASENAME}.xz"

if command -v zstd >/dev/null 2>&1; then
    benchmark "zstd" \
        "zstd -q '$INPUT_FILE' -o '$TEMP_DIR/${BASENAME}.zst'" \
        "zstd -qd '$TEMP_DIR/${BASENAME}.zst' -o '$TEMP_DIR/${BASENAME}.decompressed'" \
        "$TEMP_DIR/${BASENAME}.zst"
else
    echo "zstd not installed, skipping..."
    echo
fi

rm -rf "$TEMP_DIR"

echo -e "${BOLD}=== Benchmark Complete ===${NC}"
