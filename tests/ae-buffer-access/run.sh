#!/usr/bin/env bash
set -euo pipefail
ulimit -c 0
root=$(cd "$(dirname "$0")/../.." && pwd)
: "${LLVM_DIR:?Set LLVM_DIR to the existing LLVM installation root}"
: "${Z3_ROOT:?Set Z3_ROOT to the existing Z3 installation root}"
build=${SVF_BUILD_DIR:-"$root/Release-build"}
temporary=$(mktemp -d)
trap 'rm -rf "$temporary"' EXIT
"${CXX:-c++}" -std=c++17 -UNDEBUG -DEXPERIMENTAL_KEY_INSTRUCTIONS \
    -I"$root/svf/include" -I"$root/svf-llvm/include" -I"$build/include" \
    -I"$LLVM_DIR/include" -I"$Z3_ROOT/include" \
    "$root/tests/ae-buffer-access/check.cpp" \
    -L"$build/lib" -L"$LLVM_DIR/lib" -L"$Z3_ROOT/bin" \
    -Wl,-rpath,"$build/lib:$LLVM_DIR/lib:$Z3_ROOT/bin" \
    -lSvfLLVM -lSvfCore -lz3 -lLLVM -o "$temporary/check"
for scenario in safe stack dynamic heap nested boundary copy copy_safe range repeated; do
    "$LLVM_DIR/bin/clang" -g -O0 -Xclang -disable-O0-optnone -fno-builtin \
        -D"${scenario^^}" -S -emit-llvm "$root/tests/ae-buffer-access/access.c" \
        -o "$temporary/raw.ll"
    "$LLVM_DIR/bin/opt" -S -passes=mem2reg "$temporary/raw.ll" -o "$temporary/input.ll"
    for mode in dense semi-sparse; do
        "$temporary/check" "-ae-sparsity=$mode" -stat=false -pre-field-sensitive=false "$temporary/input.ll" \
            "$build/lib/extapi.bc" "$scenario"
    done
done
