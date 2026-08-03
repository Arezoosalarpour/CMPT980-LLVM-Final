# Results

This folder contains the final result summaries and the compact evidence files used in the report.

| File or folder | What it shows |
|---|---|
| [`STEP3C_REPORT.md`](STEP3C_REPORT.md) | Final patch behavior, FileCheck results, and regression comparison |
| [`STEP4A1_REPORT.md`](STEP4A1_REPORT.md) | Static instruction count and code-size measurements |
| [`STEP4A2_REPORT.md`](STEP4A2_REPORT.md) | `llvm-mca` throughput analysis |
| [`STEP4B1_REPORT.md`](STEP4B1_REPORT.md) | Results across several CPU models |
| [`STEP5A2_REPORT.md`](STEP5A2_REPORT.md) | Wider shuffle-mask investigation |
| [`Full_validation/results.json`](Full_validation/results.json) | Machine-readable final validation summary |
| [`Randomized_validation/`](Randomized_validation/) | Randomized semantic test, script, and recorded results |
| [`Probability/comparison_table.json`](Probability/comparison_table.json) | Machine-readable profitability comparison |

The final LLVM source and FileCheck patches are in [`../Patches/`](../Patches/). Reproduction scripts are in [`../Scripts/`](../Scripts/).

Running the scripts creates temporary assembly, logs, and intermediate analysis under `Results/generated/`. Those generated files are not committed.
