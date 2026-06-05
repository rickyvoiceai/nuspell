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
./build/src/api/test_compound --self-test          # 33 string tests
./build/src/api/test_compound --test-status        # 33 status code tests
```

The optional `res/addition/Names2020_Countries_Companies.txt` must be
present before `pack_resources` runs if you want the proper-name overlay.
Otherwise the build completes with a warning.

## CI Gap — Run API Tests Manually

The GitHub workflow only runs `ctest`. It **never** runs `test_compound`.

Before pushing changes that touch `src/api/` or `res/` always run one of:

```bash
./install.sh
```

or, equivalently, all 6 test configurations (loose files, full bundle, minimal bundle):

```bash
./build/src/api/test_compound -d res/en_US.aff --self-test
./build/src/api/test_compound -d res/en_US.aff --test-status
./build/src/api/test_compound -b res/res.bundle --self-test
./build/src/api/test_compound -b res/res.bundle --test-status
./build/src/api/test_compound -b res/res-minimal.bundle --self-test
./build/src/api/test_compound -b res/res-minimal.bundle --test-status
```

or, equivalently:

```bash
./build/src/api/test_compound --self-test          # 33 string tests
./build/src/api/test_compound --test-status        # 32 status code tests
./build/src/api/test_compound -d res/en_US.aff --self-test || true
./build/src/api/test_compound -d res/en_US.aff --test-status || true
./build/src/api/test_compound -b res/res.bundle --self-test || true
./build/src/api/test_compound -b res/res.bundle --test-status || true
./build/src/api/test_compound -b res/res-minimal.bundle --self-test || true
./build/src/api/test_compound -b res/res-minimal.bundle --test-status || true
```

## Directory Layout

| Directory | Content |
|---|---|
| `src/nuspell/` | Core spelling library |  
| `src/tools/` | `nuspell` CLI binary |  
| `src/api/` | Compound Corrector API, `test_compound`, `pack_resources` |  
| `tests/` | Catch2-based tests (run via `ctest`) |  
| `res/` | API resources: dictionary, ARPA unigrams, acronyms, test fixtures, bundle |  

## `src/nuspell/` and `src/api/` are C++14-Compatible

No `std::filesystem`, `std::optional`, or `std::string_view` in our code —
replaced with custom polyfills (`filesystem.hxx`, `string_view.hxx`).  
The final link may still need `-std=c++14` depending on compiler defaults.
Keep new code library-feature-free.

## C++14 Build Notes

If your toolchain defaults to C++11 or older, add explicitly:

```bash
cmake -B build -DBUILD_API=ON -DBUILD_ADVANCED_TESTS=OFF
```

The `BUILD_ADVANCED_TESTS` flag controls the upstream Catch2-based unit
and integration tests that use C++17 features (CTAD, `is_same_v`, UDL literals).
It defaults to `ON`, so C++14-only environments should disable it.

## Resource Bundle

`res.bundle` is generated — tracked by `.gitignore`, but `install.sh` copies it to `res/res.bundle`.  
CMake generates `build/res.bundle` automatically. The packer is `pack_resources res_dir bundle_path`.

## Proper-Name Overlay (Optional)

`pack_resources` now supports an optional `PROPER_NAMES` bundle entry
(`BundleTag = 4`) sourced from:

  `res/addition/Names2020_Countries_Companies.txt`

The file contains lowercase proper nouns (cities, countries, companies,
famous people, US states, car brands, religions, etc.) intended for ASR
downstream capitalization.

### Runtime loading

`CompoundCorrector::Impl::proper_names` is an `unordered_set<std::string>`
populated from the bundle at construction time:

```cpp
const BundleEntry* pn = bundle.find(BundleTag::PROPER_NAMES);
if (pn) { /* populate set line-by-line, skip headers */ }
```

### Backward compatibility

- **Old bundle → New code:** `find(PROPER_NAMES)` returns `nullptr`,
  `proper_names` stays empty. All existing tests pass.
- **New bundle → Old code:** Extra entries are never queried; `read_bundle`
  deserializes them fine (the format is extensible). No version bump needed.

## Adding / Updating API Test Cases

- **String tests** → `res/test_file.txt` (`input|expected`)
- **Status code tests** → `res/test_status.txt` (`input|expected_text|expected_status_csv`)
