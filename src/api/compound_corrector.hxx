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

	// Bundle constructor (reads a single binary blob produced by pack_resources).
	explicit CompoundCorrector(std::istream& bundle_stream);

	~CompoundCorrector();

	// Corrects split acronyms and compound terms in a single line.
	// When fix_single is true, tokens that still fail spell() after the
	// merge pass are passed to suggest() and replaced if a matching suggestion
	// has same length and Hamming distance == 1 (case-insensitive).
	std::string Correct(const std::string& input, bool fix_single = false) const;

	// Same as Correct() but also returns per-token status codes.
	CorrectionResult CorrectWithStatus(const std::string& input,
	                                    bool fix_single = false) const;

private:
	// Internal spell-check backend (loaded once at construction).
	// Uses nuspell::Dictionary internally.
	struct Impl;
	std::unique_ptr<Impl> impl_;
};
