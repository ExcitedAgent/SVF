#!/usr/bin/env bash
set -euo pipefail
svf_root=$(cd "$(dirname "$0")/../.." && pwd)
extent_test_root=$(mktemp -d)
trap 'rm -rf "$extent_test_root"' EXIT
"$svf_root/llvm-21.1.0.obj/bin/clang" -emit-llvm -c "$svf_root/tests/allocation-byte-extent/models.c" -o "$extent_test_root/models.bc"
"$svf_root/llvm-21.1.0.obj/bin/clang++" -std=c++17 -DEXPERIMENTAL_KEY_INSTRUCTIONS -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -UNDEBUG \
-I"$svf_root/svf/include" -I"$svf_root/svf-llvm/include" -I"$svf_root/Release-build/include" -I"$svf_root/llvm-21.1.0.obj/include" -I"$svf_root/z3.obj/include" \
"$svf_root/tests/allocation-byte-extent/check.cpp" \
-L"$svf_root/Release-build/lib" -L"$svf_root/llvm-21.1.0.obj/lib" -L"$svf_root/z3.obj/bin" -lSvfLLVM -lSvfCore -lz3 -lLLVM \
-Wl,-rpath,"$svf_root/Release-build/lib:$svf_root/llvm-21.1.0.obj/lib:$svf_root/z3.obj/bin" -o "$extent_test_root/check"
"$extent_test_root/check" "$svf_root/tests/allocation-byte-extent/matrix.ll" "$extent_test_root/models.bc"
