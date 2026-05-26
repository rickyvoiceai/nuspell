#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

enum class BundleTag : std::uint32_t {
	AFF       = 0,
	DIC       = 1,
	UG        = 2,
	ACRONYMS  = 3
};

struct BundleEntry {
	BundleTag tag;
	std::string name;
	std::string data;   // raw bytes
};

struct Bundle {
	std::vector<BundleEntry> entries;

	// Lookup by tag; returns nullptr if not found.
	// For tags that may appear multiple times, returns the first match.
	const BundleEntry* find(BundleTag tag) const;
};

// Explicit little-endian helpers.
namespace bundle_io {

inline void write_u32_le(std::ostream& out, std::uint32_t val) {
	char buf[4] = {
		static_cast<char>(val & 0xFF),
		static_cast<char>((val >> 8)  & 0xFF),
		static_cast<char>((val >> 16) & 0xFF),
		static_cast<char>((val >> 24) & 0xFF)
	};
	out.write(buf, 4);
}

inline std::uint32_t read_u32_le(std::istream& in) {
	char buf[4];
	in.read(buf, 4);
	if (!in) return 0;
	return (static_cast<std::uint32_t>(
	           static_cast<unsigned char>(buf[0])) | 
	        (static_cast<std::uint32_t>(
	           static_cast<unsigned char>(buf[1])) << 8) |
	        (static_cast<std::uint32_t>(
	           static_cast<unsigned char>(buf[2])) << 16) |
	        (static_cast<std::uint32_t>(
	           static_cast<unsigned char>(buf[3])) << 24));
}

inline void write_string(std::ostream& out, std::uint32_t len, const void* data) {
	write_u32_le(out, len);
	out.write(static_cast<const char*>(data), len);
}

} // namespace bundle_io

// Serialize bundle.
void write_bundle(std::ostream& out, const Bundle& bundle);

// Deserialize bundle.
Bundle read_bundle(std::istream& in);
