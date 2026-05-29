#pragma once

#include <iosfwd>
#include <memory>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* nuspell_corrector_handle_t;

// Create a corrector from a combined binary buffer.
// The buffer contains: [modelscope sections] [4B nuspell_bundle_size] [BNDL bundle].
// Returns NULL on failure.
nuspell_corrector_handle_t nuspell_corrector_create(const char* buffer, int size);

// Correct a single line of text. Returns the corrected string.
// fix_single: if non-zero, also fix single misspelled words.
// Caller must free with nuspell_corrector_free_result().
char* nuspell_corrector_correct(nuspell_corrector_handle_t handle,
                                const char* input);

// Same but expose the fix_single parameter.
char* nuspell_corrector_correct_ex(nuspell_corrector_handle_t handle,
                                   const char* input, int fix_single);

// Free a result string returned by nuspell_corrector_correct().
void nuspell_corrector_free_result(char* result);

// Status-aware correction result.
// status[i] for each token:
//   0 = unchanged
//   1 = merged with next (initiated merge)
//  -1 = merged by previous (consumed)
//   2 = substituted by single-word fix
struct nuspell_correction_result {
	char* text;
	int*  status;
	int   num_tokens;
};

// Correct a line and also return per-token status codes.
// On error or if handle is NULL, returns NULL.
// Caller must free with nuspell_corrector_free_status_result().
struct nuspell_correction_result* nuspell_corrector_correct_with_status(
    nuspell_corrector_handle_t handle, const char* input);

// Same but expose the fix_single parameter.
struct nuspell_correction_result* nuspell_corrector_correct_with_status_ex(
    nuspell_corrector_handle_t handle, const char* input, int fix_single);

// Free a result struct returned by nuspell_corrector_correct_with_status().
void nuspell_corrector_free_status_result(
    struct nuspell_correction_result* result);

// Runtime tunables for NuspellConfig (exposed to vengine backend).
// All fields have matching defaults to NuspellConfig.  Unchanged fields
// (-1 / NaN sentinel) leave the original config untouched.
struct nuspell_config {
	int   single_word_fix_min_len;
	int   hamming_distance_threshold;
	float short_short_arpa_threshold;
};

// Apply runtime config to an existing handle.
// Returns 0 on success, -1 on error (null handle/corrector).
int nuspell_corrector_set_config(
    nuspell_corrector_handle_t handle,
    const struct nuspell_config* cfg);

// Destroy a corrector handle.
void nuspell_corrector_destroy(nuspell_corrector_handle_t handle);

#ifdef __cplusplus
}
#endif
