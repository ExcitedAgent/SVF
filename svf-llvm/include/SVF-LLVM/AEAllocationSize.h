#pragma once
#include "SVFIR/SVFStatements.h"
#include <optional>

namespace SVF {
/// Symbolic allocation-time byte count for an alarm's value binding.
/// This is program syntax, not an abstract-state interval. No graph field is stored.
struct AEAllocationSize {
    u64_t byteScale;
    std::vector<const ValVar*> operands;
};

/// Query only when producing an alarm. Unknown descriptions return nullopt.
/// Pointers borrow the independently owned program used by the detector.
std::optional<AEAllocationSize> allocationSizeForAlarm(const AddrStmt* address);
}
