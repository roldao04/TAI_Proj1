#!/bin/bash

# Benchmark script to compare Order-0, Order-1, and Order-2 models
# Tests compression ratio, compression speed, and decompression speed

COMPRESSOR="../bin/compress"
DECOMPRESSOR="../bin/decompress"
DATA_DIR="../data"
RESULTS_FILE="model_comparison.md"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║         MODEL COMPARISON BENCHMARK (Order-0, 1, 2)             ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Initialize results file
cat > "$RESULTS_FILE" << 'EOF'
# Model Comparison Benchmark Results

**Date**: $(date)
**Range Coder**: Schindler's Byte-Normalized Range Coder

---

## Performance Comparison

EOF

# Arrays to store results
declare -A order0_compress_times
declare -A order0_decompress_times
declare -A order0_ratios
declare -A order1_compress_times
declare -A order1_decompress_times
declare -A order1_ratios
declare -A order2_compress_times
declare -A order2_decompress_times
declare -A order2_ratios

# Test files
FILES=(A B C D E F G H)

# Function to extract time from output
extract_time() {
    echo "$1" | grep -oP 'time: \K[0-9]+' | head -1
}

# Function to extract ratio from output
extract_ratio() {
    echo "$1" | grep -oP 'ratio: \K[0-9.]+' | head -1
}

# Function to get file size
get_size() {
    stat -c%s "$1" 2>/dev/null || echo "0"
}

echo "Testing each model on all files..."
echo ""

for FILE in "${FILES[@]}"; do
    INPUT="$DATA_DIR/$FILE"

    if [ ! -f "$INPUT" ]; then
        echo "⚠️  File $FILE not found, skipping..."
        continue
    fi

    ORIGINAL_SIZE=$(get_size "$INPUT")

    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "File: $FILE ($(numfmt --to=iec-i --suffix=B $ORIGINAL_SIZE))"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    # Test Order-0
    echo -n "  Order-0: Compressing... "
    OUTPUT_ORDER0="/tmp/bench_${FILE}_order0.g07"
    DECODED_ORDER0="/tmp/bench_${FILE}_order0_decoded"

    COMPRESS_OUT=$($COMPRESSOR "$INPUT" "$OUTPUT_ORDER0" --model order0 2>&1)
    COMP_TIME=$(extract_time "$COMPRESS_OUT")
    RATIO=$(extract_ratio "$COMPRESS_OUT")

    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓${NC} ${COMP_TIME}ms"
        order0_compress_times[$FILE]=$COMP_TIME
        order0_ratios[$FILE]=$RATIO

        echo -n "  Order-0: Decompressing... "
        DECOMPRESS_OUT=$($DECOMPRESSOR "$OUTPUT_ORDER0" "$DECODED_ORDER0" 2>&1)
        DECOMP_TIME=$(extract_time "$DECOMPRESS_OUT")

        if diff -q "$INPUT" "$DECODED_ORDER0" > /dev/null 2>&1; then
            echo -e "${GREEN}✓${NC} ${DECOMP_TIME}ms (lossless)"
            order0_decompress_times[$FILE]=$DECOMP_TIME
        else
            echo -e "${RED}✗${NC} FAILED (not lossless!)"
        fi
    else
        echo -e "${RED}✗${NC} FAILED"
    fi

    # Test Order-1
    echo -n "  Order-1: Compressing... "
    OUTPUT_ORDER1="/tmp/bench_${FILE}_order1.g07"
    DECODED_ORDER1="/tmp/bench_${FILE}_order1_decoded"

    COMPRESS_OUT=$($COMPRESSOR "$INPUT" "$OUTPUT_ORDER1" --model order1 2>&1)
    COMP_TIME=$(extract_time "$COMPRESS_OUT")
    RATIO=$(extract_ratio "$COMPRESS_OUT")

    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓${NC} ${COMP_TIME}ms"
        order1_compress_times[$FILE]=$COMP_TIME
        order1_ratios[$FILE]=$RATIO

        echo -n "  Order-1: Decompressing... "
        DECOMPRESS_OUT=$(timeout 10 $DECOMPRESSOR "$OUTPUT_ORDER1" "$DECODED_ORDER1" 2>&1)
        DECOMP_STATUS=$?

        if [ $DECOMP_STATUS -eq 124 ]; then
            echo -e "${YELLOW}⏱${NC} TIMEOUT (>10s)"
            order1_decompress_times[$FILE]="TIMEOUT"
        elif [ $DECOMP_STATUS -eq 0 ]; then
            DECOMP_TIME=$(extract_time "$DECOMPRESS_OUT")
            if diff -q "$INPUT" "$DECODED_ORDER1" > /dev/null 2>&1; then
                echo -e "${GREEN}✓${NC} ${DECOMP_TIME}ms (lossless)"
                order1_decompress_times[$FILE]=$DECOMP_TIME
            else
                echo -e "${RED}✗${NC} FAILED (not lossless!)"
            fi
        else
            echo -e "${RED}✗${NC} FAILED"
        fi
    else
        echo -e "${RED}✗${NC} FAILED"
    fi

    # Test Order-2
    echo -n "  Order-2: Compressing... "
    OUTPUT_ORDER2="/tmp/bench_${FILE}_order2.g07"
    DECODED_ORDER2="/tmp/bench_${FILE}_order2_decoded"

    COMPRESS_OUT=$(timeout 30 $COMPRESSOR "$INPUT" "$OUTPUT_ORDER2" --model order2 2>&1)
    COMP_STATUS=$?

    if [ $COMP_STATUS -eq 124 ]; then
        echo -e "${YELLOW}⏱${NC} TIMEOUT (>30s)"
        order2_compress_times[$FILE]="TIMEOUT"
        order2_ratios[$FILE]="N/A"
    elif [ $COMP_STATUS -eq 0 ]; then
        COMP_TIME=$(extract_time "$COMPRESS_OUT")
        RATIO=$(extract_ratio "$COMPRESS_OUT")
        echo -e "${GREEN}✓${NC} ${COMP_TIME}ms"
        order2_compress_times[$FILE]=$COMP_TIME
        order2_ratios[$FILE]=$RATIO

        echo -n "  Order-2: Decompressing... "
        DECOMPRESS_OUT=$(timeout 30 $DECOMPRESSOR "$OUTPUT_ORDER2" "$DECODED_ORDER2" 2>&1)
        DECOMP_STATUS=$?

        if [ $DECOMP_STATUS -eq 124 ]; then
            echo -e "${YELLOW}⏱${NC} TIMEOUT (>30s)"
            order2_decompress_times[$FILE]="TIMEOUT"
        elif [ $DECOMP_STATUS -eq 0 ]; then
            DECOMP_TIME=$(extract_time "$DECOMPRESS_OUT")
            if diff -q "$INPUT" "$DECODED_ORDER2" > /dev/null 2>&1; then
                echo -e "${GREEN}✓${NC} ${DECOMP_TIME}ms (lossless)"
                order2_decompress_times[$FILE]=$DECOMP_TIME
            else
                echo -e "${RED}✗${NC} FAILED (not lossless!)"
            fi
        else
            echo -e "${RED}✗${NC} FAILED"
        fi
    else
        echo -e "${RED}✗${NC} FAILED"
    fi

    echo ""
