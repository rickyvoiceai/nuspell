#include <nuspell/dictionary.hxx>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct Entry {
	float logprob;
	std::string word;
};

static std::string strip_flags(const std::string& line) {
	auto pos = line.find('/');
	return pos == std::string::npos ? line : line.substr(0, pos);
}

static bool is_prefix_of_dic_word(const std::string& word,
                                   const std::vector<std::string>& dic_words) {
	auto it = std::lower_bound(dic_words.begin(), dic_words.end(), word);
	if (it != dic_words.end() && it->compare(0, word.size(), word) == 0) {
		if (it->size() > word.size() && it->size() <= word.size() + 2)
			return true;
	}
	if (it != dic_words.begin()) {
		--it;
		if (it->compare(0, word.size(), word) == 0) {
			if (it->size() > word.size() && it->size() <= word.size() + 2)
				return true;
		}
	}
	return false;
}

int main(int argc, char** argv) {
	if (argc != 6) {
		std::cerr << "Usage: " << argv[0]
		          << " <aff> <dic> <filter.not_exist> <out.good> <out.suspicious>\n"
		          << "\n"
		          << "Classifies words as 'good' (genuinely missing)\n"
		          << "or 'suspicious' (likely typo/truncation).\n";
		return 1;
	}

	nuspell::Dictionary dict;
	{
		std::ifstream aff(argv[1]);
		std::ifstream dic(argv[2]);
		if (!aff || !dic) {
			std::cerr << "Error: cannot open dictionary files\n";
			return 1;
		}
		dict.load_aff_dic(aff, dic);
	}
	std::cerr << "Dictionary loaded.\n";

	std::vector<std::string> dic_words;
	{
		std::ifstream dic(argv[2]);
		std::string line;
		std::getline(dic, line); // skip count
		while (std::getline(dic, line)) {
			if (line.empty()) continue;
			auto w = strip_flags(line);
			if (!w.empty())
				dic_words.push_back(w);
		}
	}
	std::sort(dic_words.begin(), dic_words.end());
	std::cerr << "Loaded " << dic_words.size() << " base dic words.\n";

	std::ifstream filter(argv[3]);
	if (!filter) {
		std::cerr << "Error: cannot open " << argv[3] << "\n";
		return 1;
	}

	std::ofstream out_good(argv[4]);
	if (!out_good) {
		std::cerr << "Error: cannot create " << argv[4] << "\n";
		return 1;
	}

	std::ofstream out_sus(argv[5]);
	if (!out_sus) {
		std::cerr << "Error: cannot create " << argv[5] << "\n";
		return 1;
	}

	std::string logprob, word;
	int total = 0, good = 0, suspicious = 0;

	while (filter >> logprob >> word) {
		++total;

		bool is_suspicious = false;

		// Check 1: prefix of a dictionary word (within +2 chars)
		if (is_prefix_of_dic_word(word, dic_words)) {
			is_suspicious = true;
		}

		// Check 2: suggest() returns candidates that are 1-2 chars longer
		if (!is_suspicious) {
			std::vector<std::string> suggs;
			dict.suggest(word, suggs);
			for (const auto& s : suggs) {
				auto diff = static_cast<int>(s.size()) - static_cast<int>(word.size());
				if (diff >= 1 && diff <= 2) {
					// Check if suggestion starts with the word
					if (s.compare(0, word.size(), word) == 0) {
						is_suspicious = true;
						break;
					}
				}
			}
		}

		// Check 3: if suggestion differs by only 1 char at the end
		// (e.g., "accordin" -> "according" adds 'g')
		if (!is_suspicious) {
			std::vector<std::string> suggs;
			dict.suggest(word, suggs);
			for (const auto& s : suggs) {
				if (s.size() == word.size() + 1) {
					// s = word + last_char, OR s has 1 insertion
					if (s.compare(0, word.size(), word) == 0) {
						is_suspicious = true;
						break;
					}
				}
			}
		}

		if (is_suspicious) {
			out_sus << logprob << " " << word << "\n";
			++suspicious;
		} else {
			out_good << logprob << " " << word << "\n";
			++good;
		}
	}

	std::cerr << "Done. Total: " << total
	          << ", Good: " << good
	          << ", Suspicious: " << suspicious << "\n";
	return 0;
}