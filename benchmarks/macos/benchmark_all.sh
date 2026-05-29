#!/bin/bash

# Comprehensive benchmark script for all data files on macOS-compatible shells.
# Compares: our compressor, gzip, bzip2, xz (lzma), zstd
# Outputs: Terminal table, CSV file, Comprehensive Markdown report

set -e

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
. "$SCRIPT_DIR/common.sh"

DATA_DIR="data"
TEMP_DIR="$SCRIPT_DIR/tmp_$$"
RESULTS_CSV="$SCRIPT_DIR/results.csv"
RESULTS_MD="$SCRIPT_DIR/results.md"
WINNERS_FILE="$TEMP_DIR/winners.list"
TOOLS="g07 gzip bzip2 xz zstd"

COMPRESS_TIMEOUT=60
DECOMPRESS_TIMEOUT=30
G07_COMPRESS_CMD="${G07_COMPRESS_CMD:-./bin/g07-v9-c}"
G07_DECOMPRESS_CMD="${G07_DECOMPRESS_CMD:-./bin/g07-v9-d}"
G07_COMPRESS_EXTRA_ARGS="${G07_COMPRESS_EXTRA_ARGS-}"

BOLD='\033[1m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

mkdir -p "$TEMP_DIR"
: > "$WINNERS_FILE"

echo "File,Original Size (bytes),Tool,Compressed Size (bytes),Compression Ratio (%),Bits/Symbol,Compress Time (ms),Decompress Time (ms),Lossless" > "$RESULTS_CSV"

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

echo -e "${BOLD}===============================================================${NC}"
echo -e "${BOLD}         COMPREHENSIVE COMPRESSION BENCHMARK                   ${NC}"
echo -e "${BOLD}===============================================================${NC}"
echo

benchmark_file_tool() {
    local file="$1"
    local base_name="$2"
    local tool_name="$3"
    local compress_cmd="$4"
    local decompress_cmd="$5"
    local compressed_file="$6"
    local original_size="$7"
    local result_file="$TEMP_DIR/${base_name}.${tool_name}.result"
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

    start=$(now_ms)
    if run_with_timeout "$COMPRESS_TIMEOUT" "$compress_cmd"; then
        exit_code=0
    else
        exit_code=$?
    fi
    end=$(now_ms)
    compress_time=$((end - start))

    if [ $exit_code -eq 124 ]; then
        echo "TIMEOUT,TIMEOUT,TIMEOUT,TIMEOUT,TIMEOUT,TIMEOUT" > "$result_file"
        return 1
    elif [ $exit_code -ne 0 ]; then
        echo "ERROR,ERROR,ERROR,ERROR,ERROR,ERROR" > "$result_file"
        return 1
    fi

    if [ ! -f "$compressed_file" ]; then
        echo "ERROR,ERROR,ERROR,ERROR,ERROR,ERROR" > "$result_file"
        return 1
    fi

    compressed_size=$(file_size "$compressed_file")
    ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $compressed_size / $original_size}")
    bits_per_symbol=$(awk "BEGIN {printf \"%.4f\", 8.0 * $compressed_size / $original_size}")

    decompressed_file="$TEMP_DIR/${base_name}.${tool_name}.decompressed"
    start=$(now_ms)
    if run_with_timeout "$DECOMPRESS_TIMEOUT" "$decompress_cmd"; then
        exit_code=0
    else
        exit_code=$?
    fi
    end=$(now_ms)
    decompress_time=$((end - start))

    if [ $exit_code -eq 124 ]; then
        echo "$compressed_size,$ratio,$bits_per_symbol,$compress_time,TIMEOUT,TIMEOUT" > "$result_file"
        rm -f "$compressed_file"
        return 1
    elif [ $exit_code -ne 0 ]; then
        echo "$compressed_size,$ratio,$bits_per_symbol,$compress_time,ERROR,NO" > "$result_file"
        rm -f "$compressed_file"
        return 1
    fi

    lossless="NO"
    if cmp -s "$file" "$decompressed_file"; then
        lossless="YES"
    fi

    echo "$compressed_size,$ratio,$bits_per_symbol,$compress_time,$decompress_time,$lossless" > "$result_file"
    rm -f "$compressed_file" "$decompressed_file"
    return 0
}

tool_display_name() {
    case "$1" in
        g07) echo "G07" ;;
        *) echo "$1" ;;
    esac
}

count_tool_wins() {
    awk -v tool="$1" '$0 == tool { count++ } END { print count + 0 }' "$WINNERS_FILE"
}

