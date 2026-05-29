#include <nuspell/dictionary.hxx>

#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
	if (argc != 5) {
		std::cerr << "Usage: " << argv[0]
		          << " <aff_path> <dic_path> <filter_file> <output_file>\n"
		          << "\n"
		          << "Reads 'logprob word' lines from <filter_file>, checks each\n"
		          << "word against the Hunspell dictionary, and writes words NOT\n"
		          << "recognized by spell() to <output_file>.\n";
		return 1;
	}

	nuspell::Dictionary dict;
	{
		std::ifstream aff(argv[1]);
		std::ifstream dic(argv[2]);
		if (!aff || !dic) {
			std::cerr << "Error: cannot open dictionary files: "
			          << argv[1] << " / " << argv[2] << "\n";
			return 1;
		}
		dict.load_aff_dic(aff, dic);
	}
	std::cerr << "Dictionary loaded.\n";

	std::ifstream filter(argv[3]);
	if (!filter) {
		std::cerr << "Error: cannot open filter file: " << argv[3] << "\n";
		return 1;
	}

	std::ofstream out(argv[4]);
	if (!out) {
		std::cerr << "Error: cannot create output file: " << argv[4] << "\n";
		return 1;
	}

	std::string logprob, word;
	int total = 0, missing = 0;
	while (filter >> logprob >> word) {
		++total;
		if (!dict.spell(word)) {
			out << logprob << " " << word << "\n";
			++missing;
		}
	}

	std::cerr << "Done. Checked: " << total
	          << ", Not in dictionary: " << missing << "\n";
	return 0;
}