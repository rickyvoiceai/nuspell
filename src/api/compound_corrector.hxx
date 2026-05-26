#pragma once

#include <iosfwd>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// Minimal std::optional replacement for C++14 compatibility.
// We expose this to callers via the bundle constructor below.
template <typename T>
class Optional {
	bool has_ = false;
  alignas(alignof(T)) unsigned char storage_[sizeof(T)];

	T* ptr() const { return reinterpret_cast<T*>(
	        const_cast<unsigned char*>(storage_)); }

public:
	Optional() = default;
	~Optional() { if (has_) ptr()->~T(); }

	Optional(const Optional&) = delete;
	Optional& operator=(const Optional&) = delete;

	Optional(Optional&& other)
	    : has_(other.has_) {
		if (has_) {
			new (ptr()) T(std::move(*other.ptr()));
			other.ptr()->~T();
			other.has_ = false;
		}
	}
	Optional& operator=(Optional&& other) {
		if (this != &other) {
			if (has_) ptr()->~T();
			has_ = other.has_;
			if (has_) {
				new (ptr()) T(std::move(*other.ptr()));
				other.ptr()->~T();
				other.has_ = false;
			}
		}
		return *this;
	}

	explicit Optional(const T& val) : has_(true) { new (ptr()) T(val); }
	explicit Optional(T&& val) : has_(true) { new (ptr()) T(std::move(val)); }

	bool has_value() const { return has_; }
	T& value() { return *ptr(); }
	const T& value() const { return *ptr(); }

	T& operator*() { return *ptr(); }
	const T& operator*() const { return *ptr(); }

	T* operator->() { return ptr(); }
	const T* operator->() const { return ptr(); }
};

struct NuspellConfig {
	int   single_word_fix_min_len    = 5;
	int   hamming_distance_threshold = 1;
	float short_short_arpa_threshold = -3.0f;
	float acronym_override_logprob   = -3.0f;
	float arpa_unigram_floor         = -10.0f;
};

struct CorrectionResult {
	std::string text;
	// Per original token status:
	//  0 = unchanged
	//  1 = merged with next (initiated merge)
	// -1 = merged by previous (consumed)
	//  2 = substituted by single-word fix
	std::vector<int> status;
};

class CompoundCorrector {
public:
	// Default path-based constructor (keeps existing behavior).
	// All resource paths are optional and default to loose files in res/.
	explicit CompoundCorrector(const std::string& aff_path,
	                             const std::string& ug_path = "res/ug",
	                             const std::string& acronyms_path = "res/acronyms.txt");

	// Config-enabled path-based constructor.
	explicit CompoundCorrector(const std::string& aff_path,
	                             const std::string& ug_path,
	                             const std::string& acronyms_path,
	                             const NuspellConfig& config);

	// Bundle constructor (reads a single binary blob produced by pack_resources).
	explicit CompoundCorrector(std::istream& bundle_stream);

	// Config-enabled bundle constructor.
	explicit CompoundCorrector(std::istream& bundle_stream,
	                             const NuspellConfig& config);

	~CompoundCorrector();

	// Corrects split acronyms and compound terms in a single line.
	// When fix_single is true, tokens that still fail spell() after the
	// merge pass are passed to suggest() and replaced if a matching suggestion
	// has same length and Hamming distance == 1 (case-insensitive).
	std::string Correct(const std::string& input, bool fix_single = false) const;

	// Same as Correct() but also returns per-token status codes.
	// When verbose_log is non-null, detailed debug strings are appended
	// explaining each keep / merge / fix decision.
	CorrectionResult CorrectWithStatus(const std::string& input,
	                                    bool fix_single = false,
	                                    std::vector<std::string>* verbose_log = nullptr) const;

private:
	// Internal spell-check backend (loaded once at construction).
	// Uses nuspell::Dictionary internally.
	struct Impl;
	std::unique_ptr<Impl> impl_;
};
