/* Copyright 2016-2024 Dimitrij Mijoski
 *
 * This file is part of Nuspell.
 *
 * Nuspell is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nuspell is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nuspell.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "filesystem.hxx"
#include "utils.hxx"

#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <set>
#include <sys/stat.h>

#if __has_include(<unistd.h>)
#include <unistd.h>
#endif
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

using namespace std;

namespace nuspell {
NUSPELL_BEGIN_INLINE_NAMESPACE

auto path::filename() const -> path
{
	auto& s = path_;
	auto pos = s.find_last_of("/\\");
	if (pos == s.npos)
		return s.empty() ? path() : s;
	return s.substr(pos + 1);
}

auto path::replace_filename(const path& new_fn) -> path&
{
	auto& s = path_;
	auto pos = s.find_last_of("/\\");
	if (pos != s.npos)
		s.erase(pos + 1);
	else
		s.clear();
	s += new_fn.path_;
	return *this;
}

auto path::stem() const -> path
{
	auto& s = path_;
	auto pos = s.find_last_of("/\\");
	auto fname = (pos == s.npos) ? s : s.substr(pos + 1);
	auto dot = fname.find_last_of('.');
	if (dot == fname.npos)
		return fname;
	return fname.substr(0, dot);
}

auto path::extension() const -> path
{
	auto& s = path_;
	auto dot = s.find_last_of('.');
	auto sep = s.find_last_of("/\\");
	if (dot == s.npos || (sep != s.npos && dot < sep))
		return path();
	return s.substr(dot);
}

auto path::replace_extension(const path& new_ext) -> path&
{
	auto& s = path_;
	auto dot = s.find_last_of('.');
	auto sep = s.find_last_of("/\\");
	if (dot != s.npos && (sep == s.npos || dot > sep))
		s.erase(dot);
	if (!new_ext.empty()) {
		if (!new_ext.path_.empty() && new_ext.path_[0] != '.')
			s += '.';
		s += new_ext.path_;
	}
	return *this;
}

auto path::has_stem() const -> bool { return !stem().empty(); }

auto path::has_extension() const -> bool { return !extension().empty(); }

auto directory_entry::is_directory() const -> bool
{
	struct stat st;
	if (stat(path_.string().c_str(), &st) != 0)
		return false;
	return S_ISDIR(st.st_mode);
}

auto directory_entry::is_regular_file() const -> bool
{
	return nuspell::v5::is_regular_file(path_);
}

auto exists(const path& p) -> bool
{
	struct stat st;
	return stat(p.string().c_str(), &st) == 0;
}

auto is_regular_file(const path& p) -> bool
{
	struct stat st;
	if (stat(p.string().c_str(), &st) != 0)
		return false;
	return S_ISREG(st.st_mode);
}

struct directory_iterator::impl {
	string base_path;
	DIR* dir = nullptr;

	impl(const string& p) : base_path(p) {}
};

directory_iterator::directory_iterator(const path& p) : impl_(new impl(p.string()))
{
	impl_->dir = opendir(impl_->base_path.c_str());
	if (!impl_->dir) {
		delete impl_;
		impl_ = nullptr;
		return;
	}
	++*this;
}

directory_iterator::directory_iterator(directory_iterator&& other) noexcept
    : impl_(other.impl_), entry_(std::move(other.entry_))
{
	other.impl_ = nullptr;
}

directory_iterator::~directory_iterator()
{
	if (impl_) {
		if (impl_->dir)
			closedir(impl_->dir);
		delete impl_;
	}
}

auto directory_iterator::operator=(directory_iterator&& other) noexcept
    -> directory_iterator&
{
	if (this != &other) {
		if (impl_) {
			if (impl_->dir)
				closedir(impl_->dir);
			delete impl_;
		}
		impl_ = other.impl_;
		entry_ = std::move(other.entry_);
		other.impl_ = nullptr;
	}
	return *this;
}

auto directory_iterator::increment() -> void
{
	if (!impl_ || !impl_->dir)
		return;
	for (;;) {
		errno = 0;
		auto ent = readdir(impl_->dir);
		if (!ent) {
			closedir(impl_->dir);
			impl_->dir = nullptr;
			delete impl_;
			impl_ = nullptr;
			return;
		}
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
			continue;
		path p = impl_->base_path;
		p /= ent->d_name;
		entry_ = directory_entry(p);
		return;
	}
}

auto directory_iterator::operator++() -> directory_iterator&
{
	increment();
	return *this;
}

NUSPELL_END_INLINE_NAMESPACE
} // namespace nuspell