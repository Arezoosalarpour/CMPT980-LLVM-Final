# CMPT 980 Final Project — LLVM AVX2 Shuffle Lowering

**Arezoo Salarpour and Seolin Jung**

This repository contains our final project for CMPT 980. We studied how LLVM lowers sparse AVX2 byte shuffles and why some `<32 x i8>` shuffle masks were compiled as `vpxor` followed by `vpblendw`, while the same shuffle could be done with one `vpshufb` instruction.

We used LLVM 17.0.6 and focused on the X86 backend.

## What we found

LLVM has several ways to lower a vector shuffle. The first successful lowering can stop LLVM from checking another option that may produce a shorter instruction sequence.

For the sparse masks in our project, LLVM could create a `PSHUFB` form at an early stage. However, a later DAG combine changed it back to a blend form. This was why changing only the earlier lowering was not enough.

Our final patch does two things:

1. It tries the `PSHUFB` lowering before the shuffle is widened.
2. It stops the later DAG combine from changing the result back to a blend, but only for the sparse mask structure we studied.

At first, we used a broader guard. It changed 31–32 functions in the LLVM regression test, including many cases outside our target. We replaced it with a narrower check for the intended identity-or-zero byte-pair pattern.

## Main results

| Check | Result |
|---|---|
| Sparse variant A | Changed from `vpxor` + `vpblendw` to one `vpshufb` |
| Sparse variant B | Stayed as one `vpshufb` |
| Sparse variant C | Changed from `vpxor` + `vpblendw` to one `vpshufb` |
| Focused FileCheck configurations | 4/4 passed |
| Broader X86 regression RUN lines | 25 passed, 0 failed |
| Original LLVM functions checked | 71 |
| Unexpected changes in those functions | 0 |
| Randomized input vectors | 1,013 |
| Total byte comparisons | 97,248 |
| Semantic mismatches | 0 |

For the target function, the instruction count went from 3 instructions to 2 when `ret` is included. The function `.text` size went from 11 bytes to 10 bytes. The new lowering also needs a 32-byte constant-pool mask.

The `llvm-mca` results did not show a throughput improvement on Haswell, Skylake, or Zen 1. Therefore, we can say that the patch gives a shorter instruction sequence for the target case, but we cannot claim that it always improves runtime performance.

## Repository contents

| Folder | What it contains |
|---|---|
| [`Patches/`](Patches/) | The final X86 backend patch and LLVM regression-test patch |
| [`tests/`](tests/) | LLVM IR test cases and C verification programs |
| [`Scripts/`](Scripts/) | Scripts used to generate and compare the results |
| [`Results/`](Results/) | Step reports, validation results, and cost results |

The main files are:

- [`Patches/X86ISelLowering.patch`](Patches/X86ISelLowering.patch) — the final change to the X86 backend.
- [`Patches/vector-shuffle-combining-avx2.ll.patch`](Patches/vector-shuffle-combining-avx2.ll.patch) — the LLVM FileCheck regression tests.
- [`Results/STEP3C_REPORT.md`](Results/STEP3C_REPORT.md) — the regression and correctness checks.
- [`Results/STEP4A1_REPORT.md`](Results/STEP4A1_REPORT.md) — instruction-count and code-size results.
- [`Results/STEP4A2_REPORT.md`](Results/STEP4A2_REPORT.md) — `llvm-mca` results.
- [`Results/Randomized_validation/RANDOMIZED_SEMANTIC_VALIDATION.md`](Results/Randomized_validation/RANDOMIZED_SEMANTIC_VALIDATION.md) — randomized correctness testing.
- [`Results/Full_validation/results.json`](Results/Full_validation/results.json) — full validation results in JSON format.
- [`Results/Probability/comparison_table.json`](Results/Probability/comparison_table.json) — profitability comparison in JSON format.

## Requirements

- LLVM 17.0.6 source code
- An X86-enabled LLVM build
- `llc`
- `FileCheck`
- `llvm-mca`
- Clang or another C compiler
- Python 3
- Bash

We used separate unpatched and patched LLVM builds so that the baseline and patched results came from different compilers.

## How to run the main experiment

Start with a clean LLVM 17.0.6 source tree. Prepare one unpatched build and one build for the patched version.

Set the paths for the LLVM tools:

```bash
export LLVM_UNPATCHED_LLC=/path/to/unpatched/bin/llc
export LLVM_PATCHED_LLC=/path/to/patched/bin/llc
export LLVM_FILECHECK=/path/to/patched/bin/FileCheck
export LLVM_MCA=/path/to/patched/bin/llvm-mca
export LLVM_PROJECT=/path/to/patched/llvm-project
```

Generate the baseline before applying the patches:

```bash
./Scripts/step1_baseline.sh
```

Apply the patches to the patched LLVM source tree and rebuild the required tools:

```bash
cd /path/to/patched/llvm-project
git apply /path/to/repository/Patches/X86ISelLowering.patch
git apply /path/to/repository/Patches/vector-shuffle-combining-avx2.ll.patch
ninja -C build-x86 llc FileCheck llvm-mca
```

Return to this repository and run the remaining scripts:

```bash
./Scripts/step2_patched.sh
./Scripts/step3c_finalize.sh
./Scripts/step4a1_measure.sh
./Scripts/step4a2_mca.sh
./Scripts/step4b1_investigate.sh
./Scripts/step5a2_investigate.sh
```

Run the randomized correctness test separately:

```bash
./Results/Randomized_validation/run_validation.sh
```

More information about each step is available in [`Scripts/README.md`](Scripts/README.md) and [`Results/README.md`](Results/README.md).

## Limitations

- The patch was developed for LLVM 17.0.6 and AVX2 sparse `<32 x i8>` shuffles.
- The cost evaluation used static measurements and `llvm-mca`, not a full application benchmark.
- The new `vpshufb` needs a 32-byte constant-pool mask. Fewer instructions do not automatically mean faster execution.
- We ran the randomized x86-64 test through Rosetta 2 on Apple Silicon, not on a native x86-64 machine.

## Conclusion

We found that a valid `PSHUFB` lowering could be created early and then changed back to a blend by a later DAG combine. The final narrow check keeps the `PSHUFB` result for the intended sparse cases without changing the unrelated cases in our regression test.

The patch reduced the target sequence from `vpxor` plus `vpblendw` to one `vpshufb`. It improved the static instruction count and slightly reduced the function `.text` size, but the `llvm-mca` results did not show a throughput improvement.
