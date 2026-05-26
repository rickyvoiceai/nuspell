# Nuspell — Agent Notes

## Quick Build

```bash
./install.sh
```

Equivalent manual steps:

```bash
cmake -B build -DBUILD_API=ON
cmake --build build
./build/src/api/pack_resources res/ build/res.bundle
cp build/res.bundle res/res.bundle
./build/src/api/test_compound --self-test          # 27 string tests
./build/src/api/test_compound --test-status        # 27 status code tests
```

## CI Gap — Run API Tests Manually

The GitHub workflow only runs `ctest`. It **never** runs `test_compound`.

Before pushing changes that touch `src/api/` or `res/` always run one of:

```bash
./install.sh
```

or, equivalently:

```bash
./build/src/api/test_compound --self-test
./build/src/api/test_compound --test-status
./build/src/api/test_compound -b res/res.bundle --self-test
```

## Directory Layout

| Directory | Content |
|---|---|
| `src/nuspell/` | Core spelling library |  
| `src/tools/` | `nuspell` CLI binary |  
| `src/api/` | Compound Corrector API, `test_compound`, `pack_resources` |  
| `tests/` | Catch2-based tests (run via `ctest`) |  
| `res/` | API resources: dictionary, ARPA unigrams, acronyms, test fixtures, bundle |  

## `src/api/` is C++14-Compatible

No `std::filesystem`, `std::optional`, `std::string_view`, or `std::make_unique`.  
Underlying `src/nuspell/` headers still use `std::string_view`, so final link still needs `-std=c++17`. Keep new code in `src/api/` library-feature-free.

## Resource Bundle

`res.bundle` is generated — tracked by `.gitignore`, but `install.sh` copies it to `res/res.bundle`.  
CMake generates `build/res.bundle` automatically. The packer is `pack_resources res_dir bundle_path`.

## Adding / Updating API Test Cases

- **String tests** → `res/test_file.txt` (`input|expected`)
- **Status code tests** → `res/test_status.txt` (`input|expected_text|expected_status_csv`)
