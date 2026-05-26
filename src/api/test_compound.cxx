#include "compound_corrector.hxx"

#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <vector>

static void print_usage(const char* prog) {
	std::cerr << "Usage:\n"
	          << "  " << prog << " [-d dict.aff] [--fix-single] [--verbose] [input.txt]\n"
	          << "  " << prog << " [-b bundle] [--fix-single] [--verbose] [input.txt]\n"
	          << "  " << prog << " -t [-d dict.aff|--b bundle] [--fix-single]\n"
	          << "  " << prog << " --test-status [-d dict.aff|--b bundle]\n"
	          << "  " << prog << " -h\n"
	          << "\n"
	          << "Options:\n"
	          << "  -d, --dictionary PATH  Path to Hunspell .aff file (default: res/en_US.aff)\n"
	          << "  -b, --bundle PATH      Use a packed resource bundle instead of loose files\n"
	          << "  -t, --self-test        Run built-in self-test (res/test_file.txt)\n"
	          << "  --test-status          Run status-code self-test (res/test_status.txt)\n"
	          << "  --fix-single           Also apply single-word spelling correction\n"
	          << "  --verbose              Show detailed merge/fix decision log to stderr\n"
	          << "  --min-len N            Min length for single-word fix (default: 5)\n"
	          << "  --hamming N            Hamming distance threshold (default: 1)\n"
	          << "  --short-threshold F    ARPA threshold for SHORT+SHORT merges (default: -3.0)\n"
	          << "  --acronym-score F      Boosted logprob for known acronyms (default: -3.0)\n"
	          << "  --arpa-floor F         Default logprob for missing ARPA words (default: -10.0)\n"
	          << "  -h, --help             Print this help and exit\n"
	          << "\n"
	          << "With no input file and no -t, reads from stdin.\n"
	          << "Output format per line:\n"
	          << "  original: <input>\n"
	          << "  corrected: <output>\n";
}

static std::string resolve_dict(int argc, char* argv[], int& pos) {
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "-d" || arg == "--dictionary") {
			if (i + 1 >= argc) {
				std::cerr << "Error: " << arg << " requires a path argument.\n";
				std::exit(1);
			}
			pos = i + 2;
			return argv[i + 1];
		}
		if (arg.compare(0, 13, "--dictionary=") == 0 && arg.size() > 13) {
			pos = i + 1;
			return arg.substr(13);
		}
	}
	return "res/en_US.aff";
}

static std::string resolve_bundle(int argc, char* argv[], int& pos) {
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "-b" || arg == "--bundle") {
			if (i + 1 >= argc) {
				std::cerr << "Error: " << arg << " requires a path argument.\n";
				std::exit(1);
			}
			pos = i + 2;
			return argv[i + 1];
		}
		if (arg.compare(0, 9, "--bundle=") == 0 && arg.size() > 9) {
			pos = i + 1;
			return arg.substr(9);
		}
	}
	return "";
}

static bool flag_present(int argc, char* argv[], const std::vector<std::string>& flags) {
	for (int i = 1; i < argc; ++i) {
		for (const auto& f : flags) {
			if (f == argv[i]) return true;
		}
	}
	return false;
}

static bool is_flag_with_value(const std::string& arg) {
	return arg == "-d" || arg == "--dictionary" ||
	       arg == "-b" || arg == "--bundle" ||
	       arg == "--min-len" ||
	       arg == "--hamming" ||
	       arg == "--short-threshold" ||
	       arg == "--acronym-score" ||
	       arg == "--arpa-floor";
}

static bool is_known_flag(const std::string& arg) {
	return arg == "-d" || arg == "--dictionary" || arg == "-b" || arg == "--bundle" ||
	       arg == "-t" || arg == "--self-test" || arg == "--test-status" ||
	       arg == "-h" || arg == "--help" || arg == "--fix-single" || arg == "--verbose" ||
	       arg == "--min-len" || arg == "--hamming" ||
	       arg == "--short-threshold" || arg == "--acronym-score" || arg == "--arpa-floor";
}

static int resolve_int_flag(int argc, char* argv[], const char* name, int default_val) {
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == name) {
			if (i + 1 >= argc) {
				std::cerr << "Error: " << arg << " requires an integer argument.\n";
				std::exit(1);
			}
			return std::stoi(argv[i + 1]);
		}
		std::string prefix = std::string(name) + "=";
		if (arg.compare(0, prefix.size(), prefix) == 0 && arg.size() > prefix.size()) {
			return std::stoi(arg.substr(prefix.size()));
		}
	}
	return default_val;
}

