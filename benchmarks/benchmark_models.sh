#!/bin/bash

# Model Comparison Benchmark Script
# Compares different models in current implementation:
# - Order-0 (static frequency model)
# - Order-1 (adaptive context model)
# - Auto-selection (intelligent model choice)

set -e

# Directories
DATA_DIR="data"
TEMP_DIR="benchmarks/tmp_models"
RESULTS_CSV="benchmarks/model_comparison.csv"
RESULTS_MD="benchmarks/model_comparison.md"

# Binary paths
COMPRESS_BIN="bin/compress"
DECOMPRESS_BIN="bin/decompress"

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
MAGENTA='\033[0;35m'
NC='\033[0m' # No Color

# Create temp directory
mkdir -p "$TEMP_DIR"

# Initialize results files
echo "File,Original Size (bytes),Model,Compressed Size (bytes),Compression Ratio (%),Bits/Symbol,Compress Time (ms),Decompress Time (ms),Lossless" > "$RESULTS_CSV"

# Initialize markdown with header
cat > "$RESULTS_MD" << 'EOF'
# Model Comparison Benchmark

**Project**: TAI - Algorithmic Information Theory - Group 07
**Date**: $(date +"%Y-%m-%d")
**Comparison**: Order-0 vs Order-1 vs Auto-Selection

---

## Executive Summary

This benchmark compares three model configurations of our compressor:

- **Order-0**: Static frequency model (fast, universal)
- **Order-1**: Adaptive context model using previous byte as context (better compression for low-entropy data)
- **Auto**: Intelligent selection based on file entropy and size

---

