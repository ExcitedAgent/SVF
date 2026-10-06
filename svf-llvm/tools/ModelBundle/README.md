# SVF model bundles

`svf-model build --application app.bc --base extapi.bc --model model.bc
--bundle NEW_DIRECTORY` admits typed external model bodies for a specific
application. Repeated `--model` arguments add modules. `--reserved NAME`
protects a consumer primitive; `--override NAME` explicitly replaces a stock
model and removes its old SVF annotations.

`svf-model verify --application app.bc --bundle DIRECTORY` checks artifact
identities and returns the JSON provenance manifest. Consumers pass the same
prepared application and ExtAPI paths to their independently owned graphs.
Consumers need no LLVM type, attribute, or body inspection.

Admission checks matching function signatures, calling conventions, parameter
and return ABI attributes, target/data layout, unique exports, explicit stock
overrides, protected names, and transitive direct dependencies. It rejects
variadics, aliases, ifuncs, visible model globals, address-taken declarations,
and indirect model dependencies. Imported declaration/call attributes are
sanitized so old purity, range, or non-null claims do not survive substitution.
The original application is untouched; outputs include prepared application,
composed ExtAPI, retained model modules, content identities and LLVM/target
metadata. Model authors remain responsible for behavioral correctness.

Run admission regressions with installed tools:

```sh
bash svf-llvm/tools/ModelBundle/tests/run.sh \
  "$PWD/Release-build/bin/svf-model" "$PWD/llvm-21.1.0.obj/bin/clang"
```
