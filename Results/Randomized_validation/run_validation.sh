#!/usr/bin/env bash
# Bounded randomized differential semantic validation (patched LLVM only).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$(cd "$(dirname "$0")" && pwd)"
TESTS="$ROOT/tests"
BUILD="$OUT/build"
LOG="$OUT/logs"
mkdir -p "$BUILD" "$LOG"

LLVM_PROJECT="${LLVM_PROJECT:-$HOME/llvm-project}"
LLC="${LLVM_PATCHED_LLC:-$LLVM_PROJECT/build-x86/bin/llc}"
CC="${CC:-clang}"

{
  echo "timestamp_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "root=$ROOT"
  echo "llvm_project=$LLVM_PROJECT"
  echo "llc=$LLC"
  echo "llc_version=$("$LLC" --version 2>&1 | head -3 | tr '\n' '; ')"
  echo "llc_mtime=$(stat -f %Sm -t %Y-%m-%dT%H:%M:%SZ "$LLC" 2>/dev/null || stat -c %y "$LLC")"
  echo "git_head=$(git -C "$LLVM_PROJECT" rev-parse HEAD 2>/dev/null || echo unknown)"
  echo "git_describe=$(git -C "$LLVM_PROJECT" describe --tags --always 2>/dev/null || echo unknown)"
  echo "backend_diff_lines=$(git -C "$LLVM_PROJECT" diff HEAD -- llvm/lib/Target/X86/X86ISelLowering.cpp 2>/dev/null | wc -l | tr -d ' ')"
  echo "host_uname=$(uname -a)"
  echo "host_arch=$(uname -m)"
  echo "host_cpu=$(sysctl -n machdep.cpu.brand_string 2>/dev/null || echo n/a)"
  echo "llc_file_info=$(file "$LLC")"
  echo "proc_translated_native=$(sysctl -n sysctl.proc_translated 2>/dev/null || echo n/a)"
  echo "proc_translated_arch_x86_64=$(arch -x86_64 /bin/sh -c 'sysctl -n sysctl.proc_translated 2>/dev/null || echo n/a')"
  echo "backend_patch_stat=$(git -C "$LLVM_PROJECT" diff HEAD --stat -- llvm/lib/Target/X86/X86ISelLowering.cpp 2>/dev/null | tail -1)"
} | tee "$LOG/environment.txt"

# Verify patched backend markers present
if ! git -C "$LLVM_PROJECT" diff HEAD -- llvm/lib/Target/X86/X86ISelLowering.cpp | grep -q isInLaneIdentityOrZeroBytePairShuffleMask; then
  echo "ERROR: patched backend markers not found in X86ISelLowering.cpp diff" | tee "$OUT/validation_status.txt"
  exit 2
fi

echo "Compiling variant objects with patched llc..." | tee "$LOG/commands.log"
for v in a b c; do
  echo "RUN: $LLC -O2 -mattr=+avx2 -mtriple=x86_64-apple-macos -filetype=obj -o $BUILD/sparse_variant_${v}.o $TESTS/update2_sparse_variant_${v}.ll" | tee -a "$LOG/commands.log"
  "$LLC" -O2 -mattr=+avx2 -mtriple=x86_64-apple-macos -filetype=obj \
    -o "$BUILD/sparse_variant_${v}.o" \
    "$TESTS/update2_sparse_variant_${v}.ll" 2>"$LOG/variant_${v}.stderr"
  "$LLC" -O2 -mattr=+avx2 -mtriple=x86_64-apple-macos -filetype=asm \
    -o "$BUILD/sparse_variant_${v}.s" \
    "$TESTS/update2_sparse_variant_${v}.ll"
done

echo "Linking randomized semantic test..." | tee -a "$LOG/commands.log"
echo "RUN: $CC -target x86_64-apple-macos -mavx2 -O2 -o $BUILD/randomized_semantic_test $OUT/randomized_semantic_test.c $BUILD/sparse_variant_*.o" | tee -a "$LOG/commands.log"
"$CC" -target x86_64-apple-macos -mavx2 -O2 \
  -o "$BUILD/randomized_semantic_test" \
  "$OUT/randomized_semantic_test.c" \
  "$BUILD"/sparse_variant_*.o

echo "Executing under arch -x86_64 (Rosetta if host is Apple Silicon)..." | tee -a "$LOG/commands.log"
echo "RUN: arch -x86_64 $BUILD/randomized_semantic_test" | tee -a "$LOG/commands.log"
if arch -x86_64 "$BUILD/randomized_semantic_test" | tee "$LOG/verify.log"; then
  echo "overall: PASS" | tee "$OUT/validation_status.txt"
  exit 0
else
  echo "overall: FAIL" | tee "$OUT/validation_status.txt"
  exit 1
fi
