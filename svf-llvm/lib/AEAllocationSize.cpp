#include "SVF-LLVM/AEAllocationSize.h"
#include "SVF-LLVM/LLVMModule.h"
#include "SVF-LLVM/LLVMUtil.h"
#include <charconv>
#include <limits>

using namespace SVF;

std::optional<AEAllocationSize> SVF::allocationSizeForAlarm(const AddrStmt* edge)
{
    if (!edge) return std::nullopt;
    auto* modules = LLVMModuleSet::getLLVMModuleSet();
    auto* pag = SVFIR::getPAG();
    const auto* object = SVFUtil::dyn_cast<BaseObjVar>(edge->getRHSVar());
    if (!object || !modules->hasLLVMValue(object))
        return std::nullopt;
    const llvm::Value* allocation = modules->getLLVMValue(object);
    std::vector<const llvm::Value*> values;
    const llvm::Type* layout = nullptr;
    if (const auto* global = llvm::dyn_cast<llvm::GlobalVariable>(allocation))
        layout = global->getValueType();
    else if (const auto* stack = llvm::dyn_cast<llvm::AllocaInst>(allocation))
    {
        layout = stack->getAllocatedType();
        values.push_back(stack->getArraySize());
    }
    else if (const auto* call = llvm::dyn_cast<llvm::CallBase>(allocation))
    {
        const auto* function = call->getCalledFunction();
        if (!object->isHeap() || !function) return std::nullopt;
        std::string description;
        bool found = false;
        for (auto annotation : modules->getExtFuncAnnotations(function))
        {
            while (!annotation.empty() && annotation.back() == '\0') annotation.pop_back();
            if (annotation.rfind("AllocSize:", 0) != 0) continue;
            if (found) return std::nullopt;
            found = true;
            description = annotation.substr(10);
        }
        if (!found) return std::nullopt;
        if (description == "UNKNOWN") return std::nullopt;
        if (description.empty()) return std::nullopt;
        std::size_t begin = 0;
        while (begin < description.size())
        {
            const auto end = description.find('*', begin);
            const auto term = description.substr(begin, end == std::string::npos ? end : end - begin);
            if (term.rfind("Arg", 0) != 0 || term.size() == 3)
                return std::nullopt;
            u32_t index = 0;
            const auto parsed = std::from_chars(term.data() + 3, term.data() + term.size(), index);
            if (parsed.ec != std::errc() || parsed.ptr != term.data() + term.size()
                || index >= call->arg_size()) return std::nullopt;
            values.push_back(call->getArgOperand(index));
            if (end == std::string::npos) break;
            begin = end + 1;
            if (begin == description.size()) return std::nullopt;
        }
    }
    else return std::nullopt;

    u64_t scale = 1;
    if (layout)
    {
        if (!layout->isSized()) return std::nullopt;
        const auto size = modules->getMainLLVMModule()->getDataLayout()
                              .getTypeAllocSize(const_cast<llvm::Type*>(layout));
        if (size.isScalable()) return std::nullopt;
        scale = size.getFixedValue();
        if (scale > static_cast<u64_t>(std::numeric_limits<s64_t>::max()))
            return std::nullopt;
    }
    std::vector<const ValVar*> operands;
    for (const auto* value : values)
    {
        if (!value || !value->getType()->isIntegerTy())
            return std::nullopt;
        if (value->getType()->getIntegerBitWidth() > 64)
            return std::nullopt;
        if (const auto* constant = llvm::dyn_cast<llvm::ConstantInt>(value))
        {
            // Allocation sizes are nonnegative; retain only constants whose
            // published signed and unsigned SVF interpretations agree.
            const auto integer = LLVMUtil::getIntegerValue(constant);
            if (integer.first < 0 || static_cast<u64_t>(integer.first) != integer.second)
                return std::nullopt;
        }
        const auto* operand = SVFUtil::dyn_cast<ValVar>(pag->getGNode(modules->getValueNode(value)));
        if (!operand || !SVFUtil::isa<SVFIntegerType>(operand->getType()))
            return std::nullopt;
        operands.push_back(operand);
    }
    return AEAllocationSize{scale, std::move(operands)};
}
