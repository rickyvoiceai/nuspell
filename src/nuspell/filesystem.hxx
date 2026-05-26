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

#ifndef NUSPELL_FILESYSTEM_HXX
#define NUSPELL_FILESYSTEM_HXX

#include "defines.hxx"
#include "nuspell_export.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace nuspell {
NUSPELL_BEGIN_INLINE_NAMESPACE

class NUSPELL_EXPORT path {
      public:
	using string_type = std::string;
	using value_type = char;

	path() = default;
	path(const char* p) : path_(p) {}
	path(const std::string& p) : path_(p) {}
	path(std::string&& p) : path_(std::move(p)) {}

	auto operator/=(const path& p) -> path&
	{
		if (!path_.empty() && path_.back() != '/' && path_.back() != '\\')
			path_ += '/';
		path_ += p.path_;
		return *this;
	}
	auto operator+=(const path& p) -> path&
	{
		path_ += p.path_;
		return *this;
	}
	auto operator+=(const std::string& s) -> path&
	{
		path_ += s;
		return *this;
	}
	auto operator+=(const char* s) -> path&
	{
		path_ += s;
		return *this;
	}

	auto string() const -> const string_type& { return path_; }

	auto stem() const -> path;
	auto filename() const -> path;
	auto extension() const -> path;
	auto replace_extension(const path& new_ext = path()) -> path&;
	auto replace_filename(const path& new_fn) -> path&;
	auto has_stem() const -> bool;
	auto has_extension() const -> bool;

	auto clear() -> void { path_.clear(); }
	auto empty() const -> bool { return path_.empty(); }

	auto make_preferred() -> path& { return *this; }

	friend auto operator==(const path& lhs, const path& rhs) -> bool
	{
		return lhs.path_ == rhs.path_;
	}
	friend auto operator!=(const path& lhs, const path& rhs) -> bool
	{
		return lhs.path_ != rhs.path_;
	}
	friend auto operator<(const path& lhs, const path& rhs) -> bool
	{
		return lhs.path_ < rhs.path_;
	}
	friend auto operator/(const path& lhs, const path& rhs) -> path
	{
		auto result = lhs;
		result /= rhs;
		return result;
	}

      private:
	std::string path_;
};

class NUSPELL_EXPORT filesystem_error : public std::runtime_error {
      public:
	using std::runtime_error::runtime_error;
};

class NUSPELL_EXPORT directory_entry {
      public:
	directory_entry() = default;
	explicit directory_entry(const path& p) : path_(p) {}

	auto path() const -> const class path& { return path_; }
	auto is_directory() const -> bool;
	auto is_regular_file() const -> bool;

      private:
	class path path_;
};

class NUSPELL_EXPORT directory_iterator {
      public:
	using value_type = directory_entry;
	using difference_type = std::ptrdiff_t;
	using pointer = const directory_entry*;
	using reference = const directory_entry&;

	directory_iterator() noexcept = default;
	explicit directory_iterator(const path& p);
	directory_iterator(const directory_iterator&) = delete;
	directory_iterator(directory_iterator&& other) noexcept;
	~directory_iterator();
	auto operator=(const directory_iterator&) = delete;
	auto operator=(directory_iterator&& other) noexcept
	    -> directory_iterator&;

	auto operator*() const -> reference { return entry_; }
	auto operator->() const -> pointer { return &entry_; }
	auto operator++() -> directory_iterator&;
	auto increment() -> void;

	directory_iterator begin() { return std::move(*this); }
	directory_iterator end() { return {}; }

	friend auto operator==(const directory_iterator& lhs,
	                       const directory_iterator& rhs) -> bool
	{
		return lhs.impl_ == rhs.impl_;
	}
	friend auto operator!=(const directory_iterator& lhs,
	                       const directory_iterator& rhs) -> bool
	{
		return !(lhs == rhs);
	}

      private:
	struct impl;
	impl* impl_ = nullptr;
	directory_entry entry_;
};

NUSPELL_EXPORT auto exists(const path& p) -> bool;
NUSPELL_EXPORT auto is_regular_file(const path& p) -> bool;

NUSPELL_EXPORT auto
append_default_dir_paths(std::vector<path>& paths) -> void;
NUSPELL_EXPORT auto
append_libreoffice_dir_paths(std::vector<path>& paths) -> void;
NUSPELL_EXPORT auto
search_dirs_for_one_dict(const std::vector<path>& dir_paths,
                         const path& dict_name_stem) -> path;
NUSPELL_EXPORT auto
search_dirs_for_dicts(const std::vector<path>& dir_paths,
                      std::vector<path>& dict_list) -> void;
NUSPELL_EXPORT auto search_default_dirs_for_dicts()
    -> std::vector<path>;

NUSPELL_END_INLINE_NAMESPACE
} // namespace nuspell

#endif // NUSPELL_FILESYSTEM_HXX