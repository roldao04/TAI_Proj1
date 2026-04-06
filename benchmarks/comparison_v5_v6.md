# v5.0 vs v6.0 Compression Comparison

**Date:** Mon Apr  6 05:50:56 PM WEST 2026

**v5.0:** BWT + MTF + ZRLE + Order-1 PPM (fast)
**v6.0:** Multi-Order PPM + Context Mixing (maximum compression)

| File | Original | v5.0 Ratio | v5.0 Time | v6.0 Ratio | v6.0 Time | Improvement | Slowdown |
|------|----------|------------|-----------|------------|-----------|-------------|----------|
| A | 1.3MiB | 53.30% | 91ms | 60.61% | 7755ms | ✗ 7.31 pp | 85.21× |
| B | 1.2MiB | 17.34% | 55ms | 19.39% | 1631ms | ✗ 2.05 pp | 29.65× |
| C | 2.0MiB | 25.93% | 103ms | 28.84% | 5414ms | ✗ 2.91 pp | 52.56× |
| D | 2.0MiB | 100.00% | 10ms | 100.00% | 22845ms | 0 pp | 2284.50× |
| E | 989KiB | 86.03% | 43ms | 98.86% | 8898ms | ✗ 12.83 pp | 206.93× |
| F | 2.0MiB | 82.42% | 30ms | 84.38% | 19953ms | ✗ 1.96 pp | 665.10× |
| G | 2.5MiB | 29.27% | 95ms | 35.43% | 10136ms | ✗ 6.16 pp | 106.69× |
| H | 999KiB | 44.72% | 87ms | 58.70% | 4399ms | ✗ 13.98 pp | 50.56× |

## Overall Comparison

| Metric | v5.0 | v6.0 | Difference |
|--------|------|------|------------|
| Average Ratio | 54.92% | 59.85% | -4.93pp |
| Total Compressed | 6.9MiB | 7.5MiB | - |
| Total Time | 514ms | 81031ms | 157.64× slower |
| Files Won | 8 | 0 | - |
