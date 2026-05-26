# About Nuspell

Nuspell is a fast and safe spelling checker software program. It is designed
for languages with rich morphology and complex word compounding.
Nuspell is written in modern C++ and it supports Hunspell dictionaries.

Main features of Nuspell spelling checker:

  - Provides software library and command-line tool.
  - Suggests high-quality spelling corrections.
  - Backward compatibility with Hunspell dictionary file format.
  - Up to 3.5 times faster than Hunspell.
  - Full Unicode support backed by ICU.
  - **Compound Corrector API** (`src/api/`) for fixing split acronyms
    and compound terms in ASR transcripts (e.g. `ap i` → `API`).
  - Twofold affix stripping (for agglutinative languages, like Azeri,
    Basque, Estonian, Finnish, Hungarian, Turkish, etc.).
  - Supports complex compounds (for example, Hungarian, German and Dutch).
  - Supports advanced features, for example: special casing rules
    (Turkish dotted i or German sharp s), conditional affixes, circumfixes,
    fogemorphemes, forbidden words, pseudoroots and homonyms.
  - Free and open source software. Licensed under GNU LGPL v3 or later.

# Building Nuspell

## Dependencies

Build-only dependencies:

  - C++ 17 compiler with support for `std::filesystem`, e.g. GCC >= v9
  - CMake >= v3.18
  - Catch2 >= v3.1.1 (optional, needed only when building the tests is enabled)
  - Getopt (Needed only on Windows + MSVC and only when the CLI tool or
    the tests are built. It is available in Vcpkg. Other platforms provide
    it out of the box.)
  - Pandoc (optional, needed only when building the man-pages is enabled)
  - Doxygen (optional, needed only when building the API docs is enabled)

Run-time (and build-time) dependencies:

  - ICU4C

Recommended tools for developers: qtcreator, ninja, clang-format, gdb, vim.

## Building on GNU/Linux and Unixes

We first need to download the dependencies. Some may already be
preinstalled.

For Ubuntu and Debian:

```bash
sudo apt install g++ cmake libicu-dev catch2 pandoc doxygen
```

Then run the following commands inside the Nuspell directory:

```bash
mkdir build
cd build
cmake .. -DBUILD_API=ON
cmake --build .
sudo cmake --install .
sudo ldconfig       # needed only sometimes and only on Linux
```

For faster build process run `cmake --build . -j`, or use Ninja instead
of Make.

If you are making a Linux distribution package (deb, rpm) you need
some additional configurations on the CMake invocation. For example:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
```

To also include the Compound Corrector API and its demo tool, add `-DBUILD_API=ON`.
A convenience script is also provided:

```bash
./install.sh    # builds with -DBUILD_API=ON, then runs self-tests
```

## Building on OSX and macOS

1.  Install Apple's Command-line tools.
2.  Install Homebrew package manager.
3.  Install dependencies with the next commands.

<!-- end list -->

```bash
brew install cmake icu4c catch2 pandoc doxygen
export ICU_ROOT=$(brew --prefix icu4c)
```

Then run the standard cmake and make. See above. The ICU\_ROOT variable
is needed because icu4c is keg-only package in Homebrew and CMake can
not find it by default. Alternatively, you can use `-DICU_ROOT=...` on
the cmake command line.

If you want to build with GCC instead of Clang, you need to pull GCC
with Homebrew and rebuild all the dependencies with it. See Homewbrew
manuals.

## Building on Windows

### Compiling with Visual C++

1.  Install Visual Studio 2017 or newer. Alternatively, you can use
    Visual Studio Build Tools.
2.  Install Git for Windows and Cmake.
3.  Install Vcpkg in some folder, e.g. in `c:\vcpkg`.
4.  Install Pandoc. You can manually install or use `choco install pandoc`.
4.  Install Doxygen. You can manually install or use
    `choco install doxygen.install`.
5.  Run the commands below. Vcpkg will work in manifest mode and it will
    automatically install the dependencies.

<!-- end list -->

```bat
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=c:\vcpkg\scripts\buildsystems\vcpkg.cmake -A x64
cmake --build .
```

### Compiling with Mingw64 and MSYS2

Download MSYS2, update everything and install the following packages:

```bash
pacman -S base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-icu \
          mingw-w64-x86_64-cmake mingw-w64-x86_64-catch mingw-w64-x86_64-doxygen
