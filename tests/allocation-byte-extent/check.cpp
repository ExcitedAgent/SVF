#include "SVF-LLVM/LLVMModule.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "Util/ExtAPI.h"
#include "Util/SVFUtil.h"
#include <iostream>
#include <map>
#include <string>
using namespace SVF;
int main(int argc, char** argv) {
    if (argc != 3 || !ExtAPI::setExtBcPath(argv[2])) return 2;
    LLVMModuleSet::getLLVMModuleSet()->buildSVFModule({argv[1]});
    SVFIRBuilder builder;
    auto* graph = builder.build();
    using E = AllocationByteExtent;
    const std::map<std::string, E::Status> expected{
        {"g511", E::Exact}, {"g512", E::Exact}, {"g513", E::Exact},
        {"g4096", E::Exact}, {"gzero", E::Exact}, {"gunsized", E::UnavailableLayout},
        {"gwide", E::Exact}, {"stack", E::Exact}, {"dynamic", E::Exact},
        {"good", E::Exact}, {"product", E::Exact}, {"missing", E::MissingDescription},
        {"unknown", E::UnknownDescription}, {"malformed", E::InvalidDescription},
        {"trailing", E::InvalidDescription}, {"conflict", E::ConflictingDescriptions},
        {"badindex", E::InvalidOperand}, {"pointer", E::InvalidOperand},
        {"too_wide", E::UnrepresentableOperand}, {"zero", E::Exact},
        {"negative", E::UnrepresentableOperand}};
    std::map<std::string, const AddrStmt*> addresses;
    for (auto* statement : graph->getSVFStmtSet(SVFStmt::Addr)) {
        auto* address = SVFUtil::cast<AddrStmt>(statement);
        addresses[address->getLHSVar()->getName()] = address;
    }
    unsigned checks = 0, failures = 0;
    auto check = [&](bool value, const std::string& label) {
        ++checks;
        if (!value) { ++failures; std::cerr << "FAIL " << label << "\n"; }
    };
    for (const auto& [name, status] : expected) {
        auto it = addresses.find(name);
        check(it != addresses.end(), name + " address");
        if (it == addresses.end()) continue;
        const auto& extent = it->second->getAllocationByteExtent();
        check(extent.getStatus() == status, name + " status");
        std::cout << name << " status=" << extent.getStatus() << " scale=" << extent.getByteScale() << " operands=" << extent.getOperands().size() << "\n";
        if (status != E::Exact) continue;
        if (name == "g511" || name == "g512" || name == "g513" || name == "g4096")
            check(extent.getByteScale() == std::stoull(name.substr(1)), name + " exact bytes");
        if (name == "gzero") check(extent.getByteScale() == 0 && extent.getOperands().empty(), "known zero");
        if (name == "gwide") check(extent.getByteScale() == 8589934592ULL, "no u32 layout truncation");
        if (name == "stack") check(extent.getByteScale() == 513 && extent.getOperands().size() == 1, "stack type/count");
        if (name == "dynamic") check(extent.getByteScale() == 4 && extent.getOperands().size() == 1 && extent.getOperands()[0]->getName() == "n", "dynamic allocation-time operand");
        if (name == "good" || name == "product" || name == "zero") {
            auto* call = SVFUtil::cast<CallICFGNode>(it->second->getICFGNode());
            if (name == "good") check(extent.getOperands().size() == 1 && extent.getOperands()[0] == call->getArgument(1), "argument-one identity");
            if (name == "product") check(extent.getOperands().size() == 2 && extent.getOperands()[0] == call->getArgument(0) && extent.getOperands()[1] == call->getArgument(1), "product identities");
            if (name == "zero") check(extent.getOperands().size() == 1 && SVFUtil::cast<ConstIntValVar>(extent.getOperands()[0])->getSExtValue() == 0, "zero operand preserved");
        }
    }
    std::cout << "Allocation extent checks: " << checks << ", failures: " << failures << "\n";
    ExtAPI::destory();
    SVFIR::releaseSVFIR();
    LLVMModuleSet::releaseLLVMModuleSet();
    return failures ? 1 : 0;
}
