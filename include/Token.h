#pragma once

#include <string>
#include <ostream>

// ─────────────────────────────────────────────────────────────────────────────
// TokenType
//
// Every distinct "kind" of thing the lexer can recognise in CVM source code.
// ─────────────────────────────────────────────────────────────────────────────
enum class TokenType {
    // ── Literals ──────────────────────────────────────────────────────────
    NUMBER,         // integer literal, e.g. 42
    TRUE_LIT,       // boolean literal: true
    FALSE_LIT,      // boolean literal: false
    STRING_LIT,     // string literal: "hello"  (reserved for future use)

    // ── Identifiers ───────────────────────────────────────────────────────
    IDENTIFIER,     // variable / function name, e.g. myVar

    // ── Keywords ──────────────────────────────────────────────────────────
    LET,            // let
    IF,             // if
    ELSE,           // else
    WHILE,          // while
    PRINT,          // print
    INPUT,          // input

    // ── Arithmetic operators ───────────────────────────────────────────────
    PLUS,           // +
    MINUS,          // -
    STAR,           // *
    SLASH,          // /

    // ── Comparison / assignment operators ─────────────────────────────────
    EQ,             // =
    EQEQ,           // ==
    BANG,           // !
    BANG_EQ,        // !=
    LT,             // <
    LT_EQ,          // <=
    GT,             // >
    GT_EQ,          // >=

    // ── Delimiters ────────────────────────────────────────────────────────
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    SEMICOLON,      // ;

    // ── Special ───────────────────────────────────────────────────────────
    EOF_TOKEN,      // end of input
    ERROR_TOKEN     // unrecognised character (for error recovery)
};

// ─────────────────────────────────────────────────────────────────────────────
// Token
//
// Pairs a TokenType with the raw text it came from (lexeme) and the source
// line it appeared on — the line number is stored to enable useful error
// messages later in the compiler pipeline.
// ─────────────────────────────────────────────────────────────────────────────
struct Token {
    TokenType   type;
    std::string lexeme;
    int         line;

    Token(TokenType type, std::string lexeme, int line)
        : type(type), lexeme(std::move(lexeme)), line(line) {}
};

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

// Return the human-readable name of a TokenType (useful for debug output).
inline std::string tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::NUMBER:      return "NUMBER";
        case TokenType::TRUE_LIT:    return "TRUE";
        case TokenType::FALSE_LIT:   return "FALSE";
        case TokenType::STRING_LIT:  return "STRING";
        case TokenType::IDENTIFIER:  return "IDENTIFIER";
        case TokenType::LET:         return "LET";
        case TokenType::IF:          return "IF";
        case TokenType::ELSE:        return "ELSE";
        case TokenType::WHILE:       return "WHILE";
        case TokenType::PRINT:       return "PRINT";
        case TokenType::INPUT:       return "INPUT";
        case TokenType::PLUS:        return "PLUS";
        case TokenType::MINUS:       return "MINUS";
        case TokenType::STAR:        return "STAR";
        case TokenType::SLASH:       return "SLASH";
        case TokenType::EQ:          return "EQ";
        case TokenType::EQEQ:        return "EQEQ";
        case TokenType::BANG:        return "BANG";
        case TokenType::BANG_EQ:     return "BANG_EQ";
        case TokenType::LT:          return "LT";
        case TokenType::LT_EQ:       return "LT_EQ";
        case TokenType::GT:          return "GT";
        case TokenType::GT_EQ:       return "GT_EQ";
        case TokenType::LPAREN:      return "LPAREN";
        case TokenType::RPAREN:      return "RPAREN";
        case TokenType::LBRACE:      return "LBRACE";
        case TokenType::RBRACE:      return "RBRACE";
        case TokenType::SEMICOLON:   return "SEMICOLON";
        case TokenType::EOF_TOKEN:   return "EOF";
        case TokenType::ERROR_TOKEN: return "ERROR";
    }
    return "UNKNOWN";
}

// Stream operator for TokenType (needed by test macros and debug output).
inline std::ostream& operator<<(std::ostream& os, TokenType t) {
    return os << tokenTypeName(t);
}

// Stream operator so Token can be printed directly with std::cout.
inline std::ostream& operator<<(std::ostream& os, const Token& tok) {
    os << "[line " << tok.line << "] "
       << tokenTypeName(tok.type);
    if (!tok.lexeme.empty()) {
        os << " '" << tok.lexeme << "'";
    }
    return os;
}
