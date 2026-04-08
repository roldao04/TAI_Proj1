# Melhorias desde a versão 4.0

**Grupo 07**

---

## Resumo de Desempenho

| Versão | Rácio Médio | Velocidade Decomp. |
|--------|------------|-------------------|
| v4.0 | 56.39% | ~100ms |
| v5.0 | 54.87% | 63ms |
| v5.2 (atual) | **54.70%** | **60ms** (PGO) |
| **bzip2** | **54.93%** | **72ms** |

> A ferramenta bate o bzip2 em **rácio de compressão** (−0.23pp) e em **velocidade de descompressão** (−12ms).

---

## v4.0 → v5.0 · Foco: Velocidade

### O que mudou

**Paralelismo total do pipeline**
Cada bloco de 600 KB executa o pipeline completo (BWT → MTF → ZRLE → Order-1) numa thread independente em simultâneo. Antes, só o BWT era paralelo.

**Encode sem alocações de heap**
A função `encode_symbol_fast()` passou a devolver um struct de tamanho fixo na stack em vez de um `std::vector`, eliminando ~900 000 alocações por bloco.

**rANS para ficheiros de alta entropia**
Ficheiros E e F passaram a usar o codificador rANS (ryg) em vez do range coder de Schindler. O rANS não faz divisões por símbolo e usa uma tabela de 16 384 entradas — descompressão de E: 55ms → 11ms, F: 126ms → 21ms.

**LTO (`-flto=auto`)**
Inlining entre módulos nas funções mais chamadas. −30% no tempo de compressão de E.

### Resultados
- Rácio mantido em **56.39%**
- Compressão: G 117ms→53ms, C 96ms→39ms, E 49ms→9ms
- Descompressão: −38% média em ficheiros Order-1, −82% em ficheiros Order-0

---

## v5.0 → v6.0 · Foco: Rácio de Compressão

### O que mudou

**PPM Método C (exclusões)**
Quando o modelo de Order-1 não conhece o símbolo atual no contexto presente, escapa para Order-0. Com o Método C, os símbolos que já foram vistos no contexto de Order-1 são *excluídos* da distribuição de Order-0 — a massa de probabilidade concentra-se no que é realmente possível, gastando menos bits.

**RLE0**
Substituiu o ZRLE anterior (marcador + contagem) por codificação bijective base-2 de runs de zeros usando dois símbolos: RUNA e RUNB. Runs longas de zeros (muito comuns após BWT+MTF) ficam representadas em O(log n) bytes em vez de n bytes.

**Limiar de entropia 6.8 → 7.2**
Os ficheiros E e F (entropia ~7.0 bits/símbolo) passaram a usar Order-1 em vez de rANS Order-0. O modelo Order-1 tem melhor compressão neste intervalo de entropia: F ganhou 5.84pp, E ganhou 1.99pp.

### Resultados
- Rácio melhorou de 56.39% → **55.16%** (−1.23pp)
- F: 87.62% → 81.78%; E: 88.17% → 86.18%

---

## v6.0 → v7.0 · Foco: Rácio + Correções + Velocidade de Descompressão

### O que mudou

**Estimador de escape PPMD (singleton)**
O estimador de escape do PPM passou de `max(1, seen/4)` para `max(1, singleton_count)`, onde `singleton_count` conta os símbolos com frequência == 1 no contexto atual. A probabilidade de escape reflete melhor a real probabilidade de um símbolo novo aparecer.

**Tamanho de bloco dinâmico (máx. 2000 KB, divisão uniforme)**
Substituiu o bloco fixo de 900 KB. O número de blocos é `ceil(tamanho / 2000KB)` e o tamanho de cada bloco é `ceil(tamanho / nº_blocos)` — todos os blocos ficam iguais, sem bloco residual pequeno. B, C e H passaram a caber num único bloco → melhor clustering do BWT:
- B: 18.18% → 17.34% (−0.84pp)
- C: 26.95% → 25.93% (−1.02pp)

**Order-1 paralelo para ficheiros de alta entropia (E e F)**
E e F não usam BWT e eram codificados numa única thread sequencial (E: 181ms, F: 218ms de descompressão). Passaram a ser divididos em blocos de 512 KB e codificados em paralelo usando o mesmo formato PARALLEL com `transform_flags = 0` (sem BWT/MTF/ZRLE). O descompressor já suportava este formato — nenhuma alteração necessária no lado da descompressão.
- E: 181ms → 140ms; F: 218ms → 67ms
- Média de descompressão: 94ms → **64ms**

### Resultados
- Rácio médio: **54.88%** (bzip2: 54.93%) → **bate o bzip2**
- Descompressão média: **64ms** (bzip2: 72ms) → **bate o bzip2**
- Ficheiros ganhos vs bzip2: **5 em 8** (A, B, C, D, E)

---

## v5.0 → v5.2 · Foco: Rácio (ZRLE fix) + Velocidade

### O que mudou

**Fix ZRLE rank-255 (escape byte)**
O byte 255 passou a ser um prefixo de escape: ranks 1-253 → bytes 2-254 (como antes), ranks 254/255 → bytes `255, rank`. O custo de 1 byte extra por rank-254/255 é negligível (~51 ocorrências em 2.5MB para G). O guard `has_rank255` foi removido — ZRLE é agora ativado em todos os blocos.

**Otimizações do caminho quente**
- Decode exclusões fundido: eliminado scan redundante de 258 elementos (3 scans → 2)
- Encode exclusões fundido: 2 loops → 1 pass
- Cumulativa escape rápida: O(1) via `total - escape_freq` em vez de O(256)
- MTF memmove: `std::memmove()` com SIMD substitui loop manual; inverse path elimina array `positions`

**PGO (Profile-Guided Optimization)**
Novo target `make pgo`: compila instrumentado, treina no corpus, recompila otimizado. Decompressão média: 63ms → 60ms.

### Resultados
- Rácio melhorou de 54.87% → **54.70%** (−0.17pp)
- G: 29.27% → **28.81%** (−0.46pp) — ZRLE agora ativado
- H: 44.72% → **43.79%** (−0.93pp) — ZRLE agora ativado
- Decompressão média: 63ms → **60ms** (PGO)

---

## Limitações Conhecidas

| Ficheiro | Nós | bzip2 | Diferença | Causa |
|---------|-----|-------|----------|-------|
| H | 43.79% | 42.34% | −1.45pp | Modelo Order-1 insuficiente para H |
| F | 82.42% | 81.06% | −1.36pp | 4 resets de contexto (sem BWT) |
| G | 28.81% | 28.70% | −0.11pp | Gap quase fechado (era −0.58pp) |
