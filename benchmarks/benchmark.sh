#!/bin/bash

# G07 All Versions Benchmark
# Compares: v5, v6, v7, v8, v9 on all test files
# Outputs: CSV + Markdown table

set -e

# Configuration
DATA_DIR="data"
TEMP_DIR="benchmarks/tmp"
RESULTS_CSV="benchmarks/results.csv"
RESULTS_MD="benchmarks/results.md"
COMPRESS_TIMEOUT=600  # 10 minutes for slow versions

# Colors
BOLD='\033[1m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

mkdir -p "$TEMP_DIR"

# Auto-detect G07 versions
echo "Detecting G07 versions..."
G07_VERSIONS=()
for bin in bin/g07-v*-c; do
    if [ -f "$bin" ]; then
        version=$(basename "$bin" | sed 's/g07-v\(.*\)-c/\1/')
        G07_VERSIONS+=("$version")
        echo "  Found: g07-v${version}"
    fi
done

if [ ${#G07_VERSIONS[@]} -eq 0 ]; then
    echo "ERROR: No G07 binaries found. Run 'make' first."
    exit 1
fi
echo

# Auto-detect data files
DATA_FILES=()
for file in "$DATA_DIR"/*; do
    if [ -f "$file" ] && [[ ! "$file" =~ \.(compressed|decompressed|v[0-9].*|gz|bz2|xz|zst)$ ]]; then
        DATA_FILES+=("$file")
    fi
done

if [ ${#DATA_FILES[@]} -eq 0 ]; then
    echo "ERROR: No data files found in $DATA_DIR/"
    exit 1
fi

echo "Data files to test: ${DATA_FILES[@]}"
echo

# Initialize CSV
echo "Version,File,OrigSize,CompSize,Ratio,BitsPerSym,CompTime,DecompTime,CompSpeed,DecompSpeed,Verified" > "$RESULTS_CSV"

# Initialize Markdown
cat > "$RESULTS_MD" << 'EOF'
# G07 Versions Comparison

**Generated:** $(date +"%Y-%m-%d %H:%M")

## Summary Table

| Version | Avg Ratio | Avg Comp Speed | Avg Decomp Speed | Description |
|---------|-----------|----------------|------------------|-------------|
EOF

echo -e "${BOLD}╔══════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}║     G07 ALL VERSIONS BENCHMARK                   ║${NC}"
echo -e "${BOLD}╚══════════════════════════════════════════════════╝${NC}"
echo

# Arrays for statistics
declare -A version_total_original
declare -A version_total_compressed
declare -A version_total_comp_time
declare -A version_total_decomp_time
declare -A version_file_count
declare -A version_failures

# Initialize counters
for v in "${G07_VERSIONS[@]}"; do
    version_total_original[$v]=0
    version_total_compressed[$v]=0
    version_total_comp_time[$v]=0
    version_total_decomp_time[$v]=0
    version_file_count[$v]=0
    version_failures[$v]=0
done

# Benchmark each version on each file
for file in "${DATA_FILES[@]}"; do
    basename=$(basename "$file")
    original_size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null)

    echo -e "${BOLD}Testing file: $basename${NC} ($(numfmt --to=iec-i --suffix=B $original_size))"
    echo "────────────────────────────────────────────────────"

    for version in "${G07_VERSIONS[@]}"; do
        compressor="./bin/g07-v${version}-c"
        decompressor="./bin/g07-v${version}-d"
        compressed="$TEMP_DIR/${basename}.v${version}"
        decompressed="$TEMP_DIR/${basename}.v${version}.dec"

        printf "  v%-2s: " "$version"

        # Compress
        comp_start=$(date +%s%N)
        if timeout $COMPRESS_TIMEOUT $compressor "$file" "$compressed" > /dev/null 2>&1; then
            comp_end=$(date +%s%N)
            comp_time=$(( (comp_end - comp_start) / 1000000 ))  # milliseconds

            if [ ! -f "$compressed" ]; then
                echo -e "${RED}FAIL${NC} (no output file)"
                version_failures[$version]=$((${version_failures[$version]} + 1))
                echo "$version,$basename,$original_size,0,0,0,$comp_time,0,0,0,FAIL" >> "$RESULTS_CSV"
                continue
            fi

            compressed_size=$(stat -f%z "$compressed" 2>/dev/null || stat -c%s "$compressed" 2>/dev/null)

            # Decompress
            decomp_start=$(date +%s%N)
            if timeout $COMPRESS_TIMEOUT $decompressor "$compressed" "$decompressed" > /dev/null 2>&1; then
                decomp_end=$(date +%s%N)
                decomp_time=$(( (decomp_end - decomp_start) / 1000000 ))

                # Verify
                if cmp -s "$file" "$decompressed"; then
                    verified="PASS"
                    verified_color="${GREEN}"
                else
                    verified="FAIL"
                    verified_color="${RED}"
                    version_failures[$version]=$((${version_failures[$version]} + 1))
                fi

                # Calculate metrics
                ratio=$(awk "BEGIN {printf \"%.2f\", ($compressed_size * 100.0 / $original_size)}")
                bits_per_sym=$(awk "BEGIN {printf \"%.2f\", ($compressed_size * 8.0 / $original_size)}")
                comp_speed=$(awk "BEGIN {printf \"%.1f\", ($original_size / 1048576.0) / ($comp_time / 1000.0)}")
                decomp_speed=$(awk "BEGIN {printf \"%.1f\", ($original_size / 1048576.0) / ($decomp_time / 1000.0)}")

                # Display
                echo -e "${ratio}% | ${comp_time}ms comp | ${decomp_time}ms decomp | ${verified_color}${verified}${NC}"

                # Save to CSV
                echo "$version,$basename,$original_size,$compressed_size,$ratio,$bits_per_sym,$comp_time,$decomp_time,$comp_speed,$decomp_speed,$verified" >> "$RESULTS_CSV"

                # Update statistics (only for successful tests)
                if [ "$verified" == "PASS" ]; then
                    version_total_original[$version]=$((${version_total_original[$version]} + original_size))
                    version_total_compressed[$version]=$((${version_total_compressed[$version]} + compressed_size))
                    version_total_comp_time[$version]=$((${version_total_comp_time[$version]} + comp_time))
                    version_total_decomp_time[$version]=$((${version_total_decomp_time[$version]} + decomp_time))
                    version_file_count[$version]=$((${version_file_count[$version]} + 1))
                fi

                # Cleanup
                rm -f "$decompressed"
            else
                echo -e "${RED}FAIL${NC} (decompression timeout/error)"
                version_failures[$version]=$((${version_failures[$version]} + 1))
                echo "$version,$basename,$original_size,$compressed_size,$ratio,$bits_per_sym,$comp_time,0,0,0,DECOMP_FAIL" >> "$RESULTS_CSV"
            fi

            rm -f "$compressed"
        else
            echo -e "${RED}FAIL${NC} (compression timeout/error)"
            version_failures[$version]=$((${version_failures[$version]} + 1))
            echo "$version,$basename,$original_size,0,0,0,0,0,0,0,COMP_FAIL" >> "$RESULTS_CSV"
        fi
    done
    echo
done

# Generate summary table
echo -e "${BOLD}╔══════════════════════════════════════════════════╗${NC}"
echo -e "${BOLD}║              SUMMARY STATISTICS                  ║${NC}"
echo -e "${BOLD}╚══════════════════════════════════════════════════╝${NC}"
echo
printf "%-8s | %-10s | %-12s | %-14s | %-10s\n" "Version" "Avg Ratio" "Comp Speed" "Decomp Speed" "Failures"
echo "─────────────────────────────────────────────────────────────────────"

# Version descriptions
declare -A version_desc
version_desc[5]="Balanced (Best ratio)"
version_desc[6]="Multi-order PPM (Experimental)"
version_desc[7]="Speed-optimized rANS"
version_desc[8]="Ultra-fast LZ77"
version_desc[9]="Bit-level PPM (Max compression)"

for version in "${G07_VERSIONS[@]}"; do
    if [ ${version_file_count[$version]} -gt 0 ]; then
        avg_ratio=$(awk "BEGIN {printf \"%.2f\", (${version_total_compressed[$version]} * 100.0 / ${version_total_original[$version]})}")
        avg_comp_speed=$(awk "BEGIN {printf \"%.1f\", (${version_total_original[$version]} / 1048576.0) / (${version_total_comp_time[$version]} / 1000.0)}")
        avg_decomp_speed=$(awk "BEGIN {printf \"%.1f\", (${version_total_original[$version]} / 1048576.0) / (${version_total_decomp_time[$version]} / 1000.0)}")

        printf "v%-7s | %-10s | %-12s | %-14s | %-10s\n" \
            "$version" \
            "${avg_ratio}%" \
            "${avg_comp_speed} MB/s" \
            "${avg_decomp_speed} MB/s" \
            "${version_failures[$version]}"

        # Append to markdown summary
        echo "| v$version | $avg_ratio% | $avg_comp_speed MB/s | $avg_decomp_speed MB/s | ${version_desc[$version]:-Unknown} |" >> "$RESULTS_MD"
    else
        printf "v%-7s | ${RED}ALL FAILED${NC}\n" "$version"
        echo "| v$version | - | - | - | ALL TESTS FAILED |" >> "$RESULTS_MD"
    fi
done

echo
echo -e "${GREEN}Results saved to:${NC}"
echo "  - $RESULTS_CSV (machine-readable)"
echo "  - $RESULTS_MD (human-readable)"
echo

# Append detailed results to Markdown
cat >> "$RESULTS_MD" << 'EOF'

## Detailed Results by Version and File

```csv
EOF
cat "$RESULTS_CSV" >> "$RESULTS_MD"
cat >> "$RESULTS_MD" << 'EOF'
```

---

**Legend:**
- **Ratio**: Compressed size / Original size × 100%
- **Comp Speed**: Compression throughput in MB/s
- **Decomp Speed**: Decompression throughput in MB/s
- **Verified**: PASS = lossless, FAIL = data corruption
EOF

echo -e "${BOLD}Benchmark complete!${NC}"
