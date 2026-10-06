# Buffer-access observations

Motivating consumer: BANDA PR #103, reworking merged BANDA PR #59.

`BufOverflowDetector::BufferAccessInfo` retains the values already used by an
unsafe buffer check. `getBufferAccesses()` exposes these records after analysis.
Each record contains the check node, checked pointer, associated base object,
checked byte-offset interval, and buffer-size value used in the comparison.

The only production changes are this struct, its getter/storage, and two
record insertions in the existing GEP and memory-API failure branches.
No detector predicate, transfer, allocation model, graph representation,
assertion handling, factory, or LLVM integration is changed.

`checkedOffset` is the GEP result's offset for address calculations. For memory
APIs it is the checked last-byte offset, including the length adjustment already
performed by AE. It must not be confused with the starting offset or access width.
`bufferSize` is exactly AE's existing comparison value; it is not a new guarantee
of an uncapped allocation extent or an over-approximating size range.
These records explain candidate checks; they do not prove an error.

Records preserve individual unsafe observations, including repeated visits and
distinct objects. Existing textual reports may deduplicate by source location.
The memory checker still returns at its first unsafe object; this API does not
introduce additional checks. SAFE/UNSAFE_BUFACCESS test stubs use that same
checker. Thus records describe unsafe checks, not only emitted textual bugs.
SVF's existing one-past test and its existing treatment of access widths remain
unchanged. This API does not add missing load/store detection.

The vector belongs to the detector; its elements can move during analysis.
Native pointers borrow the analyzed graph and must not outlive it. Read after
`runOnModule()` and copy values/source locations before graph teardown.

The regression matrix covers safe, stack, variable-sized stack, heap, nested
GEP, one-past, safe/unsafe memmove, interval offsets, and repeated visits in dense and semi-sparse mode, with assertions
enabled. Use `LLVM_DIR` and `Z3_ROOT` pointing to existing installations and run
`bash tests/ae-buffer-access/run.sh` after building SVF.
All 20 runs (10 scenarios in each mode) pass on base `12ad599` with assertions,
RTTI, and exceptions enabled. The test uses the standalone AE tool's existing
`-pre-field-sensitive=false` setting.

This API intentionally does not reproduce the rejected symbolic-allocation
extension. BANDA must retain its own proof obligations: these diagnostic
interval endpoints cannot replace allocation-correlated program values.
