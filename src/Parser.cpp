#include "Parser.h"
#include "AST.h"
#include "Token.h"

#include <iostream>
#include <stdexcept>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────
Parser::Parser(std::vector<Token> tokens)
    : m_tokens(std::move(tokens)) {}

// ─────────────────────────────────────────────────────────────────────────────
// parse()
//
// Entry point.  Parses the full token stream into a list of top-level
// statements until EOF is reached.
// ─────────────────────────────────────────────────────────────────────────────
std::vector<StmtPtr> Parser::parse() {
    std::vector<StmtPtr> program;

    while (!isAtEnd()) {
        // Skip any error tokens left by the lexer.
        if (check(TokenType::ERROR_TOKEN)) {
            error(peek(), "Unrecognised token: '" + peek().lexeme + "'");
            advance();
            continue;
        }

        try {
            program.push_back(parseStatement());
        } catch (const std::runtime_error&) {
            // Error was already printed; attempt recovery.
            synchronize();
        }
    }

    return program;
}

// ═════════════════════════════════════════════════════════════════════════════
//  STATEMENT  parsers
// ═════════════════════════════════════════════════════════════════════════════

StmtPtr Parser::parseStatement() {
    if (match(TokenType::LET))   return parseLetStmt();
    if (match(TokenType::IF))    return parseIfStmt();
    if (match(TokenType::WHILE)) return parseWhileStmt();
    if (match(TokenType::PRINT)) return parsePrintStmt();
    if (check(TokenType::LBRACE)) { advance(); return parseBlock(); }
    return parseAssignOrExprStmt();
}

// ── let name = expr ; ────────────────────────────────────────────────────────
StmtPtr Parser::parseLetStmt() {
    int line = prev().line;

    const Token& nameToken = consume(TokenType::IDENTIFIER,
                                     "Expected variable name after 'let'.");
    std::string name = nameToken.lexeme;

    consume(TokenType::EQ, "Expected '=' after variable name in 'let'.");
    ExprPtr init = parseExpr();
    consume(TokenType::SEMICOLON, "Expected ';' after 'let' initialiser.");

    return makeLet(name, std::move(init), line);
}

// ── if ( expr ) block [ else block ] ────────────────────────────────────────
StmtPtr Parser::parseIfStmt() {
    int line = prev().line;

    consume(TokenType::LPAREN, "Expected '(' after 'if'.");
    ExprPtr cond = parseExpr();
    consume(TokenType::RPAREN, "Expected ')' after if-condition.");

    consume(TokenType::LBRACE, "Expected '{' to open if-body.");
    StmtPtr thenBranch = parseBlock();

    StmtPtr elseBranch;
    if (match(TokenType::ELSE)) {
        if (check(TokenType::IF)) {
            // else-if chain — parse recursively as an if-statement
            advance();  // consume 'if'
            elseBranch = parseIfStmt();
        } else {
            consume(TokenType::LBRACE, "Expected '{' to open else-body.");
            elseBranch = parseBlock();
        }
    }

    return makeIf(std::move(cond), std::move(thenBranch), std::move(elseBranch), line);
}

// ── while ( expr ) block ─────────────────────────────────────────────────────
StmtPtr Parser::parseWhileStmt() {
    int line = prev().line;

    consume(TokenType::LPAREN, "Expected '(' after 'while'.");
    ExprPtr cond = parseExpr();
    consume(TokenType::RPAREN, "Expected ')' after while-condition.");

    consume(TokenType::LBRACE, "Expected '{' to open while-body.");
    StmtPtr body = parseBlock();

    return makeWhile(std::move(cond), std::move(body), line);
}

// ── print expr ; ─────────────────────────────────────────────────────────────
StmtPtr Parser::parsePrintStmt() {
    int line = prev().line;
    ExprPtr value = parseExpr();
    consume(TokenType::SEMICOLON, "Expected ';' after 'print' expression.");
    return makePrint(std::move(value), line);
}

// ── name = expr ;   or   expr ; ─────────────────────────────────────────────
// Disambiguated by peeking: if current is IDENTIFIER and next is EQ, it's an
// assignment; otherwise it falls through to a bare expression statement.
StmtPtr Parser::parseAssignOrExprStmt() {
    // Assignment: IDENTIFIER "=" expr ";"
    if (check(TokenType::IDENTIFIER)) {
        // Look ahead to check for '='
        std::size_t saved = m_current;
        const Token& nameTok = advance();
        if (check(TokenType::EQ)) {
            advance();  // consume '='
            ExprPtr value = parseExpr();
            consume(TokenType::SEMICOLON,
                    "Expected ';' after assignment to '" + nameTok.lexeme + "'.");
            return makeAssign(nameTok.lexeme, std::move(value), nameTok.line);
        }
        // Not an assignment — backtrack and parse as expression statement.
        m_current = saved;
    }

    // Expression statement (currently not executed, but valid syntax).
    int line = peek().line;
    ExprPtr e = parseExpr();
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
    return makeExprStmt(std::move(e), line);
}

// ── { stmt* } ─────────────────────────────────────────────────────────────
// The opening '{' has already been consumed by the caller.
StmtPtr Parser::parseBlock() {
    int line = prev().line;
    std::vector<StmtPtr> stmts;

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        if (check(TokenType::ERROR_TOKEN)) {
            error(peek(), "Unrecognised token: '" + peek().lexeme + "'");
            advance();
            continue;
        }
        try {
            stmts.push_back(parseStatement());
        } catch (const std::runtime_error&) {
            synchronize();
        }
    }

    consume(TokenType::RBRACE, "Expected '}' to close block.");
    return makeBlock(std::move(stmts), line);
}

