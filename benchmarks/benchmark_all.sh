#!/bin/bash

# Comprehensive benchmark script for all data files
# Compares: our compressor, gzip, bzip2, xz (lzma), zstd
# Outputs: Terminal table, CSV file, Comprehensive Markdown report

set -e

# Directories
DATA_DIR="data"
TEMP_DIR="benchmarks/tmp"
RESULTS_CSV="benchmarks/results.csv"
RESULTS_MD="benchmarks/results.md"

# Timeout configuration (seconds)
COMPRESS_TIMEOUT=60    # Maximum time for compression
DECOMPRESS_TIMEOUT=30  # Maximum time for decompression

# Colors
BOLD='\033[1m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Create temp directory
mkdir -p "$TEMP_DIR"

# Initialize results files
echo "File,Original Size (bytes),Tool,Compressed Size (bytes),Compression Ratio (%),Bits/Symbol,Compress Time (ms),Decompress Time (ms),Lossless" > "$RESULTS_CSV"

# Initialize markdown with header
cat > "$RESULTS_MD" << EOF
# Comprehensive Compression Benchmark Results

**Project**: TAI - Algorithmic Information Theory
**Date**: $(date +"%Y-%m-%d")
**Tools Compared**: G07, gzip, bzip2, xz (LZMA), zstd

---

