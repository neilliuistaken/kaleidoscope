#include "Lexer.h"

#include <unordered_map>
#include <string>
#include <iostream>

#include "AST.h"

static std::string IdentifierStr;
static double NumVal;
static int currentToken;
static std::unordered_map<char, int> binopPrecedence = {
    {'<', 10},
    {'+', 20},
    {'-', 20},
    {'*', 40},
};

int GetCurrentToken()
{
    return currentToken;
}

static int GetTokPrecendence()
{
    if (!isascii(currentToken)) {
        return -1;
    }

    int tokPrec = binopPrecedence[currentToken];
    if (tokPrec <= 0) {
        return -1;
    }

    return tokPrec;
}

static int gettok() {
    static int LastChar = ' ';

    while (isspace(LastChar)) {
        LastChar = getchar();
    }

    if (isalpha(LastChar)) {
        IdentifierStr = LastChar;
        while (isalnum((LastChar = getchar()))) {
            IdentifierStr += LastChar;
        }

        if (IdentifierStr == "def") {
            return tok_def;
        }
        if (IdentifierStr == "extern") {
            return tok_extern;
        }
        if (IdentifierStr == "if") {
            return tok_if;
        }
        if (IdentifierStr == "then") {
            return tok_then;
        }
        if (IdentifierStr == "else") {
            return tok_else;
        }
        return tok_identifier;
    }

    if (isdigit(LastChar) || LastChar == '.') {
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while (isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), nullptr);
        return tok_number;
    }

    if (LastChar == '#') {
        do {
            LastChar = getchar();
        } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

        if (LastChar != EOF) {
            return gettok();
        }
    }

    if (LastChar == EOF) {
        return tok_eof;
    }

    int thisChar = LastChar;
    LastChar = getchar();
    return thisChar;
}

int GetNextToken() {
    return currentToken = gettok();
}

std::unique_ptr<ExprAST> LogError(const char* str) {
    fprintf(stderr, "Error: %s\n", str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char* str) {
    LogError(str);
    return nullptr;
}

std::unique_ptr<ExprAST> ParseExpression();

static std::unique_ptr<ExprAST> ParseNumberExpr()
{
    auto result = std::make_unique<NumberExprAST>(NumVal);
    GetNextToken(); // Consume the number
    return std::move(result);
}

static std::unique_ptr<ExprAST> ParseParenthesisExpr()
{
    GetNextToken(); // Consume '('
    auto v = ParseExpression();
    if (!v) {
        return nullptr;
    }
    if (currentToken != ')') {
        return LogError("Expect ')'");
    }
    GetNextToken(); // eat ')'
    return v;
}

static std::unique_ptr<ExprAST> ParseIdentifierExpr()
{
    std::string idName = IdentifierStr;
    GetNextToken(); // Consume identifier
    if (currentToken != '(') {
        return std::make_unique<VariableExprAST>(idName);
    }
    GetNextToken();
    std::vector<std::unique_ptr<ExprAST>> args;
    if (currentToken != ')') {
        while(true) {
            if (auto arg = ParseExpression()) {
                args.push_back(std::move(arg));
            } else {
                return nullptr;
            }

            if (currentToken == ')') {
                break;
            }

            if (currentToken != ',') {
                return LogError("Expected ')' or ',' in argument list");
            }
            
            GetNextToken();
        }
    }

    GetNextToken(); // Consume ')'

    return std::make_unique<CallExprAST>(idName, std::move(args));
}

std::unique_ptr<ExprAST> ParseIfExpr()
{
    GetNextToken();

    auto condExpr = ParseExpression();
    if (!condExpr) {
        return nullptr;
    }

    if (currentToken != tok_then) {
        return LogError("expected then");
    }
    GetNextToken();

    auto thenExpr = ParseExpression();
    if (!thenExpr) {
        return nullptr;
    }
    if (currentToken != tok_else) {
        return LogError("expected else");
    }
    GetNextToken();

    auto elseExpr = ParseExpression();
    if (!elseExpr) {
        return nullptr;
    }

    return std::make_unique<IfExprAST>(std::move(condExpr), std::move(thenExpr), std::move(elseExpr));
}

static std::unique_ptr<ExprAST> ParsePrimary()
{
    switch (currentToken) {
        case tok_identifier:
            return ParseIdentifierExpr();
        case tok_number:
            return ParseNumberExpr();
        case '(':
            return ParseParenthesisExpr();
        case tok_if:
            return ParseIfExpr();
        default:
            return LogError("unknown token when expecting an expression");    
    }
}

std::unique_ptr<ExprAST> ParseBinOpRHS(int exprPrec, std::unique_ptr<ExprAST> LHS)
{
    while (true) {
        int tokPrec = GetTokPrecendence();

        if (tokPrec < exprPrec) {
            return LHS;
        }

        int binOp = currentToken; // Current binary operator
        GetNextToken();

        auto RHS = ParsePrimary();
        if (!RHS) {
            return nullptr;
        }

        int nextPrec = GetTokPrecendence(); // Get next binary operator, or -1 if it's not a binary operator
        if (tokPrec < nextPrec) {
            RHS = ParseBinOpRHS(tokPrec + 1, std::move(RHS));
            if (!RHS) {
                return nullptr;
            }
        }

        LHS = std::make_unique<BinaryExprAST>(binOp, std::move(LHS), std::move(RHS));
    }    
}

std::unique_ptr<PrototypeAST> ParsePrototype()
{
    if (currentToken != tok_identifier) {
        return LogErrorP("Expected function name in prototype");
    }

    std::string fnName = IdentifierStr;
    GetNextToken();

    if (currentToken != '(') {
        LogErrorP("Expected '(' in prototype");
    }

    std::vector<std::string> argNames;
    while (GetNextToken() == tok_identifier) {
        argNames.push_back(IdentifierStr);
    }
    if (currentToken != ')') {
        LogErrorP("Expected ')' in prototype");
    }
    GetNextToken();
    return std::make_unique<PrototypeAST>(fnName, std::move(argNames));
}

std::unique_ptr<FunctionAST> ParseDefinition()
{
    GetNextToken(); // Consume 'def' keyword
    auto proto = ParsePrototype();
    if (!proto) {
        return nullptr;
    }

    if (auto exp = ParseExpression()) {
        return std::make_unique<FunctionAST>(std::move(proto), std::move(exp));
    }
    return nullptr;
}

std::unique_ptr<PrototypeAST> ParseExtern()
{
    GetNextToken(); // Consume 'def' keyword
    return ParsePrototype();
}

std::unique_ptr<FunctionAST> ParseTopLevelExpr()
{
    if (auto e = ParseExpression()) {
        auto proto = std::make_unique<PrototypeAST>("__anonymours_expr", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(proto), std::move(e));
    }
    return nullptr;
}

std::unique_ptr<ExprAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if (!LHS) {
        return nullptr;
    }

    return ParseBinOpRHS(0, std::move(LHS));
}