## Table of Contents
1. [Quick Summary](#quick-summary)
2. [Detailed File-by-File Analysis](#detailed-file-by-file-analysis)
3. [Model Performance Characteristics](#model-performance-characteristics)
4. [Speed vs Compression Trade-offs](#speed-vs-compression-trade-offs)
5. [Auto-Selection Effectiveness](#auto-selection-effectiveness)
6. [Recommendations](#recommendations)

---

EOF

echo -e "${BOLD}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}║         MODEL COMPARISON BENCHMARK                             ║${NC}"
echo -e "${BOLD}╚════════════════════════════════════════════════════════════════╝${NC}"
echo

# Check if binaries exist
if [ ! -f "$COMPRESS_BIN" ] || [ ! -f "$DECOMPRESS_BIN" ]; then
    echo -e "${RED}ERROR: Binaries not found in bin/${NC}"
    echo "Please run 'make' to build the project"
    exit 1
fi

echo -e "${CYAN}✓ Binaries found${NC}"
echo

# Arrays to store results
declare -A model_total_original
declare -A model_total_compressed
declare -A model_file_count
declare -A model_total_comp_time
declare -A model_total_decomp_time
declare -A model_wins
declare -A file_best_model
declare -A file_best_ratio

# Function to benchmark one file with one model
benchmark_file_model() {
    local file="$1"
    local basename="$2"
    local model_name="$3"
    local model_arg="$4"
    local original_size="$5"

    local compressed_file="$TEMP_DIR/${basename}.${model_name}.compressed"
    local decompressed_file="$TEMP_DIR/${basename}.${model_name}.decompressed"

    # Compression with timeout
    local compress_output="$TEMP_DIR/${basename}.${model_name}.compress.log"
    local start=$(date +%s%N)
    timeout $COMPRESS_TIMEOUT "$COMPRESS_BIN" "$file" "$compressed_file" $model_arg --yes > "$compress_output" 2>&1
    local exit_code=$?
    local end=$(date +%s%N)
    local compress_time=$(( (end - start) / 1000000 ))

    # Check for timeout or error
    if [ $exit_code -eq 124 ]; then
        echo "TIMEOUT,TIMEOUT,TIMEOUT,TIMEOUT,TIMEOUT,TIMEOUT" >> "$TEMP_DIR/${basename}.${model_name}.result"
        return 1
    elif [ $exit_code -ne 0 ]; then
        echo "ERROR,ERROR,ERROR,ERROR,ERROR,ERROR" >> "$TEMP_DIR/${basename}.${model_name}.result"
        return 1
    fi

    # Check if compression succeeded
    if [ ! -f "$compressed_file" ]; then
        echo "ERROR,ERROR,ERROR,ERROR,ERROR,ERROR" >> "$TEMP_DIR/${basename}.${model_name}.result"
        return 1
    fi

    local compressed_size=$(stat -c%s "$compressed_file")
    local ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $compressed_size / $original_size}")
    local bits_per_symbol=$(awk "BEGIN {printf \"%.4f\", 8.0 * $compressed_size / $original_size}")

    # Decompression with timeout
    start=$(date +%s%N)
    timeout $DECOMPRESS_TIMEOUT "$DECOMPRESS_BIN" "$compressed_file" "$decompressed_file" > /dev/null 2>&1
    exit_code=$?
    end=$(date +%s%N)
    local decompress_time=$(( (end - start) / 1000000 ))

    # Check for timeout
    if [ $exit_code -eq 124 ]; then
        echo "$compressed_size,$ratio,$bits_per_symbol,$compress_time,TIMEOUT,NO" >> "$TEMP_DIR/${basename}.${model_name}.result"
        rm -f "$compressed_file" "$decompressed_file"
        return 1
    elif [ $exit_code -ne 0 ]; then
        echo "$compressed_size,$ratio,$bits_per_symbol,$compress_time,ERROR,NO" >> "$TEMP_DIR/${basename}.${model_name}.result"
        rm -f "$compressed_file" "$decompressed_file"
        return 1
    fi

    # Verify lossless
    local lossless="NO"
    if cmp -s "$file" "$decompressed_file"; then
        lossless="YES"
    fi

    # Save results to temp file
    echo "$compressed_size,$ratio,$bits_per_symbol,$compress_time,$decompress_time,$lossless" >> "$TEMP_DIR/${basename}.${model_name}.result"

    # Update totals
    model_total_original[$model_name]=$(( ${model_total_original[$model_name]:-0} + original_size ))
    model_total_compressed[$model_name]=$(( ${model_total_compressed[$model_name]:-0} + compressed_size ))
    model_file_count[$model_name]=$(( ${model_file_count[$model_name]:-0} + 1 ))
    model_total_comp_time[$model_name]=$(( ${model_total_comp_time[$model_name]:-0} + compress_time ))
    model_total_decomp_time[$model_name]=$(( ${model_total_decomp_time[$model_name]:-0} + decompress_time ))

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

    # Test Order-0
    echo -n "  Testing Order-0... "
    if benchmark_file_model "$file" "$basename" "order0" \
        "--model order0" \
        "$original_size"; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}⏱️ TIMEOUT/ERROR${NC}"
    fi

    # Test Order-1
    echo -n "  Testing Order-1... "
    if benchmark_file_model "$file" "$basename" "order1" \
        "--model order1" \
        "$original_size"; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}⏱️ TIMEOUT/ERROR${NC}"
    fi

    # Test Auto-selection
    echo -n "  Testing Auto... "
    if benchmark_file_model "$file" "$basename" "auto" \
        "--model auto" \
        "$original_size"; then
        echo -e "${GREEN}✓${NC}"
    else
        echo -e "${YELLOW}⏱️ TIMEOUT/ERROR${NC}"
    fi

    echo

    # Collect results and write to CSV
    for model in order0 order1 auto; do
        result_file="$TEMP_DIR/${basename}.${model}.result"
        if [ -f "$result_file" ]; then
            result=$(cat "$result_file")
            echo "$basename,$original_size,$model,$result" >> "$RESULTS_CSV"
        fi
    done

    # Find best model for this file
    best_ratio=999.99
    best_model=""
    for model in order0 order1 auto; do
        result_file="$TEMP_DIR/${basename}.${model}.result"
        if [ -f "$result_file" ]; then
            ratio=$(cat "$result_file" | cut -d',' -f2)
            if (( $(echo "$ratio < $best_ratio" | bc -l) )); then
                best_ratio=$ratio
                best_model=$model
            fi
        fi
    done
    file_best_model[$basename]=$best_model
    file_best_ratio[$basename]=$best_ratio
    model_wins[$best_model]=$(( ${model_wins[$best_model]:-0} + 1 ))
done

# Generate markdown report
echo "## Quick Summary" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "### Compression Ratio Comparison" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| File | Size | Order-0 | Order-1 | Auto | Best Model | Winner Ratio |" >> "$RESULTS_MD"
echo "|------|------|---------|---------|------|------------|--------------|" >> "$RESULTS_MD"

# Process each file for quick summary
for file in "$DATA_DIR"/*; do
    if [ ! -f "$file" ]; then
        continue
    fi

    basename=$(basename "$file")
    original_size=$(stat -c%s "$file")
    size_display=$(numfmt --to=iec-i --suffix=B $original_size)

    order0_result="$TEMP_DIR/${basename}.order0.result"
    order1_result="$TEMP_DIR/${basename}.order1.result"
    auto_result="$TEMP_DIR/${basename}.auto.result"

    order0_ratio=""
    order1_ratio=""
    auto_ratio=""

    if [ -f "$order0_result" ]; then
        order0_ratio=$(cat "$order0_result" | cut -d',' -f2)
    fi
    if [ -f "$order1_result" ]; then
        order1_ratio=$(cat "$order1_result" | cut -d',' -f2)
    fi
    if [ -f "$auto_result" ]; then
        auto_ratio=$(cat "$auto_result" | cut -d',' -f2)
    fi

    best_model=${file_best_model[$basename]}
    best_ratio=${file_best_ratio[$basename]}

    # Mark best with star
    if [ "$best_model" = "order0" ]; then
        order0_ratio="**${order0_ratio}%** ⭐"
    else
        order0_ratio="${order0_ratio}%"
    fi

    if [ "$best_model" = "order1" ]; then
        order1_ratio="**${order1_ratio}%** ⭐"
    else
        order1_ratio="${order1_ratio}%"
    fi

    if [ "$best_model" = "auto" ]; then
        auto_ratio="**${auto_ratio}%** ⭐"
    else
        auto_ratio="${auto_ratio}%"
    fi

    echo "| $basename | $size_display | $order0_ratio | $order1_ratio | $auto_ratio | $best_model | ${best_ratio}% |" >> "$RESULTS_MD"
done

echo >> "$RESULTS_MD"
echo "⭐ = Best model for that file" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Model wins summary
echo "### Files Won by Each Model" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
for model in order0 order1 auto; do
    wins=${model_wins[$model]:-0}
    echo "- **$model**: $wins file(s)" >> "$RESULTS_MD"
done

echo >> "$RESULTS_MD"
echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Detailed file-by-file analysis
echo "## Detailed File-by-File Analysis" >> "$RESULTS_MD"
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
    echo "| Model | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved |" >> "$RESULTS_MD"
    echo "|-------|-----------------|-------|-------------|---------------|-----------------|-------------|" >> "$RESULTS_MD"

    for model in order0 order1 auto; do
        result_file="$TEMP_DIR/${basename}.${model}.result"
        if [ -f "$result_file" ]; then
            IFS=',' read -r comp_size ratio bits comp_time decomp_time lossless <<< $(cat "$result_file")

            comp_size_display=$(numfmt --to=iec-i --suffix=B $comp_size)
            saved=$(( original_size - comp_size ))
            saved_display=$(numfmt --to=iec-i --suffix=B $saved)

            echo "| $model | $comp_size_display ($comp_size) | ${ratio}% | ${bits} | ${comp_time} ms | ${decomp_time} ms | $saved_display |" >> "$RESULTS_MD"
        fi
    done

    # Analysis for this file
    order0_result="$TEMP_DIR/${basename}.order0.result"
    order1_result="$TEMP_DIR/${basename}.order1.result"
    auto_result="$TEMP_DIR/${basename}.auto.result"

    if [ -f "$order0_result" ] && [ -f "$order1_result" ]; then
        order0_ratio=$(cat "$order0_result" | cut -d',' -f2)
        order0_time=$(cat "$order0_result" | cut -d',' -f4)
        order1_ratio=$(cat "$order1_result" | cut -d',' -f2)
        order1_time=$(cat "$order1_result" | cut -d',' -f4)

        improvement=$(awk "BEGIN {printf \"%.2f\", $order0_ratio - $order1_ratio}")
        improvement_pct=$(awk "BEGIN {printf \"%.1f\", 100.0 * ($order0_ratio - $order1_ratio) / $order0_ratio}")
        time_diff=$(awk "BEGIN {printf \"%.1fx\", $order1_time / $order0_time}")

        echo >> "$RESULTS_MD"
        echo "**Analysis**:" >> "$RESULTS_MD"
        if (( $(echo "$improvement > 0" | bc -l) )); then
            echo "- Order-1 is ${improvement}pp better (${improvement_pct}% improvement), but ${time_diff} slower" >> "$RESULTS_MD"
        elif (( $(echo "$improvement < 0" | bc -l) )); then
            echo "- Order-0 is ${improvement#-}pp better, and faster" >> "$RESULTS_MD"
        else
            echo "- Both models achieve similar compression" >> "$RESULTS_MD"
        fi

        # Check if auto made the right choice
        best_model=${file_best_model[$basename]}
        auto_result_ratio=$(cat "$auto_result" | cut -d',' -f2 2>/dev/null || echo "N/A")
        if [ "$auto_result_ratio" != "N/A" ]; then
            best_result_ratio=$(cat "$TEMP_DIR/${basename}.${best_model}.result" | cut -d',' -f2)
            if (( $(echo "$auto_result_ratio == $best_result_ratio" | bc -l) )); then
                echo "- Auto-selection chose **$best_model** ✓ (optimal choice)" >> "$RESULTS_MD"
            else
                echo "- Auto-selection did not choose the optimal model" >> "$RESULTS_MD"
            fi
        fi
    fi

    echo >> "$RESULTS_MD"
done

echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Model Performance Characteristics
echo "## Model Performance Characteristics" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

echo "### Average Compression Ratio" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| Model | Avg Ratio | Total Compressed | Relative Performance |" >> "$RESULTS_MD"
echo "|-------|-----------|------------------|----------------------|" >> "$RESULTS_MD"

best_avg_ratio=999.99
for model in order0 order1 auto; do
    total_orig=${model_total_original[$model]:-0}
    total_comp=${model_total_compressed[$model]:-0}
    if [ $total_orig -gt 0 ]; then
        avg_ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $total_comp / $total_orig}")
        if (( $(echo "$avg_ratio < $best_avg_ratio" | bc -l) )); then
            best_avg_ratio=$avg_ratio
        fi
    fi
done

for model in order0 order1 auto; do
    total_orig=${model_total_original[$model]:-0}
    total_comp=${model_total_compressed[$model]:-0}
    if [ $total_orig -gt 0 ]; then
        avg_ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $total_comp / $total_orig}")
        total_comp_display=$(numfmt --to=iec-i --suffix=B $total_comp)

        if (( $(echo "$avg_ratio == $best_avg_ratio" | bc -l) )); then
            performance="🥇 Best"
        else
            diff=$(awk "BEGIN {printf \"%.2f\", $avg_ratio - $best_avg_ratio}")
            performance="+${diff}pp vs best"
        fi

        echo "| $model | ${avg_ratio}% | $total_comp_display | $performance |" >> "$RESULTS_MD"
    fi
done

echo >> "$RESULTS_MD"

echo "### Average Speed" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| Model | Avg Compress Time | Avg Decompress Time | Speed Rating |" >> "$RESULTS_MD"
echo "|-------|-------------------|---------------------|--------------|" >> "$RESULTS_MD"

fastest_comp_time=999999999
for model in order0 order1 auto; do
    file_count=${model_file_count[$model]:-1}
    avg_comp_time=$(( ${model_total_comp_time[$model]:-0} / $file_count ))
    if [ $avg_comp_time -lt $fastest_comp_time ]; then
        fastest_comp_time=$avg_comp_time
    fi
done

for model in order0 order1 auto; do
    file_count=${model_file_count[$model]:-1}
    avg_comp_time=$(( ${model_total_comp_time[$model]:-0} / $file_count ))
    avg_decomp_time=$(( ${model_total_decomp_time[$model]:-0} / $file_count ))

    if [ $avg_comp_time -eq $fastest_comp_time ]; then
        speed_rating="⚡ Fastest"
    else
        slowdown=$(awk "BEGIN {printf \"%.1fx\", $avg_comp_time / $fastest_comp_time}")
        speed_rating="${slowdown} slower"
    fi

    echo "| $model | ${avg_comp_time} ms | ${avg_decomp_time} ms | $speed_rating |" >> "$RESULTS_MD"
done

echo >> "$RESULTS_MD"
echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Speed vs Compression Trade-offs
echo "## Speed vs Compression Trade-offs" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "This section analyzes the trade-off between compression ratio and speed:" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

order0_avg_ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * ${model_total_compressed[order0]:-0} / ${model_total_original[order0]:-1}}")
order1_avg_ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * ${model_total_compressed[order1]:-0} / ${model_total_original[order1]:-1}}")
order0_avg_time=$(( ${model_total_comp_time[order0]:-0} / ${model_file_count[order0]:-1} ))
order1_avg_time=$(( ${model_total_comp_time[order1]:-0} / ${model_file_count[order1]:-1} ))

compression_gain=$(awk "BEGIN {printf \"%.2f\", $order0_avg_ratio - $order1_avg_ratio}")
compression_gain_pct=$(awk "BEGIN {printf \"%.1f\", 100.0 * ($order0_avg_ratio - $order1_avg_ratio) / $order0_avg_ratio}")
time_cost=$(awk "BEGIN {printf \"%.1fx\", $order1_avg_time / $order0_avg_time}")

echo "**Order-1 vs Order-0**:" >> "$RESULTS_MD"
echo "- Compression gain: ${compression_gain}pp (${compression_gain_pct}% better)" >> "$RESULTS_MD"
echo "- Time cost: ${time_cost} slower" >> "$RESULTS_MD"
echo "- **Trade-off**: Spend ${time_cost} more time to save ${compression_gain_pct}% more space" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

echo "**When to use each model**:" >> "$RESULTS_MD"
echo "- **Order-0**: Fast compression needed, file already low-entropy, or file < 100KB" >> "$RESULTS_MD"
echo "- **Order-1**: Maximum compression needed, low-entropy data with patterns, large files" >> "$RESULTS_MD"
echo "- **Auto**: Let the compressor decide based on file characteristics (recommended)" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Auto-Selection Effectiveness
echo "## Auto-Selection Effectiveness" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "This section evaluates how well the auto-selection algorithm performs:" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

auto_optimal_count=0
total_files=0
for file in "$DATA_DIR"/*; do
    if [ ! -f "$file" ]; then
        continue
    fi
    basename=$(basename "$file")
    total_files=$((total_files + 1))

    best_model=${file_best_model[$basename]}
    auto_result="$TEMP_DIR/${basename}.auto.result"
    best_result="$TEMP_DIR/${basename}.${best_model}.result"

    if [ -f "$auto_result" ] && [ -f "$best_result" ]; then
        auto_ratio=$(cat "$auto_result" | cut -d',' -f2)
        best_ratio=$(cat "$best_result" | cut -d',' -f2)

        # Consider optimal if within 0.01% (rounding tolerance)
        if (( $(echo "$auto_ratio <= $best_ratio + 0.01" | bc -l) )); then
            auto_optimal_count=$((auto_optimal_count + 1))
        fi
    fi
done

auto_success_rate=$(awk "BEGIN {printf \"%.1f\", 100.0 * $auto_optimal_count / $total_files}")

echo "| Metric | Value |" >> "$RESULTS_MD"
echo "|--------|-------|" >> "$RESULTS_MD"
echo "| Files tested | $total_files |" >> "$RESULTS_MD"
echo "| Optimal choices | $auto_optimal_count |" >> "$RESULTS_MD"
echo "| Success rate | ${auto_success_rate}% |" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

if (( $(echo "$auto_success_rate >= 90" | bc -l) )); then
    echo "**Conclusion**: Auto-selection is highly effective (${auto_success_rate}% success rate) ✓" >> "$RESULTS_MD"
elif (( $(echo "$auto_success_rate >= 70" | bc -l) )); then
    echo "**Conclusion**: Auto-selection works well (${auto_success_rate}% success rate)" >> "$RESULTS_MD"
else
    echo "**Conclusion**: Auto-selection needs improvement (${auto_success_rate}% success rate)" >> "$RESULTS_MD"
fi

echo >> "$RESULTS_MD"
echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

# Recommendations
echo "## Recommendations" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "Based on this benchmark analysis:" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "### For General Use" >> "$RESULTS_MD"
echo "- **Use `--model auto`** (default): Achieves near-optimal compression with intelligent selection" >> "$RESULTS_MD"
echo "- Success rate: ${auto_success_rate}%, making it reliable for mixed workloads" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

echo "### For Specific Scenarios" >> "$RESULTS_MD"
echo "- **Speed-critical tasks**: Use `--model order0` (${order0_avg_time}ms avg)" >> "$RESULTS_MD"
echo "- **Maximum compression**: Use `--model order1` (${compression_gain}pp better on average)" >> "$RESULTS_MD"
echo "- **Small files (<100KB)**: Use `--model order0` (adaptive overhead not worth it)" >> "$RESULTS_MD"
echo "- **Low-entropy text/structured data**: Use `--model order1` (significant gains)" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

echo "### Performance Summary" >> "$RESULTS_MD"
echo "| Model | Best For | Avg Ratio | Avg Speed | Files Won |" >> "$RESULTS_MD"
echo "|-------|----------|-----------|-----------|-----------|" >> "$RESULTS_MD"
echo "| Order-0 | Speed, small files | ${order0_avg_ratio}% | ${order0_avg_time}ms | ${model_wins[order0]:-0} |" >> "$RESULTS_MD"
echo "| Order-1 | Compression, patterns | ${order1_avg_ratio}% | ${order1_avg_time}ms | ${model_wins[order1]:-0} |" >> "$RESULTS_MD"
echo "| Auto | General use | $(awk "BEGIN {printf \"%.2f\", 100.0 * ${model_total_compressed[auto]:-0} / ${model_total_original[auto]:-1}}")% | $(( ${model_total_comp_time[auto]:-0} / ${model_file_count[auto]:-1} ))ms | ${model_wins[auto]:-0} |" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "*Report generated on $(date)*" >> "$RESULTS_MD"

# Terminal summary
echo -e "\n${BOLD}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}║                    MODEL COMPARISON SUMMARY                    ║${NC}"
echo -e "${BOLD}╚════════════════════════════════════════════════════════════════╝${NC}"
echo
printf "${BOLD}%-15s %15s %15s %15s %10s${NC}\n" "Model" "Total Compressed" "Avg Ratio" "Avg Time" "Wins"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

for model in order0 order1 auto; do
    total_comp=${model_total_compressed[$model]:-0}
    total_orig=${model_total_original[$model]:-1}
    file_count=${model_file_count[$model]:-1}
    avg_ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $total_comp / $total_orig}")
    avg_time=$(( ${model_total_comp_time[$model]:-0} / $file_count ))
    wins=${model_wins[$model]:-0}

    # Determine color
    if [ "$model" = "auto" ]; then
        color=$CYAN
    elif [ "$model" = "order0" ]; then
        color=$YELLOW
    else
        color=$GREEN
    fi

    printf "${color}%-15s %15s %14s%% %12sms %10s${NC}\n" \
        "$model" \
        "$(numfmt --to=iec-i --suffix=B $total_comp)" \
        "$avg_ratio" \
        "$avg_time" \
        "$wins"
done

echo
echo -e "${BOLD}Auto-selection success rate: ${GREEN}${auto_success_rate}%${NC}"
echo -e "${BOLD}Compression gain (Order-1 vs Order-0): ${GREEN}${compression_gain}pp${NC} ${BOLD}(${compression_gain_pct}% better)${NC}"
echo -e "${BOLD}Time cost (Order-1 vs Order-0): ${YELLOW}${time_cost}${NC}"
echo

echo -e "${BOLD}╔════════════════════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}║                    RESULTS SAVED                               ║${NC}"
echo -e "${BOLD}╚════════════════════════════════════════════════════════════════╝${NC}"
echo -e "  📊 CSV:      ${GREEN}$RESULTS_CSV${NC}"
echo -e "  📝 Markdown: ${GREEN}$RESULTS_MD${NC}"
echo

# Clean up temp directory
rm -rf "$TEMP_DIR"

echo -e "${GREEN}Model comparison benchmark complete!${NC}"
