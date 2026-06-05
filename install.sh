#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BUNDLE_FULL="${BUILD_DIR}/res.bundle"
BUNDLE_MINIMAL="${BUILD_DIR}/res-minimal.bundle"

PACK_PROPER_NAMES=1
BUILD_BOTH=0

show_help() {
	cat <<EOF
Usage: $0 [OPTIONS]

Build nuspell and pack the resource bundle(s).

Options:
  (default)              Build full bundle with proper names (res/res.bundle)
  --no-proper-names      Build minimal bundle without proper names (res/res-minimal.bundle)
  --build-both           Build both full and minimal bundles
  -h, --help             Show this help message
EOF
}

# Parse flags
while [[ $# -gt 0 ]]; do
	case "$1" in
		--no-proper-names) PACK_PROPER_NAMES=0; shift ;;
		--build-both)      BUILD_BOTH=1; shift ;;
		-h|--help)         show_help; exit 0 ;;
		*) echo "Error: unknown option $1"; echo "Use -h for help."; exit 1 ;;
	esac
done

# If --build-both, build both variants (implies full + minimal)
if [[ "$BUILD_BOTH" == 1 ]]; then
	PACK_PROPER_NAMES=1
fi

# Helper: pack bundle with or without proper names
pack_bundle() {
	local with_pn="$1"
	local out_bundle="$2"

	if [[ "$with_pn" == 0 ]]; then
		if [[ -f "${SCRIPT_DIR}/res/addition/Names2020_Countries_Companies.txt" ]]; then
			mv "${SCRIPT_DIR}/res/addition/Names2020_Countries_Companies.txt" \
			   "${SCRIPT_DIR}/res/addition/Names2020_Countries_Companies.txt.bak"
			# Ensure restoration on error/exit
			trap 'if [[ -f "${SCRIPT_DIR}/res/addition/Names2020_Countries_Companies.txt.bak" ]]; then mv "${SCRIPT_DIR}/res/addition/Names2020_Countries_Companies.txt.bak" "${SCRIPT_DIR}/res/addition/Names2020_Countries_Companies.txt"; fi' EXIT
		else
			echo "Warning: Names2020 file not found; minimal bundle == full bundle"
		fi
	fi

	"${BUILD_DIR}/src/api/pack_resources" "${SCRIPT_DIR}/res" "${out_bundle}"

	if [[ "$with_pn" == 0 ]]; then
		if [[ -f "${SCRIPT_DIR}/res/addition/Names2020_Countries_Companies.txt.bak" ]]; then
			mv "${SCRIPT_DIR}/res/addition/Names2020_Countries_Companies.txt.bak" \
			   "${SCRIPT_DIR}/res/addition/Names2020_Countries_Companies.txt"
			trap - EXIT
		fi
	fi
}

# --------------------------------------------------------------
# 1. Configure
# --------------------------------------------------------------
echo "==> Configuring build in ${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_API=ON \
    -DBUILD_TESTING=ON \
    -DBUILD_TOOLS=ON

# --------------------------------------------------------------
# 2. Build
# --------------------------------------------------------------
echo "==> Building (parallel jobs: $(nproc 2>/dev/null || echo 1))"
cmake --build . --parallel

# --------------------------------------------------------------
# 3. Pack resources & copy to res/ directory
# --------------------------------------------------------------
echo ""

if [[ "$BUILD_BOTH" == 1 ]]; then
	echo "==> Building both full and minimal bundles"
	pack_bundle 1 "${BUNDLE_FULL}" "${SCRIPT_DIR}/res"
	cp -f "${BUNDLE_FULL}" "${SCRIPT_DIR}/res/res.bundle"

	pack_bundle 0 "${BUNDLE_MINIMAL}" "${SCRIPT_DIR}/res"
	cp -f "${BUNDLE_MINIMAL}" "${SCRIPT_DIR}/res/res-minimal.bundle"
elif [[ "$PACK_PROPER_NAMES" == 1 ]]; then
	echo "==> Packing full resources into res.bundle (with proper names)"
	pack_bundle 1 "${BUNDLE_FULL}" "${SCRIPT_DIR}/res"
	cp -f "${BUNDLE_FULL}" "${SCRIPT_DIR}/res/res.bundle"
else
	echo "==> Packing minimal resources into res-minimal.bundle (without proper names)"
	pack_bundle 0 "${BUNDLE_MINIMAL}" "${SCRIPT_DIR}/res"
	cp -f "${BUNDLE_MINIMAL}" "${SCRIPT_DIR}/res/res-minimal.bundle"
fi

# --------------------------------------------------------------
# 4. Sanity tests (7 runs total)
#   proper-names test only on full bundle (loose/minimal have no data)
# --------------------------------------------------------------
echo ""
echo "==> Quick sanity run of built tools"
echo "---"

cd "${SCRIPT_DIR}"

echo "[nuspell --help]"
"${BUILD_DIR}/src/tools/nuspell" --help || true

# --- Loose files mode ---
if [[ -f "${SCRIPT_DIR}/res/en_US.aff" ]]; then
	echo ""
	echo "[test_compound -d res/en_US.aff --fix-single --self-test (loose files)]"
	"${BUILD_DIR}/src/api/test_compound" -d res/en_US.aff --fix-single --self-test || true

	echo ""
	echo "[test_compound -d res/en_US.aff --test-status (loose files)]"
	"${BUILD_DIR}/src/api/test_compound" -d res/en_US.aff --test-status || true
fi

# --- Full bundle ---
if [[ -f "${SCRIPT_DIR}/res/res.bundle" ]]; then
	echo ""
	echo "[test_compound -b res/res.bundle --fix-single --self-test (full bundle)]"
	"${BUILD_DIR}/src/api/test_compound" -b res/res.bundle --fix-single --self-test || true

	echo ""
	echo "[test_compound -b res/res.bundle --test-status (full bundle)]"
	"${BUILD_DIR}/src/api/test_compound" -b res/res.bundle --test-status || true

	echo ""
	echo "[test_compound -b res/res.bundle --test-proper-names (full bundle)]"
	"${BUILD_DIR}/src/api/test_compound" -b res/res.bundle --test-proper-names || true
fi

# --- Minimal bundle (no proper-names data) ---
if [[ -f "${SCRIPT_DIR}/res/res-minimal.bundle" ]]; then
	echo ""
	echo "[test_compound -b res/res-minimal.bundle --fix-single --self-test (minimal bundle)]"
	"${BUILD_DIR}/src/api/test_compound" -b res/res-minimal.bundle --fix-single --self-test || true

	echo ""
	echo "[test_compound -b res/res-minimal.bundle --test-status (minimal bundle)]"
	"${BUILD_DIR}/src/api/test_compound" -b res/res-minimal.bundle --test-status || true
fi

echo ""
echo "All done."
