#include "bundle.hxx"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sstream>

// C++14-compatible path helpers (avoids <filesystem>).
static std::string join_path(const std::string& a, const std::string& b) {
	if (a.empty()) return b;
	if (b.empty()) return a;
	if (a.back() == '/' || a.back() == '\\')
		return a + b;
	return a + '/' + b;
}

static bool file_exists(const std::string& path) {
	std::ifstream in(path.c_str(), std::ios::binary);
	return in.good();
}

static std::size_t file_size(const std::string& path) {
	std::ifstream in(path.c_str(), std::ios::binary | std::ios::ate);
	if (!in) return 0;
	return static_cast<std::size_t>(in.tellg());
}

int main(int argc, char* argv[]) {
	std::string in_dir = "res";
	std::string out_file = "res.bundle";
	if (argc >= 2) {
		in_dir = argv[1];
	}
	if (argc >= 3) {
		out_file = argv[2];
	}

	// Map filenames to bundle tags.
	struct FileMapping {
		const char* filename;
		BundleTag tag;
	};
	FileMapping mappings[] = {
		{"en_US.aff",    BundleTag::AFF},
		{"en_US.dic",    BundleTag::DIC},
		{"ug",           BundleTag::UG},
		{"acronyms.txt", BundleTag::ACRONYMS},
	};

	Bundle bundle;
	for (const auto& m : mappings) {
		std::string p = join_path(in_dir, m.filename);
		if (!file_exists(p)) {
			std::fprintf(stderr, "Error: required file not found: %s\n", p.c_str());
			return 1;
		}
		BundleEntry entry;
		entry.tag  = m.tag;
		entry.name = m.filename;
		std::ifstream in(p, std::ios::binary);
		if (!in) {
			std::fprintf(stderr, "Error: can not open: %s\n", p.c_str());
			return 1;
		}
		auto sz = file_size(p);
		entry.data.resize(sz);
		in.read(entry.data.data(), sz);
		bundle.entries.push_back(std::move(entry));
		std::printf("Packed %s (%zu bytes)\n", m.filename, sz);
	}

	std::ofstream out(out_file, std::ios::binary);
	if (!out) {
		std::fprintf(stderr, "Error: can not create: %s\n", out_file.c_str());
		return 1;
	}
	write_bundle(out, bundle);
	std::printf("Wrote %s (%zu entries)\n", out_file.c_str(), bundle.entries.size());
	return 0;
}
