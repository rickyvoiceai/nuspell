#include "bundle.hxx"

#include <stdexcept>
#include <string>

using namespace bundle_io;

static constexpr std::uint32_t BUNDLE_MAGIC   = 0x424E444C; // 'BNDL' as integer
static constexpr std::uint32_t BUNDLE_VERSION   = 1;

void write_bundle(std::ostream& out, const Bundle& bundle) {
	write_u32_le(out, BUNDLE_MAGIC);
	write_u32_le(out, BUNDLE_VERSION);
	write_u32_le(out, static_cast<std::uint32_t>(bundle.entries.size()));
	for (const auto& e : bundle.entries) {
		write_u32_le(out, static_cast<std::uint32_t>(e.tag));
		write_string(out, static_cast<std::uint32_t>(e.name.size()), e.name.data());
		write_string(out, static_cast<std::uint32_t>(e.data.size()), e.data.data());
	}
}

Bundle read_bundle(std::istream& in) {
	std::uint32_t magic = read_u32_le(in);
	if (magic != BUNDLE_MAGIC) {
		throw std::runtime_error("Invalid bundle magic number");
	}
	std::uint32_t version = read_u32_le(in);
	if (version != BUNDLE_VERSION) {
		throw std::runtime_error("Unsupported bundle version");
	}
	std::uint32_t count = read_u32_le(in);
	Bundle bundle;
	bundle.entries.reserve(count);
	for (std::uint32_t i = 0; i < count; ++i) {
		BundleEntry entry;
		std::uint32_t tag = read_u32_le(in);
		entry.tag = static_cast<BundleTag>(tag);

		std::uint32_t name_len = read_u32_le(in);
		entry.name.resize(name_len);
		in.read(entry.name.data(), name_len);
		if (!in) throw std::runtime_error("Failed reading bundle entry name");

		std::uint32_t data_len = read_u32_le(in);
		entry.data.resize(data_len);
		in.read(entry.data.data(), data_len);
		if (!in) throw std::runtime_error("Failed reading bundle entry data");

		bundle.entries.push_back(std::move(entry));
	}
	return bundle;
}

const BundleEntry* Bundle::find(BundleTag tag) const {
	for (const auto& e : entries) {
		if (e.tag == tag) return &e;
	}
	return nullptr;
}