```

Then from inside the Nuspell folder run:

```bash
mkdir build
cd build
cmake .. -G "Unix Makefiles" -DBUILD_MAN=OFF
make
make install
```

### Building in Cygwin environment

Download the above mentioned dependencies with Cygwin package manager.
Then compile the same way as on Linux. Cygwin builds depend on
Cygwin1.dll.

## Building on FreeBSD

Install the following required packages

```bash
pkg cmake icu catch2 pandoc doxygen
```

Then run the standard cmake and make as on Linux. See above.

# Using the software

## Using the command-line tool

The main executable is located in `src/nuspell`.

After compiling and installing you can run the Nuspell spell checker
with a Nuspell, Hunspell or Myspell dictionary:

    nuspell -d en_US text.txt

For more details run see the [man-page](docs/nuspell.1.md).

<!-- old hunspell v1 stuff
The src/tools directory contains ten executables after compiling.

  - The main executable:
      - nuspell: main program for spell checking and others (see manual)
  - Example tools:
      - analyze: example of spell checking, stemming and morphological
        analysis
      - chmorph: example of automatic morphological generation and
        conversion
      - example: example of spell checking and suggestion
  - Tools for dictionary development:
      - affixcompress: dictionary generation from large (millions of
        words) vocabularies
      - makealias: alias compression (Nuspell only, not back compatible
        with MySpell)
      - wordforms: word generation (Nuspell version of unmunch)
      - ~~hunzip: decompressor of hzip format~~ (DEPRECATED)
      - ~~hzip: compressor of hzip format~~ (DEPRECATED)
      - munch (DEPRECATED, use affixcompress): dictionary generation
        from vocabularies (it needs an affix file, too).
      - unmunch (DEPRECATED, use wordforms): list all recognized words
        of a MySpell dictionary
-->

## Using the Library

Sample program:

```cpp
#include <iostream>
#include <nuspell/dictionary.hxx>
#include <nuspell/finder.hxx>

using namespace std;

int main()
{
	auto dirs = vector<filesystem::path>();
	nuspell::append_default_dir_paths(dirs);
	auto dict_path = nuspell::search_dirs_for_one_dict(dirs, "en_US");
	if (empty(dict_path))
		return 1; // Return error because we can not find the requested
		          // dictionary.

	auto dict = nuspell::Dictionary();
	try {
		dict.load_aff_dic(dict_path);
	}
	catch (const nuspell::Dictionary_Loading_Error& e) {
		cerr << e.what() << '\n';
		return 1;
	}
	auto word = string();
	auto sugs = vector<string>();
	while (cin >> word) {
		if (dict.spell(word)) {
			cout << "Word \"" << word << "\" is ok.\n";
			continue;
		}

		cout << "Word \"" << word << "\" is incorrect.\n";
		dict.suggest(word, sugs);
		if (sugs.empty())
			continue;
		cout << "  Suggestions are: ";
		for (auto& sug : sugs)
			cout << sug << ' ';
		cout << '\n';
	}
}
```

On the command line you can link like this:

```bash
g++ example.cxx -std=c++17 -lnuspell -licuuc -licudata
# or better, use pkg-config
g++ example.cxx -std=c++17 $(pkg-config --cflags --libs nuspell)
```

Within Cmake you can use `find_package()` to link. For example:

```cmake
find_package(Nuspell)
add_executable(myprogram main.cpp)
target_link_libraries(myprogram Nuspell::nuspell)
```

## Using the Compound Corrector API

Nuspell also provides a higher-level **Compound Corrector** API (`src/api/`) that wraps `nuspell::Dictionary` with logic for correcting split acronyms and compound terms (e.g. `ap i` &rarr; `API`, `tachy cardia` &rarr; `tachycardia`).

### Features

- **Sentence-level input** — designed for short-to-medium ASR transcripts (≈10–40 tokens).
- **Chinese/CJK safety** — treats CJK characters as inert; never merges them.
- **Numbers / punctuation safety** — pure-digit, punctuation, and symbol tokens are inert.
- **3-token lookahead** — resolves ambiguous merges (e.g. `a ce o`) by comparing ARPA unigram log-probabilities.
- **ARPA unigram support** — loads a standard ARPA-format 1-gram file for frequency-aware disambiguation.
- **Acronym override file** — loads `res/acronyms.txt` so common medical / technical acronyms (e.g. `PTSD`, `ADHD`, `DNA`) merge even when not in the Hunspell dictionary.
- **Stop-word guarding** — prevents merging of common English function-word pairs (`i am`, `in to`, `be at`, etc.).

### Build

```bash
cmake .. -DBUILD_API=ON
```

Or simply run the provided convenience script:

```bash
./install.sh    # builds, packs res.bundle, copies it to res/,
                # then runs test_compound sanity tests (default path + fix_single,
                # and bundle mode + fix_single)