static float resolve_float_flag(int argc, char* argv[], const char* name, float default_val) {
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == name) {
			if (i + 1 >= argc) {
				std::cerr << "Error: " << arg << " requires a float argument.\n";
				std::exit(1);
			}
			return std::stof(argv[i + 1]);
		}
		std::string prefix = std::string(name) + "=";
		if (arg.compare(0, prefix.size(), prefix) == 0 && arg.size() > prefix.size()) {
			return std::stof(arg.substr(prefix.size()));
		}
	}
	return default_val;
}

static bool arg_is_param(const std::string& arg) {
	if (arg.compare(0, 13, "--dictionary=") == 0) return false;
	if (arg.compare(0, 9, "--bundle=") == 0) return false;
	if (arg.compare(0, 11, "--min-len=") == 0) return false;
	if (arg.compare(0, 11, "--hamming=") == 0) return false;
	if (arg.compare(0, 17, "--short-threshold=") == 0) return false;
	if (arg.compare(0, 15, "--acronym-score=") == 0) return false;
	if (arg.compare(0, 13, "--arpa-floor=") == 0) return false;
	return true;
}

static std::string resolve_input(int argc, char* argv[], int consumed) {
	for (int i = consumed; i < argc; ++i) {
		std::string arg = argv[i];
		if (is_known_flag(arg)) continue;
		if (i > 0 && is_flag_with_value(argv[i - 1])) continue;
		if (!arg_is_param(arg)) continue;
		return arg;
	}
	return "";
}

static bool file_exists(const std::string& path) {
	std::ifstream in(path.c_str());
	return in.good();
}

static int run_self_test(const CompoundCorrector& corrector, const std::string& test_path, bool fix_single) {
	std::ifstream in(test_path.c_str());
	if (!in) {
		std::cerr << "Error: Can not open test file: " << test_path << "\n";
		return 1;
	}

	int passed = 0;
	int failed = 0;
	std::string line;
	while (std::getline(in, line)) {
		if (line.empty() || (!line.empty() && line[0] == '#')) continue;
		std::size_t sep = line.find('|');
		if (sep == std::string::npos) continue;

		std::string input = line.substr(0, sep);
		std::string expected = line.substr(sep + 1);

		std::string actual = corrector.Correct(input, fix_single);
		if (actual == expected) {
			std::cout << "PASS: \"" << input << "\" -> \"" << actual << "\"\n";
			++passed;
		} else {
			std::cout << "FAIL: \"" << input << "\" -> \"" << actual
			          << "\" (expected: \"" << expected << "\")\n";
			++failed;
		}
	}

	std::cout << "\nResults: " << passed << " passed, " << failed << " failed\n";
	return failed > 0 ? 1 : 0;
}

static std::string join_status(const std::vector<int>& status, char sep = ',') {
	std::string out;
	for (size_t i = 0; i < status.size(); ++i) {
		if (i > 0) out.push_back(sep);
		out += std::to_string(status[i]);
	}
	return out;
}

static int run_status_test(const CompoundCorrector& corrector, const std::string& test_path) {
	std::ifstream in(test_path.c_str());
	if (!in) {
		std::cerr << "Error: Can not open status test file: " << test_path << "\n";
		return 1;
	}

	int passed = 0;
	int failed = 0;
	std::string line;
	while (std::getline(in, line)) {
		if (line.empty() || (!line.empty() && line[0] == '#')) continue;
		// Format: input|expected_text|expected_status_csv
		std::size_t sep1 = line.find('|');
		if (sep1 == std::string::npos) continue;
		std::size_t sep2 = line.find('|', sep1 + 1);
		if (sep2 == std::string::npos) continue;

		std::string input   = line.substr(0, sep1);
		std::string exp_text = line.substr(sep1 + 1, sep2 - sep1 - 1);
		std::string exp_status_s = line.substr(sep2 + 1);

		auto result = corrector.CorrectWithStatus(input, false);
		std::string act_status_s = join_status(result.status);
		if (result.text == exp_text && act_status_s == exp_status_s) {
			std::cout << "PASS: \"" << input << "\" text=\"" << result.text
			          << "\" status=" << act_status_s << "\n";
			++passed;
		} else {
			std::cout << "FAIL: \"" << input << "\"\n";
			std::cout << "  text expected: \"" << exp_text << "\" actual: \"" << result.text << "\"\n";
			std::cout << "  status expected: " << exp_status_s << " actual: " << act_status_s << "\n";
			++failed;
		}
	}

	std::cout << "\nStatus Results: " << passed << " passed, " << failed << " failed\n";
	return failed > 0 ? 1 : 0;
}

