# Benchmarks

Comprehensive benchmarking tools for G07 compression versions.

## Files

- **benchmark_all.sh** - Comprehensive benchmark (G07 versions + external tools)
- **test.sh** - Quick single-file test script
- **results.csv** - Machine-readable results (generated after benchmark)
- **results.md** - Human-readable comprehensive report (generated after benchmark)
- **tmp/** - Temporary files during benchmarking

## Usage

### Option 1: Using Makefile (Recommended)

```bash
# Run comprehensive benchmark (G07 vs gzip/bzip2/xz/zstd)
make benchmark

# Quick test specific version on specific file
make test FILE=data/A VERSION=5

# Clean results before new run
make clean-results
```

### Option 2: Direct Script Execution

```bash
# Comprehensive benchmark (all G07 versions + external tools)
./benchmarks/benchmark_all.sh

# Single file test
./benchmarks/test.sh data/A 5
```

## Output

The benchmark compares:
- **G07 versions**: v5 (best compression), v7 (speed), v8 (ultra-fast), v9 (experimental)
- **External tools**: gzip, bzip2, xz (LZMA), zstd

**Note:** v6 is skipped by default (failed experiment: 157× slower than v5 with worse compression ratio).
To include v6, edit `benchmark_all.sh` and remove `SKIP_VERSIONS="6"`.

Metrics measured:
- Compression ratio (%)
- Bits per symbol
- Compression time (ms)
- Decompression time (ms)
- Lossless verification
- Rankings and winner tracking

Results are saved to:
- `results.csv` - Machine-readable CSV data
- `results.md` - Comprehensive markdown report with tables, rankings, and statistics

## Quick Reference

| Command | Description |
|---------|-------------|
| `make benchmark` | Full benchmark (G07 v5/v7/v8/v9 vs gzip/bzip2/xz/zstd) |
| `make test FILE=data/A VERSION=5` | Test v5 on file A |
| `make test FILE=data/B VERSION=9` | Test v9 on file B |
| `make clean-results` | Remove old results |
| `cat benchmarks/results.md` | View latest comprehensive report |

## Versions Tested

- **v5** - Production (Best compression: 54.77% avg)
- **v6** - SKIPPED (Failed experiment: 59.85% avg, 157× slower)
- **v7** - Production (Speed-optimized: 59.2% avg, 51 MB/s decompress)
- **v8** - Production (Ultra-fast LZ77: 76.42%, ~200 MB/s)
- **v9** - Production (Best ratio: 54.40% avg, slower than v5)

## Benchmark Time Estimate

- **With v6 excluded**: ~2-3 minutes
- **With v6 included**: ~10-15 minutes (v6 is 157× slower than v5)
