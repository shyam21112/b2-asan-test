#!/bin/bash
# B2: Build wallet-core with ASan and run the real API test
# Usage: bash run_test.sh
# Requirements: Linux with clang, cmake, ninja, boost, protobuf-dev, Rust

set -e
set -o pipefail

echo "=== B2: Building wallet-core with AddressSanitizer ==="
echo "Date: $(date)"
echo "Commit: $(git -C wallet-core rev-parse HEAD 2>/dev/null || echo 'N/A')"
echo ""

# 1. Clone wallet-core if not present
if [ ! -d wallet-core ]; then
    echo "[1/4] Cloning wallet-core..."
    git clone --depth 1 https://github.com/trustwallet/wallet-core.git
fi
cd wallet-core
COMMIT=$(git rev-parse HEAD)
echo "    Commit: $COMMIT"
cd ..

# 2. Bootstrap dependencies
echo "[2/4] Bootstrapping wallet-core dependencies..."
cd wallet-core
./bootstrap.sh 2>&1 | tail -3
cd ..

# 3. Build wallet-core with ASan
echo "[3/4] Building wallet-core with ASan..."
cd wallet-core
cmake -H. -Bbuild \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g" \
    -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" \
    -G Ninja
ninja -C build -j$(nproc) 2>&1 | tail -5
cd ..

# 4. Build and run the test
echo "[4/4] Building and running test..."
mkdir -p test_build
cd test_build
cmake .. \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" \
    -DWALLET_CORE_DIR=../wallet-core \
    -DWALLET_CORE_BUILD=../wallet-core/build \
    -G Ninja
ninja -j$(nproc)

echo ""
echo "=== RUNNING TEST (under ASan) ==="
echo ""

# Run with ASan options to continue after the first error (so we see both attacks)
ASAN_OPTIONS=detect_leaks=0:halt_on_error=0 ./test_b2_asan 2>&1 | tee ../asan_output.txt

EXIT_CODE=$?
echo ""
echo "Test exit code: $EXIT_CODE"
echo "Full output saved to: $(pwd)/../asan_output.txt"
echo "Commit tested: $COMMIT"

if grep -q "ERROR: AddressSanitizer" ../asan_output.txt; then
    echo ""
    echo "🔴 ADDRESSSANITIZER ERROR DETECTED — OOB READ CONFIRMED"
    grep "ERROR: AddressSanitizer" ../asan_output.txt
else
    echo "No ASan error — OOB may not have triggered or was handled"
fi
