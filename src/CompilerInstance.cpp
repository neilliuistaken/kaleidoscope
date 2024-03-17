#include "CompilerInstance.h"

#include <iostream>

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"
#include "Kaleidoscope-JIT.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/Shared/ExecutorAddress.h"

#include "AST.h"
#include "Codegen.h"
#include "Lexer.h"

using namespace llvm;
using namespace llvm::orc;

static std::unique_ptr<KaleidoscopeJIT> theJIT;
static ExitOnError ExitOnErr;
static std::map<std::string, std::unique_ptr<PrototypeAST>> functionProtoASTs;
static std::map<std::string, std::unique_ptr<FunctionAST>> functionASTs;

void InitializeJIT()
{
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    LLVMInitializeNativeAsmParser();
    theJIT = ExitOnErr(KaleidoscopeJIT::Create());
}

void HandleDefinition()
{
    auto def = ParseDefinition();
    if (def) {
        std::cout << "===============   AST   ===============" << std::endl;
        def->PrettyPrint();
        auto llvmFunc = GenerateCodeForFunction(def.get());
        if (llvmFunc == nullptr) {
            std::cout << "Codegen error occurred" << std::endl;
            return;
        }
        // JIT consumes llvm module, therefore we store AST so the function can be regenerate
        // again in the future.
        functionASTs[def->GetPrototype()->GetName()] = std::move(def);
        std::cout << "=============== LLVM IR ===============" << std::endl;
        llvmFunc->print(llvm::outs());
        RunOptmizationPasses(llvmFunc);
        std::cout << "=============== LLVM IR (OPTed) ===============" << std::endl;
        llvmFunc->print(llvm::outs());
    }
     else {
        std::cout << "Parse definition failed" << std::endl;
    }
}

void HandleExtern()
{
    auto def = ParseExtern();
    if (def) {
        std::cout << "===============   AST   ===============" << std::endl;
        def->PrettyPrint();
        auto llvmFunc = GenerateCodeForPrototype(def.get());
        if (llvmFunc == nullptr) {
            std::cout << "Codegen error occurred" << std::endl;
            return;
        }
        functionProtoASTs[def->GetName()] = std::move(def);
        std::cout << "=============== LLVM IR ===============" << std::endl;
        llvmFunc->print(llvm::outs());
    } else {
        std::cout << "Parse extern failed" << std::endl;
    }
}

void HandleTopLevelExpression()
{
    auto func = ParseTopLevelExpr();
    if (func) {
        std::cout << "===============   AST   ===============" << std::endl;
        func->PrettyPrint();
        auto llvmFunc = GenerateCodeForFunction(func.get());
        if (llvmFunc == nullptr) {
            std::cout << "Codegen error occurred" << std::endl;
            return;
        }
        std::cout << "=============== LLVM IR ===============" << std::endl;
        llvmFunc->print(llvm::outs());
        RunOptmizationPasses(llvmFunc);
        std::cout << "=============== LLVM IR (OPTed) ===============" << std::endl;
        llvmFunc->print(llvm::outs());
        std::cout << "=============== RESULT ===============" << std::endl;
        auto rt = theJIT->getMainJITDylib().createResourceTracker();
        auto module = GetModuleUniquePtr();
        auto context = GetContextUniquePtr();
        auto tsm = ThreadSafeModule(std::move(module), std::move(context));
        ExitOnErr(theJIT->addModule(std::move(tsm), rt));
        auto exprSymbol = ExitOnErr(theJIT->lookup("__anonymours_expr"));
        assert(exprSymbol && "__anonymours_expr function not found");
        auto executorAddr = ExecutorAddr(exprSymbol.getAddress());
        double (*FP)() = executorAddr.toPtr<double (*)()>();
        fprintf(stdout, "Evaluated to %f\n", FP());
        ExitOnErr(rt->remove());
        // The original module has been moved and consumed. We initialize a new one here.
        InitializeModule();
        // All previous generated functions are gone as well. We regenerate them here.
        for (auto& item : functionASTs) {
            (void) GenerateCodeForFunction(item.second.get());
        }
        for (auto& item : functionProtoASTs) {
            (void) GenerateCodeForPrototype(item.second.get());
        }
    } else {
        std::cout << "Parse top-level expression failed" << std::endl;
    }
}

bool Parse() {
    switch (GetCurrentToken()) {
        case tok_eof:
            return false;
        case ';':
            GetNextToken();
            break;
        case tok_def:
            HandleDefinition();
            break;
        case tok_extern:
            HandleExtern();
            break;
        default:
            HandleTopLevelExpression();
            break;
    }
    return true;
}

void ReadEvalPrintLoop()
{
    InitializeJIT();
    InitializeModule();
    InitializePassManagers();
    bool run = true;
    while (run) {
        std::cout << "ready > ";
        GetNextToken();
        run = Parse();
    }
    auto theModule = GetModule();
    theModule->print(llvm::outs(), nullptr);
}
