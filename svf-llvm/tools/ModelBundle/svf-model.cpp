// SVF-owned external-model admission. Consumers use the bundle interface;
// they do not inspect LLVM types, attributes, or imported model bodies.
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/raw_ostream.h"
#include <filesystem>
#include <set>
#include <stdexcept>

using namespace llvm;
namespace fs = std::filesystem;
static void require(bool condition, const std::string &message)
{
    if (!condition) throw std::runtime_error(message);
}
static std::string digest(const std::string &path)
{
    auto data = MemoryBuffer::getFile(path);
    require(bool(data), "cannot read " + path);
    return toHex(SHA256::hash(arrayRefFromStringRef((*data)->getBuffer())), true);
}
static std::unique_ptr<Module> readModule(const std::string &path, LLVMContext &context)
{
    SMDiagnostic error;
    auto module = parseIRFile(path, error, context);
    require(bool(module), "cannot load SVF model input " + path);
    require(!verifyModule(*module, &errs()), "invalid module " + path);
    return module;
}
static void writeModule(const Module &module, const fs::path &path)
{
    std::error_code error;
    raw_fd_ostream output(path.string(), error);
    require(!error, "cannot write " + path.string());
    WriteBitcodeToFile(module, output);
    output.flush();
    require(!output.has_error(), "failed writing " + path.string());
}
// These attributes affect the calling ABI and cannot be silently discarded.
static AttributeSet abi(LLVMContext &context, AttributeSet input)
{
    AttrBuilder builder(context);
    for (Attribute attribute : input)
    {
        if (attribute.isStringAttribute()) continue;
        switch (attribute.getKindAsEnum())
        {
        case Attribute::SExt: case Attribute::ZExt: case Attribute::InReg:
        case Attribute::StructRet: case Attribute::ByVal: case Attribute::ByRef:
        case Attribute::InAlloca: case Attribute::Preallocated: case Attribute::Nest:
        case Attribute::SwiftSelf: case Attribute::SwiftError:
        case Attribute::SwiftAsync:
        case Attribute::Alignment:
            builder.addAttribute(attribute); break;
        default: break;
        }
    }
    return AttributeSet::get(context, builder);
}
static void compatible(Function &declaration, Function &model)
{
    const auto name = model.getName().str();
    require(declaration.getFunctionType() == model.getFunctionType(), "signature mismatch: " + name);
    require(declaration.getCallingConv() == model.getCallingConv(), "calling convention mismatch: " + name);
    require(!model.isVarArg(), "variadic model admission is unavailable: " + name);
    auto &context = model.getContext();
    require(abi(context, declaration.getAttributes().getRetAttrs()) ==
            abi(context, model.getAttributes().getRetAttrs()), "return ABI mismatch: " + name);
    for (unsigned i = 0; i < model.arg_size(); ++i)
        require(abi(context, declaration.getAttributes().getParamAttrs(i)) ==
                abi(context, model.getAttributes().getParamAttrs(i)), "argument ABI mismatch: " + name);
}
static AttributeList sanitized(Function &function)
{
    auto &context = function.getContext();
    SmallVector<AttributeSet> arguments;
    for (unsigned i = 0; i < function.arg_size(); ++i)
        arguments.push_back(abi(context, function.getAttributes().getParamAttrs(i)));
    return AttributeList::get(context, AttributeSet(),
        abi(context, function.getAttributes().getRetAttrs()), arguments);
}
// An explicitly replaced body must not retain the stock model's annotations.
static void removeAnnotations(Module &module, Function &function)
{
    auto *global = module.getGlobalVariable("llvm.global.annotations");
    if (!global) return;
    auto *array = dyn_cast<ConstantArray>(global->getInitializer());
    require(array, "unrecognized SVF base annotation table");
    SmallVector<Constant *> retained;
    for (auto &operand : array->operands())
    {
        auto *record = dyn_cast<ConstantStruct>(operand.get());
        require(record && record->getNumOperands() >= 2, "malformed SVF base annotation");
        if (record->getOperand(0)->stripPointerCasts() != &function)
            retained.push_back(record);
    }
    if (retained.size() == array->getNumOperands()) return;
    require(global->use_empty(), "referenced base annotation table");
    auto *type = ArrayType::get(array->getType()->getElementType(), retained.size());
    auto *replacement = new GlobalVariable(module, type, global->isConstant(),
        global->getLinkage(), ConstantArray::get(type, retained), "");
    replacement->copyAttributesFrom(global);
    replacement->takeName(global);
    global->eraseFromParent();
}
int main(int argc, char **argv)
{
    try
    {
        require(argc >= 2, "usage: svf-model build|verify --application FILE --bundle DIRECTORY [--base FILE --model FILE ... --reserved NAME ... --override NAME ...]");
        std::string command = argv[1], application, bundle, base;
        std::vector<std::string> models;
        std::set<std::string> reserved, overrides;
        for (int i = 2; i < argc; i += 2)
        {
            require(i + 1 < argc, "missing option value");
            std::string option = argv[i], value = argv[i + 1];
            if (option == "--application") application = value;
            else if (option == "--bundle") bundle = value;
            else if (option == "--base") base = value;
            else if (option == "--model") models.push_back(value);
            else if (option == "--reserved") reserved.insert(value);
            else if (option == "--override") overrides.insert(value);
            else throw std::runtime_error("unknown option: " + option);
        }
        require(!application.empty() && !bundle.empty(), "application and bundle required");
        fs::path directory(bundle);
        if (command == "verify")
        {
            auto bytes = MemoryBuffer::getFile((directory / "manifest.json").string());
            require(bool(bytes), "missing bundle manifest");
            auto parsed = json::parse((*bytes)->getBuffer());
            require(bool(parsed), "invalid bundle manifest");
            auto *manifest = parsed->getAsObject();
            require(manifest && manifest->getInteger("schemaVersion") == 1, "unsupported bundle version");
            require(manifest->getString("applicationIdentity") == digest(application), "bundle belongs to another application");
            require(manifest->getString("preparedIdentity") == digest((directory / "application.bc").string()), "prepared application changed");
            require(manifest->getString("extAPIIdentity") == digest((directory / "extapi.bc").string()), "SVF model bundle changed");
            auto *inputs = manifest->getArray("models");
            require(inputs && !inputs->empty(), "missing model identities");
            for (size_t i = 0; i < inputs->size(); ++i)
            {
                auto *input = (*inputs)[i].getAsObject();
                auto path = "model-" + std::to_string(i) + ".bc";
                require(input && input->getString("path") == path &&
                    input->getString("identity") == digest((directory / path).string()), "retained model changed");
            }
            auto *names = manifest->getArray("exports");
            require(names, "missing model exports");
            for (auto &entry : *names)
            {
                auto name = entry.getAsString();
                require(name && !reserved.count(name->str()), "protected model replacement");
            }
            outs() << formatv("{0:2}\n", *parsed);
            return 0;
        }
        require(command == "build", "unknown command");
        require(!base.empty() && !models.empty(), "base and model inputs required");
        require(!fs::exists(directory), "bundle destination already exists");
        LLVMContext context;
        auto app = readModule(application, context);
        auto library = readModule(base, context);
        std::set<std::string> exports;
        json::Array sourceIdentities, displaced, signatures;
        for (const auto &path : models)
        {
            auto model = readModule(path, context);
            require(model->getTargetTriple() == app->getTargetTriple() &&
                model->getDataLayout() == app->getDataLayout(), "model target/data-layout mismatch");
            require(model->alias_empty() && model->ifunc_empty(), "model aliases/ifuncs are not admitted");
            for (const auto &global : model->globals())
                require(global.hasLocalLinkage() || global.isDeclaration(), "externally visible model globals or annotations are not admitted");
            for (Function &function : *model)
            {
                if (function.isDeclaration() || function.hasLocalLinkage()) continue;
                auto name = function.getName().str();
                require(exports.insert(name).second, "duplicate model: " + name);
                require(!reserved.count(name), "protected model: " + name);
                auto *original = app->getFunction(name);
                require(original && original->isDeclaration(), "model needs an application declaration: " + name);
                compatible(*original, function);
                std::string signature;
                raw_string_ostream signatureOut(signature);
                function.getFunctionType()->print(signatureOut);
                signatures.push_back(json::Object{{"name", name}, {"type", signature},
                    {"callingConvention", int64_t(function.getCallingConv())},
                    {"attributes", function.getAttributes().getAsString(AttributeList::FunctionIndex)}});
                if (auto *existing = library->getFunction(name); existing && !existing->isDeclaration())
                {
                    require(overrides.erase(name) == 1, "base-model replacement needs explicit override: " + name);
                    compatible(*existing, function);
                    removeAnnotations(*library, *existing);
                    displaced.push_back(name);
                }
                // Imported declarations and call sites must not retain library
                // purity/non-null/range assumptions absent from the chosen model.
                original->setAttributes(sanitized(*original));
                for (User *user : original->users())
                {
                    if (auto *call = dyn_cast<CallBase>(user))
                    {
                        require(call->getCalledFunction() == original, "indirect model call admission unavailable");
                        require(call->getFunctionType() == function.getFunctionType() &&
                            call->getCallingConv() == function.getCallingConv(), "call-site ABI mismatch: " + name);
                        require(abi(context, call->getAttributes().getRetAttrs()) ==
                            abi(context, function.getAttributes().getRetAttrs()), "call-site return ABI mismatch: " + name);
                        for (unsigned i = 0; i < function.arg_size(); ++i)
                            require(abi(context, call->getAttributes().getParamAttrs(i)) ==
                                abi(context, function.getAttributes().getParamAttrs(i)), "call-site argument ABI mismatch: " + name);
                        call->setAttributes(sanitized(*original));
                    }
                    else throw std::runtime_error("address-taken model declaration is not admitted: " + name);
                }
            }
            sourceIdentities.push_back(json::Object{{"path", "model-" + std::to_string(sourceIdentities.size()) + ".bc"},
                {"identity", digest(path)}});
            require(!Linker::linkModules(*library, std::move(model), Linker::Flags::OverrideFromSrc), "SVF model linking failed");
        }
        require(overrides.empty(), "unused override selection");
        // Every transitive direct dependency must have a model body or be an
        // explicitly protected primitive. Missing semantics never become no-op.
        std::vector<Function *> pending;
        std::set<Function *> visited;
        for (const auto &name : exports) pending.push_back(library->getFunction(name));
        while (!pending.empty())
        {
            auto *function = pending.back();
            pending.pop_back();
            require(function, "missing linked export");
            if (!visited.insert(function).second) continue;
                for (BasicBlock &block : *function)
                    for (Instruction &instruction : block)
                        if (auto *call = dyn_cast<CallBase>(&instruction))
                        {
                            auto *target = call->getCalledFunction();
                            require(target, "indirect call in model dependency closure");
                            require(!target->isDeclaration() || target->isIntrinsic() || reserved.count(target->getName().str()),
                                "unresolved model dependency: " + target->getName().str());
                            if (!target->isDeclaration()) pending.push_back(target);
                        }
        }
        require(!verifyModule(*app, &errs()) && !verifyModule(*library, &errs()), "prepared model verification failed");
        fs::create_directories(directory);
        for (size_t i = 0; i < models.size(); ++i)
            fs::copy_file(models[i], directory / ("model-" + std::to_string(i) + ".bc"));
        writeModule(*app, directory / "application.bc");
        writeModule(*library, directory / "extapi.bc");
        json::Array names;
        for (const auto &name : exports) names.push_back(name);
        json::Object manifest{{"schemaVersion", 1}, {"applicationIdentity", digest(application)},
            {"llvmVersion", LLVM_VERSION_STRING},
            {"targetTriple", app->getTargetTriple().str()},
            {"dataLayout", app->getDataLayoutStr()},
            {"preparedIdentity", digest((directory / "application.bc").string())},
            {"extAPIIdentity", digest((directory / "extapi.bc").string())},
            {"baseIdentity", digest(base)}, {"exports", std::move(names)},
            {"signatures", std::move(signatures)},
            {"models", std::move(sourceIdentities)}, {"overrides", std::move(displaced)},
            {"assumption", "Results are conditional on the selected author-supplied SVF models."}};
        std::error_code error;
        raw_fd_ostream out((directory / "manifest.json").string(), error);
        require(!error, "cannot write manifest");
        out << formatv("{0:2}\n", json::Value(std::move(manifest)));
        out.flush();
        require(!out.has_error(), "manifest write failed");
        return 0;
    }
    catch (const std::exception &error)
    {
        errs() << "svf-model: " << error.what() << '\n';
        return 2;
    }
}
