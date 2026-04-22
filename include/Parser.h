#pragma once

#include "AST.h"
#include "Token.h"

#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Parser
//
// Responsibility: consume the flat token list produced by the Lexer and build
// a tree of Stmt / Expr nodes (the AST).
//
// Usage:
//     Parser parser(tokens);
//     std::vector<StmtPtr> program = parser.parse();
//     if (parser.hadError()) { /* handle */ }
//
// The parser uses recursive descent and implements the grammar documented in
// AST.h.  On encountering a syntax error it prints a message and attempts
// *panic-mode recovery* — consuming tokens until it reaches the next statement
// boundary (;  or  }) so that it can continue parsing and report as many
// errors as possible in a single pass.
// ─────────────────────────────────────────────────────────────────────────────
class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // Parse the full token list and return a vector of top-level statements.
    std::vector<StmtPtr> parse();

    // True if any parse error was encountered during the last parse() call.
    bool hadError() const { return m_hadError; }

private:
    // ── Statement parsers ────────────────────────────────────────────────
    StmtPtr parseStatement();
    StmtPtr parseLetStmt();
    StmtPtr parseIfStmt();
    StmtPtr parseWhileStmt();
    StmtPtr parsePrintStmt();
    StmtPtr parseAssignOrExprStmt();
    StmtPtr parseBlock();

    // ── Expression parsers (Pratt / recursive descent) ───────────────────
    ExprPtr parseExpr();
    ExprPtr parseEquality();
    ExprPtr parseComparison();
    ExprPtr parseAddition();
    ExprPtr parseMultiply();
    ExprPtr parseUnary();
    ExprPtr parsePrimary();

    // ── Token stream helpers ──────────────────────────────────────────────
    const Token& peek()  const;           // current token (not consumed)
    const Token& prev()  const;           // last consumed token
    bool  check(TokenType t) const;       // peek().type == t, no consume
    bool  match(TokenType t);             // consume if check(t), return bool
    bool  match(TokenType a, TokenType b);
    bool  match(TokenType a, TokenType b, TokenType c, TokenType d);
    const Token& advance();               // consume and return current token
    const Token& consume(TokenType t, const std::string& msg);  // or error
    bool  isAtEnd() const;

    // ── Error handling ────────────────────────────────────────────────────
    void  error(const Token& tok, const std::string& msg);
    void  synchronize();                  // panic-mode recovery

    // ── State ─────────────────────────────────────────────────────────────
    std::vector<Token> m_tokens;
    std::size_t        m_current = 0;
    bool               m_hadError = false;
};
