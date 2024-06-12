#include "Codegen.h"

#include <map>

#include "llvm/IR/Constant.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Analysis/CGSCCPassManager.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Passes/StandardInstrumentations.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar/Reassociate.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Scalar/SimplifyCFG.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

static std::unique_ptr<LLVMContext> theContext;
static std::unique_ptr<Module> theModule;
static std::unique_ptr<IRBuilder<>> builder;
static std::map<std::string, Value *> namedValues;

std::unique_ptr<FunctionPassManager> theFPM;
std::unique_ptr<LoopAnalysisManager> theLAM;
std::unique_ptr<FunctionAnalysisManager> theFAM;
std::unique_ptr<CGSCCAnalysisManager> theCGAM;
std::unique_ptr<ModuleAnalysisManager> theMAM;
std::unique_ptr<PassInstrumentationCallbacks> thePIC;
std::unique_ptr<StandardInstrumentations> theSI;

void InitializeModule()
{
    theContext = std::make_unique<LLVMContext>();
    theModule = std::make_unique<Module>("Kale JIT", *theContext);
    builder = std::make_unique<IRBuilder<>>(*theContext);
}

void InitializePassManagers()
{
    // theModule->setDataLayout();
    theFPM = std::make_unique<FunctionPassManager>();
    theLAM = std::make_unique<LoopAnalysisManager>();
    theFAM = std::make_unique<FunctionAnalysisManager>();
    theCGAM = std::make_unique<CGSCCAnalysisManager>();
    theMAM = std::make_unique<ModuleAnalysisManager>();
    thePIC = std::make_unique<PassInstrumentationCallbacks>();
    theSI = std::make_unique<StandardInstrumentations>(*theContext, true);

    theSI->registerCallbacks(*thePIC, theFAM.get());

    theFPM->addPass(InstCombinePass());
    theFPM->addPass(ReassociatePass());
    theFPM->addPass(GVNPass());
    theFPM->addPass(SimplifyCFGPass());
    PassBuilder pb;
    pb.registerModuleAnalyses(*theMAM);
    pb.registerFunctionAnalyses(*theFAM);
    pb.crossRegisterProxies(*theLAM, *theFAM, *theCGAM, *theMAM);
}


std::unique_ptr<LLVMContext> GetContextUniquePtr()
{
    return std::move(theContext);
}

std::unique_ptr<Module> GetModuleUniquePtr()
{
    return std::move(theModule);
}

Module* GetModule()
{
    return theModule.get();
}

Value *LogErrorV(const std::string& str) {
    fprintf(stderr, "Error: %s\n", str.c_str());
    return nullptr;
}

Value* GenerateCodeForExpr(const ExprAST* exprAST);

Value* GenerateCodeForNumberExpr(const NumberExprAST* numberExprAST)
{
    return ConstantFP::get(*theContext, APFloat(numberExprAST->GetValue()));
}

Value* GenerateCodeForVariableExpr(const VariableExprAST* variableExprAST)
{
    Value* v = namedValues[variableExprAST->GetName()];
    if (!v) {
        LogErrorV("Unknown variable name: " + variableExprAST->GetName());
    }
    return v;
}

Value* GenerateCodeForBinaryExpr(const BinaryExprAST* binaryExprAST)
{
    Value* LHS = GenerateCodeForExpr(binaryExprAST->LHS.get());
    Value* RHS = GenerateCodeForExpr(binaryExprAST->RHS.get());
    if (!LHS || !RHS) {
        return nullptr;
    }
    switch (binaryExprAST->GetOp())
    {
        case '+':
            return builder->CreateFAdd(LHS, RHS, "addtemp");
        case '-':
            return builder->CreateFSub(LHS, RHS, "subtemp");
        case '*':
            return builder->CreateFMul(LHS, RHS, "multemp");
        case '/':
            return builder->CreateFDiv(LHS, RHS, "divtemp");
        default:
            LogErrorV("Invalid binary operator: " + std::string(binaryExprAST->GetOp(), 1));
            return nullptr;
    }
}

Value* GenerateCodeForCallExpr(const CallExprAST* callExprAST)
{
    Function* callee = theModule->getFunction(callExprAST->GetCallee());
    if (!callee) {
        return LogErrorV("Unknown function referenced: " + callExprAST->GetCallee());
    }

    if (callee->arg_size() != callExprAST->GetArgs().size()) {
        return LogErrorV("Incorrect # arguments passed");
    }

    std::vector<Value*> argsV;
    for (unsigned int i = 0; i < callExprAST->GetArgs().size(); i++) {
        argsV.push_back(GenerateCodeForExpr(callExprAST->GetArgs()[i].get()));
        if (!argsV.back()) {
            return nullptr;
        }
    }
    return builder->CreateCall(callee, argsV, "calltemp");
}