## Table of Contents
1. [Quick Summary](#quick-summary)
2. [Detailed Results by File](#detailed-results-by-file)
3. [Performance Metrics](#performance-metrics)
4. [Rankings](#rankings)
5. [Overall Statistics](#overall-statistics)

---

EOF

echo -e "${BOLD}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}║         COMPREHENSIVE COMPRESSION BENCHMARK                    ║${NC}"
echo -e "${BOLD}╚════════════════════════════════════════════════════════════════╝${NC}"
echo

# Arrays to store results
declare -A tool_total_original
declare -A tool_total_compressed
declare -A tool_file_count
declare -A tool_wins
declare -A file_results

# Function to benchmark one file with one tool
benchmark_file_tool() {
    local file="$1"
    local basename="$2"
    local tool_name="$3"
    local compress_cmd="$4"
    local decompress_cmd="$5"
    local compressed_file="$6"
    local original_size="$7"

    # Compression with timeout
    local start=$(date +%s%N)
    timeout $COMPRESS_TIMEOUT bash -c "$compress_cmd" > /dev/null 2>&1
    local exit_code=$?
    local end=$(date +%s%N)
    local compress_time=$(( (end - start) / 1000000 ))

    # Check for timeout or error
    if [ $exit_code -eq 124 ]; then
        echo "TIMEOUT,TIMEOUT,TIMEOUT,TIMEOUT,TIMEOUT,TIMEOUT" >> "$TEMP_DIR/${basename}.${tool_name}.result"
        return 1
    elif [ $exit_code -ne 0 ]; then
        echo "ERROR,ERROR,ERROR,ERROR,ERROR,ERROR" >> "$TEMP_DIR/${basename}.${tool_name}.result"
        return 1
    fi

    # Check if compression succeeded
    if [ ! -f "$compressed_file" ]; then
        echo "ERROR,ERROR,ERROR,ERROR,ERROR,ERROR" >> "$TEMP_DIR/${basename}.${tool_name}.result"
        return 1
    fi

    local compressed_size=$(stat -c%s "$compressed_file")
    local ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $compressed_size / $original_size}")
    local bits_per_symbol=$(awk "BEGIN {printf \"%.4f\", 8.0 * $compressed_size / $original_size}")

    # Decompression with timeout
    local decompressed_file="$TEMP_DIR/${basename}.${tool_name}.decompressed"
    start=$(date +%s%N)
    timeout $DECOMPRESS_TIMEOUT bash -c "$decompress_cmd" > /dev/null 2>&1
    exit_code=$?
    end=$(date +%s%N)
    local decompress_time=$(( (end - start) / 1000000 ))

    # Check for timeout or error
    if [ $exit_code -eq 124 ]; then
        echo "$compressed_size,$ratio,$bits_per_symbol,$compress_time,TIMEOUT,TIMEOUT" >> "$TEMP_DIR/${basename}.${tool_name}.result"
        rm -f "$compressed_file"
        return 1
    elif [ $exit_code -ne 0 ]; then
        echo "$compressed_size,$ratio,$bits_per_symbol,$compress_time,ERROR,NO" >> "$TEMP_DIR/${basename}.${tool_name}.result"
        rm -f "$compressed_file"
        return 1
    fi

    # Verify lossless
    local lossless="NO"
    if cmp -s "$file" "$decompressed_file"; then
        lossless="YES"
    fi

    # Save results to temp file
    echo "$compressed_size,$ratio,$bits_per_symbol,$compress_time,$decompress_time,$lossless" >> "$TEMP_DIR/${basename}.${tool_name}.result"

    # Update totals for summary
    tool_total_original[$tool_name]=$(( ${tool_total_original[$tool_name]:-0} + original_size ))
    tool_total_compressed[$tool_name]=$(( ${tool_total_compressed[$tool_name]:-0} + compressed_size ))
    tool_file_count[$tool_name]=$(( ${tool_file_count[$tool_name]:-0} + 1 ))

    # Clean up
    rm -f "$compressed_file" "$decompressed_file"

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

    # Test our compressor
    echo -n "  Testing G07... "
    if benchmark_file_tool "$file" "$basename" "g07" \
        "./bin/compress '$file' '$TEMP_DIR/${basename}.g07' --yes" \
        "./bin/decompress '$TEMP_DIR/${basename}.g07' '$TEMP_DIR/${basename}.g07.decompressed'" \
        "$TEMP_DIR/${basename}.g07" \
        "$original_size"; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}⏱️ TIMEOUT/ERROR${NC}"
    fi

    # Test gzip
    echo -n "  Testing gzip... "
    if benchmark_file_tool "$file" "$basename" "gzip" \
        "gzip -c '$file' > '$TEMP_DIR/${basename}.gz'" \
        "gzip -dc '$TEMP_DIR/${basename}.gz' > '$TEMP_DIR/${basename}.gzip.decompressed'" \
        "$TEMP_DIR/${basename}.gz" \
        "$original_size"; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}⏱️ TIMEOUT/ERROR${NC}"
    fi

    # Test bzip2
    echo -n "  Testing bzip2... "
    if benchmark_file_tool "$file" "$basename" "bzip2" \
        "bzip2 -c '$file' > '$TEMP_DIR/${basename}.bz2'" \
        "bzip2 -dc '$TEMP_DIR/${basename}.bz2' > '$TEMP_DIR/${basename}.bzip2.decompressed'" \
        "$TEMP_DIR/${basename}.bz2" \
        "$original_size"; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}⏱️ TIMEOUT/ERROR${NC}"
    fi

    # Test xz
    echo -n "  Testing xz... "
    if benchmark_file_tool "$file" "$basename" "xz" \
        "xz -c '$file' > '$TEMP_DIR/${basename}.xz'" \
        "xz -dc '$TEMP_DIR/${basename}.xz' > '$TEMP_DIR/${basename}.xz.decompressed'" \
        "$TEMP_DIR/${basename}.xz" \
        "$original_size"; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}⏱️ TIMEOUT/ERROR${NC}"
    fi

    # Test zstd (if available)
    if command -v zstd &> /dev/null; then
        echo -n "  Testing zstd... "
        if benchmark_file_tool "$file" "$basename" "zstd" \
            "zstd -q '$file' -o '$TEMP_DIR/${basename}.zst'" \
            "zstd -qd '$TEMP_DIR/${basename}.zst' -o '$TEMP_DIR/${basename}.zstd.decompressed'" \
            "$TEMP_DIR/${basename}.zst" \
            "$original_size"; then
            echo -e "${GREEN}✓${NC}"
        else
            echo -e "${YELLOW}⏱️ TIMEOUT/ERROR${NC}"
        fi
    fi

    echo

    # Collect results and write to CSV
    for tool in g07 gzip bzip2 xz zstd; do
        result_file="$TEMP_DIR/${basename}.${tool}.result"
        if [ -f "$result_file" ]; then
            result=$(cat "$result_file")
            echo "$basename,$original_size,$tool,$result" >> "$RESULTS_CSV"
        fi
    done
done

# Generate comprehensive markdown report
echo "## Quick Summary" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "### Compression Ratio Comparison" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| File | Size | G07 | gzip | bzip2 | xz | zstd | Winner |" >> "$RESULTS_MD"
echo "|------|------|----------|------|-------|----|----|--------|" >> "$RESULTS_MD"

# Process each file for quick summary
for file in "$DATA_DIR"/*; do
    if [ ! -f "$file" ]; then
        continue
    fi

    basename=$(basename "$file")
    original_size=$(stat -c%s "$file")
    size_display=$(numfmt --to=iec-i --suffix=B $original_size)

    echo -n "| $basename | $size_display |" >> "$RESULTS_MD"

    # Find best ratio for this file
    best_ratio=999.99
    best_tool=""

    for tool in g07 gzip bzip2 xz zstd; do
        result_file="$TEMP_DIR/${basename}.${tool}.result"
        if [ -f "$result_file" ]; then
            ratio=$(cat "$result_file" | cut -d',' -f2)

            # Check if this tool wins
            if (( $(echo "$ratio < $best_ratio" | bc -l) )); then
                best_ratio=$ratio
                best_tool=$tool
            fi

            # Mark if our tool
            if [ "$tool" = "g07" ] && [ "$best_tool" = "g07" ]; then
                echo -n " **${ratio}%** ⭐ |" >> "$RESULTS_MD"
            elif [ "$tool" = "$best_tool" ]; then
                echo -n " **${ratio}%** ⭐ |" >> "$RESULTS_MD"
            else
                echo -n " ${ratio}% |" >> "$RESULTS_MD"
            fi
        else
            echo -n " N/A |" >> "$RESULTS_MD"
        fi
    done

    echo " $best_tool |" >> "$RESULTS_MD"

    # Count wins
    tool_wins[$best_tool]=$(( ${tool_wins[$best_tool]:-0} + 1 ))
done

echo >> "$RESULTS_MD"
echo "⭐ = Best compressor for that file" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Detailed results by file
echo "## Detailed Results by File" >> "$RESULTS_MD"
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
    echo "| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |" >> "$RESULTS_MD"
    echo "|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|" >> "$RESULTS_MD"

    for tool in g07 gzip bzip2 xz zstd; do
        result_file="$TEMP_DIR/${basename}.${tool}.result"
        if [ -f "$result_file" ]; then
            IFS=',' read -r comp_size ratio bits comp_time decomp_time lossless <<< $(cat "$result_file")

            # Handle timeout cases
            if [ "$comp_size" = "TIMEOUT" ]; then
                echo "| $tool | ⏱️ TIMEOUT (>${COMPRESS_TIMEOUT}s) | - | - | - | - | - | ⏱️ |" >> "$RESULTS_MD"
                continue
            elif [ "$comp_size" = "ERROR" ]; then
                echo "| $tool | ❌ ERROR | - | - | - | - | - | ❌ |" >> "$RESULTS_MD"
                continue
            fi

            comp_size_display=$(numfmt --to=iec-i --suffix=B $comp_size)
            saved=$(( original_size - comp_size ))
            saved_display=$(numfmt --to=iec-i --suffix=B $saved | sed 's/^-//')

            if [ $saved -ge 0 ]; then
                saved_str="✓ $saved_display saved"
            else
                saved_str="✗ $saved_display overhead"
            fi

            # Performance indicator
            perf_indicator=""
            if (( $(echo "$ratio < 50" | bc -l) )); then
                perf_indicator="🟢"
            elif (( $(echo "$ratio < 80" | bc -l) )); then
                perf_indicator="🟡"
            else
                perf_indicator="🔴"
            fi

            lossless_mark="✓"
            if [ "$lossless" = "TIMEOUT" ]; then
                lossless_mark="⏱️"
                decomp_time="TIMEOUT (>${DECOMPRESS_TIMEOUT}s)"
            elif [ "$lossless" != "YES" ]; then
                lossless_mark="✗"
            fi

            # Handle decompress timeout in time display
            if [ "$decomp_time" = "ERROR" ]; then
                decomp_time="ERROR"
            fi

            echo "| $tool | $comp_size_display ($comp_size) | $perf_indicator ${ratio}% | ${bits} | ${comp_time} ms | ${decomp_time} | $saved_str | $lossless_mark |" >> "$RESULTS_MD"
        fi
    done

    echo >> "$RESULTS_MD"
done

echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Performance Metrics
echo "## Performance Metrics" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

echo "### Compression Speed (Average)" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| Tool | Avg Compress Time | Ranking |" >> "$RESULTS_MD"
echo "|------|-------------------|---------|" >> "$RESULTS_MD"

# Calculate averages and rank (skip TIMEOUT and ERROR)
declare -A avg_compress_time
for tool in g07 gzip bzip2 xz zstd; do
    total_time=0
    count=0
    for file in "$DATA_DIR"/*; do
        if [ ! -f "$file" ]; then continue; fi
        basename=$(basename "$file")
        result_file="$TEMP_DIR/${basename}.${tool}.result"
        if [ -f "$result_file" ]; then
            time=$(cat "$result_file" | cut -d',' -f4)
            # Skip TIMEOUT and ERROR
            if [ "$time" != "TIMEOUT" ] && [ "$time" != "ERROR" ]; then
                total_time=$(( total_time + time ))
                count=$(( count + 1 ))
            fi
        fi
    done
    if [ $count -gt 0 ]; then
        avg_compress_time[$tool]=$(( total_time / count ))
    fi
done

# Sort and display
rank=1
for tool in $(for t in "${!avg_compress_time[@]}"; do echo "$t ${avg_compress_time[$t]}"; done | sort -k2 -n | cut -d' ' -f1); do
    time_ms=${avg_compress_time[$tool]}
    medal=""
    case $rank in
        1) medal="🥇" ;;
        2) medal="🥈" ;;
        3) medal="🥉" ;;
    esac
    echo "| $tool | ${time_ms} ms | $medal #$rank |" >> "$RESULTS_MD"
    rank=$(( rank + 1 ))
done

echo >> "$RESULTS_MD"

echo "### Decompression Speed (Average)" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| Tool | Avg Decompress Time | Ranking |" >> "$RESULTS_MD"
echo "|------|---------------------|---------|" >> "$RESULTS_MD"

# Calculate averages and rank (skip TIMEOUT and ERROR)
declare -A avg_decompress_time
for tool in g07 gzip bzip2 xz zstd; do
    total_time=0
    count=0
    for file in "$DATA_DIR"/*; do
        if [ ! -f "$file" ]; then continue; fi
        basename=$(basename "$file")
        result_file="$TEMP_DIR/${basename}.${tool}.result"
        if [ -f "$result_file" ]; then
            time=$(cat "$result_file" | cut -d',' -f5)
            # Skip TIMEOUT and ERROR
            if [ "$time" != "TIMEOUT" ] && [ "$time" != "ERROR" ]; then
                total_time=$(( total_time + time ))
                count=$(( count + 1 ))
            fi
        fi
    done
    if [ $count -gt 0 ]; then
        avg_decompress_time[$tool]=$(( total_time / count ))
    fi
done

# Sort and display
rank=1
for tool in $(for t in "${!avg_decompress_time[@]}"; do echo "$t ${avg_decompress_time[$t]}"; done | sort -k2 -n | cut -d' ' -f1); do
    time_ms=${avg_decompress_time[$tool]}
    medal=""
    case $rank in
        1) medal="🥇" ;;
        2) medal="🥈" ;;
        3) medal="🥉" ;;
    esac
    echo "| $tool | ${time_ms} ms | $medal #$rank |" >> "$RESULTS_MD"
    rank=$(( rank + 1 ))
done

echo >> "$RESULTS_MD"
echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Rankings
echo "## Rankings" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

echo "### Overall Compression Ratio Ranking" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| Rank | Tool | Average Ratio | Files Won | Total Compressed | Performance |" >> "$RESULTS_MD"
echo "|------|------|---------------|-----------|------------------|-------------|" >> "$RESULTS_MD"

rank=1
for tool in $(for t in "${!tool_total_original[@]}"; do
    avg=$(awk "BEGIN {printf \"%.2f\", 100.0 * ${tool_total_compressed[$t]} / ${tool_total_original[$t]}}")
    echo "$t $avg"
done | sort -k2 -n | cut -d' ' -f1); do
    total_orig=${tool_total_original[$tool]}
    total_comp=${tool_total_compressed[$tool]}
    avg_ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $total_comp / $total_orig}")
    wins=${tool_wins[$tool]:-0}
    total_comp_display=$(numfmt --to=iec-i --suffix=B $total_comp)

    medal=""
    perf=""
    case $rank in
        1) medal="🥇"; perf="⭐⭐⭐" ;;
        2) medal="🥈"; perf="⭐⭐" ;;
        3) medal="🥉"; perf="⭐" ;;
    esac

    echo "| $medal $rank | **$tool** | ${avg_ratio}% | $wins | $total_comp_display | $perf |" >> "$RESULTS_MD"
    rank=$(( rank + 1 ))
done

echo >> "$RESULTS_MD"

echo "### Files Won by Each Tool" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
for tool in g07 gzip bzip2 xz zstd; do
    wins=${tool_wins[$tool]:-0}
    echo "- **$tool**: $wins file(s)" >> "$RESULTS_MD"
done

echo >> "$RESULTS_MD"
echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Overall Statistics
echo "## Overall Statistics" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| Tool | Total Original | Total Compressed | Average Ratio | Bits/Symbol | Files Tested |" >> "$RESULTS_MD"
echo "|------|---------------|------------------|---------------|-------------|--------------|" >> "$RESULTS_MD"

for tool in g07 gzip bzip2 xz zstd; do
    if [ ${tool_file_count[$tool]:-0} -gt 0 ]; then
        total_orig=${tool_total_original[$tool]}
        total_comp=${tool_total_compressed[$tool]}
        avg_ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $total_comp / $total_orig}")
        avg_bits=$(awk "BEGIN {printf \"%.4f\", 8.0 * $total_comp / $total_orig}")

        echo "| $tool | $(numfmt --to=iec-i --suffix=B $total_orig) | $(numfmt --to=iec-i --suffix=B $total_comp) | ${avg_ratio}% | ${avg_bits} | ${tool_file_count[$tool]} |" >> "$RESULTS_MD"
    fi
done

echo >> "$RESULTS_MD"
echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "*Report generated on $(date)*" >> "$RESULTS_MD"

# Generate summary statistics for terminal
echo -e "\n${BOLD}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}║                    SUMMARY STATISTICS                          ║${NC}"
echo -e "${BOLD}╚════════════════════════════════════════════════════════════════╝${NC}"
echo
printf "${BOLD}%-15s %15s %15s %15s${NC}\n" "Tool" "Total Original" "Total Compressed" "Avg Ratio"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

for tool in g07 gzip bzip2 xz zstd; do
    if [ ${tool_file_count[$tool]:-0} -gt 0 ]; then
        total_orig=${tool_total_original[$tool]}
        total_comp=${tool_total_compressed[$tool]}
        avg_ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $total_comp / $total_orig}")

        # Determine color based on performance
        if (( $(echo "$avg_ratio < 60" | bc -l) )); then
            color=$GREEN
        elif (( $(echo "$avg_ratio < 80" | bc -l) )); then
            color=$YELLOW
        else
            color=$NC
        fi

        printf "${color}%-15s %15s %15s %14s%%${NC}\n" \
            "$tool" \
            "$(numfmt --to=iec-i --suffix=B $total_orig)" \
            "$(numfmt --to=iec-i --suffix=B $total_comp)" \
            "$avg_ratio"
    fi
done

echo
echo -e "${BOLD}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}║                    RESULTS SAVED                               ║${NC}"
echo -e "${BOLD}╚════════════════════════════════════════════════════════════════╝${NC}"
echo -e "  📊 CSV:      ${GREEN}$RESULTS_CSV${NC}"
echo -e "  📝 Markdown: ${GREEN}$RESULTS_MD${NC}"
echo

# Clean up temp directory
rm -rf "$TEMP_DIR"

echo -e "${GREEN}Benchmark complete!${NC}"
