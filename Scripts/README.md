# Reproduction scripts

These scripts reproduce the main experiments used in the final report. Run them from the repository root after configuring separate unpatched and patched LLVM 17.0.6 builds.

## Required tools

Set these variables to the tools in your LLVM builds:

```bash
export LLVM_UNPATCHED_LLC=/path/to/unpatched/bin/llc
export LLVM_PATCHED_LLC=/path/to/patched/bin/llc
export LLVM_FILECHECK=/path/to/patched/bin/FileCheck
export LLVM_MCA=/path/to/patched/bin/llvm-mca
export LLVM_PROJECT=/path/to/llvm-project
```

Apply the two project patches from [`../Patches/`](../Patches/) before building the patched tools.

## Run order

```bash
./Scripts/step1_baseline.sh
./Scripts/step2_patched.sh
./Scripts/step3c_finalize.sh
./Scripts/step4a1_measure.sh
./Scripts/step4a2_mca.sh
./Scripts/step4b1_investigate.sh
./Scripts/step5a2_investigate.sh
```

Generated assembly, logs, and temporary analysis files are written under `Results/generated/`. They are intentionally excluded by `.gitignore` because the committed reports and JSON files record the final results.

## Main files

| File | Purpose |
|---|---|
| `common.sh` | Shared environment and safety checks |
| `step1_baseline.sh` | Builds the unpatched baseline outputs |
| `step2_patched.sh` | Builds the patched outputs |
| `step3c_finalize.sh` | Runs FileCheck, comparison, and semantic checks |
| `step4a1_measure.sh` | Measures static instruction and code size |
| `step4a2_mca.sh` | Runs `llvm-mca` analysis |
| `step4b1_investigate.sh` | Runs the multi-CPU investigation |
| `step5a2_investigate.sh` | Runs the wider mask search |

Final summaries are indexed in [`../Results/README.md`](../Results/README.md).