```

### Constructor

#### Path-based constructor (loose files)

```cpp
#include <compound_corrector.hxx>

// All three resource paths are optional and default to loose files in res/.
CompoundCorrector corrector(
    "res/en_US.aff",        // Hunspell dictionary
    "res/ug",               // ARPA 1-gram file (frequency tie-breaking)
    "res/acronyms.txt"       // curated uppercase acronyms (dictionary override)
);
```

#### Bundle constructor (single binary file)

You can also load everything from a single packed resource bundle produced by `pack_resources`:

```cpp
#include <compound_corrector.hxx>

std::ifstream bundle_in("res.bundle", std::ios::binary);
CompoundCorrector corrector(bundle_in);  // loads aff, dic, ug, acronyms from the bundle
```

This is useful for deployment — everything is shipped as a single file, no relative path issues.

CMake generates the bundle automatically at build time: `build/res.bundle`.


### Example usage

```cpp
#include <compound_corrector.hxx>
#include <iostream>

int main() {
    CompoundCorrector corrector("res/en_US.aff");

    std::cout << corrector.Correct("ap i")          << "\n"; // API
    std::cout << corrector.Correct("tachy cardia")   << "\n"; // tachycardia
    std::cout << corrector.Correct("she gets pts d")<< "\n"; // she gets PTSD
    std::cout << corrector.Correct("i am")         << "\n"; // i am (preserved)

    // Optional: also fix single-word misspellings
    std::cout << corrector.Correct("seperate", true)    << "\n"; // separate

    // Per-token status tracking (useful for ASR alignment)
    auto result = corrector.CorrectWithStatus("ap i", false);
    std::cout << result.text << "\n";     // "API"
    // result.status == {1, -1}
    //  1 = first token initiated a merge with the next
    // -1 = second token was consumed by the previous merge
    return 0;
}
```

### Standalone test / demo tool (`test_compound`)

Built automatically when `-DBUILD_API=ON`. It is a line-processing CLI that demonstrates the API.

```bash
# self-test (27 built-in cases)
./build/src/api/test_compound --self-test

# process file or stdin
echo "she is a ce o" | ./build/src/api/test_compound
cat input.txt | ./build/src/api/test_compound
./build/src/api/test_compound input.txt

# custom dictionary / ARPA / acronyms
./build/src/api/test_compound -d /path/to/it_IT.aff input.txt

# also enable single-word spelling correction
./build/src/api/test_compound --fix-single input.txt

