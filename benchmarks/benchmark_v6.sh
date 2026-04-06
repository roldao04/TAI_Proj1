#!/bin/bash

# Benchmark script for v6.0 compressor
# Tests compression ratio and speed on all data files

set -e

COMPRESSOR="../bin/compress_v6"
DECOMPRESSOR="../bin/decompress_v6"
DATA_DIR="../data"
RESULTS_FILE="results_v6.csv"
RESULTS_MD="results_v6.md"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "╔════════════════════════════════════════════════════════════╗"
echo "║       v6.0 Compression Benchmark (Maximum Compression)     ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Check if compressor exists
if [ ! -f "$COMPRESSOR" ]; then
    echo -e "${RED}Error: compress_v6 not found. Run 'make v6' first.${NC}"
    exit 1
fi

# Create temporary results directory in /tmp
TMP_RESULTS_DIR="/tmp/v6_benchmark_$$"
mkdir -p "$TMP_RESULTS_DIR"

# Cleanup temporary files on exit
trap "rm -rf $TMP_RESULTS_DIR" EXIT INT TERM

# CSV header
echo "File,Original Size (bytes),Compressed Size (bytes),Ratio (%),Bits/Symbol,Compress Time (ms),Decompress Time (ms),Throughput (KB/s)" > "$RESULTS_FILE"

# Markdown header
echo "# v6.0 Compression Benchmark Results" > "$RESULTS_MD"
echo "" >> "$RESULTS_MD"
echo "**Date:** $(date)" >> "$RESULTS_MD"
echo "" >> "$RESULTS_MD"
echo "**Configuration:** Multi-Order PPM (Orders: 1,2,4,6,8,12,16,24,32)" >> "$RESULTS_MD"
echo "" >> "$RESULTS_MD"
echo "| File | Original Size | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Throughput |" >> "$RESULTS_MD"
echo "|------|---------------|-----------------|-------|-------------|---------------|-----------------|------------|" >> "$RESULTS_MD"

# Initialize totals
total_original=0
total_compressed=0
total_compress_time=0
total_decompress_time=0
file_count=0

