//
// test_lexer.cpp
//
// Self-contained test suite for the CVM++ Lexer.
// No external testing framework required — just compile and run.
// Exit code 0 = all tests passed.  Non-zero = at least one failure.
//

#include "Lexer.h"
#include "Token.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Mini test harness
// ─────────────────────────────────────────────────────────────────────────────
static int s_passed = 0;
static int s_failed = 0;

#define EXPECT_EQ(actual, expected, label)                                  \
    do {                                                                     \
        if ((actual) == (expected)) {                                        \
            ++s_passed;                                                      \
        } else {                                                             \
            ++s_failed;                                                      \
            std::cerr << "  FAIL  " << (label)                              \
                      << "\n        expected: " << (expected)               \
                      << "\n        got:      " << (actual) << "\n";        \
        }                                                                    \
    } while (false)

#define EXPECT_TRUE(cond, label)   EXPECT_EQ(!!(cond), true,  label)
#define EXPECT_FALSE(cond, label)  EXPECT_EQ(!!(cond), false, label)

// Helper: lex source, return type of token at index i.
static TokenType typeAt(const std::vector<Token>& toks, std::size_t i) {
    return toks[i].type;
}
static std::string lexAt(const std::vector<Token>& toks, std::size_t i) {
    return toks[i].lexeme;
}

// ─────────────────────────────────────────────────────────────────────────────
// Test cases
// ─────────────────────────────────────────────────────────────────────────────

static void test_empty_source() {
    std::cout << "test_empty_source\n";
    Lexer lex("");
    auto toks = lex.tokenize();
    EXPECT_EQ(toks.size(), std::size_t(1), "only EOF token");
    EXPECT_EQ(typeAt(toks, 0), TokenType::EOF_TOKEN, "type is EOF");
}

static void test_whitespace_only() {
    std::cout << "test_whitespace_only\n";
    Lexer lex("   \t\n\r  ");
    auto toks = lex.tokenize();
    EXPECT_EQ(toks.size(), std::size_t(1), "only EOF token");
}

static void test_single_line_comment() {
    std::cout << "test_single_line_comment\n";
    Lexer lex("// this is a comment\n42");
    auto toks = lex.tokenize();
    // Should get NUMBER, EOF
    EXPECT_EQ(toks.size(), std::size_t(2), "comment + number + EOF");
    EXPECT_EQ(typeAt(toks, 0), TokenType::NUMBER, "first real token is NUMBER");
    EXPECT_EQ(lexAt(toks, 0), "42", "number lexeme");
}

static void test_integer_literals() {
    std::cout << "test_integer_literals\n";
    Lexer lex("0 1 42 999");
    auto toks = lex.tokenize();
    // 4 numbers + EOF = 5
    EXPECT_EQ(toks.size(), std::size_t(5), "four numbers + EOF");
    EXPECT_EQ(lexAt(toks, 0), "0",   "lexeme 0");
    EXPECT_EQ(lexAt(toks, 1), "1",   "lexeme 1");
    EXPECT_EQ(lexAt(toks, 2), "42",  "lexeme 42");
    EXPECT_EQ(lexAt(toks, 3), "999", "lexeme 999");
}

static void test_boolean_literals() {
    std::cout << "test_boolean_literals\n";
    Lexer lex("true false");
    auto toks = lex.tokenize();
    EXPECT_EQ(typeAt(toks, 0), TokenType::TRUE_LIT,  "true");
    EXPECT_EQ(typeAt(toks, 1), TokenType::FALSE_LIT, "false");
}

static void test_keywords() {
    std::cout << "test_keywords\n";
    Lexer lex("let if else while print input");
    auto toks = lex.tokenize();
    EXPECT_EQ(typeAt(toks, 0), TokenType::LET,   "let");
    EXPECT_EQ(typeAt(toks, 1), TokenType::IF,    "if");
    EXPECT_EQ(typeAt(toks, 2), TokenType::ELSE,  "else");
    EXPECT_EQ(typeAt(toks, 3), TokenType::WHILE, "while");
    EXPECT_EQ(typeAt(toks, 4), TokenType::PRINT, "print");
    EXPECT_EQ(typeAt(toks, 5), TokenType::INPUT, "input");
}

