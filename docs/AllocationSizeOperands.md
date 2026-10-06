# Allocation size operands

Status: proposed implementation for review, 2026-10-06.
Owner: BANDA integration maintainers.
Authority: implementation documentation; not a new BANDA semantic contract.
Motivation: preserve [BANDA #59](https://github.com/ExcitedSpider/BANDA-Swift/pull/59) while replacing [SVF #1](https://github.com/ExcitedSpider/SVF/pull/1)'s separate allocation-extent graph extension.

The existing `AddrStmt::getArrSize()` vector contains factors whose product is the allocation's byte count.
An empty vector means no validated description is available.
A zero constant is a known zero factor.
Stack allocations retain their allocation-time count operand and a literal allocated-type byte size.
Globals have one literal layout factor.
Heap allocations use the operands selected by the SVF-owned `AllocSize:ArgN(*ArgN)*` registration.
Malformed, missing, conflicting, unknown, unsupported, or unrepresentable descriptions publish no factors.
Validation completes before any factor is attached.

Layout factors are ordinary SVF integer literals, with signed and unsigned interpretations agreeing.
They are not capped to the field limit or narrowed to 32 bits.
The LLVM-facing producer remains entirely inside SVF.
No new graph field, extent class, status enumeration, or allocation-extent accessor is introduced.
The existing AE size consumer reads literal factors directly and retains its existing field-limit approximation.
Its returned scalar remains a capped estimate, not a sound upper bound on arbitrarily large allocations.

This is not an AE-only replacement.
Simply exporting the existing capped scalar loses the byte-count expression required for BANDA's allocation-correlated proofs.
The old operand API also selects argument zero for unknown allocator names, including `memalign`, and omits layout scaling.
This patch corrects that existing producer rather than adding a second representation.
It retains the earlier AE library-control commit as its base; those controls are independent of the allocation-size rework.

Run `bash tests/allocation-size-operands/run.sh` with the existing LLVM and Z3 directories and shared SVF build.
The 21-case matrix checks 53 properties, including sizes around 512, a layout larger than 32 bits, dynamic operand identity, argument-one selection, products, zero, and rejected descriptions.