# Test each file in data directory
for file in "$DATA_DIR"/*; do
    if [ -f "$file" ]; then
        filename=$(basename "$file")

        echo -e "${BLUE}Testing: $filename${NC}"

        # Get original size
        original_size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null)

        # Compress
        compressed_file="$TMP_RESULTS_DIR/${filename}.v6.compressed"
        echo "  Compressing..."

        compress_start=$(date +%s%3N)
        set +e  # Temporarily disable exit on error
        $COMPRESSOR "$file" "$compressed_file" --yes --quiet > /tmp/compress_v6_output_$$.txt 2>&1
        compress_exit=$?
        set -e  # Re-enable exit on error
        compress_end=$(date +%s%3N)
        compress_time=$((compress_end - compress_start))

        if [ $compress_exit -ne 0 ]; then
            echo -e "  ${RED}Compression failed! (exit code: $compress_exit)${NC}"
            if [ -f /tmp/compress_v6_output_$$.txt ]; then
                echo "  Error output:"
                cat /tmp/compress_v6_output_$$.txt | head -10
                rm -f /tmp/compress_v6_output_$$.txt
            fi
            continue
        fi
        rm -f /tmp/compress_v6_output_$$.txt

        # Get compressed size
        compressed_size=$(stat -f%z "$compressed_file" 2>/dev/null || stat -c%s "$compressed_file" 2>/dev/null)

        # Calculate ratio
        ratio=$(echo "scale=2; $compressed_size * 100 / $original_size" | bc)
        bits_per_symbol=$(echo "scale=2; $compressed_size * 8 / $original_size" | bc)

        # Decompress
        decompressed_file="$TMP_RESULTS_DIR/${filename}.decompressed"
        echo "  Decompressing..."

        decompress_start=$(date +%s%3N)
        set +e
        $DECOMPRESSOR "$compressed_file" "$decompressed_file" --quiet > /dev/null 2>&1
        decompress_exit=$?
        set -e
        decompress_end=$(date +%s%3N)
        decompress_time=$((decompress_end - decompress_start))

        if [ $decompress_exit -ne 0 ]; then
            echo -e "  ${RED}Decompression failed! (exit code: $decompress_exit)${NC}"
            continue
        fi

        # Verify correctness
        if ! diff -q "$file" "$decompressed_file" > /dev/null 2>&1; then
            echo -e "  ${RED}ERROR: Decompressed file differs from original!${NC}"
            continue
        fi

        # Calculate throughput (KB/s)
        throughput=$(echo "scale=2; $original_size / 1024 / ($compress_time / 1000)" | bc)

        # Update totals
        total_original=$((total_original + original_size))
        total_compressed=$((total_compressed + compressed_size))
        total_compress_time=$((total_compress_time + compress_time))
        total_decompress_time=$((total_decompress_time + decompress_time))
        file_count=$((file_count + 1))

        # Print results
        echo -e "  ${GREEN}✓ Success${NC}"
        echo "  Original:    $(numfmt --to=iec-i --suffix=B $original_size)"
        echo "  Compressed:  $(numfmt --to=iec-i --suffix=B $compressed_size)"
        echo "  Ratio:       ${ratio}%"
        echo "  Bits/symbol: ${bits_per_symbol}"
        echo "  Compress:    ${compress_time}ms"
        echo "  Decompress:  ${decompress_time}ms"
        echo "  Throughput:  ${throughput} KB/s"
        echo ""

        # Write to CSV
        echo "$filename,$original_size,$compressed_size,$ratio,$bits_per_symbol,$compress_time,$decompress_time,$throughput" >> "$RESULTS_FILE"

        # Write to Markdown
        echo "| $filename | $(numfmt --to=iec-i --suffix=B $original_size) | $(numfmt --to=iec-i --suffix=B $compressed_size) | ${ratio}% | ${bits_per_symbol} | ${compress_time}ms | ${decompress_time}ms | ${throughput} KB/s |" >> "$RESULTS_MD"
    fi
done

# Calculate averages
if [ $file_count -gt 0 ]; then
    avg_ratio=$(echo "scale=2; $total_compressed * 100 / $total_original" | bc)
    avg_bits=$(echo "scale=2; $total_compressed * 8 / $total_original" | bc)
    avg_compress_time=$(echo "scale=2; $total_compress_time / $file_count" | bc)
    avg_decompress_time=$(echo "scale=2; $total_decompress_time / $file_count" | bc)
    avg_throughput=$(echo "scale=2; $total_original / 1024 / ($total_compress_time / 1000)" | bc)

    echo "═══════════════════════════════════════════════════════════"
    echo "SUMMARY"
    echo "═══════════════════════════════════════════════════════════"
    echo "Files tested:        $file_count"
    echo "Total original:      $(numfmt --to=iec-i --suffix=B $total_original)"
    echo "Total compressed:    $(numfmt --to=iec-i --suffix=B $total_compressed)"
    echo -e "${YELLOW}Average ratio:       ${avg_ratio}%${NC}"
    echo -e "${YELLOW}Average bits/symbol: ${avg_bits}${NC}"
    echo "Avg compress time:   ${avg_compress_time}ms"
    echo "Avg decompress time: ${avg_decompress_time}ms"
    echo "Overall throughput:  ${avg_throughput} KB/s"
    echo ""

    # Write summary to files
    echo "AVERAGE,,$total_compressed,$avg_ratio,$avg_bits,$avg_compress_time,$avg_decompress_time,$avg_throughput" >> "$RESULTS_FILE"

    echo "" >> "$RESULTS_MD"
    echo "## Summary" >> "$RESULTS_MD"
    echo "" >> "$RESULTS_MD"
    echo "- **Files tested:** $file_count" >> "$RESULTS_MD"
    echo "- **Total original size:** $(numfmt --to=iec-i --suffix=B $total_original)" >> "$RESULTS_MD"
    echo "- **Total compressed size:** $(numfmt --to=iec-i --suffix=B $total_compressed)" >> "$RESULTS_MD"
    echo "- **Average compression ratio:** ${avg_ratio}%" >> "$RESULTS_MD"
    echo "- **Average bits/symbol:** ${avg_bits}" >> "$RESULTS_MD"
    echo "- **Average compression time:** ${avg_compress_time}ms" >> "$RESULTS_MD"
    echo "- **Average decompression time:** ${avg_decompress_time}ms" >> "$RESULTS_MD"
    echo "- **Overall throughput:** ${avg_throughput} KB/s" >> "$RESULTS_MD"
fi

echo -e "${GREEN}Results saved to:${NC}"
echo "  - $RESULTS_FILE"
echo "  - $RESULTS_MD"
