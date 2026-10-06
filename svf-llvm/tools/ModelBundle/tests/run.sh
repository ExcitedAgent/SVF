#!/usr/bin/env bash
set -euo pipefail
tool=$1
clang=$2
here=$(cd "$(dirname "$0")" && pwd)
work=$(mktemp -d -t svf-model-tests-XXXXXX)
trap 'rm -rf "$work"' EXIT
for name in application base model wrong transitive annotated; do
    "$clang" -g -O0 -emit-llvm -c "$here/$name.c" -o "$work/$name.bc"
done
expect_failure() {
    if "$@"; then
        printf 'Expected admission rejection\n' >&2
        exit 1
    fi
}
common=(build --application "$work/application.bc" --base "$work/base.bc")
"$tool" "${common[@]}" --model "$work/model.bc" --bundle "$work/valid"
"$tool" verify --application "$work/application.bc" --bundle "$work/valid"
expect_failure "$tool" "${common[@]}" --model "$work/wrong.bc" --bundle "$work/wrong"
expect_failure "$tool" "${common[@]}" --model "$work/model.bc" --model "$work/model.bc" --bundle "$work/duplicate"
expect_failure "$tool" "${common[@]}" --model "$work/model.bc" --reserved compute --bundle "$work/protected"
expect_failure "$tool" "${common[@]}" --model "$work/transitive.bc" --bundle "$work/transitive"
expect_failure "$tool" build --application "$work/application.bc" --base "$work/annotated.bc" --model "$work/model.bc" --bundle "$work/conflict"
"$tool" build --application "$work/application.bc" --base "$work/annotated.bc" --model "$work/model.bc" --override compute --bundle "$work/override"
expect_failure "$tool" verify --application "$work/model.bc" --bundle "$work/valid"
cp "$work/wrong.bc" "$work/valid/extapi.bc"
expect_failure "$tool" verify --application "$work/application.bc" --bundle "$work/valid"
printf 'SVF model bundle tests passed\n'
