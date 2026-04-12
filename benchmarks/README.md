# Benchmarks

Simple benchmarking tools for G07 compression versions.

## Files

- **benchmark.sh** - Main benchmark script (all versions on all files)
- **test.sh** - Quick single-file test script
- **results.csv** - Machine-readable results (generated after benchmark)
- **results.md** - Human-readable table (generated after benchmark)
- **tmp/** - Temporary files during benchmarking

## Usage

### Option 1: Using Makefile (Recommended)

```bash
# Run full benchmark
make benchmark

# Quick test specific version on specific file
make test FILE=data/A VERSION=5

# Clean results before new run
make clean-results
```

### Option 2: Direct Script Execution

```bash
# Full benchmark
./benchmarks/benchmark.sh

# Single test
./benchmarks/test.sh data/A 5
```

## Output

Both scripts verify lossless compression (checksum validation) and measure:
- Compression ratio (%)
- Bits per symbol
- Compression time (ms)
- Decompression time (ms)
- Compression speed (MB/s)
- Decompression speed (MB/s)

Results are saved to:
- `results.csv` - For spreadsheet analysis
- `results.md` - For reports and documentation

## Quick Reference

| Command | Description |
|---------|-------------|
| `make benchmark` | Benchmark all versions (v5-v9) on all files |
| `make test FILE=data/A VERSION=5` | Test v5 on file A |
| `make test FILE=data/B VERSION=9` | Test v9 on file B |
| `make clean-results` | Remove old results |
| `cat benchmarks/results.md` | View latest results |

## Versions Tested

- **v5** - Best compression (54.77% avg)
- **v6** - Multi-order PPM (experimental, 58.41%)
- **v7** - Speed-optimized (59.52% avg, 60 MB/s decompress)
- **v8** - Ultra-fast LZ77 (76.42%, 133 MB/s decompress)
- **v9** - Bit-level PPM (experimental, 58.75%)
