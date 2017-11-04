#include <map>
#include "llvm/ADT/STLExtras.h"
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "token.h"

static int currentToken;
static int getNextToken() { return currentToken = Lexer::getToken(); }

static std::map<char, int> binaryOperatorsPrecedence;

static int getTokenPrecedence() {
    if (!isascii(currentToken)) {
        return -1;
    }

    int tokenPrecedence = binaryOperatorsPrecedence[currentToken];
    if (tokenPrecedence <= 0)
        return -1;
    return tokenPrecedence;
}

std::unique_ptr<ExprAST> logError(const char *Str) {
    fprintf(stderr, "Error: %s\n", Str);
    return nullptr;
}
std::unique_ptr<PrototypeAST> logErrorP(const char *Str) {
    logError(Str);
    return nullptr;
}

static std::unique_ptr<ExprAST> parseExpression();

/// numberexpr ::= number
static std::unique_ptr<ExprAST> parseNumberExpr() {
    auto Result = llvm::make_unique<NumberExprAST>(Lexer::NumVal);
    getNextToken(); // consume the number
    return std::move(Result);
}

/// parenexpr ::= '(' expression ')'
static std::unique_ptr<ExprAST> parseParenExpr() {
    getNextToken(); // eat (.
    auto V = parseExpression();
    if (!V) {
        return nullptr;
    }

    if (currentToken != ')') {
        return logError("expected ')'");
    }
    getNextToken(); // eat ).
    return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
static std::unique_ptr<ExprAST> parseIdentifierExpr() {
    std::string IdName = Lexer::IdentifierStr;

    getNextToken(); // eat identifier.

    if (currentToken != '(') // Simple variable ref.
        return llvm::make_unique<VariableExprAST>(IdName);

    // Call.
    getNextToken(); // eat (
    std::vector<std::unique_ptr<ExprAST>> Args;
    if (currentToken != ')') {
        while (true) {
            if (auto Arg = parseExpression())
                Args.push_back(std::move(Arg));
            else
                return nullptr;

            if (currentToken == ')')
                break;

            if (currentToken != ',')
                return logError("Expected ')' or ',' in argument list");
            getNextToken();
        }
    }

    // Eat the ')'.
    getNextToken();

    return llvm::make_unique<CallExprAST>(IdName, std::move(Args));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
static std::unique_ptr<ExprAST> parsePrimary() {
    switch (currentToken) {
        default:
            return logError("unknown token when expecting an expression");
        case static_cast<int>(Token::identifier):
            return parseIdentifierExpr();
        case static_cast<int>(Token::number):
            return parseNumberExpr();
        case '(':
            return parseParenExpr();
    }
}

/// binoprhs
///   ::= ('+' primary)*
static std::unique_ptr<ExprAST> parseBinOpRHS(int exprPrec, std::unique_ptr<ExprAST> lhs) {
    // If this is a binop, find its precedence.
    while (true) {
        int TokPrec = getTokenPrecedence();

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (TokPrec < exprPrec) {
            return lhs;
        }

        // Okay, we know this is a binop.
        int binOp = currentToken;
        getNextToken(); // eat binop

        // Parse the primary expression after the binary operator.
        auto rhs = parsePrimary();
        if (!rhs) {
            return nullptr;
        }

        // If binOp binds less tightly with rhs than the operator after rhs, let
        // the pending operator take rhs as its lhs.
        int NextPrec = getTokenPrecedence();
        if (TokPrec < NextPrec) {
            rhs = parseBinOpRHS(TokPrec + 1, std::move(rhs));
            if (!rhs) {
                return nullptr;
            }
        }

        // Merge lhs/rhs.
        lhs = llvm::make_unique<BinaryExprAST>(binOp, std::move(lhs), std::move(rhs));
    }
}

/// expression
///   ::= primary binoprhs
///
static std::unique_ptr<ExprAST> parseExpression() {
    auto lhs = parsePrimary();
    if (!lhs) {
        return nullptr;
    }

    return parseBinOpRHS(0, std::move(lhs));
}

/// prototype
///   ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> parsePrototype() {
    if (static_cast<Token>(currentToken) != Token::identifier) {
        return logErrorP("Expected function name in prototype");
    }

    std::string FnName = Lexer::IdentifierStr;
    getNextToken();

    if (currentToken != '(') {
        return logErrorP("Expected '(' in prototype");
    }

    std::vector<std::string> ArgNames;
    while (static_cast<Token>(getNextToken()) == Token::identifier) {
        ArgNames.push_back(Lexer::IdentifierStr);
    }
    if (currentToken != ')') {
        return logErrorP("Expected ')' in prototype");
    }

    // success.
    getNextToken(); // eat ')'.

    return llvm::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

/// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> parseDefinition() {
    getNextToken(); // eat def.
    auto proto = parsePrototype();
    if (!proto) {
        return nullptr;
    }

    if (auto E = parseExpression()) {
        return llvm::make_unique<FunctionAST>(std::move(proto), std::move(E));
    }
    return nullptr;
}

/// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> parseTopLevelExpr() {
    if (auto expr = parseExpression()) {
        // Make an anonymous proto.
        auto Proto = llvm::make_unique<PrototypeAST>("__anon_expr", std::vector<std::string>());
        return llvm::make_unique<FunctionAST>(std::move(Proto), std::move(expr));
    }
    return nullptr;
}

/// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> parseExtern() {
    getNextToken(); // eat extern.
    return parsePrototype();
}

//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//

static void handleDefinition() {
    if (parseDefinition()) {
        fprintf(stderr, "Parsed a function definition.\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void handleExtern() {
    if (parseExtern()) {
        fprintf(stderr, "Parsed an extern\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void handleTopLevelExpression() {
    // Evaluate a top-level expression into an anonymous function.
    if (parseTopLevelExpr()) {
        fprintf(stderr, "Parsed a top-level expr\n");
    } else {
        // Skip token for error recovery.
        getNextToken();
    }
}

/// top ::= definition | external | expression | ';'
static void mainLoop() {
    while (true) {
        fprintf(stderr, "ready> ");
        switch (currentToken) {
            case static_cast<int>(Token::eof):
                return;
            case ';': // ignore top-level semicolons.
                getNextToken();
                break;
            case static_cast<int>(Token::def):
                handleDefinition();
                break;
            case static_cast<int>(Token::ext):
                handleExtern();
                break;
            default:
                handleTopLevelExpression();
                break;
        }
    }
}

namespace Parser {
    void Parse() {
        // Install standard binary operators.
        // 1 is lowest precedence.
        binaryOperatorsPrecedence['<'] = 10;
        binaryOperatorsPrecedence['+'] = 20;
        binaryOperatorsPrecedence['-'] = 20;
        binaryOperatorsPrecedence['*'] = 40; // highest.

        getNextToken();

        mainLoop();
    }
}