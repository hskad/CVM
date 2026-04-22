#pragma once

#include "Token.h"

#include <memory>
#include <string>
#include <vector>
#include <ostream>

// ─────────────────────────────────────────────────────────────────────────────
// Forward declarations (needed for mutual recursion between Expr and Stmt)
// ─────────────────────────────────────────────────────────────────────────────
struct Expr;
struct Stmt;

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// ═════════════════════════════════════════════════════════════════════════════
//  EXPRESSION  nodes
//
//  Grammar (precedence, lowest → highest):
//    expr        → equality
//    equality    → comparison ( ("==" | "!=") comparison )*
//    comparison  → addition   ( ("<" | "<=" | ">" | ">=") addition )*
//    addition    → multiply   ( ("+" | "-") multiply )*
//    multiply    → unary      ( ("*" | "/") unary )*
//    unary       → ("!" | "-") unary | primary
//    primary     → NUMBER | "true" | "false" | IDENTIFIER | "(" expr ")"
//                | "input"
// ═════════════════════════════════════════════════════════════════════════════

// Tag used to dispatch on the active node variant.
enum class ExprKind {
    NumberLit,      // integer literal
    BoolLit,        // true / false
    Variable,       // identifier read
    Unary,          // - expr   or   ! expr
    Binary,         // expr op expr
    Grouping,       // ( expr )
    Input,          // input  (reads a number from stdin at runtime)
};

struct Expr {
    ExprKind kind;
    int      line;  // source line, for error messages

    // ── NumberLit ─────────────────────────────────────────────────────────
    int         intValue  = 0;

    // ── BoolLit ───────────────────────────────────────────────────────────
    bool        boolValue = false;

    // ── Variable ──────────────────────────────────────────────────────────
    std::string name;           // also used by Unary for op lexeme

    // ── Unary / Binary ────────────────────────────────────────────────────
    TokenType   op  = TokenType::EOF_TOKEN;
    ExprPtr     lhs;            // Binary: left operand; Unary: operand
    ExprPtr     rhs;            // Binary: right operand (unused for Unary)

    // ── Grouping ──────────────────────────────────────────────────────────
    // reuses lhs

    // ── Constructors (factory functions below are preferred) ──────────────
    explicit Expr(ExprKind k, int ln) : kind(k), line(ln) {}
    Expr(const Expr&) = delete;
    Expr& operator=(const Expr&) = delete;
};

// ── Factory helpers ───────────────────────────────────────────────────────
inline ExprPtr makeNumber(int value, int line) {
    auto e = std::make_unique<Expr>(ExprKind::NumberLit, line);
    e->intValue = value;
    return e;
}

inline ExprPtr makeBool(bool value, int line) {
    auto e = std::make_unique<Expr>(ExprKind::BoolLit, line);
    e->boolValue = value;
    return e;
}

inline ExprPtr makeVariable(const std::string& name, int line) {
    auto e = std::make_unique<Expr>(ExprKind::Variable, line);
    e->name = name;
    return e;
}

inline ExprPtr makeUnary(TokenType op, ExprPtr operand, int line) {
    auto e = std::make_unique<Expr>(ExprKind::Unary, line);
    e->op  = op;
    e->lhs = std::move(operand);
    return e;
}

inline ExprPtr makeBinary(TokenType op, ExprPtr left, ExprPtr right, int line) {
    auto e = std::make_unique<Expr>(ExprKind::Binary, line);
    e->op  = op;
    e->lhs = std::move(left);
    e->rhs = std::move(right);
    return e;
}

inline ExprPtr makeGrouping(ExprPtr inner, int line) {
    auto e = std::make_unique<Expr>(ExprKind::Grouping, line);
    e->lhs = std::move(inner);
    return e;
}

inline ExprPtr makeInput(int line) {
    return std::make_unique<Expr>(ExprKind::Input, line);
}

// ═════════════════════════════════════════════════════════════════════════════
//  STATEMENT  nodes
//
//  Grammar:
//    program     → stmt* EOF
//    stmt        → letStmt | ifStmt | whileStmt | printStmt | exprStmt
//    letStmt     → "let" IDENTIFIER "=" expr ";"
//    ifStmt      → "if" "(" expr ")" block ( "else" block )?
//    whileStmt   → "while" "(" expr ")" block
//    printStmt   → "print" expr ";"
//    exprStmt    → expr ";"       (assignment: IDENTIFIER "=" expr ";")
//    block       → "{" stmt* "}"
// ═════════════════════════════════════════════════════════════════════════════

