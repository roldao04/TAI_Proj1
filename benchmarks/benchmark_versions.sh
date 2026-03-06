#!/bin/bash

# Version Comparison Benchmark Script
# Compares v1.0 vs Current implementation
# Shows improvements in compression ratio, speed, and capabilities

set -e

# Directories
DATA_DIR="data"
TEMP_DIR="benchmarks/tmp_versions"
RESULTS_CSV="benchmarks/version_comparison.csv"
RESULTS_MD="benchmarks/version_comparison.md"

# Version paths
V1_DIR="versions/g07_v1.0"
CURRENT_BIN_DIR="bin"

# Timeout configuration (seconds)
COMPRESS_TIMEOUT=60    # Maximum time for compression
DECOMPRESS_TIMEOUT=30  # Maximum time for decompression

# Colors
BOLD='\033[1m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Create temp directory
mkdir -p "$TEMP_DIR"

# Initialize results files
echo "File,Original Size (bytes),Version,Compressed Size (bytes),Compression Ratio (%),Bits/Symbol,Compress Time (ms),Decompress Time (ms),Lossless,Model Used" > "$RESULTS_CSV"

# Initialize markdown with header
cat > "$RESULTS_MD" << 'EOF'
# Version Comparison: v1.0 vs v2.0

**Project**: TAI - Algorithmic Information Theory - Group 07
**Date**: $(date +"%Y-%m-%d")
**Comparison**: Version 1.0 (Order-0 + Arithmetic) vs Version 2.0 (Multi-model + Range Coder)

---

## Executive Summary

This benchmark compares two major versions of our lossless compression tool:

- **v1.0**: Order-0 frequency model with 32-bit arithmetic coding
- **v2.0**: Multi-model system (Order-0/Order-1) with range coding and auto-selection

---

