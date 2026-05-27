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

#include "utils.hxx"
#include "unicode.hxx"

#include <algorithm>
#include <cctype>
#include <locale>

#if ' ' != 32 || '.' != 46 || 'A' != 65 || 'Z' != 90 || 'a' != 97 || 'z' != 122
#error "Basic execution character set is not ASCII"
#endif

using namespace std;

namespace nuspell {
NUSPELL_BEGIN_INLINE_NAMESPACE

auto split_on_any_of(string_view, const char*,
                     std::vector<std::string>& out) -> std::vector<std::string>&
{
	return out;
}

auto utf32_to_utf8(u32string_view in, std::string& out) -> void
{
	out.clear();
	for (size_t i = 0; i != in.size(); ++i) {
		auto cp = in[i];
		auto enc_cp = U8_Encoded_CP(cp);
		out += enc_cp;
	}
}
auto utf32_to_utf8(u32string_view in) -> std::string
{
	auto out = string();
	utf32_to_utf8(in, out);
	return out;
}

auto valid_utf8_to_32(string_view in, std::u32string& out) -> void
{
	out.clear();
	for (size_t i = 0; i != in.size();) {
		char32_t cp;
		valid_u8_advance_cp(in, i, cp);
		out.push_back(cp);
	}
}
auto valid_utf8_to_32(string_view in) -> std::u32string
{
	auto out = u32string();
	valid_utf8_to_32(in, out);
	return out;
}

// UTF-8 to UTF-16 conversion (simple, non-surrogate-pair for BMP only)
auto utf8_to_16(string_view in) -> std::u16string
{
	auto out = u16string();
	utf8_to_16(in, out);
	return out;
}

bool utf8_to_16(string_view in, std::u16string& out)
{
	out.clear();
	for (size_t i = 0; i < in.size();) {
		char32_t cp;
		valid_u8_advance_cp(in, i, cp);
		if (cp > 0xFFFF) cp = 0xFFFD; // replacement char for non-BMP
		out.push_back(static_cast<char16_t>(cp));
	}
	return true;
}

// Simple UTF-8 validation: just check that all bytes are valid UTF-8 sequences.
bool validate_utf8(string_view s)
{
	for (size_t i = 0; i < s.size();) {
		auto c = static_cast<unsigned char>(s[i]);
		int n;
		if (c < 0x80)       { n = 1; }
		else if (c < 0xC0)  { return false; }
		else if (c < 0xE0)  { n = 2; }
		else if (c < 0xF0)  { n = 3; }
		else if (c < 0xF8)  { n = 4; }
		else                { return false; }
		for (int j = 1; j < n; ++j) {
			if (i + j >= s.size()) return false;
			auto t = static_cast<unsigned char>(s[i + j]);
			if ((t & 0xC0) != 0x80) return false;
		}
		i += n;
	}
	return true;
}

auto static is_ascii(char c) -> bool
{
	return static_cast<unsigned char>(c) <= 127;
}

auto is_all_ascii(string_view s) -> bool
{
	return all_of(begin(s), end(s), is_ascii);
}

auto static widen_latin1(char c) -> char16_t
{
	return static_cast<unsigned char>(c);
}

auto latin1_to_ucs2(string_view s) -> std::u16string
{
	u16string ret;
	latin1_to_ucs2(s, ret);
	return ret;
}
auto latin1_to_ucs2(string_view s, std::u16string& out) -> void
{
	out.resize(s.size());
	transform(begin(s), end(s), begin(out), widen_latin1);
}

auto static is_surrogate_pair(char16_t c) -> bool
{
	return 0xD800 <= c && c <= 0xDFFF;
}
auto is_all_bmp(u16string_view s) -> bool
{
	return none_of(begin(s), end(s), is_surrogate_pair);
}

auto to_upper_ascii(std::string& s) -> void
{
	auto& char_type = use_facet<ctype<char>>(locale::classic());
	char_type.toupper(begin_ptr(s), end_ptr(s));
}

// --- ASCII-only case folding (replaces ICU) ---

auto inline ascii_to_lower(std::string& s) -> void {
	for (auto& c : s) {
		if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
	}
}

auto inline ascii_to_upper(std::string& s) -> void {
	for (auto& c : s) {
		if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
	}
}

auto inline ascii_to_title(std::string& s) -> void {
	if (!s.empty() && s[0] >= 'a' && s[0] <= 'z')
		s[0] = static_cast<char>(s[0] - 'a' + 'A');
	for (size_t i = 1; i < s.size(); ++i) {
		if (s[i] >= 'A' && s[i] <= 'Z')
			s[i] = static_cast<char>(s[i] - 'A' + 'a');
	}
}

auto to_upper(string_view in) -> std::string
{
	auto out = std::string(in);
	ascii_to_upper(out);
	return out;
}
auto to_title(string_view in) -> std::string
{
	auto out = std::string(in);
	ascii_to_title(out);
	return out;
}
auto to_lower(string_view in) -> std::string
{
	auto out = std::string(in);
	ascii_to_lower(out);
	return out;
}

auto to_upper(string_view in, string& out) -> void
{
	out = in;
	ascii_to_upper(out);
}
auto to_title(string_view in, string& out) -> void
{
	out = in;
	ascii_to_title(out);
}
auto to_lower(u32string_view in, u32string& out) -> void
{
	out = in;
	for (auto& c : out) {
		if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
	}
}
auto to_lower(string_view in, string& out) -> void
{
	out = in;
	ascii_to_lower(out);
}

auto to_lower_char_at(std::string& s, size_t i) -> void
{
	if (i < s.size() && s[i] >= 'A' && s[i] <= 'Z')
		s[i] = static_cast<char>(s[i] - 'A' + 'a');
}
auto to_title_char_at(std::string& s, size_t i) -> void
{
	if (i < s.size() && s[i] >= 'a' && s[i] <= 'z')
		s[i] = static_cast<char>(s[i] - 'a' + 'A');
}

auto classify_casing(string_view s) -> Casing
{
	size_t upper = 0;
	size_t lower = 0;
	for (size_t i = 0; i != s.size();) {
		char32_t c;
		valid_u8_advance_cp(s, i, c);
		if (c >= 'A' && c <= 'Z')
			upper++;
		else if (c >= 'a' && c <= 'z')
			lower++;
		// else neutral
	}
	if (upper == 0)
		return Casing::SMALL;

	auto first_cp = valid_u8_next_cp(s, 0);
	auto first_capital = (first_cp.cp >= 'A' && first_cp.cp <= 'Z');
	if (first_capital && upper == 1)
		return Casing::INIT_CAPITAL;

	if (lower == 0)
		return Casing::ALL_CAPITAL;

	if (first_capital)
		return Casing::PASCAL;
	else
		return Casing::CAMEL;
}

auto has_uppercase_at_compound_word_boundary(string_view word, size_t i) -> bool
{
	auto cp = valid_u8_next_cp(word, i);
	auto cp_prev = valid_u8_prev_cp(word, i);
	bool upper_cp = (cp.cp >= 'A' && cp.cp <= 'Z');
	bool alpha_cp = ((cp.cp >= 'A' && cp.cp <= 'Z') || (cp.cp >= 'a' && cp.cp <= 'z'));
	bool upper_prev = (cp_prev.cp >= 'A' && cp_prev.cp <= 'Z');
	bool alpha_prev = ((cp_prev.cp >= 'A' && cp_prev.cp <= 'Z') || (cp_prev.cp >= 'a' && cp_prev.cp <= 'z'));
	if (upper_cp) {
		if (alpha_prev) return true;
	}
	else if (upper_prev && alpha_cp)
		return true;
	return false;
}

auto replace_ascii_char(string& s, char from, char to) -> void
{
	for (auto i = s.find(from); i != s.npos; i = s.find(from, i + 1)) {
		s[i] = to;
	}
}

auto erase_chars(string& s, string_view erase_chars) -> void
{
	if (erase_chars.empty())
		return;
	for (size_t i = 0, next_i = 0; i != s.size(); i = next_i) {
		valid_u8_advance_index(s, next_i);
		auto enc_cp = string_view(&s[i], next_i - i);
		if (erase_chars.find(enc_cp) != erase_chars.npos) {
			s.erase(i, next_i - i);
			next_i = i;
		}
	}
	return;
}

auto is_number(string_view s) -> bool
{
	if (s.empty())
		return false;

	auto it = begin(s);
	if (s[0] == '-')
		++it;
	while (it != end(s)) {
		auto next = std::find_if(
		    it, end(s), [](auto c) { return c < '0' || c > '9'; });
		if (next == it)
			return false;
		if (next == end(s))
			return true;
		it = next;
		auto c = *it;
		if (c == '.' || c == ',' || c == '-')
			++it;
		else
			return false;
	}
	return false;
}

auto count_appereances_of(string_view haystack, string_view needles) -> size_t
{
	auto ret = size_t(0);
	for (size_t i = 0, next_i = 0; i != haystack.size(); i = next_i) {
		valid_u8_advance_index(haystack, next_i);
		auto enc_cp = string_view(&haystack[i], next_i - i);
		ret += needles.find(enc_cp) != needles.npos;
	}
	return ret;
}

NUSPELL_END_INLINE_NAMESPACE
} // namespace nuspell
