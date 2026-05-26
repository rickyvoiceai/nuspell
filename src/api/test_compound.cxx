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
	          << "  " << prog << " [-d dict.aff] [--fix-single] [input.txt]\n"
	          << "  " << prog << " [-b bundle] [--fix-single] [input.txt]\n"
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

static std::string resolve_input(int argc, char* argv[], int consumed) {
	for (int i = consumed; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "-d" || arg == "--dictionary" || arg == "-b" || arg == "--bundle" ||
		    arg == "-t" || arg == "--self-test" || arg == "--test-status" ||
		    arg == "-h" || arg == "--help" || arg == "--fix-single")
			continue;
		if (i > 0 && (std::string(argv[i - 1]) == "-d" || std::string(argv[i - 1]) == "--dictionary" ||
		                   std::string(argv[i - 1]) == "-b" || std::string(argv[i - 1]) == "--bundle"))
			continue;
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
		corrector_holder.reset(new CompoundCorrector(bundle_in));
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
		corrector_holder.reset(new CompoundCorrector(aff_path));
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
		return run_process(*corrector, in, fix_single);
	}

	return run_process(*corrector, std::cin, fix_single);
}