enum class StmtKind {
    Let,        // let name = expr ;
    Assign,     // name = expr ;
    If,         // if (cond) thenBlock [else elseBlock]
    While,      // while (cond) body
    Print,      // print expr ;
    Block,      // { stmt* }
    ExprStmt,   // expr ;   (currently unused but kept for extensibility)
};

struct Stmt {
    StmtKind              kind;
    int                   line;

    // ── Let / Assign ──────────────────────────────────────────────────────
    std::string           name;     // variable name

    // ── Let / Assign / Print / ExprStmt / If (condition) / While (cond) ──
    ExprPtr               expr;

    // ── If ────────────────────────────────────────────────────────────────
    StmtPtr               thenBranch;
    StmtPtr               elseBranch;  // may be nullptr

    // ── While ─────────────────────────────────────────────────────────────
    // condition → expr (shared with If)
    StmtPtr               body;

    // ── Block ─────────────────────────────────────────────────────────────
    std::vector<StmtPtr>  stmts;

    explicit Stmt(StmtKind k, int ln) : kind(k), line(ln) {}
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;
};

// ── Factory helpers ───────────────────────────────────────────────────────
inline StmtPtr makeLet(const std::string& name, ExprPtr init, int line) {
    auto s = std::make_unique<Stmt>(StmtKind::Let, line);
    s->name = name;
    s->expr = std::move(init);
    return s;
}

inline StmtPtr makeAssign(const std::string& name, ExprPtr value, int line) {
    auto s = std::make_unique<Stmt>(StmtKind::Assign, line);
    s->name = name;
    s->expr = std::move(value);
    return s;
}

inline StmtPtr makeIf(ExprPtr cond, StmtPtr then_, StmtPtr else_, int line) {
    auto s = std::make_unique<Stmt>(StmtKind::If, line);
    s->expr       = std::move(cond);
    s->thenBranch = std::move(then_);
    s->elseBranch = std::move(else_);
    return s;
}

inline StmtPtr makeWhile(ExprPtr cond, StmtPtr body, int line) {
    auto s = std::make_unique<Stmt>(StmtKind::While, line);
    s->expr = std::move(cond);
    s->body = std::move(body);
    return s;
}

inline StmtPtr makePrint(ExprPtr value, int line) {
    auto s = std::make_unique<Stmt>(StmtKind::Print, line);
    s->expr = std::move(value);
    return s;
}

inline StmtPtr makeBlock(std::vector<StmtPtr> stmts, int line) {
    auto s = std::make_unique<Stmt>(StmtKind::Block, line);
    s->stmts = std::move(stmts);
    return s;
}

inline StmtPtr makeExprStmt(ExprPtr e, int line) {
    auto s = std::make_unique<Stmt>(StmtKind::ExprStmt, line);
    s->expr = std::move(e);
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
// Stream helpers (needed by test harness EXPECT_EQ macro)
// ─────────────────────────────────────────────────────────────────────────────
inline std::ostream& operator<<(std::ostream& os, ExprKind k) {
    switch (k) {
        case ExprKind::NumberLit: return os << "NumberLit";
        case ExprKind::BoolLit:   return os << "BoolLit";
        case ExprKind::Variable:  return os << "Variable";
        case ExprKind::Unary:     return os << "Unary";
        case ExprKind::Binary:    return os << "Binary";
        case ExprKind::Grouping:  return os << "Grouping";
        case ExprKind::Input:     return os << "Input";
    }
    return os << "Unknown";
}

inline std::ostream& operator<<(std::ostream& os, StmtKind k) {
    switch (k) {
        case StmtKind::Let:      return os << "Let";
        case StmtKind::Assign:   return os << "Assign";
        case StmtKind::If:       return os << "If";
        case StmtKind::While:    return os << "While";
        case StmtKind::Print:    return os << "Print";
        case StmtKind::Block:    return os << "Block";
        case StmtKind::ExprStmt: return os << "ExprStmt";
    }
    return os << "Unknown";
}

// ─────────────────────────────────────────────────────────────────────────────
// AST printer  (indented text dump, for --debug-ast)
// ─────────────────────────────────────────────────────────────────────────────
void printExpr(std::ostream& os, const Expr& e, int indent = 0);
void printStmt(std::ostream& os, const Stmt& s, int indent = 0);
