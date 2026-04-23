#include "Lexer.h"
#include "Parser.h"
#include "Compiler.h"
#include "Chunk.h"
#include "AST.h"

#include <iostream>
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

#define EXPECT_FALSE(expr) EXPECT_TRUE(!(expr))

#define EXPECT_EQ(a, b) \
    do { auto _a=(a); auto _b=(b); \
         if (!(_a == _b)) { \
             std::cerr << "  FAIL  " << __FILE__ << ":" << __LINE__ \
                       << "  values differ\n"; \
             ++s_failed; } else { ++s_passed; } } while (false)

// ── Helpers ──────────────────────────────────────────────────────────────────

// Compile source string; return the Chunk (and set *ok).
static Chunk compile(const std::string& src, bool* ok = nullptr) {
    Lexer  lex(src);
    auto   tokens  = lex.tokenize();
    Parser parser(std::move(tokens));
    auto   program = parser.parse();
    Compiler comp;
    Chunk chunk = comp.compile(program);
    if (ok) *ok = !comp.hadError();
    return chunk;
}

// Return the opcode at a given byte offset.
static OpCode opAt(const Chunk& c, std::size_t off) {
    return static_cast<OpCode>(c.code[off]);
}

// Read int32 little-endian from chunk at offset.
static int32_t int32At(const Chunk& c, std::size_t off) {
    return readInt32(c.code, off);
}

// Read uint16 little-endian from chunk at offset.
static uint16_t u16At(const Chunk& c, std::size_t off) {
    return readUint16(c.code, off);
}

