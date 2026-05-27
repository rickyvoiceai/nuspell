#ifndef NUSPELL_FILESYSTEM_HXX
#define NUSPELL_FILESYSTEM_HXX

#include <string>

namespace nuspell {
namespace v5 {

class path {
	std::string p_;

      public:
	path() = default;
	path(const char* s) : p_(s) {}
	path(const std::string& s) : p_(s) {}
	auto string() const -> const std::string& { return p_; }
	auto replace_extension(const std::string& ext) -> path&
	{
		auto dot = p_.rfind('.');
		auto sep = p_.rfind('/');
		if (dot == std::string::npos || (sep != std::string::npos && dot < sep))
			p_ += ext;
		else
			p_.replace(dot, std::string::npos, ext);
		return *this;
	}
	auto empty() const -> bool { return p_.empty(); }
	auto stem() const -> path
	{
		auto sep = p_.rfind('/');
		auto dot = p_.rfind('.');
		auto start = (sep == std::string::npos) ? 0 : sep + 1;
		if (dot == std::string::npos || dot < start)
			return path(p_.substr(start));
		return path(p_.substr(start, dot - start));
	}
	friend bool operator<(const path& a, const path& b)  { return a.p_ < b.p_; }
	friend bool operator==(const path& a, const path& b) { return a.p_ == b.p_; }
	friend bool operator!=(const path& a, const path& b) { return a.p_ != b.p_; }
};

}
}

namespace std {
namespace filesystem = ::nuspell::v5;
}

#endif