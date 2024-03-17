#ifndef KALEIDOSCOPE_CODEGEN
#define KALEIDOSCOPE_CODEGEN

#include "AST.h"
#include "llvm/IR/Function.h"

void InitializeModule();
void InitializePassManagers();

std::unique_ptr<llvm::LLVMContext> GetContextUniquePtr();
std::unique_ptr<llvm::Module> GetModuleUniquePtr();
llvm::Module* GetModule();
llvm::Function* GenerateCodeForPrototype(const PrototypeAST* prototypeAST);
llvm::Function* GenerateCodeForFunction(const FunctionAST* functionAST);
llvm::Function* RunOptmizationPasses(llvm::Function* f);

#endif // KALEIDOSCOPE_CODEGEN