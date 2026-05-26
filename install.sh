#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BUNDLE_PATH="${BUILD_DIR}/res.bundle"

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
echo "==> Packing resources into res.bundle"
"${BUILD_DIR}/src/api/pack_resources" "${SCRIPT_DIR}/res" "${BUNDLE_PATH}"

echo "==> Copying res.bundle to res/ directory"
cp -f "${BUNDLE_PATH}" "${SCRIPT_DIR}/res/res.bundle"

# --------------------------------------------------------------
# 4. Sanity tests
# --------------------------------------------------------------
echo ""
echo "==> Quick sanity run of built tools"
echo "---"

cd "${SCRIPT_DIR}"

echo "[nuspell --help]"
"${BUILD_DIR}/src/tools/nuspell" --help || true

echo ""
echo "[test_compound --fix-single --self-test (default path mode + fix_single)]"
"${BUILD_DIR}/src/api/test_compound" --fix-single --self-test || true

echo ""
echo "[test_compound --test-status (status code regression test)]"
"${BUILD_DIR}/src/api/test_compound" --test-status || true

echo ""
echo "[test_compound -b res/res.bundle --fix-single --self-test (bundle mode + fix_single)]"
"${BUILD_DIR}/src/api/test_compound" -b res/res.bundle --fix-single --self-test || true
