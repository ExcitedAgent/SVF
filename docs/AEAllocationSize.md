# Allocation bindings for AE alarms

Status: proposed rework of SVF PR #1, 2026-10-06.
Owner: BANDA integration maintainers.
Authority: implementation documentation.
Motivation: retain the OOB validation specified by [BANDA #59](https://github.com/ExcitedSpider/BANDA-Swift/pull/59) without its allocation-extent graph extension.

`allocationSizeForAlarm(address)` returns an optional byte scale and allocation-time operands for a detector's symbolic alarm binding.
It is queried when producing an alarm, not during graph construction.
It adds no fields to SVFIR, creates no graph nodes, and changes no allocation operands or AE transfer rules.
The existing AE library-control changes remain separate prerequisites.

The query retains validated allocator argument identities and uncapped layout sizes, including known zero.
Unknown, malformed, conflicting, unsized, scalable, or unrepresentable descriptions return no binding.
Its implementation stays in the SVF LLVM integration library: BANDA neither sees LLVM values in this interface nor interprets model-registration strings.
The result is program-value syntax, not an interval or a reachable-state assertion.
Borrowed native values remain inside the adapter's independently owned SVF session.

The alarm producer translates these native operands into operation-relative input positions before reporting the alarm.
The consumer resolves those positions against the allocation associated with the checked access in its own graph.
Diagnostic offset/size intervals cannot replace these state-dependent values.
Exporting the old capped size estimate alone would not preserve sound allocation-correlated OOB validation.

Run `bash tests/ae-allocation-size/run.sh` using the existing LLVM/Z3 directories and a shared SVF build.
The 21-case matrix checks 74 properties, including that querying leaves existing allocation inputs unchanged.