// ═════════════════════════════════════════════════════════════════════════════
//  EXPRESSION  parsers  (recursive descent, lowest → highest precedence)
// ═════════════════════════════════════════════════════════════════════════════

ExprPtr Parser::parseExpr() {
    return parseEquality();
}

// equality → comparison ( ("==" | "!=") comparison )*
ExprPtr Parser::parseEquality() {
    ExprPtr left = parseComparison();

    while (check(TokenType::EQEQ) || check(TokenType::BANG_EQ)) {
        const Token& op = advance();
        ExprPtr right = parseComparison();
        left = makeBinary(op.type, std::move(left), std::move(right), op.line);
    }

    return left;
}

// comparison → addition ( ("<" | "<=" | ">" | ">=") addition )*
ExprPtr Parser::parseComparison() {
    ExprPtr left = parseAddition();

    while (check(TokenType::LT)    || check(TokenType::LT_EQ) ||
           check(TokenType::GT)    || check(TokenType::GT_EQ)) {
        const Token& op = advance();
        ExprPtr right = parseAddition();
        left = makeBinary(op.type, std::move(left), std::move(right), op.line);
    }

    return left;
}

// addition → multiply ( ("+" | "-") multiply )*
ExprPtr Parser::parseAddition() {
    ExprPtr left = parseMultiply();

    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        const Token& op = advance();
        ExprPtr right = parseMultiply();
        left = makeBinary(op.type, std::move(left), std::move(right), op.line);
    }

    return left;
}

// multiply → unary ( ("*" | "/") unary )*
ExprPtr Parser::parseMultiply() {
    ExprPtr left = parseUnary();

    while (check(TokenType::STAR) || check(TokenType::SLASH)) {
        const Token& op = advance();
        ExprPtr right = parseUnary();
        left = makeBinary(op.type, std::move(left), std::move(right), op.line);
    }

    return left;
}

// unary → ("!" | "-") unary | primary
ExprPtr Parser::parseUnary() {
    if (check(TokenType::BANG) || check(TokenType::MINUS)) {
        const Token& op = advance();
        ExprPtr operand = parseUnary();
        return makeUnary(op.type, std::move(operand), op.line);
    }
    return parsePrimary();
}

// primary → NUMBER | "true" | "false" | IDENTIFIER | "(" expr ")" | "input"
ExprPtr Parser::parsePrimary() {
    // Integer literal
    if (check(TokenType::NUMBER)) {
        const Token& tok = advance();
        int value = 0;
        try { value = std::stoi(tok.lexeme); }
        catch (...) { error(tok, "Integer literal out of range: " + tok.lexeme); }
        return makeNumber(value, tok.line);
    }

    // Boolean literals
    if (check(TokenType::TRUE_LIT)) {
        int line = advance().line;
        return makeBool(true, line);
    }
    if (check(TokenType::FALSE_LIT)) {
        int line = advance().line;
        return makeBool(false, line);
    }

    // input keyword — reads from stdin at runtime
    if (check(TokenType::INPUT)) {
        int line = advance().line;
        return makeInput(line);
    }

    // Identifier (variable read)
    if (check(TokenType::IDENTIFIER)) {
        const Token& tok = advance();
        return makeVariable(tok.lexeme, tok.line);
    }

    // Grouped expression: ( expr )
    if (check(TokenType::LPAREN)) {
        int line = advance().line;
        ExprPtr inner = parseExpr();
        consume(TokenType::RPAREN, "Expected ')' after expression.");
        return makeGrouping(std::move(inner), line);
    }

    // Nothing matched — report error
    error(peek(), "Expected expression, got '" + peek().lexeme + "'.");
    throw std::runtime_error("parse error");
}

// ═════════════════════════════════════════════════════════════════════════════
//  TOKEN STREAM  helpers
// ═════════════════════════════════════════════════════════════════════════════

const Token& Parser::peek() const {
    return m_tokens[m_current];
}

const Token& Parser::prev() const {
    return m_tokens[m_current - 1];
}

bool Parser::check(TokenType t) const {
    return !isAtEnd() && peek().type == t;
}

bool Parser::match(TokenType t) {
    if (!check(t)) return false;
    advance();
    return true;
}

bool Parser::match(TokenType a, TokenType b) {
    return match(a) || match(b);
}

bool Parser::match(TokenType a, TokenType b, TokenType c, TokenType d) {
    return match(a) || match(b) || match(c) || match(d);
}

const Token& Parser::advance() {
    if (!isAtEnd()) ++m_current;
    return prev();
}

const Token& Parser::consume(TokenType t, const std::string& msg) {
    if (check(t)) return advance();
    error(peek(), msg);
    throw std::runtime_error("parse error");
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::EOF_TOKEN;
}

// ─────────────────────────────────────────────────────────────────────────────
// Error handling
// ─────────────────────────────────────────────────────────────────────────────

void Parser::error(const Token& tok, const std::string& msg) {
    m_hadError = true;
    std::cerr << "[Parser] Line " << tok.line << ": " << msg << "\n";
}

// Panic-mode recovery: discard tokens until we reach what looks like a
// statement boundary (';', '}', or a keyword that starts a new statement).
void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (prev().type == TokenType::SEMICOLON) return;
        if (prev().type == TokenType::RBRACE)    return;

        switch (peek().type) {
            case TokenType::LET:
            case TokenType::IF:
            case TokenType::WHILE:
            case TokenType::PRINT:
                return;
            default:
                break;
        }
        advance();
    }
}
