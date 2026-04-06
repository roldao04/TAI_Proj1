#!/bin/bash

# Compare v5.0 vs v6.0 compression
# Shows compression ratio improvement and speed tradeoff

set -e

COMPRESSOR_V5="../bin/compress"
COMPRESSOR_V6="../bin/compress_v6"
DATA_DIR="../data"
RESULTS_FILE="comparison_v5_v6.csv"
RESULTS_MD="comparison_v5_v6.md"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

echo "╔════════════════════════════════════════════════════════════╗"
echo "║           v5.0 vs v6.0 Compression Comparison              ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# Check if compressors exist
if [ ! -f "$COMPRESSOR_V5" ]; then
    echo -e "${RED}Error: compress (v5.0) not found. Run 'make v5' first.${NC}"
    exit 1
fi

if [ ! -f "$COMPRESSOR_V6" ]; then
    echo -e "${RED}Error: compress_v6 not found. Run 'make v6' first.${NC}"
    exit 1
fi

# Create results directory
mkdir -p results_comparison

# CSV header
echo "File,Original Size,v5.0 Size,v5.0 Ratio,v5.0 Time,v6.0 Size,v6.0 Ratio,v6.0 Time,Improvement (pp),Slowdown (x)" > "$RESULTS_FILE"

# Markdown header
echo "# v5.0 vs v6.0 Compression Comparison" > "$RESULTS_MD"
echo "" >> "$RESULTS_MD"
echo "**Date:** $(date)" >> "$RESULTS_MD"
echo "" >> "$RESULTS_MD"
echo "**v5.0:** BWT + MTF + ZRLE + Order-1 PPM (fast)" >> "$RESULTS_MD"
echo "**v6.0:** Multi-Order PPM + Context Mixing (maximum compression)" >> "$RESULTS_MD"
echo "" >> "$RESULTS_MD"
echo "| File | Original | v5.0 Ratio | v5.0 Time | v6.0 Ratio | v6.0 Time | Improvement | Slowdown |" >> "$RESULTS_MD"
echo "|------|----------|------------|-----------|------------|-----------|-------------|----------|" >> "$RESULTS_MD"

# Initialize totals
total_original=0
total_v5_compressed=0
total_v6_compressed=0
total_v5_time=0
total_v6_time=0
file_count=0
files_v6_better=0

