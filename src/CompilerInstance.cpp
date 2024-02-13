#include "CompilerInstance.h"

#include <iostream>

#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Module.h"

#include "AST.h"
#include "Codegen.h"
#include "Lexer.h"

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
        std::cout << "=============== LLVM IR ===============" << std::endl;
        llvmFunc->print(llvm::outs());
    } else {
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
        llvmFunc->eraseFromParent();
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
    InitializeModule();
    bool run = true;
    while (run) {
        std::cout << "ready > ";
        GetNextToken();
        run = Parse();
    }
    auto theModule = GetModule();
    theModule->print(llvm::outs(), nullptr);
}