static void test_identifier() {
    std::cout << "test_identifier\n";
    Lexer lex("myVar _x counter2");
    auto toks = lex.tokenize();
    EXPECT_EQ(typeAt(toks, 0), TokenType::IDENTIFIER, "myVar");
    EXPECT_EQ(lexAt(toks, 0), "myVar", "lexeme myVar");
    EXPECT_EQ(typeAt(toks, 1), TokenType::IDENTIFIER, "_x");
    EXPECT_EQ(typeAt(toks, 2), TokenType::IDENTIFIER, "counter2");
}

static void test_arithmetic_operators() {
    std::cout << "test_arithmetic_operators\n";
    Lexer lex("+ - * /");
    auto toks = lex.tokenize();
    EXPECT_EQ(typeAt(toks, 0), TokenType::PLUS,  "+");
    EXPECT_EQ(typeAt(toks, 1), TokenType::MINUS, "-");
    EXPECT_EQ(typeAt(toks, 2), TokenType::STAR,  "*");
    EXPECT_EQ(typeAt(toks, 3), TokenType::SLASH, "/");
}

static void test_comparison_operators() {
    std::cout << "test_comparison_operators\n";
    Lexer lex("= == ! != < <= > >=");
    auto toks = lex.tokenize();
    EXPECT_EQ(typeAt(toks, 0), TokenType::EQ,      "=");
    EXPECT_EQ(typeAt(toks, 1), TokenType::EQEQ,    "==");
    EXPECT_EQ(typeAt(toks, 2), TokenType::BANG,     "!");
    EXPECT_EQ(typeAt(toks, 3), TokenType::BANG_EQ,  "!=");
    EXPECT_EQ(typeAt(toks, 4), TokenType::LT,       "<");
    EXPECT_EQ(typeAt(toks, 5), TokenType::LT_EQ,    "<=");
    EXPECT_EQ(typeAt(toks, 6), TokenType::GT,       ">");
    EXPECT_EQ(typeAt(toks, 7), TokenType::GT_EQ,    ">=");
}

static void test_delimiters() {
    std::cout << "test_delimiters\n";
    Lexer lex("( ) { } ;");
    auto toks = lex.tokenize();
    EXPECT_EQ(typeAt(toks, 0), TokenType::LPAREN,    "(");
    EXPECT_EQ(typeAt(toks, 1), TokenType::RPAREN,    ")");
    EXPECT_EQ(typeAt(toks, 2), TokenType::LBRACE,    "{");
    EXPECT_EQ(typeAt(toks, 3), TokenType::RBRACE,    "}");
    EXPECT_EQ(typeAt(toks, 4), TokenType::SEMICOLON, ";");
}

static void test_let_declaration() {
    std::cout << "test_let_declaration\n";
    Lexer lex("let x = 10;");
    auto toks = lex.tokenize();
    // LET IDENTIFIER EQ NUMBER SEMICOLON EOF = 6
    EXPECT_EQ(toks.size(), std::size_t(6), "six tokens");
    EXPECT_EQ(typeAt(toks, 0), TokenType::LET,        "let");
    EXPECT_EQ(typeAt(toks, 1), TokenType::IDENTIFIER, "x");
    EXPECT_EQ(lexAt(toks, 1), "x", "identifier lexeme");
    EXPECT_EQ(typeAt(toks, 2), TokenType::EQ,         "=");
    EXPECT_EQ(typeAt(toks, 3), TokenType::NUMBER,     "10");
    EXPECT_EQ(lexAt(toks, 3), "10", "number lexeme");
    EXPECT_EQ(typeAt(toks, 4), TokenType::SEMICOLON,  ";");
    EXPECT_EQ(typeAt(toks, 5), TokenType::EOF_TOKEN,  "eof");
}

static void test_expression() {
    std::cout << "test_expression\n";
    Lexer lex("let x = 10 + 5;");
    auto toks = lex.tokenize();
    // LET IDENTIFIER EQ NUMBER PLUS NUMBER SEMICOLON EOF = 8
    EXPECT_EQ(toks.size(), std::size_t(8), "eight tokens");
    EXPECT_EQ(typeAt(toks, 4), TokenType::PLUS,   "+");
    EXPECT_EQ(typeAt(toks, 5), TokenType::NUMBER, "5");
    EXPECT_EQ(lexAt(toks, 5), "5", "lexeme 5");
}

