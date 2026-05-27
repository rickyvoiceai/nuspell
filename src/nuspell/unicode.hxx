/* Copyright 2021-2024 Dimitrij Mijoski
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
#ifndef NUSPELL_UNICODE_HXX
#define NUSPELL_UNICODE_HXX
#include "defines.hxx"

#include <string>
#include "string_view.hxx"

// ============================================================
// Self-contained UTF-8 helpers (no ICU dependency)
// ============================================================

#define U8_MAX_LENGTH 4

namespace nuspell {
namespace unicode_detail {

// -- Malformed-tolerant decode: reads one codepoint, returns -1 on error --
template <class Range>
inline auto u8_decode(const Range& str, size_t& i, size_t len, int32_t& cp) -> void {
	if (i >= len) { cp = -1; return; }
	auto c = static_cast<unsigned char>(str[i++]);
	if (c < 0x80) { cp = c; return; }
	int n;
	if      ((c & 0xE0) == 0xC0) { n = 2; cp = c & 0x1F; }
	else if ((c & 0xF0) == 0xE0) { n = 3; cp = c & 0x0F; }
	else if ((c & 0xF8) == 0xF0) { n = 4; cp = c & 0x07; }
	else { cp = -1; return; }
	for (int j = 1; j < n; ++j) {
		if (i >= len) { cp = -1; return; }
		c = static_cast<unsigned char>(str[i++]);
		if ((c & 0xC0) != 0x80) { cp = -1; return; }
		cp = (cp << 6) | (c & 0x3F);
	}
	// Reject overlong encodings
	if ((n == 2 && cp < 0x80) || (n == 3 && cp < 0x800) || (n == 4 && cp < 0x10000))
		cp = -1;
}

// -- Malformed-tolerant skip-forward by one codepoint --
template <class Range>
inline auto u8_skip_fwd(const Range& str, size_t& i, size_t len) -> void {
	if (i >= len) return;
	auto c = static_cast<unsigned char>(str[i]);
	if (c < 0x80) { ++i; return; }
	if (c < 0xC0 || c > 0xF7) { ++i; return; } // invalid lead byte, skip one
	int n = (c >= 0xF0) ? 4 : (c >= 0xE0) ? 3 : 2;
	i = (i + n > len) ? len : i + n;
}

// -- Malformed-tolerant skip-backward by one codepoint --
template <class Range>
inline auto u8_skip_back(const Range& str, size_t /*start*/, size_t& i) -> void {
	while (i > 0) {
		auto c = static_cast<unsigned char>(str[--i]);
		if ((c & 0xC0) != 0x80) return;
	}
}

// -- Malformed-tolerant decode-reverse --
template <class Range>
inline auto u8_decode_rev(const Range& str, size_t /*start*/, size_t& i, int32_t& cp) -> void {
	u8_skip_back(str, 0, i);
	cp = static_cast<unsigned char>(str[i]);
	if (cp < 0x80) return; // single byte
	// Re-decode forward from this position
	size_t j = i;
	size_t len = str.size();
	int n;
	auto c = static_cast<unsigned char>(str[j++]);
	if      ((c & 0xE0) == 0xC0) { n = 2; cp = c & 0x1F; }
	else if ((c & 0xF0) == 0xE0) { n = 3; cp = c & 0x0F; }
	else if ((c & 0xF8) == 0xF0) { n = 4; cp = c & 0x07; }
	else return;
	for (int k = 1; k < n; ++k) {
		if (j >= len) { cp = -1; return; }
		c = static_cast<unsigned char>(str[j++]);
		if ((c & 0xC0) != 0x80) { cp = -1; return; }
		cp = (cp << 6) | (c & 0x3F);
	}
}