done

# Generate markdown table
echo ""
echo "Generating results table..."

cat >> "$RESULTS_FILE" << 'EOF'

### Compression Ratio Comparison

| File | Original | Order-0 | Order-1 | Order-2 | Best |
|------|----------|---------|---------|---------|------|
EOF

for FILE in "${FILES[@]}"; do
    INPUT="$DATA_DIR/$FILE"
    if [ ! -f "$INPUT" ]; then
        continue
    fi

    ORIGINAL_SIZE=$(get_size "$INPUT")
    SIZE_HR=$(numfmt --to=iec-i --suffix=B $ORIGINAL_SIZE)

    RATIO0="${order0_ratios[$FILE]:-N/A}"
    RATIO1="${order1_ratios[$FILE]:-N/A}"
    RATIO2="${order2_ratios[$FILE]:-N/A}"

    # Find best ratio
    BEST="Order-0"
    BEST_VAL=$RATIO0
    if [[ "$RATIO1" != "N/A" ]] && (( $(echo "$RATIO1 < $BEST_VAL" | bc -l) )); then
        BEST="Order-1"
        BEST_VAL=$RATIO1
    fi
    if [[ "$RATIO2" != "N/A" ]] && (( $(echo "$RATIO2 < $BEST_VAL" | bc -l) )); then
        BEST="Order-2"
        BEST_VAL=$RATIO2
    fi

    echo "| $FILE | $SIZE_HR | ${RATIO0}% | ${RATIO1}% | ${RATIO2}% | $BEST |" >> "$RESULTS_FILE"
done

cat >> "$RESULTS_FILE" << 'EOF'

### Compression Speed Comparison

| File | Order-0 (ms) | Order-1 (ms) | Order-2 (ms) | Fastest |
|------|--------------|--------------|--------------|---------|
EOF

for FILE in "${FILES[@]}"; do
    TIME0="${order0_compress_times[$FILE]:-N/A}"
    TIME1="${order1_compress_times[$FILE]:-N/A}"
    TIME2="${order2_compress_times[$FILE]:-N/A}"

    echo "| $FILE | $TIME0 | $TIME1 | $TIME2 | TBD |" >> "$RESULTS_FILE"
done

cat >> "$RESULTS_FILE" << 'EOF'

### Decompression Speed Comparison

| File | Order-0 (ms) | Order-1 (ms) | Order-2 (ms) | Fastest |
|------|--------------|--------------|--------------|---------|
EOF

for FILE in "${FILES[@]}"; do
    TIME0="${order0_decompress_times[$FILE]:-N/A}"
    TIME1="${order1_decompress_times[$FILE]:-N/A}"
    TIME2="${order2_decompress_times[$FILE]:-N/A}"

    echo "| $FILE | $TIME0 | $TIME1 | $TIME2 | TBD |" >> "$RESULTS_FILE"
done

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo -e "${GREEN}✅ Benchmark complete!${NC}"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Results saved to: $RESULTS_FILE"
echo ""
