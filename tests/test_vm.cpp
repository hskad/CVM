#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"
#include "VM.h"
#include "Chunk.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Minimal test harness
// ─────────────────────────────────────────────────────────────────────────────
static int s_passed = 0;
static int s_failed = 0;

#define EXPECT_TRUE(expr) \
    do { if (!(expr)) { \
        std::cerr << "  FAIL  " << __FILE__ << ":" << __LINE__ \
                  << "  expected true: " #expr "\n"; \
        ++s_failed; } else { ++s_passed; } } while (false)

#define EXPECT_EQ(a, b) \
    do { auto _a=(a); auto _b=(b); \
         if (!(_a == _b)) { \
             std::cerr << "  FAIL  " << __FILE__ << ":" << __LINE__ \
                       << "  got: [" << _a << "]  want: [" << _b << "]\n"; \
             ++s_failed; } else { ++s_passed; } } while (false)

// ─────────────────────────────────────────────────────────────────────────────
// run(src) → captured stdout string, VM result
// ─────────────────────────────────────────────────────────────────────────────
static std::string run(const std::string& src, VMResult* result = nullptr) {
    // Compile
    Lexer  lex(src);
    auto   tokens  = lex.tokenize();
    Parser parser(std::move(tokens));
    auto   program = parser.parse();
    Compiler comp;
    Chunk chunk = comp.compile(program);

    // Redirect stdout to a string
    std::ostringstream capture;
    std::streambuf* oldBuf = std::cout.rdbuf(capture.rdbuf());

    VM vm;
    VMResult res = vm.execute(chunk);

    std::cout.rdbuf(oldBuf);

    if (result) *result = res;
    return capture.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

// ── Literals & print ─────────────────────────────────────────────────────────
static void test_print_integer() {
    std::cout << "test_print_integer\n";
    EXPECT_EQ(run("print 42;"), "42\n");
}

static void test_print_negative() {
    std::cout << "test_print_negative\n";
    EXPECT_EQ(run("print -7;"), "-7\n");
}

static void test_print_true() {
    std::cout << "test_print_true\n";
    EXPECT_EQ(run("print true;"), "true\n");
}

static void test_print_false() {
    std::cout << "test_print_false\n";
    EXPECT_EQ(run("print false;"), "false\n");
}

// ── Variables ─────────────────────────────────────────────────────────────────
static void test_let_and_print() {
    std::cout << "test_let_and_print\n";
    EXPECT_EQ(run("let x = 10; print x;"), "10\n");
}

static void test_assign_and_print() {
    std::cout << "test_assign_and_print\n";
    EXPECT_EQ(run("let x = 1; x = 99; print x;"), "99\n");
}

static void test_multiple_variables() {
    std::cout << "test_multiple_variables\n";
    EXPECT_EQ(run("let a = 3; let b = 4; print a; print b;"), "3\n4\n");
}

// ── Arithmetic ────────────────────────────────────────────────────────────────
static void test_addition() {
    std::cout << "test_addition\n";
    EXPECT_EQ(run("print 2 + 3;"), "5\n");
}

static void test_subtraction() {
    std::cout << "test_subtraction\n";
    EXPECT_EQ(run("print 10 - 4;"), "6\n");
}

static void test_multiplication() {
    std::cout << "test_multiplication\n";
    EXPECT_EQ(run("print 6 * 7;"), "42\n");
}

static void test_division() {
    std::cout << "test_division\n";
    EXPECT_EQ(run("print 15 / 3;"), "5\n");
}

static void test_precedence() {
    std::cout << "test_precedence\n";
    // 2 + 3 * 4 = 14  (not 20)
    EXPECT_EQ(run("print 2 + 3 * 4;"), "14\n");
}

static void test_grouping() {
    std::cout << "test_grouping\n";
    EXPECT_EQ(run("print (2 + 3) * 4;"), "20\n");
}

static void test_unary_negate() {
    std::cout << "test_unary_negate\n";
    EXPECT_EQ(run("let x = 5; print -x;"), "-5\n");
}

static void test_complex_arithmetic() {
    std::cout << "test_complex_arithmetic\n";
    EXPECT_EQ(run("let a = 10; let b = 3; print a - b * 2;"), "4\n");
}

// ── Boolean logic ─────────────────────────────────────────────────────────────
static void test_not_true() {
    std::cout << "test_not_true\n";
    EXPECT_EQ(run("print !true;"), "false\n");
}

static void test_not_false() {
    std::cout << "test_not_false\n";
    EXPECT_EQ(run("print !false;"), "true\n");
}

// ── Comparisons ───────────────────────────────────────────────────────────────
static void test_eq_true() {
    std::cout << "test_eq_true\n";
    EXPECT_EQ(run("print 3 == 3;"), "true\n");
}

static void test_eq_false() {
    std::cout << "test_eq_false\n";
    EXPECT_EQ(run("print 3 == 4;"), "false\n");
}

static void test_neq() {
    std::cout << "test_neq\n";
    EXPECT_EQ(run("print 3 != 4;"), "true\n");
}

static void test_lt() {
    std::cout << "test_lt\n";
    EXPECT_EQ(run("print 2 < 5;"), "true\n");
    EXPECT_EQ(run("print 5 < 2;"), "false\n");
}

static void test_lte() {
    std::cout << "test_lte\n";
    EXPECT_EQ(run("print 3 <= 3;"), "true\n");
    EXPECT_EQ(run("print 4 <= 3;"), "false\n");
}

static void test_gt() {
    std::cout << "test_gt\n";
    EXPECT_EQ(run("print 5 > 2;"), "true\n");
    EXPECT_EQ(run("print 2 > 5;"), "false\n");
}

static void test_gte() {
    std::cout << "test_gte\n";
    EXPECT_EQ(run("print 3 >= 3;"), "true\n");
    EXPECT_EQ(run("print 2 >= 3;"), "false\n");
}

// ── If/else ───────────────────────────────────────────────────────────────────
static void test_if_taken() {
    std::cout << "test_if_taken\n";
    EXPECT_EQ(run("if (true) { print 1; }"), "1\n");
}

static void test_if_not_taken() {
    std::cout << "test_if_not_taken\n";
    EXPECT_EQ(run("if (false) { print 1; }"), "");
}

static void test_if_else_true_branch() {
    std::cout << "test_if_else_true_branch\n";
    EXPECT_EQ(run("if (true) { print 1; } else { print 2; }"), "1\n");
}

static void test_if_else_false_branch() {
    std::cout << "test_if_else_false_branch\n";
    EXPECT_EQ(run("if (false) { print 1; } else { print 2; }"), "2\n");
}

static void test_if_with_condition() {
    std::cout << "test_if_with_condition\n";
    EXPECT_EQ(run("let x = 5; if (x > 3) { print 1; } else { print 0; }"), "1\n");
    EXPECT_EQ(run("let x = 2; if (x > 3) { print 1; } else { print 0; }"), "0\n");
}

static void test_else_if_chain() {
    std::cout << "test_else_if_chain\n";
    const char* src =
        "let x = 2;"
        "if (x == 1) { print 10; }"
        "else if (x == 2) { print 20; }"
        "else { print 30; }";
    EXPECT_EQ(run(src), "20\n");
}

// ── While ─────────────────────────────────────────────────────────────────────
static void test_while_not_entered() {
    std::cout << "test_while_not_entered\n";
    EXPECT_EQ(run("while (false) { print 1; }"), "");
}

static void test_while_counts_down() {
    std::cout << "test_while_counts_down\n";
    // prints 3, 2, 1
    EXPECT_EQ(run("let n = 3; while (n > 0) { print n; n = n - 1; }"), "3\n2\n1\n");
}

static void test_while_accumulates() {
    std::cout << "test_while_accumulates\n";
    // sum 1..5 = 15
    EXPECT_EQ(run(
        "let i = 1; let s = 0;"
        "while (i <= 5) { s = s + i; i = i + 1; }"
        "print s;"),
        "15\n");
}

// ── Nested structures ─────────────────────────────────────────────────────────
static void test_nested_if_in_while() {
    std::cout << "test_nested_if_in_while\n";
    // Print only even numbers 1-6
    EXPECT_EQ(run(
        "let i = 1;"
        "while (i <= 6) {"
        "  if (i == 2) { print i; }"
        "  else if (i == 4) { print i; }"
        "  else if (i == 6) { print i; }"
        "  i = i + 1;"
        "}"),
        "2\n4\n6\n");
}

// ── Runtime errors ────────────────────────────────────────────────────────────
static void test_undefined_variable() {
    std::cout << "test_undefined_variable\n";
    VMResult res;
    run("print x;", &res);
    EXPECT_TRUE(res == VMResult::RUNTIME_ERROR);
}

static void test_division_by_zero() {
    std::cout << "test_division_by_zero\n";
    VMResult res;
    run("print 5 / 0;", &res);
    EXPECT_TRUE(res == VMResult::RUNTIME_ERROR);
}

static void test_assign_undefined() {
    std::cout << "test_assign_undefined\n";
    VMResult res;
    run("x = 5;", &res);
    EXPECT_TRUE(res == VMResult::RUNTIME_ERROR);
}

// ── Full programs ─────────────────────────────────────────────────────────────
static void test_fibonacci() {
    std::cout << "test_fibonacci\n";
    // Iterative fib(7) = 13
    EXPECT_EQ(run(
        "let a = 0; let b = 1; let i = 0;"
        "while (i < 7) {"
        "  let tmp = a + b;"
        "  a = b;"
        "  b = tmp;"
        "  i = i + 1;"
        "}"
        "print a;"),
        "13\n");
}

static void test_factorial() {
    std::cout << "test_factorial\n";
    // 5! = 120
    EXPECT_EQ(run(
        "let n = 5; let result = 1;"
        "while (n > 1) { result = result * n; n = n - 1; }"
        "print result;"),
        "120\n");
}

// ─────────────────────────────────────────────────────────────────────────────
// Runner
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "\n=== CVM++ VM Tests ===\n\n";

    test_print_integer();
    test_print_negative();
    test_print_true();
    test_print_false();
    test_let_and_print();
    test_assign_and_print();
    test_multiple_variables();
    test_addition();
    test_subtraction();
    test_multiplication();
    test_division();
    test_precedence();
    test_grouping();
    test_unary_negate();
    test_complex_arithmetic();
    test_not_true();
    test_not_false();
    test_eq_true();
    test_eq_false();
    test_neq();
    test_lt();
    test_lte();
    test_gt();
    test_gte();
    test_if_taken();
    test_if_not_taken();
    test_if_else_true_branch();
    test_if_else_false_branch();
    test_if_with_condition();
    test_else_if_chain();
    test_while_not_entered();
    test_while_counts_down();
    test_while_accumulates();
    test_nested_if_in_while();
    test_undefined_variable();
    test_division_by_zero();
    test_assign_undefined();
    test_fibonacci();
    test_factorial();

    std::cout << "\n─────────────────────────────\n";
    std::cout << "  Passed: " << s_passed << "\n";
    std::cout << "  Failed: " << s_failed << "\n";
    std::cout << "─────────────────────────────\n\n";

    return s_failed == 0 ? 0 : 1;
}
