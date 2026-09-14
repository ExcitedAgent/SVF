# Allocation byte extent graph fact

Status: implementation API contract proposed for review under BANDA issue #60.
This specifies a native SVF fact; it does not amend BANDA's numerical or allocation-site semantics.

`AddrStmt.getAllocationByteExtent()` returns a borrowed, SVF-owned `AllocationByteExtent`.
Its status is `Exact` or an explicit unavailability reason.
When exact, the expression is `byteScale * product(operands)` in mathematical byte units, using the numerical values of the published SVF integer operands at the allocation definition.
An empty operand vector is the empty product (one); status, not vector emptiness or the numeric scale, determines availability.
Thus scale zero with no operands is a known zero extent.
Operand values belong to the address statement's associated allocation and are not later loads of mutable source variables.
The fact does not resolve a downstream pointer to that allocation or establish freshness, liveness, non-nullness, subobject bounds, or dereferenceability.

The LLVM frontend produces the fact inside SVF from the object definition.
For globals it uses the allocated value type's layout; for stack objects it uses the allocated type's layout multiplied by the allocation count.
Unsized and scalable layouts are unavailable, irrespective of the legacy type-size field's zero/one sentinel behavior.
Fixed layout sizes are read before narrowing and retained up to `INT64_MAX`; larger scales are explicitly unrepresentable.
The separate legacy object/type byte-size fields and `getArrSize()` are unchanged and do not certify this fact.

For heap objects SVF selects exactly one `AllocSize:` description from its own model annotations.
The supported grammar is a nonempty `Arg` followed by decimal argument-index digits, with terms separated by `*`.
`UNKNOWN`, absent, malformed, conflicting, and out-of-range descriptions are unavailable.
The parser consumes each full term and obtains the indicated allocation call argument; it never substitutes argument zero as a fallback.
Each operand must map to an SVF integer value and have at most 64 bits.
Constant operands must have identical nonnegative signed/unsigned public SVF interpretations; otherwise they are unavailable.
The expression is not evaluated in a machine-size accumulator, so a product is never silently capped, truncated, or wrapped.
Consumers retain their approved numerical operand interpretation and must explicitly decline expressions outside their representational capabilities.

Unavailable status values are `MissingDescription`, `UnknownDescription`, `InvalidDescription`, `ConflictingDescriptions`, `UnavailableLayout`, `UnrepresentableLayout`, `InvalidOperand`, and `UnrepresentableOperand`.
Their scale and operand payloads have no semantic meaning.
Objects/graphs constructed without this producer (including current graph-database restoration) default to missing information, never an exact zero.
Registration syntax and LLVM values remain internal to SVF; graph consumers see only status, byte scale, and native SVF operand references.

For BANDA the bridge consists only of field projection and borrowed-vector count/index accessors.
Swift translates the exact expression once into existing owned numerical operands; unavailable facts do not become seeds.
There is no new model-registration API, BANDA-shaped C++ record, or cross-sibling graph dependency.
These concrete field types, expression grammar, representability limits, and reason names are engineering choices, not requirements imposed by the OOB theory.
