#include "Lexer.h"
#include "Parser.h"
#include "AST.h"
#include "Token.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Minimal test harness (mirrors the one used by test_lexer.cpp)
// ─────────────────────────────────────────────────────────────────────────────
static int s_passed = 0;
static int s_failed = 0;

#define EXPECT_TRUE(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "  FAIL  " << __FILE__ << ":" << __LINE__ \
                      << "  expected true: " #expr "\n"; \
            ++s_failed; \
        } else { ++s_passed; } \
    } while (false)

#define EXPECT_FALSE(expr) EXPECT_TRUE(!(expr))

#define EXPECT_EQ(a, b) \
    do { \
        auto _a = (a); auto _b = (b); \
        if (_a != _b) { \
            std::cerr << "  FAIL  " << __FILE__ << ":" << __LINE__ \
                      << "  " << _a << " != " << _b << "\n"; \
            ++s_failed; \
        } else { ++s_passed; } \
    } while (false)

// ── Helpers ──────────────────────────────────────────────────────────────────

// Parse source and return the program; set *ok=false if parser had errors.
static std::vector<StmtPtr> parse(const std::string& src, bool* ok = nullptr) {
    Lexer  lex(src);
    auto   tokens  = lex.tokenize();
    Parser parser(std::move(tokens));
    auto   program = parser.parse();
    if (ok) *ok = !parser.hadError();
    return program;
}

