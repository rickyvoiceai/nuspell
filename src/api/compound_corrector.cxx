#include "compound_corrector.hxx"
#include "bundle.hxx"

#include <nuspell/dictionary.hxx>

#include <cctype>
#include <fstream>
#include <ios>
#include <limits>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// C++14 make_unique replacement in a detail namespace to avoid clash with std::make_unique.
template <typename T, typename... Args>
std::unique_ptr<T> make_unique_14(Args&&... args) {
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

struct CompoundCorrector::Impl {
	nuspell::Dictionary dict;
	std::unordered_map<std::string, float> unigrams;
	std::unordered_set<std::string> acronyms;
	NuspellConfig config;

	Impl() = default;

	Impl(const std::string& aff_path,
	     const std::string& ug_path,
	     const std::string& acronyms_path,
	     const NuspellConfig& cfg = NuspellConfig())
		: config(cfg) {
		dict.load_aff_dic(aff_path);
		load_unigrams(ug_path);
		load_acronyms(acronyms_path);
	}

	static bool is_upper(char c) { return std::isupper(static_cast<unsigned char>(c)); }

	static std::string to_upper(const std::string& s) {
		std::string out;
		out.reserve(s.size());
		for (char c : s) {
			out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
		}
		return out;
	}

	static bool has_chinese_utf8(const std::string& s) {
		for (size_t i = 0; i < s.size(); ++i) {
			unsigned char c = static_cast<unsigned char>(s[i]);
			if (c >= 0xE4 && c <= 0xE9 && i + 2 < s.size()) {
				unsigned char c2 = static_cast<unsigned char>(s[i + 1]);
				unsigned char c3 = static_cast<unsigned char>(s[i + 2]);
				if ((c2 & 0xC0) == 0x80 && (c3 & 0xC0) == 0x80)
					return true;
			}
			if (c >= 0xF0 && c <= 0xF4 && i + 3 < s.size()) {
				unsigned char c2 = static_cast<unsigned char>(s[i + 1]);
				unsigned char c3 = static_cast<unsigned char>(s[i + 2]);
				unsigned char c4 = static_cast<unsigned char>(s[i + 3]);
				if ((c2 & 0xC0) == 0x80 && (c3 & 0xC0) == 0x80 && (c4 & 0xC0) == 0x80)
					return true;
			}
		}
		return false;
	}

	static bool is_pure_ascii_non_letter(const std::string& s) {
		for (char ch : s) {
			unsigned char c = static_cast<unsigned char>(ch);
			if (c > 127) return false;
			if (std::isalpha(c)) return false;
		}
		return true;
	}

	static bool is_stop_word(const std::string& s) {
		static const std::unordered_set<std::string> stops = {
			"a", "i", "am", "an", "as", "at", "be", "by", "do", "go",
			"he", "if", "in", "is", "it", "me", "my", "no", "of", "oh",
			"on", "or", "so", "to", "up", "us", "we"};
		return stops.find(to_lower(s)) != stops.end();
	}

	static std::string to_lower(const std::string& s) {
		std::string out;
		out.reserve(s.size());
		for (char c : s) {
			out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
		}
		return out;
	}

	void load_unigrams_from_stream(std::istream& in) {
		bool in_1grams = false;
		std::string line;
		while (std::getline(in, line)) {
			if (line.empty()) continue;
			if (line.size() >= 9 && line.compare(0, 9, "\\1-grams:") == 0) {
				in_1grams = true;
				continue;
			}
			if (!in_1grams) continue;
			if (line.size() >= 9 && line.compare(0, 9, "\\2-grams:") == 0) break;
			if (line.size() >= 9 && line.compare(0, 9, "\\3-grams:") == 0) break;

			std::istringstream iss(line);
			float prob = 0.0f;
			std::string word;
			if (iss >> prob >> word) {
				unigrams[word] = prob;
			}
		}
	}

	void load_unigrams(const std::string& ug_path) {
		std::ifstream in(ug_path);
		if (!in) return;
		load_unigrams_from_stream(in);
	}

	void load_acronyms_from_stream(std::istream& in) {
		std::string line;
		while (std::getline(in, line)) {
			if (line.empty() || line[0] == '#') continue;
			acronyms.insert(line);
		}
	}

	void load_acronyms(const std::string& acronyms_path) {
		std::ifstream in(acronyms_path);
		if (!in) return;
		load_acronyms_from_stream(in);
	}

	bool is_known_acronym(const std::string& s) const {
		return acronyms.find(s) != acronyms.end();
	}

	float get_logprob(const std::string& s) const {
		auto it = unigrams.find(s);
		if (it != unigrams.end()) return it->second;
		it = unigrams.find(to_lower(s));
		if (it != unigrams.end()) return it->second;
		return config.arpa_unigram_floor;
	}

	static int hamming_distance_ci(const std::string& a, const std::string& b) {
		if (a.size() != b.size()) return -1;
		int diff = 0;
		for (size_t i = 0; i < a.size(); ++i) {
			unsigned char ca = static_cast<unsigned char>(a[i]);
			unsigned char cb = static_cast<unsigned char>(b[i]);
			if (std::tolower(ca) != std::tolower(cb)) ++diff;
		}
		return diff;
	}

	Optional<std::string> try_single_fix(const std::string& word,
	                                    std::vector<std::string>* verbose_log) const {
		if (word.length() < static_cast<size_t>(config.single_word_fix_min_len)) {
			if (verbose_log) verbose_log->push_back(
			    "  fix word=\"" + word + "\": too short (<" +
			    std::to_string(config.single_word_fix_min_len) + ") → SKIP");
			return Optional<std::string>();
		}
		std::vector<std::string> suggs;
		dict.suggest(word, suggs);
		if (verbose_log) {
			verbose_log->push_back(
			    "  fix word=\"" + word + "\": suggest() returned " +
			    std::to_string(suggs.size()) + " candidate(s)");
		}
		for (const auto& candidate : suggs) {
			if (candidate.length() != word.length()) {
				if (verbose_log) verbose_log->push_back(
				    "    candidate \"" + candidate + "\" len=" +
				    std::to_string(candidate.length()) + " != input len=" +
				    std::to_string(word.length()) + " → REJECT (len mismatch)");
				continue;
			}
			int hd = hamming_distance_ci(word, candidate);
			if (hd == config.hamming_distance_threshold) {
				if (verbose_log) verbose_log->push_back(
				    "    candidate \"" + candidate + "\" hamming=" +
				    std::to_string(config.hamming_distance_threshold) + " → ACCEPT");
				return Optional<std::string>(candidate);
			} else {
				if (verbose_log) verbose_log->push_back(
				    "    candidate \"" + candidate + "\" hamming=" +
				    std::to_string(hd) + " → REJECT (!=" +
				    std::to_string(config.hamming_distance_threshold) + ")");
			}
		}
		if (verbose_log) verbose_log->push_back(
		    "  no hamming-" + std::to_string(config.hamming_distance_threshold) +
		    " match → NO FIX");
		return Optional<std::string>();
	}
};

CompoundCorrector::CompoundCorrector(const std::string& aff_path,
                                     const std::string& ug_path,
                                     const std::string& acronyms_path)
    : impl_(make_unique_14<Impl>(aff_path, ug_path, acronyms_path, NuspellConfig())) {}

CompoundCorrector::CompoundCorrector(const std::string& aff_path,
                                     const std::string& ug_path,
                                     const std::string& acronyms_path,
                                     const NuspellConfig& config)
    : impl_(make_unique_14<Impl>(aff_path, ug_path, acronyms_path, config)) {}

CompoundCorrector::CompoundCorrector(std::istream& bundle_stream)
    : CompoundCorrector(bundle_stream, NuspellConfig()) {}

CompoundCorrector::CompoundCorrector(std::istream& bundle_stream,
                                     const NuspellConfig& config)
    : impl_(make_unique_14<Impl>()) {
	impl_->config = config;
	Bundle bundle = read_bundle(bundle_stream);
	const BundleEntry* aff = bundle.find(BundleTag::AFF);
	const BundleEntry* dic = bundle.find(BundleTag::DIC);
	const BundleEntry* ug  = bundle.find(BundleTag::UG);
	const BundleEntry* acr = bundle.find(BundleTag::ACRONYMS);
	if (!aff || !dic) {
		throw std::runtime_error("Bundle missing Hunspell dictionary (.aff or .dic)");
	}
	std::istringstream aff_ss(aff->data);
	std::istringstream dic_ss(dic->data);
	impl_->dict.load_aff_dic(aff_ss, dic_ss);
	if (ug) {
		std::istringstream ug_ss(ug->data);
		impl_->load_unigrams_from_stream(ug_ss);
	}
	if (acr) {
		std::istringstream acr_ss(acr->data);
		impl_->load_acronyms_from_stream(acr_ss);
	}
}

CompoundCorrector::~CompoundCorrector() = default;

std::string CompoundCorrector::Correct(const std::string& input,
                                       bool fix_single) const {
	return CorrectWithStatus(input, fix_single).text;
}

CorrectionResult CompoundCorrector::CorrectWithStatus(
    const std::string& input, bool fix_single,
    std::vector<std::string>* verbose_log) const {

	CorrectionResult result;
	std::vector<std::string> tokens;
	// Step 1 — Tokenize.
	{
		std::istringstream iss(input);
		std::string tok;
		while (iss >> tok) tokens.push_back(tok);
	}
	result.status.assign(tokens.size(), 0);

	if (verbose_log) {
		verbose_log->push_back("[tokenize] " + std::to_string(tokens.size()) +
		                       " token(s)");
		for (size_t i = 0; i < tokens.size(); ++i) {
			verbose_log->push_back("  token[" + std::to_string(i) + "] = \"" +
			                       tokens[i] + "\"");
		}
	}

	if (tokens.size() < 2) {
		if (!fix_single || tokens.empty()) {
			if (verbose_log)
				verbose_log->push_back("  single token, fix_single=false → PASS-THROUGH");
			result.text = input;
			return result;
		}
		// Single token — attempt single-word correction.
		const std::string& word = tokens[0];
		bool is_chinese = Impl::has_chinese_utf8(word);
		bool is_non_letter = Impl::is_pure_ascii_non_letter(word);
		bool spelled = impl_->dict.spell(word);
		if (verbose_log) {
			std::string reason;
			if (is_chinese) reason = "CJK/inert";
			else if (is_non_letter) reason = "pure ASCII non-letter/inert";
			else if (spelled) reason = "dict OK";
			else reason = "not in dict";
			verbose_log->push_back("  single token \"" + word +
			                       "\" fix_single=true: " + reason);
		}
		if (!is_chinese && !is_non_letter && !spelled) {
			auto fix = impl_->try_single_fix(word, verbose_log);
			if (fix.has_value()) {
				result.text = fix.value();
				result.status[0] = 2;
				if (verbose_log)
					verbose_log->push_back("  → FIX: \"" + word + "\" → \"" +
					                     result.text + "\" (status=2)");
				return result;
			}
		}
		result.text = input;
		if (verbose_log)
			verbose_log->push_back("  → NO FIX, PASS-THROUGH");
		return result;
	}

	// Step 2 — Classify each token.
	enum TokenClass { INVALID, SHORT_VALID, LONG_VALID };
	std::vector<TokenClass> classes(tokens.size());
	for (size_t i = 0; i < tokens.size(); ++i) {
		bool inert = Impl::has_chinese_utf8(tokens[i]) ||
		             Impl::is_pure_ascii_non_letter(tokens[i]);
		bool valid  = inert || impl_->dict.spell(tokens[i]);
		bool short_ = !inert && tokens[i].length() <= 2;
		if (short_)  classes[i] = SHORT_VALID;
		else if (!valid)       classes[i] = INVALID;
		else              classes[i] = LONG_VALID;
		if (verbose_log) {
			std::string cls;
			if (classes[i] == INVALID) cls = "INVALID";
			else if (classes[i] == SHORT_VALID) cls = "SHORT_VALID";
			else cls = "LONG_VALID";
			std::string reason;
			if (inert) reason = "inert (CJK/punctuation)";
			else if (short_) reason = "short (len<=2, dict check skipped)";
			else if (valid) reason = "dict OK";
			else reason = "not in dict";
			verbose_log->push_back("  class[" + std::to_string(i) +
			                       "] = " + cls + " (\"" + tokens[i] +
			                       "\") — " + reason);
		}
	}

	// Step 3 — Precompute all adjacent merge candidates.
	struct MergeInfo {
		bool valid = false;
		std::string word;
		float logprob = 0.0f;
		MergeInfo() = default;
		explicit MergeInfo(float floor) : logprob(floor) {}
	};
	std::vector<MergeInfo> merge_info;
	merge_info.reserve(tokens.size() - 1);
	for (size_t i = 0; i + 1 < tokens.size(); ++i) {
		MergeInfo info(impl_->config.arpa_unigram_floor);
		TokenClass left = classes[i];
		TokenClass right = classes[i + 1];
		bool try_merge = false;
                bool left_stop_word = Impl::is_stop_word(tokens[i]);
                bool right_stop_word = Impl::is_stop_word(tokens[i + 1]);
                bool two_stop_word = left_stop_word && right_stop_word;
		if (left == INVALID && right == INVALID)     try_merge = true;
		else if (left == INVALID && right == SHORT_VALID) try_merge = true;
		else if (left == SHORT_VALID && right == INVALID) try_merge = true;
		else if (left == SHORT_VALID && right == SHORT_VALID &&
		         (tokens[i].length() + tokens[i + 1].length() == 3 ||
		          (tokens[i].length() == 2 && tokens[i + 1].length() == 2 &&
		           impl_->is_known_acronym(Impl::to_upper(tokens[i] + tokens[i + 1])))) &&
		         !two_stop_word)
			try_merge = true;

		if (verbose_log) {
			std::string pair = "merge[" + std::to_string(i) + "+" +
			                   std::to_string(i + 1) + "] ";
			pair += "(\"" + tokens[i] + "\", \"" + tokens[i + 1] + "\") ";
			if (!try_merge) {
				pair += "try_merge=false — ";
				if (left == LONG_VALID)
					pair += "left is LONG_VALID";
				else if (right == LONG_VALID)
					pair += "right is LONG_VALID";
				else
					pair += "stop-word guard blocked";
				verbose_log->push_back("  " + pair + " → SKIP");
			} else {
				verbose_log->push_back("  " + pair + "try_merge=true");
			}
		}

		if (try_merge) {
			bool is_short_short = (left == SHORT_VALID && right == SHORT_VALID);
			bool spelled = false;
			// Normal pairs: try lowercase first.
			if (!is_short_short) {
				std::string candidate = tokens[i] + tokens[i + 1];
				if (impl_->dict.spell(candidate)) {
					info.valid = true;
					info.word = candidate;
					info.logprob = impl_->get_logprob(candidate);
					spelled = true;
					if (verbose_log) verbose_log->push_back(
					    "    candidate \"" + candidate +
					    "\" spell()=true → CANDIDATE (logprob=" +
					    std::to_string(info.logprob) + ")");
				} else if (verbose_log) {
					verbose_log->push_back(
					    "    candidate \"" + candidate +
					    "\" spell()=false → REJECT");
				}
			}
			if (!spelled) {
				std::string upper_candidate =
				    Impl::to_upper(tokens[i] + tokens[i + 1]);
				bool dict_ok = impl_->dict.spell(upper_candidate);
				bool acronym_ok = impl_->is_known_acronym(upper_candidate);
				if (verbose_log) {
					verbose_log->push_back(
					    "    candidate \"" + upper_candidate +
					    "\" dict=" + (dict_ok ? "true" : "false") +
					    " acro=" + (acronym_ok ? "true" : "false"));
					if (is_short_short) {
						float lp = acronym_ok ? impl_->config.acronym_override_logprob
						                      : impl_->get_logprob(upper_candidate);
						verbose_log->push_back(
						    "    SHORT+SHORT ARPA threshold check: lp=" +
						    std::to_string(lp) + " >= " +
						    std::to_string(impl_->config.short_short_arpa_threshold) +
						    "? " +
						    (lp >= impl_->config.short_short_arpa_threshold
						         ? "PASS" : "FAIL"));
					}
				}
				if (dict_ok || acronym_ok) {
					float lp = acronym_ok ? impl_->config.acronym_override_logprob
					                      : impl_->get_logprob(upper_candidate);
					if (!is_short_short || lp >= impl_->config.short_short_arpa_threshold) {
						info.valid = true;
						info.word = acronym_ok ? upper_candidate
						                       : (tokens[i] + tokens[i + 1]);
						info.logprob = lp;
						if (verbose_log) verbose_log->push_back(
						    "    → CANDIDATE (logprob=" + std::to_string(lp) + ")");
					} else if (verbose_log) {
						verbose_log->push_back(
						    "    → REJECT (SHORT+SHORT threshold not met)");
					}
				} else if (verbose_log) {
					verbose_log->push_back(
					    "    → REJECT (neither dict nor acronym matches)");
				}
			}
		}
		merge_info.push_back(info);
	}

	// Step 4 — Greedy 3-token lookahead scan.
	std::vector<std::string> merged;
	std::vector<size_t> orig_start;
	merged.reserve(tokens.size());
	orig_start.reserve(tokens.size());
	for (size_t i = 0; i < tokens.size();) {
		bool c1_valid = (i + 1 < tokens.size()) && merge_info[i].valid;
		bool c2_valid = (i + 2 < tokens.size()) && merge_info[i + 1].valid;
		if (verbose_log) {
			verbose_log->push_back(
			    "  lookahead at token[" + std::to_string(i) +
			    "] c1=" + (c1_valid ? "true" : "false") +
			    " c2=" + (c2_valid ? "true" : "false"));
		}

		if (c1_valid && c2_valid) {
			bool left_wins = merge_info[i].logprob >= merge_info[i + 1].logprob;
			if (verbose_log) {
				verbose_log->push_back(
				    "    compare: lp[" + std::to_string(i) + "]=" +
				    std::to_string(merge_info[i].logprob) +
				    " vs lp[" + std::to_string(i + 1) + "]=" +
				    std::to_string(merge_info[i + 1].logprob) + " → " +
				    (left_wins ? "merge left" : "skip left, advance"));
			}
			if (left_wins) {
				merged.push_back(merge_info[i].word);
				orig_start.push_back(i);
				result.status[i] = 1;
				result.status[i + 1] = -1;
				i += 2;
			} else {
				merged.push_back(tokens[i]);
				orig_start.push_back(i);
				++i;
			}
		} else if (c1_valid) {
			if (verbose_log) verbose_log->push_back(
			    "    → MERGE tokens[" + std::to_string(i) + "+" +
			    std::to_string(i + 1) + "] into \"" + merge_info[i].word + "\"");
			merged.push_back(merge_info[i].word);
			orig_start.push_back(i);
			result.status[i] = 1;
			result.status[i + 1] = -1;
			i += 2;
		} else {
			if (verbose_log) verbose_log->push_back(
			    "    → KEEP token[" + std::to_string(i) + "] \"" + tokens[i] + "\" (unchanged)");
			merged.push_back(tokens[i]);
			orig_start.push_back(i);
			++i;
		}
	}

	// Step 5 — Optional single-word correction pass.
	if (fix_single) {
		if (verbose_log) verbose_log->push_back("[fix_single pass]");
		for (size_t k = 0; k < merged.size(); ++k) {
			std::string& word = merged[k];
			if (Impl::has_chinese_utf8(word)) {
				if (verbose_log) verbose_log->push_back(
				    "  word[\"" + word + "\"] CJK/inert → SKIP");
				continue;
			}
			if (Impl::is_pure_ascii_non_letter(word)) {
				if (verbose_log) verbose_log->push_back(
				    "  word[\"" + word + "\"] pure ASCII non-letter → SKIP");
				continue;
			}
			if (impl_->dict.spell(word)) {
				if (verbose_log) verbose_log->push_back(
				    "  word[\"" + word + "\"] dict OK → SKIP");
				continue;
			}
			auto fix = impl_->try_single_fix(word, verbose_log);
			if (fix.has_value()) {
				word = fix.value();
				result.status[orig_start[k]] = 2;
				if (verbose_log) verbose_log->push_back(
				    "  → FIX applied \"" + word + "\" (status=2)");
			} else {
				if (verbose_log) verbose_log->push_back(
				    "  → NO FIX for \"" + word + "\" (unchanged)");
			}
		}
	}

	// Step 6 — Reconstruct.
	result.text.clear();
	for (size_t i = 0; i < merged.size(); ++i) {
		if (i > 0) result.text.push_back(' ');
		result.text += merged[i];
	}

	if (verbose_log) {
		verbose_log->push_back("[result] \"" + input + "\" → \"" +
		                       result.text + "\"");
		for (size_t i = 0; i < result.status.size(); ++i) {
			std::string label;
			if (result.status[i] == 1) label = "merged-with-next";
			else if (result.status[i] == -1) label = "merged-by-prev";
			else if (result.status[i] == 2) label = "substituted";
			else label = "unchanged";
			verbose_log->push_back(
			    "  status[" + std::to_string(i) + "]=" +
			    std::to_string(result.status[i]) + " (" + label + ")");
		}
	}
	return result;
}
