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

#ifndef NUSPELL_STRING_VIEW_HXX
#define NUSPELL_STRING_VIEW_HXX

#include "defines.hxx"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iosfwd>
#include <limits>
#include <stdexcept>
#include <string>

namespace nuspell {
NUSPELL_BEGIN_INLINE_NAMESPACE

template <class CharT, class Traits = std::char_traits<CharT>>
class basic_string_view {
      public:
	using traits_type = Traits;
	using value_type = CharT;
	using pointer = const CharT*;
	using const_pointer = const CharT*;
	using reference = const CharT&;
	using const_reference = const CharT&;
	using const_iterator = const_pointer;
	using iterator = const_iterator;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using reverse_iterator = const_reverse_iterator;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	static constexpr size_type npos = size_type(-1);

      private:
	const CharT* data_ = nullptr;
	size_type size_ = 0;

      public:
	basic_string_view() noexcept = default;
	basic_string_view(const basic_string_view&) noexcept = default;
	basic_string_view& operator=(const basic_string_view&) noexcept = default;

	basic_string_view(const CharT* str, size_type len) noexcept
	    : data_(str), size_(len)
	{
	}
	basic_string_view(const CharT* str)
	    : data_(str), size_(str ? Traits::length(str) : 0)
	{
	}
	template <class Alloc>
	basic_string_view(
	    const std::basic_string<CharT, Traits, Alloc>& str) noexcept
	    : data_(str.data()), size_(str.size())
	{
	}

	auto data() const noexcept -> const_pointer { return data_; }
	auto size() const noexcept -> size_type { return size_; }
	auto length() const noexcept -> size_type { return size_; }
	auto empty() const noexcept -> bool { return size_ == 0; }

	auto operator[](size_type pos) const -> const_reference
	{
		return data_[pos];
	}
	auto at(size_type pos) const -> const_reference
	{
		if (pos >= size_)
			throw std::out_of_range("basic_string_view::at");
		return data_[pos];
	}
	auto front() const -> const_reference { return data_[0]; }
	auto back() const -> const_reference { return data_[size_ - 1]; }

	auto begin() const noexcept -> const_iterator { return data_; }
	auto end() const noexcept -> const_iterator { return data_ + size_; }
	auto cbegin() const noexcept -> const_iterator { return data_; }
	auto cend() const noexcept -> const_iterator { return data_ + size_; }
	auto rbegin() const -> const_reverse_iterator { return {end()}; }
	auto rend() const -> const_reverse_iterator { return {begin()}; }
	auto crbegin() const -> const_reverse_iterator { return {end()}; }
	auto crend() const -> const_reverse_iterator { return {begin()}; }

	void remove_prefix(size_type n) { data_ += n; size_ -= n; }
	void remove_suffix(size_type n) { size_ -= n; }

	auto substr(size_type pos = 0, size_type count = npos) const
	    -> basic_string_view
	{
		if (pos > size_)
			throw std::out_of_range("basic_string_view::substr");
		auto rcount = std::min(count, size_ - pos);
		return {data_ + pos, rcount};
	}

	operator std::basic_string<CharT, Traits>() const
	{
		return std::basic_string<CharT, Traits>(data_, size_);
	}
	auto copy(CharT* dest, size_type count, size_type pos = 0) const
	    -> size_type
	{
		if (pos > size_)
			throw std::out_of_range("basic_string_view::copy");
		auto rcount = std::min(count, size_ - pos);
		Traits::copy(dest, data_ + pos, rcount);
		return rcount;
	}