static int run_process_verbose(const CompoundCorrector& corrector,
                              std::istream& in, bool fix_single) {
	std::string line;
	bool first = true;
	while (std::getline(in, line)) {
		if (!first) std::cout << "\n";
		first = false;
		std::vector<std::string> debug_log;
		auto result = corrector.CorrectWithStatus(line, fix_single, &debug_log);
		std::cout << "original: " << line << "\ncorrected: " << result.text << "\n";
		for (const auto& entry : debug_log) {
			std::cerr << "  " << entry << "\n";
		}
	}
	return 0;
}

static int run_process(const CompoundCorrector& corrector, std::istream& in, bool fix_single) {
	std::string line;
	bool first = true;
	while (std::getline(in, line)) {
		if (!first) std::cout << "\n";
		first = false;
		std::string corrected = corrector.Correct(line, fix_single);
		std::cout << "original: " << line << "\ncorrected: " << corrected << "\n";
	}
	return 0;
}

int main(int argc, char* argv[]) {
	if (flag_present(argc, argv, {"-h", "--help"})) {
		print_usage(argv[0]);
		return 0;
	}

	bool self_test   = flag_present(argc, argv, {"-t", "--self-test"});
	bool test_status = flag_present(argc, argv, {"--test-status"});
	bool fix_single  = flag_present(argc, argv, {"--fix-single"});
	bool verbose     = flag_present(argc, argv, {"--verbose"});

	// Build NuspellConfig from CLI overrides.
	NuspellConfig config;
	config.single_word_fix_min_len    = resolve_int_flag(argc, argv, "--min-len", 5);
	config.hamming_distance_threshold = resolve_int_flag(argc, argv, "--hamming", 1);
	config.short_short_arpa_threshold = resolve_float_flag(argc, argv, "--short-threshold", -3.0f);
	config.acronym_override_logprob   = resolve_float_flag(argc, argv, "--acronym-score", -3.0f);
	config.arpa_unigram_floor         = resolve_float_flag(argc, argv, "--arpa-floor", -10.0f);

	int consumed = 1;
	std::string bundle_path = resolve_bundle(argc, argv, consumed);
	std::string input_path = resolve_input(argc, argv, consumed);

	CompoundCorrector* corrector = nullptr;
	std::unique_ptr<CompoundCorrector> corrector_holder;
	std::ifstream bundle_in;

	if (!bundle_path.empty()) {
		bundle_in.open(bundle_path.c_str(), std::ios::binary);
		if (!bundle_in) {
			std::cerr << "Error: Can not open bundle: " << bundle_path << "\n";
			return 1;
		}
		std::cout << "Loading bundle: " << bundle_path << "\n";
		corrector_holder.reset(new CompoundCorrector(bundle_in, config));
		corrector = corrector_holder.get();
	} else {
		consumed = 1;
		std::string aff_path = resolve_dict(argc, argv, consumed);
		input_path = resolve_input(argc, argv, consumed);
		if (!file_exists(aff_path)) {
			std::cerr << "Error: Dictionary not found: " << aff_path << "\n";
			return 1;
		}
		std::cout << "Loading dictionary: " << aff_path << "\n";
		corrector_holder.reset(new CompoundCorrector(aff_path, "res/ug", "res/acronyms.txt", config));
		corrector = corrector_holder.get();
	}

	if (self_test) {
		return run_self_test(*corrector, "res/test_file.txt", fix_single);
	}
	if (test_status) {
		return run_status_test(*corrector, "res/test_status.txt");
	}

	if (!input_path.empty()) {
		std::ifstream in(input_path.c_str());
		if (!in) {
			std::cerr << "Error: Can not open input file: " << input_path << "\n";
			return 1;
		}
		if (verbose) return run_process_verbose(*corrector, in, fix_single);
		return run_process(*corrector, in, fix_single);
	}

	if (verbose) return run_process_verbose(*corrector, std::cin, fix_single);
	return run_process(*corrector, std::cin, fix_single);
}