// Dump the AST of the first statement to a string so tests can inspect it.
static std::string dumpFirst(const std::vector<StmtPtr>& prog) {
    if (prog.empty()) return "<empty>";
    std::ostringstream oss;
    printStmt(oss, *prog[0]);
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

// ── Let statements ───────────────────────────────────────────────────────────
static void test_let_integer() {
    std::cout << "test_let_integer\n";
    bool ok;
    auto prog = parse("let x = 42;", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog.size(), 1u);
    EXPECT_EQ(prog[0]->kind, StmtKind::Let);
    EXPECT_EQ(prog[0]->name, std::string("x"));
    EXPECT_EQ(prog[0]->expr->kind, ExprKind::NumberLit);
    EXPECT_EQ(prog[0]->expr->intValue, 42);
}

static void test_let_boolean_true() {
    std::cout << "test_let_boolean_true\n";
    bool ok;
    auto prog = parse("let flag = true;", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->kind, StmtKind::Let);
    EXPECT_EQ(prog[0]->expr->kind, ExprKind::BoolLit);
    EXPECT_TRUE(prog[0]->expr->boolValue);
}

static void test_let_boolean_false() {
    std::cout << "test_let_boolean_false\n";
    bool ok;
    auto prog = parse("let flag = false;", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->expr->kind, ExprKind::BoolLit);
    EXPECT_FALSE(prog[0]->expr->boolValue);
}

static void test_let_identifier_rhs() {
    std::cout << "test_let_identifier_rhs\n";
    bool ok;
    auto prog = parse("let y = x;", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->kind, StmtKind::Let);
    EXPECT_EQ(prog[0]->expr->kind, ExprKind::Variable);
    EXPECT_EQ(prog[0]->expr->name, std::string("x"));
}

// ── Assignment ───────────────────────────────────────────────────────────────
static void test_assignment() {
    std::cout << "test_assignment\n";
    bool ok;
    auto prog = parse("let x = 0; x = 10;", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog.size(), 2u);
    EXPECT_EQ(prog[1]->kind, StmtKind::Assign);
    EXPECT_EQ(prog[1]->name, std::string("x"));
    EXPECT_EQ(prog[1]->expr->intValue, 10);
}

// ── Arithmetic expressions ────────────────────────────────────────────────────
static void test_addition_expr() {
    std::cout << "test_addition_expr\n";
    bool ok;
    auto prog = parse("let r = 1 + 2;", &ok);
    EXPECT_TRUE(ok);
    auto& e = prog[0]->expr;
    EXPECT_EQ(e->kind, ExprKind::Binary);
    EXPECT_EQ(e->op, TokenType::PLUS);
    EXPECT_EQ(e->lhs->intValue, 1);
    EXPECT_EQ(e->rhs->intValue, 2);
}

static void test_operator_precedence() {
    std::cout << "test_operator_precedence\n";
    // 2 + 3 * 4  should parse as  2 + (3 * 4)
    bool ok;
    auto prog = parse("let r = 2 + 3 * 4;", &ok);
    EXPECT_TRUE(ok);
    auto& e = prog[0]->expr;
    EXPECT_EQ(e->kind, ExprKind::Binary);
    EXPECT_EQ(e->op, TokenType::PLUS);          // outer is +
    EXPECT_EQ(e->lhs->intValue, 2);
    EXPECT_EQ(e->rhs->kind, ExprKind::Binary);
    EXPECT_EQ(e->rhs->op, TokenType::STAR);     // inner is *
}

static void test_grouping_overrides_precedence() {
    std::cout << "test_grouping_overrides_precedence\n";
    // (2 + 3) * 4  should parse as  (2 + 3) * 4
    bool ok;
    auto prog = parse("let r = (2 + 3) * 4;", &ok);
    EXPECT_TRUE(ok);
    auto& e = prog[0]->expr;
    EXPECT_EQ(e->kind, ExprKind::Binary);
    EXPECT_EQ(e->op, TokenType::STAR);           // outer is *
    EXPECT_EQ(e->lhs->kind, ExprKind::Grouping); // left is a group
    EXPECT_EQ(e->rhs->intValue, 4);
}

static void test_unary_negate() {
    std::cout << "test_unary_negate\n";
    bool ok;
    auto prog = parse("let r = -5;", &ok);
    EXPECT_TRUE(ok);
    auto& e = prog[0]->expr;
    EXPECT_EQ(e->kind, ExprKind::Unary);
    EXPECT_EQ(e->op, TokenType::MINUS);
    EXPECT_EQ(e->lhs->intValue, 5);
}

static void test_unary_not() {
    std::cout << "test_unary_not\n";
    bool ok;
    auto prog = parse("let r = !true;", &ok);
    EXPECT_TRUE(ok);
    auto& e = prog[0]->expr;
    EXPECT_EQ(e->kind, ExprKind::Unary);
    EXPECT_EQ(e->op, TokenType::BANG);
    EXPECT_EQ(e->lhs->kind, ExprKind::BoolLit);
}

// ── Comparison / equality ────────────────────────────────────────────────────
static void test_comparison_lt() {
    std::cout << "test_comparison_lt\n";
    bool ok;
    auto prog = parse("let r = a < b;", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->expr->op, TokenType::LT);
}

static void test_equality_eqeq() {
    std::cout << "test_equality_eqeq\n";
    bool ok;
    auto prog = parse("let r = x == 0;", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->expr->op, TokenType::EQEQ);
}

static void test_equality_neq() {
    std::cout << "test_equality_neq\n";
    bool ok;
    auto prog = parse("let r = x != 0;", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->expr->op, TokenType::BANG_EQ);
}

// ── Print ─────────────────────────────────────────────────────────────────────
static void test_print_stmt() {
    std::cout << "test_print_stmt\n";
    bool ok;
    auto prog = parse("print 42;", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->kind, StmtKind::Print);
    EXPECT_EQ(prog[0]->expr->intValue, 42);
}

static void test_print_variable() {
    std::cout << "test_print_variable\n";
    bool ok;
    auto prog = parse("print result;", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->kind, StmtKind::Print);
    EXPECT_EQ(prog[0]->expr->kind, ExprKind::Variable);
    EXPECT_EQ(prog[0]->expr->name, std::string("result"));
}

// ── Input ────────────────────────────────────────────────────────────────────
static void test_input_expr() {
    std::cout << "test_input_expr\n";
    bool ok;
    auto prog = parse("let n = input;", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->expr->kind, ExprKind::Input);
}

// ── If / else ────────────────────────────────────────────────────────────────
static void test_if_no_else() {
    std::cout << "test_if_no_else\n";
    bool ok;
    auto prog = parse("if (x) { print x; }", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->kind, StmtKind::If);
    EXPECT_TRUE(prog[0]->thenBranch != nullptr);
    EXPECT_TRUE(prog[0]->elseBranch == nullptr);
}

static void test_if_with_else() {
    std::cout << "test_if_with_else\n";
    bool ok;
    auto prog = parse("if (x) { print 1; } else { print 2; }", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->kind, StmtKind::If);
    EXPECT_TRUE(prog[0]->thenBranch != nullptr);
    EXPECT_TRUE(prog[0]->elseBranch != nullptr);
}

static void test_if_else_if_chain() {
    std::cout << "test_if_else_if_chain\n";
    bool ok;
    auto prog = parse(
        "if (a) { print 1; } else if (b) { print 2; } else { print 3; }", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->kind, StmtKind::If);
    // else branch should itself be an If
    EXPECT_TRUE(prog[0]->elseBranch != nullptr);
    EXPECT_EQ(prog[0]->elseBranch->kind, StmtKind::If);
    // that if should have its own else
    EXPECT_TRUE(prog[0]->elseBranch->elseBranch != nullptr);
}

// ── While ────────────────────────────────────────────────────────────────────
static void test_while_loop() {
    std::cout << "test_while_loop\n";
    bool ok;
    auto prog = parse("while (n > 0) { n = n - 1; }", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->kind, StmtKind::While);
    EXPECT_EQ(prog[0]->expr->op, TokenType::GT);
    EXPECT_TRUE(prog[0]->body != nullptr);
    EXPECT_EQ(prog[0]->body->kind, StmtKind::Block);
}

// ── Blocks / multiple statements ─────────────────────────────────────────────
static void test_block_multiple_stmts() {
    std::cout << "test_block_multiple_stmts\n";
    bool ok;
    auto prog = parse("{ let a = 1; let b = 2; print a; }", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->kind, StmtKind::Block);
    EXPECT_EQ(prog[0]->stmts.size(), 3u);
}

static void test_multiple_top_level_stmts() {
    std::cout << "test_multiple_top_level_stmts\n";
    bool ok;
    auto prog = parse("let a = 1; let b = 2; print a;", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog.size(), 3u);
}

// ── Nested structures ─────────────────────────────────────────────────────────
static void test_nested_while_if() {
    std::cout << "test_nested_while_if\n";
    bool ok;
    auto prog = parse(
        "while (n > 0) {"
        "  if (n == 1) { print n; }"
        "  n = n - 1;"
        "}", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(prog[0]->kind, StmtKind::While);
    auto& body = prog[0]->body;
    EXPECT_EQ(body->kind, StmtKind::Block);
    EXPECT_EQ(body->stmts.size(), 2u);
    EXPECT_EQ(body->stmts[0]->kind, StmtKind::If);
    EXPECT_EQ(body->stmts[1]->kind, StmtKind::Assign);
}

// ── AST dump sanity checks ───────────────────────────────────────────────────
static void test_ast_dump_let() {
    std::cout << "test_ast_dump_let\n";
    auto prog = parse("let x = 7;");
    std::string dump = dumpFirst(prog);
    EXPECT_TRUE(dump.find("Let(x)") != std::string::npos);
    EXPECT_TRUE(dump.find("Number(7)") != std::string::npos);
}

static void test_ast_dump_binary() {
    std::cout << "test_ast_dump_binary\n";
    auto prog = parse("let r = 3 + 4;");
    std::string dump = dumpFirst(prog);
    EXPECT_TRUE(dump.find("Binary(PLUS)") != std::string::npos);
    EXPECT_TRUE(dump.find("Number(3)") != std::string::npos);
    EXPECT_TRUE(dump.find("Number(4)") != std::string::npos);
}

// ── Error recovery ───────────────────────────────────────────────────────────
static void test_error_missing_semicolon() {
    std::cout << "test_error_missing_semicolon\n";
    bool ok;
    // Missing ';' — parser should report error
    parse("let x = 5", &ok);
    EXPECT_FALSE(ok);
}

static void test_error_recovery_continues() {
    std::cout << "test_error_recovery_continues\n";
    // First statement is broken, second is valid — should still parse second
    bool ok;
    auto prog = parse("let x = ; let y = 2;", &ok);
    EXPECT_FALSE(ok);           // error was reported
    // At least one statement should have been recovered
    EXPECT_TRUE(prog.size() >= 1u);
}

static void test_error_missing_rparen() {
    std::cout << "test_error_missing_rparen\n";
    bool ok;
    parse("if (x { print x; }", &ok);
    EXPECT_FALSE(ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// Runner
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "\n=== CVM++ Parser Tests ===\n\n";

    test_let_integer();
    test_let_boolean_true();
    test_let_boolean_false();
    test_let_identifier_rhs();
    test_assignment();
    test_addition_expr();
    test_operator_precedence();
    test_grouping_overrides_precedence();
    test_unary_negate();
    test_unary_not();
    test_comparison_lt();
    test_equality_eqeq();
    test_equality_neq();
    test_print_stmt();
    test_print_variable();
    test_input_expr();
    test_if_no_else();
    test_if_with_else();
    test_if_else_if_chain();
    test_while_loop();
    test_block_multiple_stmts();
    test_multiple_top_level_stmts();
    test_nested_while_if();
    test_ast_dump_let();
    test_ast_dump_binary();
    test_error_missing_semicolon();
    test_error_recovery_continues();
    test_error_missing_rparen();

    std::cout << "\n─────────────────────────────\n";
    std::cout << "  Passed: " << s_passed << "\n";
    std::cout << "  Failed: " << s_failed << "\n";
    std::cout << "─────────────────────────────\n\n";

    return s_failed == 0 ? 0 : 1;
}
