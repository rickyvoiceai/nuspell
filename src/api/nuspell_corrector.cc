#include "nuspell_corrector.h"
#include "compound_corrector.hxx"
#include "bundle.hxx"

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>

struct NuspellCorrectorState {
	std::unique_ptr<CompoundCorrector> corrector;
};

// --- Helpers for reading the modelscope binary format ---

// Read a 4-byte little-endian int32_t from the buffer at offset.
static int read_int32_le(const char* buf, int offset) {
	return (static_cast<int>(static_cast<unsigned char>(buf[offset]))) |
	       (static_cast<int>(static_cast<unsigned char>(buf[offset + 1])) << 8) |
	       (static_cast<int>(static_cast<unsigned char>(buf[offset + 2])) << 16) |
	       (static_cast<int>(static_cast<unsigned char>(buf[offset + 3])) << 24);
}

// Read a 4-byte little-endian uint32_t from the buffer at offset.
static unsigned read_uint32_le(const char* buf, int offset) {
	return (static_cast<unsigned>(static_cast<unsigned char>(buf[offset]))) |
	       (static_cast<unsigned>(static_cast<unsigned char>(buf[offset + 1])) << 8) |
	       (static_cast<unsigned>(static_cast<unsigned char>(buf[offset + 2])) << 16) |
	       (static_cast<unsigned>(static_cast<unsigned char>(buf[offset + 3])) << 24);
}

// Advance past a modelscope section: [16-byte identifier] [4-byte totalLength] [data...]
// Returns new offset, or -1 if the section header doesn't match.
static int skip_modelscope_section(const char* buf, int offset, int buf_size,
                                   const char* expected_id) {
	if (offset + 20 > buf_size) return -1;
	if (std::strncmp(buf + offset, expected_id, std::strlen(expected_id)) != 0)
		return -1;
	offset += 16;
	int data_len = read_int32_le(buf, offset);
	offset += 4;
	if (data_len < 0 || offset + data_len > buf_size) return -1;
	return offset + data_len;
}

nuspell_corrector_handle_t nuspell_corrector_create(const char* buffer, int size) {
	if (!buffer || size < 20) return nullptr;

	auto* state = new NuspellCorrectorState();
	int offset = 0;

	// Skip: tokenlist: [16B] [4B totalLen] [entries...]
	offset = skip_modelscope_section(buffer, offset, size, "tokenlist:");
	if (offset < 0) { delete state; return nullptr; }

	// Skip: punclist: [16B] [4B totalLen] [entries...]
	offset = skip_modelscope_section(buffer, offset, size, "punclist:");
	if (offset < 0) { delete state; return nullptr; }

	// Skip: punconnx: [16B] [4B onnx_size] [onnx_data...]
	offset = skip_modelscope_section(buffer, offset, size, "punconnx:");
	if (offset < 0) { delete state; return nullptr; }

	// Now at nuspell section: [4B bundle_size] [BNDL bundle data]
	if (offset + 4 > size) { delete state; return nullptr; }
	unsigned bundle_size = read_uint32_le(buffer, offset);
	offset += 4;
	if (bundle_size == 0 || bundle_size > 100 * 1024 * 1024) {
		delete state; return nullptr;
	}
	if (offset + static_cast<int>(bundle_size) > size) {
		delete state; return nullptr;
	}

	// Load CompoundCorrector from the bundle data.
	try {
		std::string bundle_data(buffer + offset, bundle_size);
		std::istringstream iss(bundle_data);
		NuspellConfig config; // use defaults
		state->corrector.reset(new CompoundCorrector(iss, config));
	} catch (const std::exception&) {
		delete state;
		return nullptr;
	}

	return static_cast<nuspell_corrector_handle_t>(state);
}

char* nuspell_corrector_correct(nuspell_corrector_handle_t handle,
                                const char* input) {
	return nuspell_corrector_correct_ex(handle, input, 0);
}

char* nuspell_corrector_correct_ex(nuspell_corrector_handle_t handle,
                                   const char* input, int fix_single) {
	if (!handle || !input) {
		char* out = new char[1];
		out[0] = '\0';
		return out;
	}
	auto* state = static_cast<NuspellCorrectorState*>(handle);
	if (!state->corrector) {
		char* out = new char[1];
		out[0] = '\0';
		return out;
	}

	std::string result = state->corrector->Correct(input, fix_single != 0);
	char* c_result = new char[result.size() + 1];
	std::memcpy(c_result, result.c_str(), result.size() + 1);
	return c_result;
}

void nuspell_corrector_free_result(char* result) {
	delete[] result;
}

struct nuspell_correction_result*
nuspell_corrector_correct_with_status(nuspell_corrector_handle_t handle,
                                      const char* input) {
	return nuspell_corrector_correct_with_status_ex(handle, input, 0);
}

struct nuspell_correction_result*
nuspell_corrector_correct_with_status_ex(nuspell_corrector_handle_t handle,
                                            const char* input, int fix_single) {
	if (!handle || !input)
		return nullptr;
	auto* state = static_cast<NuspellCorrectorState*>(handle);
	if (!state->corrector)
		return nullptr;

	try {
		CorrectionResult cpp_result =
		    state->corrector->CorrectWithStatus(input, fix_single != 0);

		auto* c_result = new nuspell_correction_result();
		c_result->text = new char[cpp_result.text.size() + 1];
		std::memcpy(c_result->text, cpp_result.text.c_str(),
		           cpp_result.text.size() + 1);

		c_result->num_tokens = static_cast<int>(cpp_result.status.size());
		if (c_result->num_tokens > 0) {
			c_result->status = new int[c_result->num_tokens];
			std::memcpy(c_result->status, cpp_result.status.data(),
			           c_result->num_tokens * sizeof(int));
		} else {
			c_result->status = nullptr;
		}
		return c_result;
	} catch (const std::exception&) {
		return nullptr;
	}
}

void nuspell_corrector_free_status_result(
    struct nuspell_correction_result* result) {
	if (!result)
		return;
	delete[] result->text;
	delete[] result->status;
	delete result;
}

// Apply runtime config to an existing handle.
// Returns 0 on success, -1 on error (null handle/corrector).
int nuspell_corrector_set_config(
    nuspell_corrector_handle_t handle,
    const struct nuspell_config* cfg) {
	if (!handle || !cfg)
		return -1;
	auto* state = static_cast<NuspellCorrectorState*>(handle);
	if (!state->corrector)
		return -1;
	NuspellConfig nc;
	nc.single_word_fix_min_len = cfg->single_word_fix_min_len;
	nc.hamming_distance_threshold = cfg->hamming_distance_threshold;
	nc.short_short_arpa_threshold = cfg->short_short_arpa_threshold;
	state->corrector->SetConfig(nc);
	return 0;
}

void nuspell_corrector_destroy(nuspell_corrector_handle_t handle) {
	if (!handle) return;
	auto* state = static_cast<NuspellCorrectorState*>(handle);
	delete state;
}
