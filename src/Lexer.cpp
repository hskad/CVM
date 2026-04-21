#include "Lexer.h"

#include <cctype>
#include <iostream>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// Keyword table — maps reserved word strings to their TokenType.
// Checked after scanning an identifier to decide if it is a keyword.
// ─────────────────────────────────────────────────────────────────────────────
const std::unordered_map<std::string, TokenType> Lexer::s_keywords = {
    { "let",   TokenType::LET   },
    { "if",    TokenType::IF    },
    { "else",  TokenType::ELSE  },
    { "while", TokenType::WHILE },
    { "print", TokenType::PRINT },
    { "input", TokenType::INPUT },
    { "true",  TokenType::TRUE_LIT  },
    { "false", TokenType::FALSE_LIT },
};

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
Lexer::Lexer(std::string source)
    : m_source(std::move(source)) {}

// ─────────────────────────────────────────────────────────────────────────────
// tokenize()
//
// Main entry point. Repeatedly calls nextToken() until EOF is reached,
// collecting every token into a vector.
// ─────────────────────────────────────────────────────────────────────────────
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        if (tok.type == TokenType::EOF_TOKEN) break;
    }

    return tokens;
}

// ─────────────────────────────────────────────────────────────────────────────
// nextToken()
//
// Scans one token from the current position and returns it.
// ─────────────────────────────────────────────────────────────────────────────
Token Lexer::nextToken() {
    skipWhitespaceAndComments();

    m_start = m_current;  // mark the beginning of this token

    if (isAtEnd()) return makeToken(TokenType::EOF_TOKEN);

    char c = advance();

    // ── Digits → number literal ───────────────────────────────────────────
    if (std::isdigit(c)) return readNumber();

    // ── Letters / underscore → identifier or keyword ──────────────────────
    if (std::isalpha(c) || c == '_') return readIdentifierOrKeyword();

    // ── Single- and double-character operators / delimiters ───────────────
    switch (c) {
        case '(': return makeToken(TokenType::LPAREN);
        case ')': return makeToken(TokenType::RPAREN);
        case '{': return makeToken(TokenType::LBRACE);
        case '}': return makeToken(TokenType::RBRACE);
        case ';': return makeToken(TokenType::SEMICOLON);
        case '+': return makeToken(TokenType::PLUS);
        case '-': return makeToken(TokenType::MINUS);
        case '*': return makeToken(TokenType::STAR);
        case '/': return makeToken(TokenType::SLASH);

        case '=':
            return makeToken(match('=') ? TokenType::EQEQ : TokenType::EQ);

        case '!':
            return makeToken(match('=') ? TokenType::BANG_EQ : TokenType::BANG);

        case '<':
            return makeToken(match('=') ? TokenType::LT_EQ : TokenType::LT);

        case '>':
            return makeToken(match('=') ? TokenType::GT_EQ : TokenType::GT);

        case '"': return readString();

        default:
            break;
    }

    // Anything else is an unrecognised character.
    m_hadError = true;
    return errorToken("Unexpected character '" + std::string(1, c) + "'");
}

// ─────────────────────────────────────────────────────────────────────────────
// skipWhitespaceAndComments()
//
// Advances past spaces, tabs, newlines (incrementing the line counter), and
// single-line comments starting with //.
// ─────────────────────────────────────────────────────────────────────────────
void Lexer::skipWhitespaceAndComments() {
    while (!isAtEnd()) {
        char c = peek();

        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;

            case '\n':
                m_line++;
                advance();
                break;

            case '/':
                // Check for // comment
                if (peekNext() == '/') {
                    // Consume the rest of the line.
                    while (!isAtEnd() && peek() != '\n') advance();
                } else {
                    return;  // lone '/' is a SLASH token — leave it for nextToken()
                }
                break;

            default:
                return;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// readNumber()
//
// We have already consumed the first digit. Keep consuming digits until we
// hit a non-digit character (integers only for now).
// ─────────────────────────────────────────────────────────────────────────────
Token Lexer::readNumber() {
    while (std::isdigit(peek())) advance();

    // Future extension: floating-point support would go here.

    return makeToken(TokenType::NUMBER, m_source.substr(m_start, m_current - m_start));
}

// ─────────────────────────────────────────────────────────────────────────────
// readIdentifierOrKeyword()
//
// We have already consumed the first letter or underscore. Keep consuming
// alphanumeric characters and underscores, then look up the result in the
// keyword table.
// ─────────────────────────────────────────────────────────────────────────────
Token Lexer::readIdentifierOrKeyword() {
    while (std::isalnum(peek()) || peek() == '_') advance();

    std::string text = m_source.substr(m_start, m_current - m_start);

    auto it = s_keywords.find(text);
    if (it != s_keywords.end()) {
        return makeToken(it->second, text);
    }

    return makeToken(TokenType::IDENTIFIER, text);
}

// ─────────────────────────────────────────────────────────────────────────────
// readString()
//
// We have already consumed the opening '"'. Consume characters until we find
// the closing '"' (or EOF, which is an error).
// ─────────────────────────────────────────────────────────────────────────────
Token Lexer::readString() {
    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\n') m_line++;
        advance();
    }

    if (isAtEnd()) {
        m_hadError = true;
        return errorToken("Unterminated string literal");
    }

    advance();  // consume closing '"'

    // Strip the surrounding quotes from the lexeme value.
    std::string value = m_source.substr(m_start + 1, m_current - m_start - 2);
    return makeToken(TokenType::STRING_LIT, value);
}

// ─────────────────────────────────────────────────────────────────────────────
// Low-level character helpers
// ─────────────────────────────────────────────────────────────────────────────

char Lexer::advance() {
    return m_source[m_current++];
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return m_source[m_current];
}

char Lexer::peekNext() const {
    if (m_current + 1 >= m_source.size()) return '\0';
    return m_source[m_current + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (m_source[m_current] != expected) return false;
    m_current++;
    return true;
}

bool Lexer::isAtEnd() const {
    return m_current >= m_source.size();
}

// ─────────────────────────────────────────────────────────────────────────────
// Token factory helpers
// ─────────────────────────────────────────────────────────────────────────────

Token Lexer::makeToken(TokenType type) const {
    return Token(type, m_source.substr(m_start, m_current - m_start), m_line);
}

Token Lexer::makeToken(TokenType type, std::string lexeme) const {
    return Token(type, std::move(lexeme), m_line);
}

Token Lexer::errorToken(const std::string& message) const {
    std::cerr << "[Lexer] Line " << m_line << ": " << message << "\n";
    return Token(TokenType::ERROR_TOKEN, message, m_line);
}