# Test each file
for file in "$DATA_DIR"/*; do
    if [ -f "$file" ]; then
        filename=$(basename "$file")

        echo -e "${BLUE}═══════════════════════════════════════════${NC}"
        echo -e "${BLUE}Testing: $filename${NC}"
        echo -e "${BLUE}═══════════════════════════════════════════${NC}"

        # Get original size
        original_size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null)

        # ============ v5.0 Compression ============
        echo -e "${CYAN}[v5.0] Compressing...${NC}"
        v5_compressed="results_comparison/${filename}.v5.compressed"

        v5_start=$(date +%s%3N)
        set +e
        $COMPRESSOR_V5 "$file" "$v5_compressed" --yes > /dev/null 2>&1
        v5_exit=$?
        set -e
        v5_end=$(date +%s%3N)
        v5_time=$((v5_end - v5_start))

        if [ $v5_exit -ne 0 ]; then
            echo -e "${RED}v5.0 compression failed! (exit code: $v5_exit)${NC}"
            continue
        fi

        v5_size=$(stat -f%z "$v5_compressed" 2>/dev/null || stat -c%s "$v5_compressed" 2>/dev/null)
        v5_ratio=$(echo "scale=2; $v5_size * 100 / $original_size" | bc)

        echo "  Size:  $(numfmt --to=iec-i --suffix=B $v5_size)"
        echo "  Ratio: ${v5_ratio}%"
        echo "  Time:  ${v5_time}ms"

        # ============ v6.0 Compression ============
        echo -e "${CYAN}[v6.0] Compressing...${NC}"
        v6_compressed="results_comparison/${filename}.v6.compressed"

        v6_start=$(date +%s%3N)
        set +e
        $COMPRESSOR_V6 "$file" "$v6_compressed" --yes --quiet > /dev/null 2>&1
        v6_exit=$?
        set -e
        v6_end=$(date +%s%3N)
        v6_time=$((v6_end - v6_start))

        if [ $v6_exit -ne 0 ]; then
            echo -e "${RED}v6.0 compression failed! (exit code: $v6_exit)${NC}"
            continue
        fi

        v6_size=$(stat -f%z "$v6_compressed" 2>/dev/null || stat -c%s "$v6_compressed" 2>/dev/null)
        v6_ratio=$(echo "scale=2; $v6_size * 100 / $original_size" | bc)

        echo "  Size:  $(numfmt --to=iec-i --suffix=B $v6_size)"
        echo "  Ratio: ${v6_ratio}%"
        echo "  Time:  ${v6_time}ms"

        # ============ Comparison ============
        improvement=$(echo "scale=2; $v5_ratio - $v6_ratio" | bc)
        slowdown=$(echo "scale=2; $v6_time / $v5_time" | bc)

        echo ""
        if (( $(echo "$improvement > 0" | bc -l) )); then
            echo -e "${GREEN}✓ v6.0 BETTER by ${improvement}pp${NC}"
            files_v6_better=$((files_v6_better + 1))
        elif (( $(echo "$improvement < 0" | bc -l) )); then
            echo -e "${RED}✗ v5.0 BETTER by ${improvement#-}pp${NC}"
        else
            echo -e "${YELLOW}= TIE${NC}"
        fi
        echo -e "${YELLOW}⏱ v6.0 is ${slowdown}× slower${NC}"
        echo ""

        # Update totals
        total_original=$((total_original + original_size))
        total_v5_compressed=$((total_v5_compressed + v5_size))
        total_v6_compressed=$((total_v6_compressed + v6_size))
        total_v5_time=$((total_v5_time + v5_time))
        total_v6_time=$((total_v6_time + v6_time))
        file_count=$((file_count + 1))

        # Write to CSV
        echo "$filename,$original_size,$v5_size,$v5_ratio,$v5_time,$v6_size,$v6_ratio,$v6_time,$improvement,${slowdown}x" >> "$RESULTS_FILE"

        # Write to Markdown
        improvement_display="$improvement pp"
        if (( $(echo "$improvement > 0" | bc -l) )); then
            improvement_display="✓ $improvement pp"
        elif (( $(echo "$improvement < 0" | bc -l) )); then
            improvement_display="✗ ${improvement#-} pp"
        fi

        echo "| $filename | $(numfmt --to=iec-i --suffix=B $original_size) | ${v5_ratio}% | ${v5_time}ms | ${v6_ratio}% | ${v6_time}ms | $improvement_display | ${slowdown}× |" >> "$RESULTS_MD"
    fi
done

# Calculate overall statistics
if [ $file_count -gt 0 ]; then
    avg_v5_ratio=$(echo "scale=2; $total_v5_compressed * 100 / $total_original" | bc)
    avg_v6_ratio=$(echo "scale=2; $total_v6_compressed * 100 / $total_original" | bc)
    overall_improvement=$(echo "scale=2; $avg_v5_ratio - $avg_v6_ratio" | bc)
    overall_slowdown=$(echo "scale=2; $total_v6_time / $total_v5_time" | bc)

    echo "═══════════════════════════════════════════════════════════"
    echo "OVERALL COMPARISON"
    echo "═══════════════════════════════════════════════════════════"
    echo "Files tested:            $file_count"
    echo "Total original size:     $(numfmt --to=iec-i --suffix=B $total_original)"
    echo ""
    echo -e "${CYAN}v5.0 Results:${NC}"
    echo "  Total compressed:      $(numfmt --to=iec-i --suffix=B $total_v5_compressed)"
    echo "  Average ratio:         ${avg_v5_ratio}%"
    echo "  Total time:            ${total_v5_time}ms"
    echo ""
    echo -e "${CYAN}v6.0 Results:${NC}"
    echo "  Total compressed:      $(numfmt --to=iec-i --suffix=B $total_v6_compressed)"
    echo "  Average ratio:         ${avg_v6_ratio}%"
    echo "  Total time:            ${total_v6_time}ms"
    echo ""
    echo -e "${YELLOW}Comparison:${NC}"

    if (( $(echo "$overall_improvement > 0" | bc -l) )); then
        echo -e "  Compression improvement: ${GREEN}${overall_improvement}pp better${NC}"
    else
        echo -e "  Compression improvement: ${RED}${overall_improvement#-}pp worse${NC}"
    fi

    echo -e "  Speed tradeoff:         ${YELLOW}${overall_slowdown}× slower${NC}"
    echo "  Files where v6.0 wins:  ${files_v6_better}/${file_count}"

    # Write summary
    echo "" >> "$RESULTS_MD"
    echo "## Overall Comparison" >> "$RESULTS_MD"
    echo "" >> "$RESULTS_MD"
    echo "| Metric | v5.0 | v6.0 | Difference |" >> "$RESULTS_MD"
    echo "|--------|------|------|------------|" >> "$RESULTS_MD"
    echo "| Average Ratio | ${avg_v5_ratio}% | ${avg_v6_ratio}% | ${overall_improvement}pp |" >> "$RESULTS_MD"
    echo "| Total Compressed | $(numfmt --to=iec-i --suffix=B $total_v5_compressed) | $(numfmt --to=iec-i --suffix=B $total_v6_compressed) | - |" >> "$RESULTS_MD"
    echo "| Total Time | ${total_v5_time}ms | ${total_v6_time}ms | ${overall_slowdown}× slower |" >> "$RESULTS_MD"
    echo "| Files Won | $((file_count - files_v6_better)) | ${files_v6_better} | - |" >> "$RESULTS_MD"
fi

echo ""
echo -e "${GREEN}Results saved to:${NC}"
echo "  - $RESULTS_FILE"
echo "  - $RESULTS_MD"