// Read int16 little-endian from chunk at offset.
static int16_t i16At(const Chunk& c, std::size_t off) {
    return readInt16(c.code, off);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

// ── Literals ─────────────────────────────────────────────────────────────────
static void test_push_int() {
    std::cout << "test_push_int\n";
    // let x = 42;  → PUSH_INT 42, DEFINE x, HALT
    auto c = compile("let x = 42;");
    EXPECT_EQ(opAt(c, 0), OpCode::PUSH_INT);
    EXPECT_EQ(int32At(c, 1), 42);
    EXPECT_EQ(opAt(c, 5), OpCode::DEFINE);
    EXPECT_EQ(opAt(c, 8), OpCode::HALT);
}

static void test_push_bool_true() {
    std::cout << "test_push_bool_true\n";
    auto c = compile("let f = true;");
    EXPECT_EQ(opAt(c, 0), OpCode::PUSH_BOOL);
    EXPECT_EQ(c.code[1], 1u);
}

static void test_push_bool_false() {
    std::cout << "test_push_bool_false\n";
    auto c = compile("let f = false;");
    EXPECT_EQ(opAt(c, 0), OpCode::PUSH_BOOL);
    EXPECT_EQ(c.code[1], 0u);
}

// ── Variables ─────────────────────────────────────────────────────────────────
static void test_define_and_load() {
    std::cout << "test_define_and_load\n";
    // let x = 1; let y = x;
    // Layout:
    //   0: PUSH_INT 1 (5)
    //   5: DEFINE[0]  (3)  -> 'x'
    //   8: LOAD[0]    (3)  -> 'x' (expr for y)
    //  11: DEFINE[1]  (3)  -> 'y'
    //  14: HALT
    auto c = compile("let x = 1; let y = x;");
    // check name table
    EXPECT_EQ(c.names[0], std::string("x"));
    EXPECT_EQ(c.names[1], std::string("y"));
    // second stmt: LOAD[0] then DEFINE[1]
    EXPECT_EQ(opAt(c, 8),  OpCode::LOAD);
    EXPECT_EQ(u16At(c, 9), uint16_t(0));   // 'x' is index 0
    EXPECT_EQ(opAt(c, 11), OpCode::DEFINE);
    EXPECT_EQ(u16At(c, 12), uint16_t(1));  // 'y' is index 1
    EXPECT_EQ(opAt(c, 14), OpCode::HALT);
}

static void test_store() {
    std::cout << "test_store\n";
    // let x = 0; x = 5;  → DEFINE, PUSH_INT 5, STORE
    auto c = compile("let x = 0; x = 5;");
    // x = 5 starts at offset 8: PUSH_INT 5 (5 bytes) then STORE[0] (3 bytes)
    EXPECT_EQ(opAt(c, 8), OpCode::PUSH_INT);
    EXPECT_EQ(int32At(c, 9), 5);
    EXPECT_EQ(opAt(c, 13), OpCode::STORE);
    EXPECT_EQ(u16At(c, 14), 0u);
}

static void test_name_interning() {
    std::cout << "test_name_interning\n";
    // Same name should reuse the same index
    auto c = compile("let x = 1; x = 2; print x;");
    // 'x' should appear exactly once in the name table
    int count = 0;
    for (auto& n : c.names) if (n == "x") ++count;
    EXPECT_EQ(count, 1);
}

// ── Arithmetic ───────────────────────────────────────────────────────────────
static void test_add() {
    std::cout << "test_add\n";
    auto c = compile("let r = 1 + 2;");
    // PUSH_INT 1, PUSH_INT 2, ADD, DEFINE
    EXPECT_EQ(opAt(c, 0), OpCode::PUSH_INT);
    EXPECT_EQ(opAt(c, 5), OpCode::PUSH_INT);
    EXPECT_EQ(opAt(c, 10), OpCode::ADD);
    EXPECT_EQ(opAt(c, 11), OpCode::DEFINE);
}

static void test_sub() {
    std::cout << "test_sub\n";
    auto c = compile("let r = 5 - 3;");
    EXPECT_EQ(opAt(c, 10), OpCode::SUB);
}

static void test_mul() {
    std::cout << "test_mul\n";
    auto c = compile("let r = 2 * 4;");
    EXPECT_EQ(opAt(c, 10), OpCode::MUL);
}

static void test_div() {
    std::cout << "test_div\n";
    auto c = compile("let r = 8 / 2;");
    EXPECT_EQ(opAt(c, 10), OpCode::DIV);
}

static void test_neg() {
    std::cout << "test_neg\n";
    auto c = compile("let r = -5;");
    EXPECT_EQ(opAt(c, 0), OpCode::PUSH_INT);
    EXPECT_EQ(opAt(c, 5), OpCode::NEG);
    EXPECT_EQ(opAt(c, 6), OpCode::DEFINE);
}

static void test_not() {
    std::cout << "test_not\n";
    auto c = compile("let r = !true;");
    EXPECT_EQ(opAt(c, 0), OpCode::PUSH_BOOL);
    EXPECT_EQ(opAt(c, 2), OpCode::NOT);
}

// ── Comparisons ──────────────────────────────────────────────────────────────
static void test_comparisons() {
    std::cout << "test_comparisons\n";
    struct Case { const char* src; OpCode expected; };
    Case cases[] = {
        { "let r = 1 == 2;",  OpCode::EQ    },
        { "let r = 1 != 2;",  OpCode::NEQ   },
        { "let r = 1 < 2;",   OpCode::LT    },
        { "let r = 1 <= 2;",  OpCode::LT_EQ },
        { "let r = 1 > 2;",   OpCode::GT    },
        { "let r = 1 >= 2;",  OpCode::GT_EQ },
    };
    for (auto& tc : cases) {
        auto c = compile(tc.src);
        // PUSH_INT(5) + PUSH_INT(5) + op(1) → op is at offset 10
        EXPECT_EQ(opAt(c, 10), tc.expected);
    }
}

// ── I/O ──────────────────────────────────────────────────────────────────────
static void test_print() {
    std::cout << "test_print\n";
    // print 42;  → PUSH_INT 42, PRINT, HALT
    auto c = compile("print 42;");
    EXPECT_EQ(opAt(c, 0), OpCode::PUSH_INT);
    EXPECT_EQ(int32At(c, 1), 42);
    EXPECT_EQ(opAt(c, 5), OpCode::PRINT);
    EXPECT_EQ(opAt(c, 6), OpCode::HALT);
}

static void test_input() {
    std::cout << "test_input\n";
    // let n = input;  → INPUT, DEFINE, HALT
    auto c = compile("let n = input;");
    EXPECT_EQ(opAt(c, 0), OpCode::INPUT);
    EXPECT_EQ(opAt(c, 1), OpCode::DEFINE);
}

// ── If/else ──────────────────────────────────────────────────────────────────
static void test_if_no_else() {
    std::cout << "test_if_no_else\n";
    // if (true) { print 1; }
    // Layout: PUSH_BOOL(2) JUMP_IF_FALSE(3) PUSH_INT(5) PRINT(1) HALT(1)
    auto c = compile("if (true) { print 1; }");
    EXPECT_EQ(opAt(c, 0), OpCode::PUSH_BOOL);
    EXPECT_EQ(opAt(c, 2), OpCode::JUMP_IF_FALSE);
    // The jump offset should skip past: PUSH_INT(5) + PRINT(1) = 6 bytes
    int16_t off = i16At(c, 3);
    EXPECT_EQ(off, 6);
    EXPECT_EQ(opAt(c, 5), OpCode::PUSH_INT);
    EXPECT_EQ(opAt(c, 10), OpCode::PRINT);
    EXPECT_EQ(opAt(c, 11), OpCode::HALT);
}

static void test_if_else() {
    std::cout << "test_if_else\n";
    // if (true) { print 1; } else { print 2; }
    // Layout:
    //   0: PUSH_BOOL(2)
    //   2: JUMP_IF_FALSE(3) -> [after then + jump]
    //   5: PUSH_INT 1(5) PRINT(1)   [then body, 6 bytes]
    //  11: JUMP(3)                  [skip else]
    //  14: PUSH_INT 2(5) PRINT(1)   [else body, 6 bytes]
    //  20: HALT
    auto c = compile("if (true) { print 1; } else { print 2; }");
    EXPECT_EQ(opAt(c, 0), OpCode::PUSH_BOOL);
    EXPECT_EQ(opAt(c, 2), OpCode::JUMP_IF_FALSE);
    EXPECT_EQ(opAt(c, 5), OpCode::PUSH_INT);
    EXPECT_EQ(opAt(c, 10), OpCode::PRINT);
    EXPECT_EQ(opAt(c, 11), OpCode::JUMP);
    EXPECT_EQ(opAt(c, 14), OpCode::PUSH_INT);
    EXPECT_EQ(opAt(c, 19), OpCode::PRINT);
    EXPECT_EQ(opAt(c, 20), OpCode::HALT);
    // JUMP_IF_FALSE skips then (6) + JUMP instr (3) = 9
    EXPECT_EQ(i16At(c, 3), 9);
    // JUMP skips else body (6)
    EXPECT_EQ(i16At(c, 12), 6);
}

// ── While ────────────────────────────────────────────────────────────────────
static void test_while() {
    std::cout << "test_while\n";
    // while (false) { print 1; }
    // Layout:
    //   0: PUSH_BOOL false (2)
    //   2: JUMP_IF_FALSE +6 -> 11
    //   5: PUSH_INT 1 (5) PRINT (1)
    //  11: JUMP -14  -> 0
    //  14: HALT
    auto c = compile("while (false) { print 1; }");
    EXPECT_EQ(opAt(c, 0), OpCode::PUSH_BOOL);
    EXPECT_EQ(opAt(c, 2), OpCode::JUMP_IF_FALSE);
    EXPECT_EQ(opAt(c, 5), OpCode::PUSH_INT);
    EXPECT_EQ(opAt(c, 10), OpCode::PRINT);
    EXPECT_EQ(opAt(c, 11), OpCode::JUMP);
    EXPECT_EQ(opAt(c, 14), OpCode::HALT);
    // JUMP_IF_FALSE skips 6 bytes (PUSH_INT + PRINT) + 3 (JUMP) = 9
    EXPECT_EQ(i16At(c, 3), 9);
    // Back-jump: from after the JUMP operand (offset 14) back to 0 = -14
    EXPECT_EQ(i16At(c, 12), -14);
}

// ── Chunk structure ──────────────────────────────────────────────────────────
static void test_always_ends_with_halt() {
    std::cout << "test_always_ends_with_halt\n";
    auto c = compile("");
    EXPECT_EQ(opAt(c, c.code.size() - 1), OpCode::HALT);
}

static void test_empty_program() {
    std::cout << "test_empty_program\n";
    bool ok;
    auto c = compile("", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(c.code.size(), 1u);  // just HALT
    EXPECT_EQ(opAt(c, 0), OpCode::HALT);
}

static void test_multiple_stmts() {
    std::cout << "test_multiple_stmts\n";
    // let a = 1; let b = 2; print a;
    bool ok;
    auto c = compile("let a = 1; let b = 2; print a;", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(c.names.size(), 2u);
    // Code ends with PRINT then HALT
    EXPECT_EQ(opAt(c, c.code.size() - 1), OpCode::HALT);
    EXPECT_EQ(opAt(c, c.code.size() - 2), OpCode::PRINT);
}

// ── No-error checks ──────────────────────────────────────────────────────────
static void test_no_error_on_valid_programs() {
    std::cout << "test_no_error_on_valid_programs\n";
    const char* programs[] = {
        "let x = 42;",
        "let x = 1 + 2 * 3;",
        "if (true) { print 1; }",
        "if (false) { print 0; } else { print 1; }",
        "while (false) { print 0; }",
        "let n = input;",
        "print 99;",
        "let a = 1; let b = a + 1; print b;",
    };
    for (auto src : programs) {
        bool ok;
        compile(src, &ok);
        EXPECT_TRUE(ok);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Runner
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "\n=== CVM++ Compiler Tests ===\n\n";

    test_push_int();
    test_push_bool_true();
    test_push_bool_false();
    test_define_and_load();
    test_store();
    test_name_interning();
    test_add();
    test_sub();
    test_mul();
    test_div();
    test_neg();
    test_not();
    test_comparisons();
    test_print();
    test_input();
    test_if_no_else();
    test_if_else();
    test_while();
    test_always_ends_with_halt();
    test_empty_program();
    test_multiple_stmts();
    test_no_error_on_valid_programs();

    std::cout << "\n─────────────────────────────\n";
    std::cout << "  Passed: " << s_passed << "\n";
    std::cout << "  Failed: " << s_failed << "\n";
    std::cout << "─────────────────────────────\n\n";

    return s_failed == 0 ? 0 : 1;
}
