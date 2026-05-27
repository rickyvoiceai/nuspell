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
// Caller must free with nuspell_corrector_free_result().
char* nuspell_corrector_correct(nuspell_corrector_handle_t handle,
                                const char* input);

// Free a result string returned by nuspell_corrector_correct().
void nuspell_corrector_free_result(char* result);

// Destroy a corrector handle.
void nuspell_corrector_destroy(nuspell_corrector_handle_t handle);

#ifdef __cplusplus
}
#endif