	auto compare(basic_string_view sv) const noexcept -> int
	{
		auto rlen = std::min(size_, sv.size_);
		auto cmp = Traits::compare(data_, sv.data_, rlen);
		if (cmp != 0)
			return cmp;
		if (size_ < sv.size_)
			return -1;
		if (size_ > sv.size_)
			return 1;
		return 0;
	}
	auto compare(size_type pos1, size_type count1, basic_string_view sv) const
	    -> int
	{
		return substr(pos1, count1).compare(sv);
	}
	auto compare(size_type pos1, size_type count1, basic_string_view sv,
	             size_type pos2, size_type count2) const -> int
	{
		return substr(pos1, count1).compare(sv.substr(pos2, count2));
	}
	auto compare(const CharT* s) const -> int
	{
		return compare(basic_string_view(s));
	}
	auto compare(size_type pos1, size_type count1, const CharT* s) const
	    -> int
	{
		return substr(pos1, count1).compare(basic_string_view(s));
	}
	auto compare(size_type pos1, size_type count1, const CharT* s,
	             size_type count2) const -> int
	{
		return substr(pos1, count1).compare(
		    basic_string_view(s, count2));
	}

	auto find(basic_string_view sv, size_type pos = 0) const noexcept
	    -> size_type
	{
		if (pos > size_)
			return npos;
		if (sv.empty())
			return pos;
		if (sv.size_ > size_ - pos)
			return npos;
		for (size_type i = pos; i + sv.size_ <= size_; ++i) {
			if (Traits::compare(data_ + i, sv.data_, sv.size_) == 0)
				return i;
		}
		return npos;
	}
	auto find(CharT c, size_type pos = 0) const noexcept -> size_type
	{
		return find(basic_string_view(&c, 1), pos);
	}
	auto find(const CharT* s, size_type pos, size_type count) const
	    -> size_type
	{
		return find(basic_string_view(s, count), pos);
	}
	auto find(const CharT* s, size_type pos = 0) const -> size_type
	{
		return find(basic_string_view(s), pos);
	}

	auto rfind(basic_string_view sv, size_type pos = npos) const noexcept
	    -> size_type
	{
		if (sv.size_ > size_)
			return npos;
		if (sv.empty())
			return std::min(pos, size_);
		auto start = std::min(pos, size_ - sv.size_);
		for (auto i = start + 1; i-- > 0;) {
			if (Traits::compare(data_ + i, sv.data_, sv.size_) == 0)
				return i;
		}
		return npos;
	}
	auto rfind(CharT c, size_type pos = npos) const noexcept -> size_type
	{
		return rfind(basic_string_view(&c, 1), pos);
	}
	auto rfind(const CharT* s, size_type pos, size_type count) const
	    -> size_type
	{
		return rfind(basic_string_view(s, count), pos);
	}
	auto rfind(const CharT* s, size_type pos = npos) const -> size_type
	{
		return rfind(basic_string_view(s), pos);
	}

	auto find_first_of(basic_string_view sv, size_type pos = 0) const
	    noexcept -> size_type
	{
		if (sv.empty())
			return npos;
		for (auto i = pos; i < size_; ++i) {
			if (Traits::find(sv.data_, sv.size_, data_[i]))
				return i;
		}
		return npos;
	}
	auto find_first_of(CharT c, size_type pos = 0) const noexcept -> size_type
	{
		return find_first_of(basic_string_view(&c, 1), pos);
	}
	auto find_first_of(const CharT* s, size_type pos, size_type count) const
	    -> size_type
	{
		return find_first_of(basic_string_view(s, count), pos);
	}
	auto find_first_of(const CharT* s, size_type pos = 0) const -> size_type
	{
		return find_first_of(basic_string_view(s), pos);
	}

	auto find_last_of(basic_string_view sv, size_type pos = npos) const
	    noexcept -> size_type
	{
		if (sv.empty())
			return npos;
		for (auto i = std::min(pos, size_ - 1);; --i) {
			if (Traits::find(sv.data_, sv.size_, data_[i]))
				return i;
			if (i == 0)
				break;
		}
		return npos;
	}
	auto find_last_of(CharT c, size_type pos = npos) const noexcept
	    -> size_type
	{
		return find_last_of(basic_string_view(&c, 1), pos);
	}
	auto find_last_of(const CharT* s, size_type pos, size_type count) const
	    -> size_type
	{
		return find_last_of(basic_string_view(s, count), pos);
	}
	auto find_last_of(const CharT* s, size_type pos = npos) const -> size_type
	{
		return find_last_of(basic_string_view(s), pos);
	}

