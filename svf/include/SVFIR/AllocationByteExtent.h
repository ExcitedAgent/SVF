// SVF-owned allocation extent facts. See docs/AllocationByteExtent.md.
#ifndef SVF_ALLOCATION_BYTE_EXTENT_H
#define SVF_ALLOCATION_BYTE_EXTENT_H

#include "Util/GeneralType.h"
#include <vector>

namespace SVF
{
class ValVar;

class AllocationByteExtent
{
public:
    enum Status : u32_t
    {
        Exact, MissingDescription, UnknownDescription, InvalidDescription,
        ConflictingDescriptions, UnavailableLayout, UnrepresentableLayout,
        InvalidOperand, UnrepresentableOperand
    };

    static AllocationByteExtent unavailable(Status reason)
    {
        AllocationByteExtent result;
        result.status = reason;
        return result;
    }
    static AllocationByteExtent exact(u64_t scale, std::vector<const ValVar*> values)
    {
        AllocationByteExtent result;
        result.status = Exact;
        result.byteScale = scale;
        result.operands = std::move(values);
        return result;
    }
    Status getStatus() const { return status; }
    u64_t getByteScale() const { return byteScale; }
    const std::vector<const ValVar*>& getOperands() const { return operands; }

private:
    Status status = MissingDescription;
    u64_t byteScale = 0;
    std::vector<const ValVar*> operands;
};
}
#endif
