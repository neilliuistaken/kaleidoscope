#ifndef KALEIDOSCOPE_AST
#define KALEIDOSCOPE_AST

#include <memory>
#include <string>
#include <vector>

class ExprAST {
public:
    virtual ~ExprAST() = default;

    void PrettyPrint() const
    {
        PrettyPrint(0);
    }

    void PrettyPrint(int indent) const
    {
        PrettyPrint(indent, indent);
    }

    virtual void PrettyPrint(int indent, int titleIndent) const = 0;
};

class NumberExprAST : public ExprAST {
private:
    double value;

public:
    NumberExprAST(double value) : value(value) { }

    double GetValue() const
    {
        return value;
    }

    virtual void PrettyPrint(int indent, int titleIndent) const override;
};

class VariableExprAST : public ExprAST {
private:
    std::string name;

public:
    VariableExprAST(const std::string& name) : name(name) { }

    std::string GetName() const
    {
        return name;
    }

    virtual void PrettyPrint(int indent, int titleIndent) const override;
};

class BinaryExprAST : public ExprAST {
private:
    char op;

public:
    BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS, std::unique_ptr<ExprAST> RHS)
        : op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) { }
    
    char GetOp() const
    {
        return op;
    }
    
    virtual void PrettyPrint(int indent, int titleIndent) const override;
    std::unique_ptr<ExprAST> LHS, RHS;
};

class CallExprAST : public ExprAST {
private:
    std::string callee;
    std::vector<std::unique_ptr<ExprAST>> args;

public:
    CallExprAST(const std::string& callee, std::vector<std::unique_ptr<ExprAST>> args)
        : callee(callee), args(std::move(args)) { }
    
    std::string GetCallee() const
    {
        return callee;
    }

    const std::vector<std::unique_ptr<ExprAST>>& GetArgs() const
    {
        return args;
    }

    virtual void PrettyPrint(int indent, int titleIndent) const override;
};

class PrototypeAST {
private:
    std::string name;
    std::vector<std::string> args;

public:
    PrototypeAST(const std::string& name, std::vector<std::string> args)
        : name(name), args(std::move(args)) { }

    const std::string& GetName() const
    {
        return name;
    }

    const std::vector<std::string>& GetArgs() const
    {
        return args;
    }

    void PrettyPrint() const;
};

class FunctionAST {
private:
    std::unique_ptr<PrototypeAST> prototype;
    std::unique_ptr<ExprAST> body;

public:
    FunctionAST(std::unique_ptr<PrototypeAST> prototype, std::unique_ptr<ExprAST> body)
        : prototype(std::move(prototype)), body(std::move(body)) { }
    
    const PrototypeAST* GetPrototype() const
    {
        return prototype.get();
    }

    const ExprAST* GetBody() const
    {
        return body.get();
    }

    void PrettyPrint() const;
};

#endif // KALEIDOSCOPE_AST