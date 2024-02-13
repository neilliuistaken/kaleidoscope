#include "AST.h"

#include <string>
#include <iostream>

#define INDENT_SPACES 3

void NumberExprAST::PrettyPrint([[maybe_unused]] int indent, int titleIndent) const {
    std::cout << std::string(titleIndent, ' ');
    std::cout << "NumberExpr: " << value << std::endl;
}

void VariableExprAST::PrettyPrint([[maybe_unused]] int indent, int titleIndent) const {
    std::cout << std::string(titleIndent, ' ');
    std::cout << "VariableExprAST: " << name << std::endl;
}

void BinaryExprAST::PrettyPrint(int indent, int titleIndent) const {
    std::cout << std::string(titleIndent, ' ');
    std::cout << "BinaryExprAST: " << op << std::endl;
    std::cout << std::string(indent + INDENT_SPACES, ' ');
    std::cout << "LHS = ";
    LHS->PrettyPrint(indent + INDENT_SPACES, 0);
    std::cout << std::string(indent + INDENT_SPACES, ' ');
    std::cout << "RHS = ";
    RHS->PrettyPrint(indent + INDENT_SPACES, 0);
}

void CallExprAST::PrettyPrint(int indent, int titleIndent) const {
    std::cout << std::string(indent, ' ');
    std::cout << "CallExprAST: " << callee << std::endl;
    for (int i = 0; i < args.size(); i++) {
        std::cout << std::string(indent + INDENT_SPACES, ' ');
        std::cout << "ARG[" << i << "]:" << std::endl;
        args[i]->PrettyPrint(indent + INDENT_SPACES);
    }
}

void PrototypeAST::PrettyPrint() const
{
    std::cout << "PrototypeAST: ";
    std::string argsStr = "";
    for (auto s = args.begin(); s != args.end(); ++s) {
        argsStr += *s;
        if (s != args.end() - 1) {
            argsStr += ", ";
        }
    }
    std::cout << "def " << name << "(" << argsStr << ")" << std::endl;
}

void FunctionAST::PrettyPrint() const
{
    std::cout << "FunctionAST:" << std::endl;
    std::cout << std::string(INDENT_SPACES, ' ');
    std::cout << "prototype = ";
    prototype->PrettyPrint();
    std::cout << std::string(INDENT_SPACES, ' ');
    std::cout << "body = ";
    body->PrettyPrint(INDENT_SPACES, 0);
}