Value* GenerateCodeForIfExpr(const IfExprAST* ifExprAST) {
    Value* conditionVal = GenerateCodeForExpr(ifExprAST->GetCondtionExpr());
    if (!conditionVal) {
        return nullptr;
    }
    conditionVal = builder->CreateFCmpONE(conditionVal, ConstantFP::get(*theContext, APFloat(0.0)), "ifcond");
    Function* theFunction = builder->GetInsertBlock()->getParent();
    BasicBlock *thenBB = BasicBlock::Create(*theContext, "then", theFunction);
    BasicBlock *elseBB = BasicBlock::Create(*theContext, "else");
    BasicBlock *mergeBB = BasicBlock::Create(*theContext, "ifcont");
    builder->CreateCondBr(conditionVal, thenBB, elseBB);

    // Generate IR for then expression in the then branch.
    builder->SetInsertPoint(thenBB);
    Value* thenVal = GenerateCodeForExpr(ifExprAST->GetThenExpr());
    if (!thenVal) {
        return nullptr;
    }
    // Jump to mergeBB at the end of then branch.
    builder->CreateBr(mergeBB);
    // Get the last block of the then branch (it may be updated in the generation) for phi.
    thenBB = builder->GetInsertBlock();

    // Generate IR for else expression in the else branch.
    theFunction->insert(theFunction->end(), elseBB);
    builder->SetInsertPoint(elseBB);
    Value* elseVal = GenerateCodeForExpr(ifExprAST->GetElseExpr());
    if (!elseVal) {
        return nullptr;
    }
    // Jump to mergeBB at the end of then branch.
    builder->CreateBr(mergeBB);
    // Get the last block of the else branch (it may be updated in the generation) for phi.
    elseBB = builder->GetInsertBlock();

    theFunction->insert(theFunction->end(), mergeBB);
    builder->SetInsertPoint(mergeBB);
    PHINode* phi = builder->CreatePHI(Type::getDoubleTy(*theContext), 2, "iftmp");
    phi->addIncoming(thenVal, thenBB);
    phi->addIncoming(elseVal, elseBB);
    return phi;
}

Value* GenerateCodeForExpr(const ExprAST* exprAST)
{
    auto numberExprAST = dynamic_cast<const NumberExprAST*>(exprAST);
    if (numberExprAST != nullptr) {
        return GenerateCodeForNumberExpr(numberExprAST);
    }
    auto variableExprAST = dynamic_cast<const VariableExprAST*>(exprAST);
    if (variableExprAST != nullptr) {
        return GenerateCodeForVariableExpr(variableExprAST);
    }
    auto binaryExprAST = dynamic_cast<const BinaryExprAST*>(exprAST);
    if (binaryExprAST != nullptr) {
        return GenerateCodeForBinaryExpr(binaryExprAST);
    }
    auto callExprAST = dynamic_cast<const CallExprAST*>(exprAST);
    if (callExprAST != nullptr) {
        return GenerateCodeForCallExpr(callExprAST);
    }
    auto ifExprAST = dynamic_cast<const IfExprAST*>(exprAST);
    if (ifExprAST != nullptr) {
        return GenerateCodeForIfExpr(ifExprAST);
    }
    return nullptr;
}

Function* GenerateCodeForPrototype(const PrototypeAST* prototypeAST)
{
    std::vector<Type*> argTypes(prototypeAST->GetArgs().size(), Type::getDoubleTy(*theContext));
    FunctionType* fType = FunctionType::get(Type::getDoubleTy(*theContext), argTypes, false);
    Function* f = Function::Create(fType, Function::ExternalLinkage, prototypeAST->GetName(), theModule.get());
    unsigned int i = 0;
    for (auto& arg : f->args()) {
        arg.setName(prototypeAST->GetArgs()[i++]);
    }
    return f;
}

Function* GenerateCodeForFunction(const FunctionAST* functionAST)
{
    Function* f = theModule->getFunction(functionAST->GetPrototype()->GetName());
    if (!f) {
        f = GenerateCodeForPrototype(functionAST->GetPrototype());
    }
    if (!f) {
        return nullptr;
    }
    if (!f->empty()) {
        LogErrorV("Function cannot be redefined");
        return nullptr;
    }

    BasicBlock* bb = BasicBlock::Create(*theContext, "entry", f);
    builder->SetInsertPoint(bb);
    namedValues.clear();
    for (auto& arg : f->args()) {
        namedValues[std::string(arg.getName())] = &arg;
    }

    if (Value* returnValue = GenerateCodeForExpr(functionAST->GetBody())) {
        builder->CreateRet(returnValue);
        verifyFunction(*f);
        return f;
    }
    f->eraseFromParent();
    return nullptr;
}

Function* RunOptmizationPasses(Function* f)
{
    theFPM->run(*f, *theFAM);
    return f;
}
