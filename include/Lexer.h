#pragma once

#include "Token.h"

#include <string>
#include <vector>
#include <unordered_map>

// ─────────────────────────────────────────────────────────────────────────────
// Lexer  (also called "scanner" or "tokeniser")
//
// Responsibility: walk through the raw source string one character at a time
// and emit a flat list of Tokens.
//
// Usage:
//     Lexer lexer(sourceCode);
//     std::vector<Token> tokens = lexer.tokenize();
//
// The returned vector always ends with a single EOF_TOKEN.
// On encountering an unrecognised character the lexer emits an ERROR_TOKEN
// (rather than throwing), so the caller can decide how to handle errors.
// ─────────────────────────────────────────────────────────────────────────────
class Lexer {
public:
    explicit Lexer(std::string source);

    // Scan the entire source and return all tokens.
    std::vector<Token> tokenize();

    // True if any ERROR_TOKEN was emitted during the last tokenize() call.
    bool hadError() const { return m_hadError; }

private:
    // ── Scanning helpers ─────────────────────────────────────────────────
    Token nextToken();

    void  skipWhitespaceAndComments();

    Token readNumber();
    Token readIdentifierOrKeyword();
    Token readString();          // future: string literals

    // Single-character helpers
    char  advance();             // consume and return current char
    char  peek() const;          // look at current char without consuming
    char  peekNext() const;      // look one ahead of current without consuming
    bool  match(char expected);  // consume current if it equals expected
    bool  isAtEnd() const;

    Token makeToken(TokenType type) const;
    Token makeToken(TokenType type, std::string lexeme) const;
    Token errorToken(const std::string& message) const;

    // ── Keyword table ─────────────────────────────────────────────────────
    static const std::unordered_map<std::string, TokenType> s_keywords;

    // ── State ─────────────────────────────────────────────────────────────
    std::string m_source;
    std::size_t m_start   = 0;   // start of the current token being scanned
    std::size_t m_current = 0;   // current character position
    int         m_line    = 1;   // 1-based source line counter
    bool        m_hadError = false;
};
