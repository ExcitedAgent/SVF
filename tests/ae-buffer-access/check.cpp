#include "AE/Svfexe/AbstractInterpretation.h"
#include "AE/Svfexe/AEDetector.h"
#include "SVF-LLVM/LLVMModule.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "Util/CommandLine.h"
#include "Util/ExtAPI.h"
#include "Util/Options.h"
#include "WPA/Andersen.h"
#include <cassert>
#include <iostream>

using namespace SVF;

int main(int argc, char** argv)
{
    const auto args = OptionBase::parseOptions(argc, argv, "Buffer access record regression",
        "[options] <module> <extapi> <case>");
    assert(args.size() == 3);
    std::cerr << "Checking buffer-access " << args[2] << "\n";
    assert(ExtAPI::setExtBcPath(args[1]));
    Options::ModelArrays.setValue(true);
    Options::ModelConsts.setValue(true);
    LLVMModuleSet::getLLVMModuleSet()->buildSVFModule({args[0]});
    SVFIRBuilder builder;
    auto* graph = builder.build();
    auto* ander = AndersenWaveDiff::createAndersenWaveDiff(graph);
    builder.updateCallGraph(ander->getCallGraph());
    auto& ae = AbstractInterpretation::getAEInstance();
    auto owned = std::make_unique<BufOverflowDetector>();
    const auto* detector = owned.get();
    assert(detector->getBufferAccesses().empty());
    ae.addDetector(std::move(owned));
    ae.runOnModule();

    const auto& records = detector->getBufferAccesses();
    if (args[2] == "safe" || args[2] == "copy_safe")
        assert(records.empty());
    else
    {
        assert(!records.empty());
        bool expected = false;
        for (const auto& record : records)
        {
            assert(record.node && record.pointer && record.object);
            assert(!record.node->getSourceLoc().empty());
            assert(record.checkedOffset.ub().getIntNumeral() >= record.bufferSize);
            const bool memoryCall = SVFUtil::isa<CallICFGNode>(record.node);
            if (args[2] == "copy")
                expected |= memoryCall && record.bufferSize == 4 &&
                    record.checkedOffset.equals(IntervalValue(7));
            else if (args[2] == "boundary")
                expected |= !memoryCall && record.bufferSize == 8 &&
                    record.checkedOffset.equals(IntervalValue(8));
            else if (args[2] == "range")
                expected |= !memoryCall && record.bufferSize == 8 &&
                    record.checkedOffset.equals(IntervalValue(8, 9));
            else if (args[2] == "repeated")
                expected |= !memoryCall && record.bufferSize == 8 &&
                    record.checkedOffset.contains(9);
            else
                expected |= !memoryCall && record.bufferSize == 8 &&
                    record.checkedOffset.equals(IntervalValue(9));
        }
        assert(expected);
        if (args[2] == "repeated")
        {
            assert(records.size() >= 2);
            assert(records.front().node == records.back().node);
            assert(records.front().checkedOffset.equals(IntervalValue(8)));
        }
    }
    // Reading the getter has no effect on the detector's existing report.
    const auto count = records.size();
    detector->getBufferAccesses();
    assert(detector->getBufferAccesses().size() == count);
    std::cout << "buffer-access " << args[2] << ": passed (" << count << " records)\n";
    AndersenWaveDiff::releaseAndersenWaveDiff();
    SVFIR::releaseSVFIR();
    LLVMModuleSet::releaseLLVMModuleSet();
}