	auto find_first_not_of(basic_string_view sv, size_type pos = 0) const
	    noexcept -> size_type
	{
		for (auto i = pos; i < size_; ++i) {
			if (!Traits::find(sv.data_, sv.size_, data_[i]))
				return i;
		}
		return npos;
	}
	auto find_first_not_of(CharT c, size_type pos = 0) const noexcept
	    -> size_type
	{
		return find_first_not_of(basic_string_view(&c, 1), pos);
	}
	auto find_first_not_of(const CharT* s, size_type pos,
	                       size_type count) const -> size_type
	{
		return find_first_not_of(basic_string_view(s, count), pos);
	}
	auto find_first_not_of(const CharT* s, size_type pos = 0) const
	    -> size_type
	{
		return find_first_not_of(basic_string_view(s), pos);
	}

	auto find_last_not_of(basic_string_view sv, size_type pos = npos) const
	    noexcept -> size_type
	{
		for (auto i = std::min(pos, size_ - 1);; --i) {
			if (!Traits::find(sv.data_, sv.size_, data_[i]))
				return i;
			if (i == 0)
				break;
		}
		return npos;
	}
	auto find_last_not_of(CharT c, size_type pos = npos) const noexcept
	    -> size_type
	{
		return find_last_not_of(basic_string_view(&c, 1), pos);
	}
	auto find_last_not_of(const CharT* s, size_type pos,
	                      size_type count) const -> size_type
	{
		return find_last_not_of(basic_string_view(s, count), pos);
	}
	auto find_last_not_of(const CharT* s, size_type pos = npos) const
	    -> size_type
	{
		return find_last_not_of(basic_string_view(s), pos);
	}

	friend auto operator==(basic_string_view lhs, basic_string_view rhs) noexcept
	    -> bool
	{
		return lhs.compare(rhs) == 0;
	}
	friend auto operator!=(basic_string_view lhs, basic_string_view rhs) noexcept
	    -> bool
	{
		return lhs.compare(rhs) != 0;
	}
	friend auto operator<(basic_string_view lhs, basic_string_view rhs) noexcept
	    -> bool
	{
		return lhs.compare(rhs) < 0;
	}
	friend auto operator<=(basic_string_view lhs, basic_string_view rhs) noexcept
	    -> bool
	{
		return lhs.compare(rhs) <= 0;
	}
	friend auto operator>(basic_string_view lhs, basic_string_view rhs) noexcept
	    -> bool
	{
		return lhs.compare(rhs) > 0;
	}
	friend auto operator>=(basic_string_view lhs, basic_string_view rhs) noexcept
	    -> bool
	{
		return lhs.compare(rhs) >= 0;
	}
};

using string_view = basic_string_view<char>;
using u16string_view = basic_string_view<char16_t>;
using u32string_view = basic_string_view<char32_t>;

template <class CharT, class Traits, class Alloc>
auto operator==(basic_string_view<CharT, Traits> sv,
                const std::basic_string<CharT, Traits, Alloc>& str) noexcept
    -> bool
{
	return sv.compare(str) == 0;
}
template <class CharT, class Traits, class Alloc>
auto operator==(const std::basic_string<CharT, Traits, Alloc>& str,
                basic_string_view<CharT, Traits> sv) noexcept -> bool
{
	return sv.compare(str) == 0;
}

template <class CharT, class Traits>
auto operator<<(std::basic_ostream<CharT, Traits>& os,
                basic_string_view<CharT, Traits> sv)
    -> std::basic_ostream<CharT, Traits>&
{
	return os.write(sv.data(), sv.size());
}

NUSPELL_END_INLINE_NAMESPACE
} // namespace nuspell

namespace std {
template <class CharT, class Traits>
struct hash<nuspell::v5::basic_string_view<CharT, Traits>> {
	auto operator()(nuspell::v5::basic_string_view<CharT, Traits> sv) const
	    noexcept -> size_t
	{
		auto h = size_t();
		for (auto c : sv)
			h = h * 131 + static_cast<size_t>(c);
		return h;
	}
};
} // namespace std

#endif // NUSPELL_STRING_VIEW_HXX