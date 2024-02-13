#ifndef KALEIDOSCOPE_LEXER
#define KALEIDOSCOPE_LEXER

#include "AST.h"

enum Token {
    tok_eof = -1,

    tok_def = -2,
    tok_extern = -3,

    tok_identifier = -4,
    tok_number = -5
};

int GetCurrentToken();
int GetNextToken();
std::unique_ptr<PrototypeAST> ParseExtern();
std::unique_ptr<FunctionAST> ParseDefinition();
std::unique_ptr<FunctionAST> ParseTopLevelExpr();

#endif // KALEIDOSCOPE_LEXER