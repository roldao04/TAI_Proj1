#!/bin/bash

# Verification script for lossless compression
# Tests that compress + decompress produces identical output

set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo "=== Lossless Compression Verification Test ==="
echo

# Create test directory
TEST_DIR="tests/tmp"
mkdir -p "$TEST_DIR"

# Test 1: Simple text file
echo "Test 1: Simple text file"
echo "Hello, World! This is a test of the lossless compression system." > "$TEST_DIR/test1.txt"
echo "The quick brown fox jumps over the lazy dog." >> "$TEST_DIR/test1.txt"
echo "Algorithmic Information Theory - Project 1" >> "$TEST_DIR/test1.txt"

./bin/compress "$TEST_DIR/test1.txt" "$TEST_DIR/test1.compressed"
./bin/decompress "$TEST_DIR/test1.compressed" "$TEST_DIR/test1.decompressed"

if cmp -s "$TEST_DIR/test1.txt" "$TEST_DIR/test1.decompressed"; then
    echo -e "${GREEN}✓ Test 1 PASSED${NC}"
else
    echo -e "${RED}✗ Test 1 FAILED${NC}"
    exit 1
fi
echo

# Test 2: Binary data (random bytes)
echo "Test 2: Binary data"
dd if=/dev/urandom of="$TEST_DIR/test2.bin" bs=1024 count=10 2>/dev/null

./bin/compress "$TEST_DIR/test2.bin" "$TEST_DIR/test2.compressed"
./bin/decompress "$TEST_DIR/test2.compressed" "$TEST_DIR/test2.decompressed"

if cmp -s "$TEST_DIR/test2.bin" "$TEST_DIR/test2.decompressed"; then
    echo -e "${GREEN}✓ Test 2 PASSED${NC}"
else
    echo -e "${RED}✗ Test 2 FAILED${NC}"
    exit 1
fi
echo

# Test 3: Empty file
echo "Test 3: Empty file"
touch "$TEST_DIR/test3.empty"

./bin/compress "$TEST_DIR/test3.empty" "$TEST_DIR/test3.compressed"
./bin/decompress "$TEST_DIR/test3.compressed" "$TEST_DIR/test3.decompressed"

if cmp -s "$TEST_DIR/test3.empty" "$TEST_DIR/test3.decompressed"; then
    echo -e "${GREEN}✓ Test 3 PASSED${NC}"
else
    echo -e "${RED}✗ Test 3 FAILED${NC}"
    exit 1
fi
echo

# Test 4: Highly repetitive data
echo "Test 4: Highly repetitive data"
for i in {1..1000}; do echo "AAAAAAAA"; done > "$TEST_DIR/test4.repetitive"

./bin/compress "$TEST_DIR/test4.repetitive" "$TEST_DIR/test4.compressed"
./bin/decompress "$TEST_DIR/test4.compressed" "$TEST_DIR/test4.decompressed"

if cmp -s "$TEST_DIR/test4.repetitive" "$TEST_DIR/test4.decompressed"; then
    echo -e "${GREEN}✓ Test 4 PASSED${NC}"
    # Show compression effectiveness
    ORIG_SIZE=$(stat -c%s "$TEST_DIR/test4.repetitive")
    COMP_SIZE=$(stat -c%s "$TEST_DIR/test4.compressed")
    RATIO=$(awk "BEGIN {printf \"%.2f\", 100.0 * $COMP_SIZE / $ORIG_SIZE}")
    echo "  Compression ratio: $RATIO% (highly repetitive data)"
else
    echo -e "${RED}✗ Test 4 FAILED${NC}"
    exit 1
fi
echo

# Clean up
rm -rf "$TEST_DIR"

echo -e "${GREEN}=== ALL TESTS PASSED ===${NC}"
echo "The compressor is LOSSLESS and working correctly!"
