# Randomized Semantic Validation (Bounded Differential Test)

**Date (UTC):** 2026-07-28T01:18:12Z  
**Overall result:** **PASS** (all three variants)

This task verifies that machine code produced by the **final patched LLVM 17.0.6** backend for sparse shuffle variants A, B, and C matches a trusted scalar `shufflevector` reference across varied 32-byte inputs. It is a **correctness** check only.

**Not in scope (explicitly distinguished):**

| Activity | Relationship |
|----------|--------------|
| Native performance benchmarking | Not performed here |
| 71-function lit/FileCheck regression suite | Separate evidence (`Results/STEP3C_REPORT.md`) |
| Unpatched 105-mask investigation | Separate investigation on stock LLVM |
| LLVM patch / lowering / FileCheck edits | **Not modified** |
| Report Section 4 | Incorporated in Section 4 of the final report |

---

## 1. Execution environment

| Property | Value |
|----------|-------|
| Host OS | Darwin 25.5.0 (macOS) |
| Host CPU | **Apple M1 Pro** (`arm64`) |
| Host kernel | `RELEASE_ARM64_T6000` |
| Patched `llc` binary arch | **arm64** (runs natively on host) |
| Generated test binary arch | **x86_64** (Mach-O) |
| Execution mechanism | **`arch -x86_64`** — Rosetta 2–translated x86-64 process on Apple Silicon |
| Rosetta confirmation | `sysctl.proc_translated = 1` inside `arch -x86_64` shell; `= 0` in native arm64 shell |
| AVX2 execution | Emulated/translated via Rosetta; **not native Intel/AMD x86-64 hardware** |

**Important:** This validates semantic agreement between reference logic and patched codegen output under Rosetta-translated AVX2 execution. It does **not** constitute native x86-64 silicon testing.

---

## 2. Patched compiler / build identification

| Property | Value |
|----------|-------|
| LLVM tree | `<llvm-project>` |
| Git HEAD | `6009708b4367171ccdbf4b5905cb6a803753fe18` |
| Tag | `llvmorg-17.0.6` |
| Patched `llc` | `<llvm-project>/build-x86/bin/llc` |
| `llc` mtime | `2026-07-27T12:57:40Z` |
| `llc --version` | LLVM 17.0.6, Optimized build |
| Backend patch marker | `isInLaneIdentityOrZeroBytePairShuffleMask` present in working-tree diff |
| `X86ISelLowering.cpp` diff | **+281 / −11** lines (1 file changed) |
| Host C compiler | Apple clang 21.0.0 (clang-2100.1.1.101) |

---

## 3. Source inputs inspected (unchanged)

| File | Role |
|------|------|
| `tests/verify_patched_codegen.c` | Original fixed-input semantic verifier ({1,…,32} only) |
| `tests/update2_sparse_variant_a.ll` | Variant A IR + exact shuffle mask |
| `tests/update2_sparse_variant_b.ll` | Variant B IR + exact shuffle mask |
| `tests/update2_sparse_variant_c.ll` | Variant C IR + exact shuffle mask |

Shuffle masks in the new harness match `verify_patched_codegen.c` and the `.ll` files exactly.

---

## 4. Generated assembly (final patched backend)

All three variants lower to a single **`vpshufb`** + **`retq`** (memory-form constant pool):

**Variant A** (`build/sparse_variant_a.s`):

```asm
vpshufb LCPI0_0(%rip), %ymm0, %ymm0
retq
```

**Variant B** (`build/sparse_variant_b.s`):

```asm
vpshufb LCPI0_0(%rip), %ymm0, %ymm0
retq
```

**Variant C** (`build/sparse_variant_c.s`):

```asm
vpshufb LCPI0_0(%rip), %ymm0, %ymm0
retq
```

Constant pools encode PSHUFB control bytes (128 = zero lane); masks correspond to the IR shuffle indices.

---

## 5. Test harness (new artifact)

| File | Purpose |
|------|---------|
| `randomized_semantic_test.c` | Scalar reference + differential compare for A/B/C |
| `run_validation.sh` | Reproducible build and execute script |
| `build/` | Object files, assembly, linked test binary |
| `logs/` | Environment, commands, stdout |

**Reference semantics:** For each lane index `i`, output byte `i` = input byte `sv[i]` if `sv[i] < 32`, else `0` (matching LLVM `shufflevector` with `zeroinitializer` and indices ≥32 / ≥48 as documented in Section 4 of the final report).

**Patched code under test:** Functions `sparse_variant_{a,b,c}` from objects compiled by patched `llc`.