// -- Malformed-tolerant encode --
template <class Range>
inline auto u8_encode(Range& buf, size_t& i, size_t len, int32_t cp, bool& error) -> void {
	error = false;
	if (cp < 0) { error = true; return; }
	if (cp < 0x80) {
		if (i >= len) { error = true; return; }
		buf[i++] = static_cast<char>(cp);
	} else if (cp < 0x800) {
		if (i + 1 >= len) { error = true; return; }
		buf[i++] = static_cast<char>(0xC0 | (cp >> 6));
		buf[i++] = static_cast<char>(0x80 | (cp & 0x3F));
	} else if (cp < 0x10000) {
		if (i + 2 >= len) { error = true; return; }
		buf[i++] = static_cast<char>(0xE0 | (cp >> 12));
		buf[i++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		buf[i++] = static_cast<char>(0x80 | (cp & 0x3F));
	} else if (cp < 0x110000) {
		if (i + 3 >= len) { error = true; return; }
		buf[i++] = static_cast<char>(0xF0 | (cp >> 18));
		buf[i++] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
		buf[i++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		buf[i++] = static_cast<char>(0x80 | (cp & 0x3F));
	} else {
		error = true;
	}
}

// -- Unsafe (valid-UTF-8-only) decode --
inline auto u8_decode_unsafe(const char* s, size_t& i, char32_t& cp) -> void {
	auto c = static_cast<unsigned char>(s[i++]);
	if (c < 0x80) { cp = c; return; }
	if ((c & 0xE0) == 0xC0) {
		cp = c & 0x1F;
		cp = (cp << 6) | (static_cast<unsigned char>(s[i++]) & 0x3F);
	} else if ((c & 0xF0) == 0xE0) {
		cp = c & 0x0F;
		cp = (cp << 6) | (static_cast<unsigned char>(s[i++]) & 0x3F);
		cp = (cp << 6) | (static_cast<unsigned char>(s[i++]) & 0x3F);
	} else {
		cp = c & 0x07;
		cp = (cp << 6) | (static_cast<unsigned char>(s[i++]) & 0x3F);
		cp = (cp << 6) | (static_cast<unsigned char>(s[i++]) & 0x3F);
		cp = (cp << 6) | (static_cast<unsigned char>(s[i++]) & 0x3F);
	}
}

// -- Unsafe skip-forward --
inline auto u8_skip_fwd_unsafe(const char* s, size_t& i) -> void {
	auto c = static_cast<unsigned char>(s[i]);
	if (c < 0x80)       { i += 1; }
	else if (c < 0xE0)  { i += 2; }
	else if (c < 0xF0)  { i += 3; }
	else                { i += 4; }
}

// -- Unsafe skip-backward --
inline auto u8_skip_back_unsafe(const char* s, size_t& i) -> void {
	while ((static_cast<unsigned char>(s[--i]) & 0xC0) == 0x80) {}
}

// -- Unsafe decode-reverse --
inline auto u8_decode_rev_unsafe(const char* s, size_t& i, char32_t& cp) -> void {
	u8_skip_back_unsafe(s, i);
	u8_decode_unsafe(s, i, cp);
}

// -- Unsafe encode --
inline auto u8_encode_unsafe(char* buf, size_t& i, char32_t cp) -> void {
	if (cp < 0x80) {
		buf[i++] = static_cast<char>(cp);
	} else if (cp < 0x800) {
		buf[i++] = static_cast<char>(0xC0 | (cp >> 6));
		buf[i++] = static_cast<char>(0x80 | (cp & 0x3F));
	} else if (cp < 0x10000) {
		buf[i++] = static_cast<char>(0xE0 | (cp >> 12));
		buf[i++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		buf[i++] = static_cast<char>(0x80 | (cp & 0x3F));
	} else {
		buf[i++] = static_cast<char>(0xF0 | (cp >> 18));
		buf[i++] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
		buf[i++] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		buf[i++] = static_cast<char>(0x80 | (cp & 0x3F));
	}
}

// ============================================================
// UTF-16 helpers (not used by CompoundCorrector, stub/simplify)
// ============================================================

#define U16_MAX_LENGTH 2

inline bool u16_is_surrogate(int32_t cp) { return cp >= 0xD800 && cp <= 0xDFFF; }

} // namespace unicode_detail
} // namespace nuspell

namespace nuspell {
NUSPELL_BEGIN_INLINE_NAMESPACE

// UTF-8, work on malformed

constexpr auto u8_max_cp_length = U8_MAX_LENGTH;

auto inline u8_is_cp_error(int32_t cp) -> bool { return cp < 0; }

template <class Range>
auto u8_advance_cp(const Range& str, size_t& i, int32_t& cp) -> void
{
	auto len = str.size();
	unicode_detail::u8_decode(str, i, len, cp);
}

template <class Range>
auto u8_advance_index(const Range& str, size_t& i) -> void
{
	auto len = str.size();
	unicode_detail::u8_skip_fwd(str, i, len);
}

template <class Range>
auto u8_reverse_cp(const Range& str, size_t& i, int32_t& cp) -> void
{
	unicode_detail::u8_decode_rev(str, 0, i, cp);
}

template <class Range>
auto u8_reverse_index(const Range& str, size_t& i) -> void
{
	unicode_detail::u8_skip_back(str, 0, i);
}

template <class Range>
auto u8_write_cp_and_advance(Range& buf, size_t& i, int32_t cp, bool& error)
     -> void
{
	auto len = buf.size();
	unicode_detail::u8_encode(buf, i, len, cp, error);
}

// UTF-8, valid

template <class Range>
auto valid_u8_advance_cp(const Range& str, size_t& i, char32_t& cp) -> void
{
	unicode_detail::u8_decode_unsafe(str.data(), i, cp);
}

template <class Range>
auto valid_u8_advance_index(const Range& str, size_t& i) -> void
{
	unicode_detail::u8_skip_fwd_unsafe(str.data(), i);
}

template <class Range>
auto valid_u8_reverse_cp(const Range& str, size_t& i, char32_t& cp) -> void
{
	unicode_detail::u8_decode_rev_unsafe(str.data(), i, cp);
}

template <class Range>
auto valid_u8_reverse_index(const Range& str, size_t& i) -> void
{
	unicode_detail::u8_skip_back_unsafe(str.data(), i);
}

template <class Range>
auto valid_u8_write_cp_and_advance(Range& buf, size_t& i, char32_t cp) -> void
{
	unicode_detail::u8_encode_unsafe(buf.data(), i, cp);
}

// UTF-16, work on malformed

constexpr auto u16_max_cp_length = U16_MAX_LENGTH;

auto inline u16_is_cp_error(int32_t cp) -> bool { return unicode_detail::u16_is_surrogate(cp); }

// UTF-16 decode/encode stubs — not used by CompoundCorrector code path.
// Fall back to single-code-unit for unsigned shorts.

template <class Range>
auto u16_advance_cp(const Range& str, size_t& i, int32_t& cp) -> void
{
	if (i >= str.size()) { cp = -1; return; }
	cp = static_cast<int32_t>(static_cast<unsigned short>(str[i++]));
}

template <class Range>
auto u16_advance_index(const Range& str, size_t& i) -> void
{
	if (i < str.size()) ++i;
}

template <class Range>
auto u16_reverse_cp(const Range& str, size_t& i, int32_t& cp) -> void
{
	if (i == 0) { cp = -1; return; }
	cp = static_cast<int32_t>(static_cast<unsigned short>(str[--i]));
}

template <class Range>
auto u16_reverse_index(const Range& str, size_t& i) -> void
{
	if (i > 0) --i;
}

template <class Range>
auto u16_write_cp_and_advance(Range& buf, size_t& i, int32_t cp, bool& error)
     -> void
{
	error = false;
	if (i >= buf.size()) { error = true; return; }
	buf[i++] = static_cast<typename std::remove_reference<decltype(buf[0])>::type>(cp & 0xFFFF);
}

// UTF-16, valid

template <class Range>
auto valid_u16_advance_cp(const Range& str, size_t& i, char32_t& cp) -> void
{
	cp = static_cast<char32_t>(static_cast<unsigned short>(str[i++]));
}

template <class Range>
auto valid_u16_advance_index(const Range& str, size_t& i) -> void
{
	++i;
}

template <class Range>
auto valid_u16_reverse_cp(const Range& str, size_t& i, char32_t& cp) -> void
{
	cp = static_cast<char32_t>(static_cast<unsigned short>(str[--i]));
}

template <class Range>
auto valid_u16_reverse_index(const Range& str, size_t& i) -> void
{
	--i;
}

template <class Range>
auto valid_u16_write_cp_and_advance(Range& buf, size_t& i, char32_t cp) -> void
{
	buf[i++] = static_cast<typename std::remove_reference<decltype(buf[0])>::type>(cp & 0xFFFF);
}

// higher level funcs

struct U8_CP_Pos {
	size_t begin_i = 0;
	size_t end_i = begin_i;
	U8_CP_Pos() = default;
	U8_CP_Pos(size_t b, size_t e) : begin_i(b), end_i(e) {}
};

class U8_Encoded_CP {
	char d[u8_max_cp_length];
	int sz;

      public:
	U8_Encoded_CP(string_view str, U8_CP_Pos pos)
	    : sz(pos.end_i - pos.begin_i)
	{
		auto i = sz;
		auto j = pos.end_i;
		auto max_len = 4;
		do {
			d[--i] = str[--j];
		} while (i && --max_len);
	}
	U8_Encoded_CP(char32_t cp)
	{
		size_t z = 0;
		unicode_detail::u8_encode_unsafe(d, z, cp);
		sz = z;
	}
	auto size() const noexcept -> size_t { return sz; }
	auto data() const noexcept -> const char* { return d; }
	operator string_view() const noexcept
	{
		return string_view(data(), size());
	}
	operator std::string() const
	{
		return std::string(data(), size());
	}
	auto copy_to(std::string& str, size_t j) const
	{
		auto i = sz;
		j += sz;
		auto max_len = 4;
		do {
			str[--j] = d[--i];
		} while (i && --max_len);
	}
};

auto inline u8_swap_adjacent_cp(std::string& str, size_t i1, size_t i2,
                                size_t i3) -> size_t
{
	auto cp1 = U8_Encoded_CP(str, U8_CP_Pos{i1, i2});
	auto cp2 = U8_Encoded_CP(str, U8_CP_Pos{i2, i3});
	auto new_i2 = i1 + cp2.size();
	cp1.copy_to(str, new_i2);
	cp2.copy_to(str, i1);
	return new_i2;
}

auto inline u8_swap_cp(std::string& str, U8_CP_Pos pos1, U8_CP_Pos pos2)
    -> std::pair<size_t, size_t>
{
	auto cp1 = U8_Encoded_CP(str, pos1);
	auto cp2 = U8_Encoded_CP(str, pos2);
	auto new_p1_end_i = pos1.begin_i + cp2.size();
	auto new_p2_begin_i = pos2.end_i - cp1.size();
	std::char_traits<char>::move(&str[new_p1_end_i], &str[pos1.end_i],
	                             pos2.begin_i - pos1.end_i);
	cp2.copy_to(str, pos1.begin_i);
	cp1.copy_to(str, new_p2_begin_i);
	return std::make_pair(new_p1_end_i, new_p2_begin_i);
}

// below go func without out-parametars

// UTF-8, can be malformed, no out-parametars

struct Idx_And_Next_CP {
	size_t end_i;
	int32_t cp;
};

struct Idx_And_Prev_CP {
	size_t begin_i;
	int32_t cp;
};

struct Write_CP_Idx_and_Error {
	size_t end_i;
	bool error;
};

template <class Range>
NUSPELL_NODISCARD auto u8_next_cp(const Range& str, size_t i) -> Idx_And_Next_CP
{
	int32_t cp;
	u8_advance_cp(str, i, cp);
	return {i, cp};
}

template <class Range>
NUSPELL_NODISCARD auto u8_next_index(const Range& str, size_t i) -> size_t
{
	u8_advance_index(str, i);
	return i;
}

template <class Range>
NUSPELL_NODISCARD auto u8_prev_cp(const Range& str, size_t i) -> Idx_And_Prev_CP
{
	int32_t cp;
	u8_reverse_cp(str, i, cp);
	return {i, cp};
}

template <class Range>
NUSPELL_NODISCARD auto u8_prev_index(const Range& str, size_t i) -> size_t
{
	u8_reverse_index(str, i);
	return i;
}

template <class Range>
NUSPELL_NODISCARD auto u8_write_cp(Range& buf, size_t i, int32_t cp)
    -> Write_CP_Idx_and_Error
{
	bool err;
	u8_write_cp_and_advance(buf, i, cp, err);
	return {i, err};
}

// UTF-8, valid, no out-parametars

struct Idx_And_Next_CP_Valid {
	size_t end_i;
	char32_t cp;
};

struct Idx_And_Prev_CP_Valid {
	size_t begin_i;
	char32_t cp;
};

template <class Range>
NUSPELL_NODISCARD auto valid_u8_next_cp(const Range& str, size_t i)
    -> Idx_And_Next_CP_Valid
{
	char32_t cp;
	valid_u8_advance_cp(str, i, cp);
	return {i, cp};
}

template <class Range>
NUSPELL_NODISCARD auto valid_u8_next_index(const Range& str, size_t i) -> size_t
{
	valid_u8_advance_index(str, i);
	return i;
}

template <class Range>
NUSPELL_NODISCARD auto valid_u8_prev_cp(const Range& str, size_t i)
    -> Idx_And_Prev_CP_Valid
{
	char32_t cp;
	valid_u8_reverse_cp(str, i, cp);
	return {i, cp};
}

template <class Range>
NUSPELL_NODISCARD auto valid_u8_prev_index(const Range& str, size_t i) -> size_t
{
	valid_u8_reverse_index(str, i);
	return i;
}

template <class Range>
NUSPELL_NODISCARD auto valid_u8_write_cp(Range& buf, size_t i, int32_t cp) -> size_t
{
	valid_u8_write_cp_and_advance(buf, i, cp);
	return i;
}
NUSPELL_END_INLINE_NAMESPACE
} // namespace nuspell
#endif // NUSPELL_UNICODE_HXX
