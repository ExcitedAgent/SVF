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
    const std::map<std::string, bool> expected{
        {"g511", true}, {"g512", true}, {"g513", true},
        {"g4096", true}, {"gzero", true}, {"gunsized", false},
        {"gwide", true}, {"stack", true}, {"dynamic", true},
        {"good", true}, {"product", true}, {"missing", false},
        {"unknown", false}, {"malformed", false},
        {"trailing", false}, {"conflict", false},
        {"badindex", false}, {"pointer", false},
        {"too_wide", false}, {"zero", true},
        {"negative", false}};
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
        const auto& factors = it->second->getArrSize();
        check(!factors.empty() == status, name + " availability");
        if (!status || factors.empty()) continue;
        auto constant = [&](std::size_t i, u64_t value) {
            auto* c = SVFUtil::dyn_cast<ConstIntValVar>(factors.at(i));
            return c && c->getZExtValue() == value;
        };
        if (name == "g511" || name == "g512" || name == "g513" || name == "g4096")
            check(factors.size() == 1 && constant(0, std::stoull(name.substr(1))), name + " exact bytes");
        if (name == "gzero") check(factors.size() == 1 && constant(0, 0), "known zero");
        if (name == "gwide") check(constant(0, 8589934592ULL), "no u32 layout truncation");
        if (name == "stack") check(factors.size() == 2 && constant(0, 1) && constant(1, 513), "stack type/count");
        if (name == "dynamic") check(factors.size() == 2 && factors[0]->getName() == "n" && constant(1, 4), "dynamic allocation-time operand");
        if (name == "good" || name == "product" || name == "zero") {
            auto* call = SVFUtil::cast<CallICFGNode>(it->second->getICFGNode());
            if (name == "good") check(factors.size() == 1 && factors[0] == call->getArgument(1), "argument-one identity");
            if (name == "product") check(factors.size() == 2 && factors[0] == call->getArgument(0) && factors[1] == call->getArgument(1), "product identities");
            if (name == "zero") check(factors.size() == 1 && constant(0, 0), "zero operand preserved");
        }
    }
    std::cout << "Allocation operand checks: " << checks << ", failures: " << failures << "\n";
    ExtAPI::destory();
    SVFIR::releaseSVFIR();
    LLVMModuleSet::releaseLLVMModuleSet();
    return failures ? 1 : 0;
}