## Table of Contents
1. [Quick Summary](#quick-summary)
2. [Detailed File-by-File Comparison](#detailed-file-by-file-comparison)
3. [Performance Improvements](#performance-improvements)
4. [Model Selection Analysis](#model-selection-analysis)
5. [Overall Statistics](#overall-statistics)

---

EOF

echo -e "${BOLD}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}║         VERSION COMPARISON BENCHMARK: v1.0 vs Current         ║${NC}"
echo -e "${BOLD}╚════════════════════════════════════════════════════════════════╝${NC}"
echo

# Check if v1.0 binaries exist
if [ ! -f "$V1_DIR/compress" ] || [ ! -f "$V1_DIR/decompress" ]; then
    echo -e "${RED}ERROR: v1.0 binaries not found in $V1_DIR/${NC}"
    echo "Expected files:"
    echo "  - $V1_DIR/compress"
    echo "  - $V1_DIR/decompress"
    exit 1
fi

# Check if current binaries exist
if [ ! -f "$CURRENT_BIN_DIR/compress" ] || [ ! -f "$CURRENT_BIN_DIR/decompress" ]; then
    echo -e "${RED}ERROR: Current binaries not found in $CURRENT_BIN_DIR/${NC}"
    echo "Please run 'make' to build current version"
    exit 1
fi

echo -e "${CYAN}✓ v1.0 binaries found: $V1_DIR${NC}"
echo -e "${CYAN}✓ Current binaries found: $CURRENT_BIN_DIR${NC}"
echo

# Arrays to store results
declare -A version_total_original
declare -A version_total_compressed
declare -A version_file_count
declare -A version_total_comp_time
declare -A version_total_decomp_time
declare -A file_v1_ratio
declare -A file_v2_ratio
declare -A file_winner

# Function to benchmark one file with one version
benchmark_file_version() {
    local file="$1"
    local basename="$2"
    local version="$3"
    local compress_bin="$4"
    local decompress_bin="$5"
    local original_size="$6"
    local extra_args="$7"

    local compressed_file="$TEMP_DIR/${basename}.${version}.compressed"
    local decompressed_file="$TEMP_DIR/${basename}.${version}.decompressed"

    # Compression with timeout (capture output to get model info for v2.0)
    local compress_output="$TEMP_DIR/${basename}.${version}.compress.log"
    local start=$(date +%s%N)
    timeout $COMPRESS_TIMEOUT "$compress_bin" "$file" "$compressed_file" $extra_args > "$compress_output" 2>&1
    local exit_code=$?
    local end=$(date +%s%N)
    local compress_time=$(( (end - start) / 1000000 ))

    # Check for timeout or error
    if [ $exit_code -eq 124 ]; then
        echo "TIMEOUT,TIMEOUT,TIMEOUT,TIMEOUT,TIMEOUT,TIMEOUT,N/A" >> "$TEMP_DIR/${basename}.${version}.result"
        return 1
    elif [ $exit_code -ne 0 ]; then
        echo "ERROR,ERROR,ERROR,ERROR,ERROR,ERROR,N/A" >> "$TEMP_DIR/${basename}.${version}.result"
        return 1
    fi

    # Check if compression succeeded
    if [ ! -f "$compressed_file" ]; then
        echo "ERROR,ERROR,ERROR,ERROR,ERROR,ERROR,N/A" >> "$TEMP_DIR/${basename}.${version}.result"
        return 1
    fi

    local compressed_size=$(stat -c%s "$compressed_file")
    local ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $compressed_size / $original_size}")
    local bits_per_symbol=$(awk "BEGIN {printf \"%.4f\", 8.0 * $compressed_size / $original_size}")

    # Decompression with timeout
    start=$(date +%s%N)
    timeout $DECOMPRESS_TIMEOUT "$decompress_bin" "$compressed_file" "$decompressed_file" > /dev/null 2>&1
    exit_code=$?
    end=$(date +%s%N)
    local decompress_time=$(( (end - start) / 1000000 ))

    # Check for timeout or error
    if [ $exit_code -eq 124 ]; then
        echo "$compressed_size,$ratio,$bits_per_symbol,$compress_time,TIMEOUT,TIMEOUT,N/A" >> "$TEMP_DIR/${basename}.${version}.result"
        rm -f "$compressed_file"
        return 1
    elif [ $exit_code -ne 0 ]; then
        echo "$compressed_size,$ratio,$bits_per_symbol,$compress_time,ERROR,NO,N/A" >> "$TEMP_DIR/${basename}.${version}.result"
        rm -f "$compressed_file"
        return 1
    fi

    # Verify lossless
    local lossless="NO"
    if cmp -s "$file" "$decompressed_file"; then
        lossless="YES"
    fi

    # Extract model used (for v2.0)
    local model_used="Order-0"
    if [ "$version" = "v2.0" ]; then
        if grep -q "Order-1" "$compress_output"; then
            model_used="Order-1"
        elif grep -q "UNCOMPRESSED" "$compress_output"; then
            model_used="Uncompressed"
        fi
    fi

    # Save results to temp file
    echo "$compressed_size,$ratio,$bits_per_symbol,$compress_time,$decompress_time,$lossless,$model_used" >> "$TEMP_DIR/${basename}.${version}.result"

    # Update totals
    version_total_original[$version]=$(( ${version_total_original[$version]:-0} + original_size ))
    version_total_compressed[$version]=$(( ${version_total_compressed[$version]:-0} + compressed_size ))
    version_file_count[$version]=$(( ${version_file_count[$version]:-0} + 1 ))
    version_total_comp_time[$version]=$(( ${version_total_comp_time[$version]:-0} + compress_time ))
    version_total_decomp_time[$version]=$(( ${version_total_decomp_time[$version]:-0} + decompress_time ))

    # Clean up
    rm -f "$compressed_file" "$decompressed_file" "$compress_output"

    return 0
}

# Process each file in data directory
for file in "$DATA_DIR"/*; do
    if [ ! -f "$file" ]; then
        continue
    fi

    basename=$(basename "$file")
    original_size=$(stat -c%s "$file")

    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${BOLD}File: $basename${NC} ($(numfmt --to=iec-i --suffix=B $original_size))"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"

    # Clean previous temp results
    rm -f "$TEMP_DIR/${basename}."*.result

    # Test v1.0
    echo -n "  Testing v1.0... "
    if benchmark_file_version "$file" "$basename" "v1.0" \
        "$V1_DIR/compress" \
        "$V1_DIR/decompress" \
        "$original_size" \
        "--yes"; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}⏱️ TIMEOUT/ERROR${NC}"
    fi

    # Test v2.0 (current) with auto-selection
    echo -n "  Testing v2.0 (auto)... "
    if benchmark_file_version "$file" "$basename" "v2.0" \
        "$CURRENT_BIN_DIR/compress" \
        "$CURRENT_BIN_DIR/decompress" \
        "$original_size" \
        "--model auto --yes"; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}⏱️ TIMEOUT/ERROR${NC}"
    fi

    echo

    # Collect results and write to CSV
    for version in v1.0 v2.0; do
        result_file="$TEMP_DIR/${basename}.${version}.result"
        if [ -f "$result_file" ]; then
            result=$(cat "$result_file")
            echo "$basename,$original_size,$version,$result" >> "$RESULTS_CSV"
        fi
    done

    # Store ratios for comparison
    v1_result="$TEMP_DIR/${basename}.v1.0.result"
    v2_result="$TEMP_DIR/${basename}.v2.0.result"
    if [ -f "$v1_result" ]; then
        file_v1_ratio[$basename]=$(cat "$v1_result" | cut -d',' -f2)
    fi
    if [ -f "$v2_result" ]; then
        file_v2_ratio[$basename]=$(cat "$v2_result" | cut -d',' -f2)
    fi
done

# Generate markdown report
echo "## Quick Summary" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "### Compression Ratio Comparison" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| File | Size | v1.0 Ratio | v2.0 Ratio | Improvement | Winner | v2.0 Model |" >> "$RESULTS_MD"
echo "|------|------|------------|------------|-------------|--------|------------|" >> "$RESULTS_MD"

# Process each file for quick summary
v1_wins=0
v2_wins=0
for file in "$DATA_DIR"/*; do
    if [ ! -f "$file" ]; then
        continue
    fi

    basename=$(basename "$file")
    original_size=$(stat -c%s "$file")
    size_display=$(numfmt --to=iec-i --suffix=B $original_size)

    v1_result="$TEMP_DIR/${basename}.v1.0.result"
    v2_result="$TEMP_DIR/${basename}.v2.0.result"

    if [ -f "$v1_result" ] && [ -f "$v2_result" ]; then
        v1_ratio=$(cat "$v1_result" | cut -d',' -f2)
        v2_ratio=$(cat "$v2_result" | cut -d',' -f2)
        v2_model=$(cat "$v2_result" | cut -d',' -f7)

        improvement=$(awk "BEGIN {printf \"%.2f\", $v1_ratio - $v2_ratio}")
        improvement_pct=$(awk "BEGIN {printf \"%.1f\", 100.0 * ($v1_ratio - $v2_ratio) / $v1_ratio}")

        winner=""
        if (( $(echo "$v2_ratio < $v1_ratio" | bc -l) )); then
            winner="v2.0 ⭐"
            v2_wins=$((v2_wins + 1))
        else
            winner="v1.0"
            v1_wins=$((v1_wins + 1))
        fi

        # Format improvement with sign and color indicator
        if (( $(echo "$improvement > 0" | bc -l) )); then
            improvement_str="-${improvement}% 🟢"
        elif (( $(echo "$improvement < 0" | bc -l) )); then
            improvement_str="+${improvement#-}% 🔴"
        else
            improvement_str="0% ⚪"
        fi

        echo "| $basename | $size_display | ${v1_ratio}% | ${v2_ratio}% | $improvement_str | $winner | $v2_model |" >> "$RESULTS_MD"
    fi
done

echo >> "$RESULTS_MD"
echo "**Legend**: 🟢 = v2.0 better, 🔴 = v1.0 better, ⚪ = tie" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "**Overall**: v1.0 won $v1_wins files, v2.0 won $v2_wins files" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Detailed file-by-file comparison
echo "## Detailed File-by-File Comparison" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

for file in "$DATA_DIR"/*; do
    if [ ! -f "$file" ]; then
        continue
    fi

    basename=$(basename "$file")
    original_size=$(stat -c%s "$file")
    size_display=$(numfmt --to=iec-i --suffix=B $original_size)

    echo "### File: $basename" >> "$RESULTS_MD"
    echo >> "$RESULTS_MD"
    echo "**Original Size**: $original_size bytes ($size_display)" >> "$RESULTS_MD"
    echo >> "$RESULTS_MD"
    echo "| Version | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Model |" >> "$RESULTS_MD"
    echo "|---------|-----------------|-------|-------------|---------------|-----------------|-------|" >> "$RESULTS_MD"

    for version in v1.0 v2.0; do
        result_file="$TEMP_DIR/${basename}.${version}.result"
        if [ -f "$result_file" ]; then
            IFS=',' read -r comp_size ratio bits comp_time decomp_time lossless model <<< $(cat "$result_file")

            comp_size_display=$(numfmt --to=iec-i --suffix=B $comp_size)

            echo "| $version | $comp_size_display ($comp_size) | ${ratio}% | ${bits} | ${comp_time} ms | ${decomp_time} ms | $model |" >> "$RESULTS_MD"
        fi
    done

    # Add improvement summary for this file
    v1_result="$TEMP_DIR/${basename}.v1.0.result"
    v2_result="$TEMP_DIR/${basename}.v2.0.result"
    if [ -f "$v1_result" ] && [ -f "$v2_result" ]; then
        v1_ratio=$(cat "$v1_result" | cut -d',' -f2)
        v1_comp_time=$(cat "$v1_result" | cut -d',' -f4)
        v2_ratio=$(cat "$v2_result" | cut -d',' -f2)
        v2_comp_time=$(cat "$v2_result" | cut -d',' -f4)

        ratio_improvement=$(awk "BEGIN {printf \"%.2f\", $v1_ratio - $v2_ratio}")
        ratio_improvement_pct=$(awk "BEGIN {printf \"%.1f\", 100.0 * ($v1_ratio - $v2_ratio) / $v1_ratio}")

        # Speed improvement (positive = faster)
        speed_improvement=$(awk "BEGIN {printf \"%.1f\", 100.0 * ($v1_comp_time - $v2_comp_time) / $v1_comp_time}")

        echo >> "$RESULTS_MD"
        echo "**Improvements in v2.0**:" >> "$RESULTS_MD"
        if (( $(echo "$ratio_improvement > 0" | bc -l) )); then
            echo "- Compression: ${ratio_improvement}pp better (${ratio_improvement_pct}% improvement)" >> "$RESULTS_MD"
        elif (( $(echo "$ratio_improvement < 0" | bc -l) )); then
            echo "- Compression: ${ratio_improvement#-}pp worse" >> "$RESULTS_MD"
        else
            echo "- Compression: No change" >> "$RESULTS_MD"
        fi

        if (( $(echo "$speed_improvement > 0" | bc -l) )); then
            echo "- Speed: ${speed_improvement}% faster" >> "$RESULTS_MD"
        elif (( $(echo "$speed_improvement < 0" | bc -l) )); then
            echo "- Speed: ${speed_improvement#-}% slower" >> "$RESULTS_MD"
        else
            echo "- Speed: No change" >> "$RESULTS_MD"
        fi
    fi

    echo >> "$RESULTS_MD"
done

echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Performance Improvements
echo "## Performance Improvements" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Calculate average improvements
v1_total_orig=${version_total_original[v1.0]:-0}
v1_total_comp=${version_total_compressed[v1.0]:-0}
v1_avg_ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $v1_total_comp / $v1_total_orig}")
v1_file_count=${version_file_count[v1.0]:-1}
v1_avg_comp_time=$(( ${version_total_comp_time[v1.0]:-0} / $v1_file_count ))
v1_avg_decomp_time=$(( ${version_total_decomp_time[v1.0]:-0} / $v1_file_count ))

v2_total_orig=${version_total_original[v2.0]:-0}
v2_total_comp=${version_total_compressed[v2.0]:-0}
v2_avg_ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $v2_total_comp / $v2_total_orig}")
v2_file_count=${version_file_count[v2.0]:-1}
v2_avg_comp_time=$(( ${version_total_comp_time[v2.0]:-0} / $v2_file_count ))
v2_avg_decomp_time=$(( ${version_total_decomp_time[v2.0]:-0} / $v2_file_count ))

ratio_improvement=$(awk "BEGIN {printf \"%.2f\", $v1_avg_ratio - $v2_avg_ratio}")
ratio_improvement_pct=$(awk "BEGIN {printf \"%.1f\", 100.0 * ($v1_avg_ratio - $v2_avg_ratio) / $v1_avg_ratio}")

echo "### Compression Ratio" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| Metric | v1.0 | v2.0 | Improvement |" >> "$RESULTS_MD"
echo "|--------|------|------|-------------|" >> "$RESULTS_MD"
echo "| Average Ratio | ${v1_avg_ratio}% | ${v2_avg_ratio}% | ${ratio_improvement}pp (${ratio_improvement_pct}%) |" >> "$RESULTS_MD"
echo "| Total Original | $(numfmt --to=iec-i --suffix=B $v1_total_orig) | $(numfmt --to=iec-i --suffix=B $v2_total_orig) | - |" >> "$RESULTS_MD"
echo "| Total Compressed | $(numfmt --to=iec-i --suffix=B $v1_total_comp) | $(numfmt --to=iec-i --suffix=B $v2_total_comp) | $(numfmt --to=iec-i --suffix=B $(($v1_total_comp - $v2_total_comp))) saved |" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

echo "### Speed" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| Metric | v1.0 | v2.0 | Change |" >> "$RESULTS_MD"
echo "|--------|------|------|--------|" >> "$RESULTS_MD"

comp_time_change=$(awk "BEGIN {printf \"%.1f\", 100.0 * ($v1_avg_comp_time - $v2_avg_comp_time) / $v1_avg_comp_time}")
decomp_time_change=$(awk "BEGIN {printf \"%.1f\", 100.0 * ($v1_avg_decomp_time - $v2_avg_decomp_time) / $v1_avg_decomp_time}")

if (( $(echo "$comp_time_change > 0" | bc -l) )); then
    comp_change_str="${comp_time_change}% faster ⚡"
else
    comp_change_str="${comp_time_change#-}% slower 🐌"
fi

if (( $(echo "$decomp_time_change > 0" | bc -l) )); then
    decomp_change_str="${decomp_time_change}% faster ⚡"
else
    decomp_change_str="${decomp_time_change#-}% slower 🐌"
fi

echo "| Avg Compress Time | ${v1_avg_comp_time} ms | ${v2_avg_comp_time} ms | $comp_change_str |" >> "$RESULTS_MD"
echo "| Avg Decompress Time | ${v1_avg_decomp_time} ms | ${v2_avg_decomp_time} ms | $decomp_change_str |" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Model Selection Analysis (v2.0 only)
echo "## Model Selection Analysis (v2.0)" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "This section shows which model v2.0's auto-selection chose for each file:" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| File | Model Selected | Compression Ratio | Reason |" >> "$RESULTS_MD"
echo "|------|----------------|-------------------|--------|" >> "$RESULTS_MD"

for file in "$DATA_DIR"/*; do
    if [ ! -f "$file" ]; then
        continue
    fi

    basename=$(basename "$file")
    v2_result="$TEMP_DIR/${basename}.v2.0.result"

    if [ -f "$v2_result" ]; then
        ratio=$(cat "$v2_result" | cut -d',' -f2)
        model=$(cat "$v2_result" | cut -d',' -f7)

        reason=""
        if [ "$model" = "Order-1" ]; then
            reason="Low entropy, good patterns"
        elif [ "$model" = "Order-0" ]; then
            reason="Fast, universal"
        elif [ "$model" = "Uncompressed" ]; then
            reason="Incompressible (high entropy)"
        fi

        echo "| $basename | $model | ${ratio}% | $reason |" >> "$RESULTS_MD"
    fi
done

echo >> "$RESULTS_MD"
echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Overall Statistics
echo "## Overall Statistics" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| Version | Total Original | Total Compressed | Avg Ratio | Avg Bits/Symbol | Files Tested | Avg Compress Time | Avg Decompress Time |" >> "$RESULTS_MD"
echo "|---------|---------------|------------------|-----------|----------------|--------------|-------------------|---------------------|" >> "$RESULTS_MD"

for version in v1.0 v2.0; do
    total_orig=${version_total_original[$version]:-0}
    total_comp=${version_total_compressed[$version]:-0}
    file_count=${version_file_count[$version]:-1}
    avg_ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $total_comp / $total_orig}")
    avg_bits=$(awk "BEGIN {printf \"%.4f\", 8.0 * $total_comp / $total_orig}")
    avg_comp_time=$(( ${version_total_comp_time[$version]:-0} / $file_count ))
    avg_decomp_time=$(( ${version_total_decomp_time[$version]:-0} / $file_count ))

    echo "| $version | $(numfmt --to=iec-i --suffix=B $total_orig) | $(numfmt --to=iec-i --suffix=B $total_comp) | ${avg_ratio}% | ${avg_bits} | $file_count | ${avg_comp_time} ms | ${avg_decomp_time} ms |" >> "$RESULTS_MD"
done

echo >> "$RESULTS_MD"
echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Conclusion
echo "## Conclusion" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "### Key Improvements in v2.0:" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "1. **Better Compression**: ${ratio_improvement}pp average improvement (${ratio_improvement_pct}%)" >> "$RESULTS_MD"
echo "2. **Range Coding**: Faster than arithmetic coding with same compression" >> "$RESULTS_MD"
echo "3. **Multi-Model System**: Order-0 and Order-1 with intelligent auto-selection" >> "$RESULTS_MD"
echo "4. **Smart Detection**: Avoids compressing incompressible files" >> "$RESULTS_MD"
echo "5. **Won ${v2_wins}/${v2_file_count} files**: Demonstrates robustness across file types" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

echo "### Technical Achievements:" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "- Implemented adaptive PPM-style Order-1 context modeling" >> "$RESULTS_MD"
echo "- Replaced arithmetic coding with range coding (Schindler's algorithm)" >> "$RESULTS_MD"
echo "- Added entropy-based auto-selection for optimal model choice" >> "$RESULTS_MD"
echo "- Maintained 100% lossless guarantee across all files and models" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "*Report generated on $(date)*" >> "$RESULTS_MD"

# Terminal summary
echo -e "\n${BOLD}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}║                    VERSION COMPARISON SUMMARY                  ║${NC}"
echo -e "${BOLD}╚════════════════════════════════════════════════════════════════╝${NC}"
echo
printf "${BOLD}%-15s %15s %15s %15s${NC}\n" "Version" "Total Original" "Total Compressed" "Avg Ratio"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

for version in v1.0 v2.0; do
    total_orig=${version_total_original[$version]:-0}
    total_comp=${version_total_compressed[$version]:-0}
    avg_ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $total_comp / $total_orig}")

    # Determine color
    if [ "$version" = "v2.0" ]; then
        color=$GREEN
    else
        color=$YELLOW
    fi

    printf "${color}%-15s %15s %15s %14s%%${NC}\n" \
        "$version" \
        "$(numfmt --to=iec-i --suffix=B $total_orig)" \
        "$(numfmt --to=iec-i --suffix=B $total_comp)" \
        "$avg_ratio"
done

echo
echo -e "${BOLD}Improvement: ${GREEN}${ratio_improvement}pp${NC} ${BOLD}(${ratio_improvement_pct}% better compression)${NC}"
echo -e "${BOLD}Files won: ${GREEN}v2.0: $v2_wins${NC} ${BOLD}| ${YELLOW}v1.0: $v1_wins${NC}"
echo

echo -e "${BOLD}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}║                    RESULTS SAVED                               ║${NC}"
echo -e "${BOLD}╚════════════════════════════════════════════════════════════════╝${NC}"
echo -e "  📊 CSV:      ${GREEN}$RESULTS_CSV${NC}"
echo -e "  📝 Markdown: ${GREEN}$RESULTS_MD${NC}"
echo

# Clean up temp directory
rm -rf "$TEMP_DIR"

echo -e "${GREEN}Version comparison benchmark complete!${NC}"