tool_average_time() {
    local tool="$1"
    local field_index="$2"
    local total=0
    local count=0
    local file
    local base_name
    local result_file
    local comp_size
    local ratio
    local bits
    local comp_time
    local decomp_time
    local lossless
    local value

    for file in "$DATA_DIR"/*; do
        if [ ! -f "$file" ]; then
            continue
        fi

        base_name=$(basename "$file")
        result_file="$TEMP_DIR/${base_name}.${tool}.result"
        if [ ! -f "$result_file" ]; then
            continue
        fi

        IFS=',' read -r comp_size ratio bits comp_time decomp_time lossless < "$result_file"
        if [ "$field_index" = "4" ]; then
            value="$comp_time"
        else
            value="$decomp_time"
        fi

        if is_numeric "$value"; then
            total=$((total + value))
            count=$((count + 1))
        fi
    done

    if [ $count -gt 0 ]; then
        echo $((total / count))
    fi
}

build_tool_stats_file() {
    local stats_file="$TEMP_DIR/tool_stats.tsv"
    local file
    local tool
    local base_name
    local result_file
    local original_size
    local total_orig
    local total_comp
    local count
    local comp_size
    local ratio
    local bits
    local comp_time
    local decomp_time
    local lossless
    local avg_ratio
    local avg_bits
    local wins

    : > "$stats_file"

    for tool in $TOOLS; do
        total_orig=0
        total_comp=0
        count=0

        for file in "$DATA_DIR"/*; do
            if [ ! -f "$file" ]; then
                continue
            fi

            base_name=$(basename "$file")
            result_file="$TEMP_DIR/${base_name}.${tool}.result"
            if [ ! -f "$result_file" ]; then
                continue
            fi

            IFS=',' read -r comp_size ratio bits comp_time decomp_time lossless < "$result_file"
            if is_numeric "$comp_size" && is_numeric "$comp_time" && is_numeric "$decomp_time"; then
                original_size=$(file_size "$file")
                total_orig=$((total_orig + original_size))
                total_comp=$((total_comp + comp_size))
                count=$((count + 1))
            fi
        done

        if [ $count -gt 0 ]; then
            avg_ratio=$(awk "BEGIN {printf \"%.2f\", 100.0 * $total_comp / $total_orig}")
            avg_bits=$(awk "BEGIN {printf \"%.4f\", 8.0 * $total_comp / $total_orig}")
            wins=$(count_tool_wins "$tool")
            printf "%s\t%s\t%s\t%s\t%s\t%s\t%s\n" \
                "$tool" "$total_orig" "$total_comp" "$count" "$avg_ratio" "$avg_bits" "$wins" >> "$stats_file"
        fi
    done

    echo "$stats_file"
}

for file in "$DATA_DIR"/*; do
    if [ ! -f "$file" ]; then
        continue
    fi

    base_name=$(basename "$file")
    original_size=$(file_size "$file")

    echo -e "${BLUE}-------------------------------------------------------------${NC}"
    echo -e "${BOLD}File: $base_name${NC} ($(human_size "$original_size"))"
    echo -e "${BLUE}-------------------------------------------------------------${NC}"

    rm -f "$TEMP_DIR/${base_name}."*.result

    echo -n "  Testing G07... "
    if benchmark_file_tool "$file" "$base_name" "g07" \
        "$G07_COMPRESS_CMD '$file' '$TEMP_DIR/${base_name}.g07'${G07_COMPRESS_EXTRA_ARGS:+ $G07_COMPRESS_EXTRA_ARGS}" \
        "$G07_DECOMPRESS_CMD '$TEMP_DIR/${base_name}.g07' '$TEMP_DIR/${base_name}.g07.decompressed'" \
        "$TEMP_DIR/${base_name}.g07" \
        "$original_size"; then
        echo -e "${GREEN}OK${NC}"
    else
        echo -e "${YELLOW}TIMEOUT/ERROR${NC}"
    fi

    echo -n "  Testing gzip... "
    if benchmark_file_tool "$file" "$base_name" "gzip" \
        "gzip -c '$file' > '$TEMP_DIR/${base_name}.gz'" \
        "gzip -dc '$TEMP_DIR/${base_name}.gz' > '$TEMP_DIR/${base_name}.gzip.decompressed'" \
        "$TEMP_DIR/${base_name}.gz" \
        "$original_size"; then
        echo -e "${GREEN}OK${NC}"
    else
        echo -e "${YELLOW}TIMEOUT/ERROR${NC}"
    fi

    echo -n "  Testing bzip2... "
    if benchmark_file_tool "$file" "$base_name" "bzip2" \
        "bzip2 -c '$file' > '$TEMP_DIR/${base_name}.bz2'" \
        "bzip2 -dc '$TEMP_DIR/${base_name}.bz2' > '$TEMP_DIR/${base_name}.bzip2.decompressed'" \
        "$TEMP_DIR/${base_name}.bz2" \
        "$original_size"; then
        echo -e "${GREEN}OK${NC}"
    else
        echo -e "${YELLOW}TIMEOUT/ERROR${NC}"
    fi

    echo -n "  Testing xz... "
    if benchmark_file_tool "$file" "$base_name" "xz" \
        "xz -c '$file' > '$TEMP_DIR/${base_name}.xz'" \
        "xz -dc '$TEMP_DIR/${base_name}.xz' > '$TEMP_DIR/${base_name}.xz.decompressed'" \
        "$TEMP_DIR/${base_name}.xz" \
        "$original_size"; then
        echo -e "${GREEN}OK${NC}"
    else
        echo -e "${YELLOW}TIMEOUT/ERROR${NC}"
    fi

    if command -v zstd >/dev/null 2>&1; then
        echo -n "  Testing zstd... "
        if benchmark_file_tool "$file" "$base_name" "zstd" \
            "zstd -q '$file' -o '$TEMP_DIR/${base_name}.zst'" \
            "zstd -qd '$TEMP_DIR/${base_name}.zst' -o '$TEMP_DIR/${base_name}.zstd.decompressed'" \
            "$TEMP_DIR/${base_name}.zst" \
            "$original_size"; then
            echo -e "${GREEN}OK${NC}"
        else
            echo -e "${YELLOW}TIMEOUT/ERROR${NC}"
        fi
    fi

    echo

    for tool in $TOOLS; do
        result_file="$TEMP_DIR/${base_name}.${tool}.result"
        if [ -f "$result_file" ]; then
            result=$(<"$result_file")
            echo "$base_name,$original_size,$tool,$result" >> "$RESULTS_CSV"
        fi
    done
done

echo "## Quick Summary" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "### Compression Ratio Comparison" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| File | Size | G07 | gzip | bzip2 | xz | zstd | Winner |" >> "$RESULTS_MD"
echo "|------|------|-----|------|-------|----|------|--------|" >> "$RESULTS_MD"

for file in "$DATA_DIR"/*; do
    if [ ! -f "$file" ]; then
        continue
    fi

    base_name=$(basename "$file")
    original_size=$(file_size "$file")
    size_display=$(human_size "$original_size")
    best_ratio=""
    best_tool=""

    for tool in $TOOLS; do
        result_file="$TEMP_DIR/${base_name}.${tool}.result"
        if [ ! -f "$result_file" ]; then
            continue
        fi

        IFS=',' read -r comp_size ratio bits comp_time decomp_time lossless < "$result_file"
        if is_numeric "$ratio"; then
            if [ -z "$best_ratio" ] || float_lt "$ratio" "$best_ratio"; then
                best_ratio="$ratio"
                best_tool="$tool"
            fi
        fi
    done

    if [ -n "$best_tool" ]; then
        echo "$best_tool" >> "$WINNERS_FILE"
    fi

    echo -n "| $base_name | $size_display |" >> "$RESULTS_MD"
    for tool in $TOOLS; do
        result_file="$TEMP_DIR/${base_name}.${tool}.result"
        if [ -f "$result_file" ]; then
            IFS=',' read -r comp_size ratio bits comp_time decomp_time lossless < "$result_file"
            if is_numeric "$ratio"; then
                if [ "$tool" = "$best_tool" ]; then
                    echo -n " **${ratio}%** * |" >> "$RESULTS_MD"
                else
                    echo -n " ${ratio}% |" >> "$RESULTS_MD"
                fi
            else
                echo -n " $ratio |" >> "$RESULTS_MD"
            fi
        else
            echo -n " N/A |" >> "$RESULTS_MD"
        fi
    done
    echo " $best_tool |" >> "$RESULTS_MD"
done

echo >> "$RESULTS_MD"
echo "* = Best compressor for that file" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

echo "## Detailed Results by File" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"

for file in "$DATA_DIR"/*; do
    if [ ! -f "$file" ]; then
        continue
    fi

    base_name=$(basename "$file")
    original_size=$(file_size "$file")
    size_display=$(human_size "$original_size")

    echo "### File: $base_name" >> "$RESULTS_MD"
    echo >> "$RESULTS_MD"
    echo "**Original Size**: $original_size bytes ($size_display)" >> "$RESULTS_MD"
    echo >> "$RESULTS_MD"
    echo "| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |" >> "$RESULTS_MD"
    echo "|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|" >> "$RESULTS_MD"

    for tool in $TOOLS; do
        result_file="$TEMP_DIR/${base_name}.${tool}.result"
        if [ ! -f "$result_file" ]; then
            continue
        fi

        IFS=',' read -r comp_size ratio bits comp_time decomp_time lossless < "$result_file"

        if [ "$comp_size" = "TIMEOUT" ]; then
            echo "| $tool | TIMEOUT (> ${COMPRESS_TIMEOUT}s) | - | - | - | - | - | TIMEOUT |" >> "$RESULTS_MD"
            continue
        elif [ "$comp_size" = "ERROR" ]; then
            echo "| $tool | ERROR | - | - | - | - | - | ERROR |" >> "$RESULTS_MD"
            continue
        fi

        comp_size_display=$(human_size "$comp_size")
        saved=$((original_size - comp_size))
        saved_abs=$saved
        if [ $saved_abs -lt 0 ]; then
            saved_abs=$((0 - saved_abs))
        fi
        saved_display=$(human_size "$saved_abs")

        if [ $saved -ge 0 ]; then
            saved_str="saved $saved_display"
        else
            saved_str="overhead $saved_display"
        fi

        perf_indicator="[red]"
        if float_lt "$ratio" "50"; then
            perf_indicator="[green]"
        elif float_lt "$ratio" "80"; then
            perf_indicator="[yellow]"
        fi

        lossless_mark="YES"
        if [ "$lossless" = "TIMEOUT" ]; then
            lossless_mark="TIMEOUT"
            decomp_time="TIMEOUT (> ${DECOMPRESS_TIMEOUT}s)"
        elif [ "$lossless" != "YES" ]; then
            lossless_mark="NO"
        fi

        if [ "$decomp_time" = "ERROR" ]; then
            decomp_time="ERROR"
        elif is_numeric "$decomp_time"; then
            decomp_time="${decomp_time} ms"
        fi

        echo "| $tool | $comp_size_display ($comp_size) | $perf_indicator ${ratio}% | ${bits} | ${comp_time} ms | ${decomp_time} | $saved_str | $lossless_mark |" >> "$RESULTS_MD"
    done

    echo >> "$RESULTS_MD"
done

echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "## Performance Metrics" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "### Compression Speed (Average)" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| Tool | Avg Compress Time | Ranking |" >> "$RESULTS_MD"
echo "|------|-------------------|---------|" >> "$RESULTS_MD"

COMPRESS_AVG_FILE="$TEMP_DIR/avg_compress.tsv"
: > "$COMPRESS_AVG_FILE"
for tool in $TOOLS; do
    avg=$(tool_average_time "$tool" "4")
    if [ -n "$avg" ]; then
        printf "%s\t%s\n" "$tool" "$avg" >> "$COMPRESS_AVG_FILE"
    fi
done

sort -k2 -n "$COMPRESS_AVG_FILE" > "$COMPRESS_AVG_FILE.sorted"
rank=1
while IFS=$'\t' read -r tool time_ms; do
    [ -n "$tool" ] || continue
    medal=""
    case $rank in
        1) medal="[1st]" ;;
        2) medal="[2nd]" ;;
        3) medal="[3rd]" ;;
    esac
    echo "| $tool | ${time_ms} ms | $medal #$rank |" >> "$RESULTS_MD"
    rank=$((rank + 1))
done < "$COMPRESS_AVG_FILE.sorted"

echo >> "$RESULTS_MD"
echo "### Decompression Speed (Average)" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| Tool | Avg Decompress Time | Ranking |" >> "$RESULTS_MD"
echo "|------|---------------------|---------|" >> "$RESULTS_MD"

DECOMPRESS_AVG_FILE="$TEMP_DIR/avg_decompress.tsv"
: > "$DECOMPRESS_AVG_FILE"
for tool in $TOOLS; do
    avg=$(tool_average_time "$tool" "5")
    if [ -n "$avg" ]; then
        printf "%s\t%s\n" "$tool" "$avg" >> "$DECOMPRESS_AVG_FILE"
    fi
done

sort -k2 -n "$DECOMPRESS_AVG_FILE" > "$DECOMPRESS_AVG_FILE.sorted"
rank=1
while IFS=$'\t' read -r tool time_ms; do
    [ -n "$tool" ] || continue
    medal=""
    case $rank in
        1) medal="[1st]" ;;
        2) medal="[2nd]" ;;
        3) medal="[3rd]" ;;
    esac
    echo "| $tool | ${time_ms} ms | $medal #$rank |" >> "$RESULTS_MD"
    rank=$((rank + 1))
done < "$DECOMPRESS_AVG_FILE.sorted"

echo >> "$RESULTS_MD"
echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "## Rankings" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "### Overall Compression Ratio Ranking" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| Rank | Tool | Average Ratio | Files Won | Total Compressed | Performance |" >> "$RESULTS_MD"
echo "|------|------|---------------|-----------|------------------|-------------|" >> "$RESULTS_MD"

TOOL_STATS_FILE=$(build_tool_stats_file)
sort -k5 -n "$TOOL_STATS_FILE" > "$TOOL_STATS_FILE.sorted"
rank=1
while IFS=$'\t' read -r tool total_orig total_comp count avg_ratio avg_bits wins; do
    [ -n "$tool" ] || continue
    total_comp_display=$(human_size "$total_comp")
    medal=""
    perf=""
    case $rank in
        1) medal="[1st]"; perf="***" ;;
        2) medal="[2nd]"; perf="**" ;;
        3) medal="[3rd]"; perf="*" ;;
    esac
    echo "| $medal $rank | **$tool** | ${avg_ratio}% | $wins | $total_comp_display | $perf |" >> "$RESULTS_MD"
    rank=$((rank + 1))
done < "$TOOL_STATS_FILE.sorted"

echo >> "$RESULTS_MD"
echo "### Files Won by Each Tool" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
for tool in $TOOLS; do
    wins=$(count_tool_wins "$tool")
    echo "- **$tool**: $wins file(s)" >> "$RESULTS_MD"
done

echo >> "$RESULTS_MD"
echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "## Overall Statistics" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "| Tool | Total Original | Total Compressed | Average Ratio | Bits/Symbol | Files Tested |" >> "$RESULTS_MD"
echo "|------|---------------|------------------|---------------|-------------|--------------|" >> "$RESULTS_MD"

while IFS=$'\t' read -r tool total_orig total_comp count avg_ratio avg_bits wins; do
    [ -n "$tool" ] || continue
    echo "| $tool | $(human_size "$total_orig") | $(human_size "$total_comp") | ${avg_ratio}% | ${avg_bits} | $count |" >> "$RESULTS_MD"
done < "$TOOL_STATS_FILE"

echo >> "$RESULTS_MD"
echo "---" >> "$RESULTS_MD"
echo >> "$RESULTS_MD"
echo "*Report generated on $(date)*" >> "$RESULTS_MD"

echo -e "\n${BOLD}===============================================================${NC}"
echo -e "${BOLD}                    SUMMARY STATISTICS                         ${NC}"
echo -e "${BOLD}===============================================================${NC}"
echo
printf "${BOLD}%-15s %15s %15s %15s${NC}\n" "Tool" "Total Original" "Total Compressed" "Avg Ratio"
echo "---------------------------------------------------------------"

while IFS=$'\t' read -r tool total_orig total_comp count avg_ratio avg_bits wins; do
    [ -n "$tool" ] || continue
    color="$NC"
    if float_lt "$avg_ratio" "60"; then
        color="$GREEN"
    elif float_lt "$avg_ratio" "80"; then
        color="$YELLOW"
    fi

    printf "${color}%-15s %15s %15s %14s%%${NC}\n" \
        "$tool" \
        "$(human_size "$total_orig")" \
        "$(human_size "$total_comp")" \
        "$avg_ratio"
done < "$TOOL_STATS_FILE"

echo
echo -e "${BOLD}===============================================================${NC}"
echo -e "${BOLD}                        RESULTS SAVED                          ${NC}"
echo -e "${BOLD}===============================================================${NC}"
echo -e "  CSV:      ${GREEN}$RESULTS_CSV${NC}"
echo -e "  Markdown: ${GREEN}$RESULTS_MD${NC}"
echo

rm -rf "$TEMP_DIR"

echo -e "${GREEN}Benchmark complete!${NC}"