static void test_if_statement() {
    std::cout << "test_if_statement\n";
    Lexer lex("if (x < 5) { print x; }");
    auto toks = lex.tokenize();
    EXPECT_EQ(typeAt(toks, 0), TokenType::IF,         "if");
    EXPECT_EQ(typeAt(toks, 1), TokenType::LPAREN,     "(");
    EXPECT_EQ(typeAt(toks, 2), TokenType::IDENTIFIER, "x");
    EXPECT_EQ(typeAt(toks, 3), TokenType::LT,         "<");
    EXPECT_EQ(typeAt(toks, 4), TokenType::NUMBER,     "5");
    EXPECT_EQ(typeAt(toks, 5), TokenType::RPAREN,     ")");
    EXPECT_EQ(typeAt(toks, 6), TokenType::LBRACE,     "{");
    EXPECT_EQ(typeAt(toks, 7), TokenType::PRINT,      "print");
}

static void test_while_statement() {
    std::cout << "test_while_statement\n";
    Lexer lex("while (i < 10) { let i = i + 1; }");
    auto toks = lex.tokenize();
    EXPECT_EQ(typeAt(toks, 0), TokenType::WHILE,  "while");
    EXPECT_EQ(typeAt(toks, 1), TokenType::LPAREN, "(");
}

static void test_multiline() {
    std::cout << "test_multiline\n";
    Lexer lex("let a = 1;\nlet b = 2;\nprint a + b;");
    auto toks = lex.tokenize();
    // tokens on line 3 should report line 3
    // find the PRINT token
    bool found = false;
    for (const auto& t : toks) {
        if (t.type == TokenType::PRINT) {
            EXPECT_EQ(t.line, 3, "print on line 3");
            found = true;
        }
    }
    EXPECT_TRUE(found, "PRINT token exists");
}

static void test_string_literal() {
    std::cout << "test_string_literal\n";
    Lexer lex("\"hello world\"");
    auto toks = lex.tokenize();
    EXPECT_EQ(typeAt(toks, 0), TokenType::STRING_LIT, "string literal type");
    EXPECT_EQ(lexAt(toks, 0), "hello world", "string literal lexeme (no quotes)");
}

static void test_error_token() {
    std::cout << "test_error_token\n";
    Lexer lex("let x = @;");  // '@' is unrecognised
    auto toks = lex.tokenize();
    bool hasError = false;
    for (const auto& t : toks) {
        if (t.type == TokenType::ERROR_TOKEN) hasError = true;
    }
    EXPECT_TRUE(hasError, "ERROR_TOKEN emitted for '@'");
    EXPECT_TRUE(lex.hadError(), "hadError() is true");
}

static void test_no_error_on_valid_input() {
    std::cout << "test_no_error_on_valid_input\n";
    Lexer lex("let counter = 0; while (counter < 10) { let counter = counter + 1; }");
    lex.tokenize();
    EXPECT_FALSE(lex.hadError(), "no error on valid input");
}

static void test_input_keyword() {
    std::cout << "test_input_keyword\n";
    Lexer lex("let x = input;");
    auto toks = lex.tokenize();
    bool found = false;
    for (const auto& t : toks) {
        if (t.type == TokenType::INPUT) { found = true; break; }
    }
    EXPECT_TRUE(found, "INPUT token found");
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "\n=== CVM++ Lexer Tests ===\n\n";

    test_empty_source();
    test_whitespace_only();
    test_single_line_comment();
    test_integer_literals();
    test_boolean_literals();
    test_keywords();
    test_identifier();
    test_arithmetic_operators();
    test_comparison_operators();
    test_delimiters();
    test_let_declaration();
    test_expression();
    test_if_statement();
    test_while_statement();
    test_multiline();
    test_string_literal();
    test_error_token();
    test_no_error_on_valid_input();
    test_input_keyword();

    std::cout << "\n─────────────────────────────\n";
    std::cout << "  Passed: " << s_passed << "\n";
    std::cout << "  Failed: " << s_failed << "\n";
    std::cout << "─────────────────────────────\n\n";

    return (s_failed == 0) ? 0 : 1;
}
