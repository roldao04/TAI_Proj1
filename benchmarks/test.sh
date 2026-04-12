#!/bin/bash

# Quick single-file test for one G07 version
# Usage: ./benchmarks/test_one.sh <file> <version>
# Example: ./benchmarks/test_one.sh data/A 9

if [ $# -ne 2 ]; then
    echo "Usage: $0 <file> <version>"
    echo "Example: $0 data/A 9"
    exit 1
fi

FILE="$1"
VERSION="$2"
TEMP_DIR="benchmarks/tmp"

if [ ! -f "$FILE" ]; then
    echo "ERROR: File '$FILE' not found"
    exit 1
fi

COMPRESSOR="./bin/g07-v${VERSION}-c"
DECOMPRESSOR="./bin/g07-v${VERSION}-d"

if [ ! -f "$COMPRESSOR" ]; then
    echo "ERROR: Compressor '$COMPRESSOR' not found"
    echo "Run 'make' to build v$VERSION"
    exit 1
fi

mkdir -p "$TEMP_DIR"

BASENAME=$(basename "$FILE")
COMPRESSED="$TEMP_DIR/${BASENAME}.v${VERSION}.test"
DECOMPRESSED="$TEMP_DIR/${BASENAME}.v${VERSION}.test.dec"

# Get original size
ORIGINAL_SIZE=$(stat -f%z "$FILE" 2>/dev/null || stat -c%s "$FILE" 2>/dev/null)

echo "════════════════════════════════════════════════════"
echo "G07 v${VERSION} - Single File Test"
echo "════════════════════════════════════════════════════"
echo "File: $FILE"
echo "Size: $(numfmt --to=iec-i --suffix=B $ORIGINAL_SIZE)"
echo

# Compress
echo "Compressing..."
COMP_START=$(date +%s%N)
$COMPRESSOR "$FILE" "$COMPRESSED" 2>&1 | grep -E "(Entropy|Decision|Model|Pipeline|ratio|time|speed|weights|usage)" || true
COMP_END=$(date +%s%N)
COMP_TIME=$(( (COMP_END - COMP_START) / 1000000 ))

if [ ! -f "$COMPRESSED" ]; then
    echo "ERROR: Compression failed"
    exit 1
fi

COMPRESSED_SIZE=$(stat -f%z "$COMPRESSED" 2>/dev/null || stat -c%s "$COMPRESSED" 2>/dev/null)
RATIO=$(awk "BEGIN {printf \"%.2f\", ($COMPRESSED_SIZE * 100.0 / $ORIGINAL_SIZE)}")
BITS_PER_SYM=$(awk "BEGIN {printf \"%.2f\", ($COMPRESSED_SIZE * 8.0 / $ORIGINAL_SIZE)}")
COMP_SPEED=$(awk "BEGIN {printf \"%.1f\", ($ORIGINAL_SIZE / 1048576.0) / ($COMP_TIME / 1000.0)}")

echo
echo "────────────────────────────────────────────────────"
echo "Compression Results:"
echo "────────────────────────────────────────────────────"
echo "  Compressed size:   $(numfmt --to=iec-i --suffix=B $COMPRESSED_SIZE)"
echo "  Compression ratio: ${RATIO}%"
echo "  Bits per symbol:   ${BITS_PER_SYM}"
echo "  Compression time:  ${COMP_TIME} ms"
echo "  Compression speed: ${COMP_SPEED} MB/s"
echo

# Decompress
echo "Decompressing..."
DECOMP_START=$(date +%s%N)
$DECOMPRESSOR "$COMPRESSED" "$DECOMPRESSED" 2>&1 | grep -E "(weights|usage|reversed)" || true
DECOMP_END=$(date +%s%N)
DECOMP_TIME=$(( (DECOMP_END - DECOMP_START) / 1000000 ))

if [ ! -f "$DECOMPRESSED" ]; then
    echo "ERROR: Decompression failed"
    exit 1
fi

DECOMP_SPEED=$(awk "BEGIN {printf \"%.1f\", ($ORIGINAL_SIZE / 1048576.0) / ($DECOMP_TIME / 1000.0)}")

echo
echo "────────────────────────────────────────────────────"
echo "Decompression Results:"
echo "────────────────────────────────────────────────────"
echo "  Decompression time:  ${DECOMP_TIME} ms"
echo "  Decompression speed: ${DECOMP_SPEED} MB/s"
echo

# Verify
echo "Verifying lossless..."
if cmp -s "$FILE" "$DECOMPRESSED"; then
    echo "✓ PASS: Files are identical (lossless compression)"
    EXIT_CODE=0
else
    echo "✗ FAIL: Files differ (data corruption!)"
    DIFF_SIZE=$(stat -f%z "$DECOMPRESSED" 2>/dev/null || stat -c%s "$DECOMPRESSED" 2>/dev/null)
    echo "  Original:     $ORIGINAL_SIZE bytes"
    echo "  Decompressed: $DIFF_SIZE bytes"
    EXIT_CODE=1
fi

# Cleanup
rm -f "$COMPRESSED" "$DECOMPRESSED"

echo
echo "════════════════════════════════════════════════════"
exit $EXIT_CODE
