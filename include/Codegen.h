#ifndef KALEIDOSCOPE_CODEGEN
#define KALEIDOSCOPE_CODEGEN

#include "AST.h"
#include "llvm/IR/Function.h"

void InitializeModule();

llvm::Module* GetModule();
llvm::Function* GenerateCodeForPrototype(const PrototypeAST* prototypeAST);
llvm::Function* GenerateCodeForFunction(const FunctionAST* functionAST);

#endif // KALEIDOSCOPE_CODEGEN