# use a bundled resource file instead of loose res/ files
./build/src/api/test_compound -b build/res.bundle --self-test
```

Output format (per line):
```
original: she is a ce o
corrected: she is a CEO
```

### Single-word spelling correction

By default `Correct()` only fixes **split acronyms and compounds** (`ap i` → `API`, `tachy cardia` → `tachycardia`). If you pass `fix_single = true`:

```cpp
std::string out = corrector.Correct("seperate", true); // "separate"
```

The single-word pass:

- Runs **after** the merge pass (i.e. it can also correct leftover tokens after compounds are merged).
- Only fires when a token is **still not valid** after merging.
- Applies only to words that are **≥ 5 letters** long.
- Scans **all suggestions** from `suggest()` and picks the **first** one that satisfies both constraints below.
- Accepts the suggestion only when it has the **same length** and a **case-insensitive Hamming distance of exactly 1**.

This keeps the correction conservative — it will fix mistakes like `seperate` (top suggestion `separate`, distance 1) but will **not** change `recieve` → `receive` (Levenshtein distance 2), `conect` → `connect` (distance 1 but different length), `tachyardia` → `tachycardia` (different length), or `ceo` → `CEO` (only 3 letters).

The CLI tool supports this via `--fix-single`:

```bash
echo "she is calender on the seperate table" | test_compound --fix-single
# corrected: she is calendar on the separate table
```

### Resource directory (`res/`)

All files are self-contained and committed to the repo for reproducible builds:

| File | Size | Description |
|---|---|---|
| `res/en_US.aff` / `res/en_US.dic` | ~550 KB | Hunspell dictionary pair |
| `res/ug` | ~5.2 MB | ARPA 1-gram log-probabilities (~280k unigrams) |
| `res/acronyms.txt` | ~2 KB | Curated uppercase acronyms (medical, business, technology) |
| `res/test_file.txt` | ~1 KB | Pipe-delimited test cases (`input|expected`) |
| `res/res.bundle` | ~5.7 MB | Auto-generated packed resource bundle (all of the above in one file) |

### Manual compilation against the API

If you are compiling a standalone program that uses `CompoundCorrector`:

```bash
g++ example.cxx -std=c++17 -I src \
    build/src/api/libcompound_corrector.a \
    -lnuspell -licuuc -licudata
# or, if installed:
g++ example.cxx -std=c++17 -lnuspell -licuuc -licudata \
    -L build/src/api -lcompound_corrector
```

**Note:** The `CompoundCorrector` source itself is C++14-compatible (no `std::filesystem`, `std::optional`, or `std::string_view`), but the underlying Nuspell library requires C++17 because its public headers use `std::string_view`. Therefore, the final link step still needs `-std=c++17`.

Bundled resources are handled by `src/api/bundle.hxx` and generated at build time by the `pack_resources` tool.

### CLI quick reference (`test_compound`)

| Flag | Description |
|---|---|
| `-d, --dictionary PATH` | Path to Hunspell `.aff` file (default: `res/en_US.aff`) |
| `-b, --bundle PATH`   | Use a packed resource bundle instead of loose files |
| `-t, --self-test`     | Run built-in test cases (`res/test_file.txt`) |
| `--test-status`       | Run status-code regression tests (`res/test_status.txt`) |
| `--fix-single`        | Enable single-word correction after merge pass |
| `-h, --help`          | Print usage and exit |

# Dictionaries

Myspell, Hunspell and Nuspell dictionaries:

<https://github.com/nuspell/nuspell/wiki/Dictionaries-and-Contacts>


# Advanced topics
## Debugging Nuspell

First, always install the debugger:

```bash
sudo apt install gdb
```

For debugging we need to create a debug build and then we need to start
`gdb`.

```bash
mkdir debug
cd debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j
gdb src/nuspell/nuspell
```

We recommend debugging to be done
[with an IDE](https://github.com/nuspell/nuspell/wiki/IDE-Setup).

## Testing

To run the tests, run the following command after building:

    ctest

# See also

Full documentation in the [wiki](https://github.com/nuspell/nuspell/wiki).

API Documentation for developers will be generated from the source files
by configuring CMake with `-DBUILD_API_DOCS=ON` (ON by default) and building.

The result can be viewed by opening `BUILD_DIR/docs/html/index.html` during
development, or by opening `INSTALL_PREFIX/share/doc/nuspell/html/index.html`
after installing.
