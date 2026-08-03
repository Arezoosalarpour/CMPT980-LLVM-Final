// Bounded differential semantic validation for sparse variants A/B/C.
// Compares patched-llc machine code against scalar shufflevector reference.
// Does not modify tests/verify_patched_codegen.c or project LLVM sources.
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef __m256i (*shuffle_fn)(__m256i);

extern __m256i sparse_variant_a(__m256i);
extern __m256i sparse_variant_b(__m256i);
extern __m256i sparse_variant_c(__m256i);

/* Exact masks from tests/verify_patched_codegen.c / update2_sparse_variant_*.ll */
static const int SV_A[32] = {
    0, 1, 32, 32, 32, 32, 32, 32, 8, 9, 32, 32, 32, 32, 32, 32,
    16, 17, 48, 48, 48, 48, 48, 48, 24, 25, 48, 48, 48, 48, 48, 48};
static const int SV_B[32] = {
    2, 3, 32, 32, 32, 32, 32, 32, 10, 11, 32, 32, 32, 32, 32, 32,
    18, 19, 48, 48, 48, 48, 48, 48, 26, 27, 48, 48, 48, 48, 48, 48};
static const int SV_C[32] = {
    0, 1, 32, 32, 4, 5, 32, 32, 8, 9, 32, 32, 12, 13, 32, 32,
    16, 17, 48, 48, 20, 21, 48, 48, 24, 25, 48, 48, 28, 29, 48, 48};

static void apply_sv_ref(const uint8_t *in, const int *sv, uint8_t *out) {
  for (int i = 0; i < 32; i++)
    out[i] = (sv[i] >= 32) ? 0 : in[sv[i]];
}

static int compare_variant(const char *name, const int *sv, shuffle_fn fn,
                           const uint8_t *in, unsigned case_id, int *mismatch_count) {
  uint8_t ref[32], got[32];
  apply_sv_ref(in, sv, ref);
  _mm256_storeu_si256((__m256i *)got, fn(_mm256_loadu_si256((__m256i *)in)));
  for (int i = 0; i < 32; i++) {
    if (ref[i] != got[i]) {
      if (*mismatch_count == 0)
        fprintf(stderr, "%s MISMATCH case=%u byte=%d ref=%u got=%u\n", name, case_id,
                i, ref[i], got[i]);
      (*mismatch_count)++;
      return 1;
    }
  }
  return 0;
}

static void fill_pattern(uint8_t *in, unsigned kind) {
  switch (kind) {
  case 0: /* all zero */
    memset(in, 0, 32);
    break;
  case 1: /* all 0xFF */
    memset(in, 0xFF, 32);
    break;
  case 2: /* ascending 0..31 */
    for (int i = 0; i < 32; i++)
      in[i] = (uint8_t)i;
    break;
  case 3: /* descending 255..224 */
    for (int i = 0; i < 32; i++)
      in[i] = (uint8_t)(255 - i);
    break;
  case 4: /* alternating 0x00 / 0xFF */
    for (int i = 0; i < 32; i++)
      in[i] = (i & 1) ? 0xFF : 0x00;
    break;
  case 5: /* alternating 0x55 / 0xAA */
    for (int i = 0; i < 32; i++)
      in[i] = (i & 1) ? 0xAA : 0x55;
    break;
  case 6: /* original fixed test {1..32} */
    for (int i = 0; i < 32; i++)
      in[i] = (uint8_t)(i + 1);
    break;
  default:
    break;
  }
}

static void fill_boundary_vector(uint8_t *in, unsigned idx) {
  static const uint8_t boundaries[] = {0, 1, 127, 128, 254, 255};
  for (int i = 0; i < 32; i++)
    in[i] = boundaries[idx % 6];
}

int main(void) {
  const unsigned RANDOM_SEED = 980202607u; /* recorded fixed seed */
  const unsigned RANDOM_COUNT = 1000u;
  const unsigned FIXED_KINDS = 7u;
  const unsigned BOUNDARY_VECTORS = 6u;
  const unsigned TOTAL_CASES = FIXED_KINDS + BOUNDARY_VECTORS + RANDOM_COUNT;

  unsigned total_byte_comparisons = TOTAL_CASES * 32u * 3u;
  int mismatches = 0;
  uint8_t in[32];

  printf("randomized_semantic_validation: START\n");
  printf("total_cases=%u byte_comparisons=%u random_seed=%u random_count=%u\n",
         TOTAL_CASES, total_byte_comparisons, RANDOM_SEED, RANDOM_COUNT);

  unsigned case_id = 0;

  for (unsigned k = 0; k < FIXED_KINDS; k++, case_id++) {
    fill_pattern(in, k);
    compare_variant("variant_a", SV_A, sparse_variant_a, in, case_id, &mismatches);
    compare_variant("variant_b", SV_B, sparse_variant_b, in, case_id, &mismatches);
    compare_variant("variant_c", SV_C, sparse_variant_c, in, case_id, &mismatches);
    if (mismatches)
      goto fail;
  }

  for (unsigned b = 0; b < BOUNDARY_VECTORS; b++, case_id++) {
    fill_boundary_vector(in, b);
    compare_variant("variant_a", SV_A, sparse_variant_a, in, case_id, &mismatches);
    compare_variant("variant_b", SV_B, sparse_variant_b, in, case_id, &mismatches);
    compare_variant("variant_c", SV_C, sparse_variant_c, in, case_id, &mismatches);
    if (mismatches)
      goto fail;
  }

  srand(RANDOM_SEED);
  for (unsigned r = 0; r < RANDOM_COUNT; r++, case_id++) {
    for (int i = 0; i < 32; i++)
      in[i] = (uint8_t)(rand() & 0xFF);
    compare_variant("variant_a", SV_A, sparse_variant_a, in, case_id, &mismatches);
    compare_variant("variant_b", SV_B, sparse_variant_b, in, case_id, &mismatches);
    compare_variant("variant_c", SV_C, sparse_variant_c, in, case_id, &mismatches);
    if (mismatches)
      goto fail;
  }

  printf("variant_a: PASS (%u cases)\n", TOTAL_CASES);
  printf("variant_b: PASS (%u cases)\n", TOTAL_CASES);
  printf("variant_c: PASS (%u cases)\n", TOTAL_CASES);
  printf("randomized_semantic_validation: PASS\n");
  return 0;

fail:
  printf("randomized_semantic_validation: FAIL (mismatches=%d)\n", mismatches);
  return 1;
}
