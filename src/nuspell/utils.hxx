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

#ifndef NUSPELL_UTILS_HXX
#define NUSPELL_UTILS_HXX

#include "defines.hxx"

#include <string>
#include "string_view.hxx"
#include <vector>


#ifdef __GNUC__
#define likely(expr) __builtin_expect(!!(expr), 1)
#define unlikely(expr) __builtin_expect(!!(expr), 0)
#else
#define likely(expr) (expr)
#define unlikely(expr) (expr)
#endif


namespace nuspell {
NUSPELL_BEGIN_INLINE_NAMESPACE

NUSPELL_DEPRECATED_EXPORT auto split_on_any_of(string_view s,
                                                const char* sep,
                                                std::vector<std::string>& out)
    -> std::vector<std::string>&;

NUSPELL_EXPORT auto utf32_to_utf8(u32string_view in, std::string& out)
    -> void;
NUSPELL_EXPORT auto utf32_to_utf8(u32string_view in) -> std::string;

auto valid_utf8_to_32(string_view in, std::u32string& out) -> void;
auto valid_utf8_to_32(string_view in) -> std::u32string;

auto utf8_to_16(string_view in) -> std::u16string;
auto utf8_to_16(string_view in, std::u16string& out) -> bool;

auto validate_utf8(string_view s) -> bool;

NUSPELL_EXPORT auto is_all_ascii(string_view s) -> bool;

NUSPELL_EXPORT auto latin1_to_ucs2(string_view s) -> std::u16string;
auto latin1_to_ucs2(string_view s, std::u16string& out) -> void;

NUSPELL_EXPORT auto is_all_bmp(u16string_view s) -> bool;

auto to_upper_ascii(std::string& s) -> void;

NUSPELL_NODISCARD NUSPELL_EXPORT auto to_upper(string_view in)
    -> std::string;
NUSPELL_NODISCARD NUSPELL_EXPORT auto to_title(string_view in)
    -> std::string;
NUSPELL_NODISCARD NUSPELL_EXPORT auto to_lower(string_view in)
    -> std::string;

auto to_upper(string_view in, std::string& out) -> void;
auto to_title(string_view in, std::string& out) -> void;
auto to_lower(u32string_view in, std::u32string& out) -> void;
auto to_lower(string_view in, std::string& out) -> void;
auto to_lower_char_at(std::string& s, size_t i) -> void;
auto to_title_char_at(std::string& s, size_t i) -> void;

/**
 * @internal
 * @brief Enum that identifies the casing type of a word.
 *
 * Neutral characters like numbers are ignored, so "abc" and "abc123abc" are
 * both classified as small.
 */
enum class Casing : char {
	SMALL,
	INIT_CAPITAL,
	ALL_CAPITAL,
	CAMEL /**< @internal camelCase i.e. mixed case with first small */,
	PASCAL /**< @internal  PascalCase i.e. mixed case with first capital */
};

NUSPELL_EXPORT auto classify_casing(string_view s) -> Casing;

auto has_uppercase_at_compound_word_boundary(string_view word, size_t i)
    -> bool;

// Encoding_Converter removed — always UTF-8.
// Stub: pass-through converter for API compatibility.
class Encoding_Converter {
      public:
	Encoding_Converter() = default;
	explicit Encoding_Converter(const char* /*enc*/) {}
	explicit Encoding_Converter(const std::string& /*enc*/) {}
	~Encoding_Converter() {}
	auto to_utf8(string_view in, std::string& out) -> bool { out = in; return true; }
	auto valid() -> bool { return true; }
};

auto replace_ascii_char(std::string& s, char from, char to) -> void;
auto erase_chars(std::string& s, string_view erase_chars) -> void;
NUSPELL_EXPORT auto is_number(string_view s) -> bool;
auto count_appereances_of(string_view haystack, string_view needles)
    -> size_t;

auto inline begins_with(string_view haystack, string_view needle)
    -> bool
{
	return haystack.compare(0, needle.size(), needle) == 0;
}

auto inline ends_with(string_view haystack, string_view needle)
    -> bool
{
	return haystack.size() >= needle.size() &&
	       haystack.compare(haystack.size() - needle.size(), needle.size(),
	                        needle) == 0;
}

template <class T>
auto begin_ptr(T& x) -> decltype(x.data())
{
	return x.data();
}
template <class T>
auto end_ptr(T& x) -> decltype(x.data() + x.size())
{
	return x.data() + x.size();
}
inline auto begin_ptr(std::string& x) -> char*
{
	return &x[0];
}
inline auto end_ptr(std::string& x) -> char*
{
	return &x[0] + x.size();
}
NUSPELL_END_INLINE_NAMESPACE
} // namespace nuspell
#endif // NUSPELL_UTILS_HXX