---

## 6. Deterministic test cases

**Total input vectors:** 1,013  
**Total byte comparisons:** 97,248 (= 1,013 cases × 32 bytes × 3 variants)

### 6.1 Fixed pattern vectors (7)

| Case ID | Pattern |
|---------|---------|
| 0 | All `0x00` |
| 1 | All `0xFF` |
| 2 | Ascending `0, 1, …, 31` |
| 3 | Descending `255, 254, …, 224` |
| 4 | Alternating `0x00`, `0xFF` |
| 5 | Alternating `0x55`, `0xAA` |
| 6 | Original fixed test `{1, 2, …, 32}` |

### 6.2 Boundary byte vectors (6)

Each vector filled uniformly with one boundary value:

| Case ID | Fill value |
|---------|------------|
| 7 | `0x00` |
| 8 | `0x01` |
| 9 | `0x7F` |
| 10 | `0x80` |
| 11 | `0xFE` |
| 12 | `0xFF` |

### 6.3 Random vectors (1,000)

| Property | Value |
|----------|-------|
| Count | 1,000 |
| PRNG | C `rand()` after `srand()` |
| **Fixed seed** | **`980202607`** |
| Generation | Case IDs 13–1012; each byte `(uint8_t)(rand() & 0xFF)` |

---

## 7. Build and execution commands

Recorded in `logs/commands.log`. Summary:

```bash
# Codegen (patched llc)
$LLVM_PROJECT/build-x86/bin/llc -O2 -mattr=+avx2 -mtriple=x86_64-apple-macos \
  -filetype=obj -o build/sparse_variant_{a,b,c}.o \
  tests/update2_sparse_variant_{a,b,c}.ll

# Link harness + variant objects (x86_64 target)
clang -target x86_64-apple-macos -mavx2 -O2 \
  -o build/randomized_semantic_test \
  randomized_semantic_test.c build/sparse_variant_*.o

# Execute (Rosetta on Apple Silicon)
arch -x86_64 build/randomized_semantic_test
```

**Re-run:**

```bash
./results/final_verification/randomized_semantic_validation/run_validation.sh
```

---

## 8. Results

### Per-variant pass/fail

| Variant | Cases | Byte comparisons | Result |
|---------|-------|------------------|--------|
| A | 1,013 | 32,416 | **PASS** |
| B | 1,013 | 32,416 | **PASS** |
| C | 1,013 | 32,416 | **PASS** |

### Mismatches

**None.** No output byte differed between reference and patched machine code.

### Stdout log (`logs/verify.log`)

```
randomized_semantic_validation: START
total_cases=1013 byte_comparisons=97248 random_seed=980202607 random_count=1000
variant_a: PASS (1013 cases)
variant_b: PASS (1013 cases)
variant_c: PASS (1013 cases)
randomized_semantic_validation: PASS
```

---

## 9. Comparison to prior semantic evidence

| Test | Inputs | Scope |
|------|--------|-------|
| `tests/verify_patched_codegen.c` (step3c) | Single vector `{1,…,32}` | Fixed-input sanity check |
| **This validation** | 13 structured + 1,000 seeded random vectors | Bounded differential correctness |

This closes the randomized semantic-testing gap identified in the report outline. Results are incorporated in Section 4 of the final report.

---

## 10. Artifact index

```
results/final_verification/randomized_semantic_validation/
├── RANDOMIZED_SEMANTIC_VALIDATION.md   (this file)
├── results.json
├── run_validation.sh
├── validation_status.txt               (overall: PASS)
├── src/
│   └── randomized_semantic_test.c
├── build/
│   ├── sparse_variant_{a,b,c}.o
│   ├── sparse_variant_{a,b,c}.s
│   └── randomized_semantic_test
└── logs/
    ├── environment.txt
    ├── commands.log
    └── verify.log
```

---

## Quick reference (report insertion)

- **Execution environment:** Apple M1 Pro (arm64); x86-64 test binary via **Rosetta** (`arch -x86_64`); not native Intel/AMD hardware.
- **Patched build:** LLVM **17.0.6** @ `6009708b4367`; `build-x86/bin/llc` rebuilt **2026-07-27**; backend **+281/−11** in `X86ISelLowering.cpp`.
- **Deterministic cases:** 7 fixed patterns + 6 boundary fills + original `{1,…,32}` vector.
- **Random seed:** **`980202607`** (1,000 vectors).
- **Comparisons:** **97,248** output bytes checked; **0 mismatches**; variants A/B/C all **PASS**.
