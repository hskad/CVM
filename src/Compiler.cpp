#include "Compiler.h"
#include "AST.h"
#include "Chunk.h"
#include "Token.h"

#include <iostream>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// compile()  —  entry point
// ─────────────────────────────────────────────────────────────────────────────
Chunk Compiler::compile(const std::vector<StmtPtr>& program) {
    Chunk chunk;
    m_chunk    = &chunk;
    m_hadError = false;

    for (const auto& stmt : program) {
        compileStmt(*stmt);
        if (m_hadError) break;
    }

    chunk.emit(OpCode::HALT, 0);
    m_chunk = nullptr;
    return chunk;
}

// ─────────────────────────────────────────────────────────────────────────────
// Statement compilers
// ─────────────────────────────────────────────────────────────────────────────

void Compiler::compileStmt(const Stmt& s) {
    switch (s.kind) {
        case StmtKind::Let:      compileLet(s);    break;
        case StmtKind::Assign:   compileAssign(s); break;
        case StmtKind::If:       compileIf(s);     break;
        case StmtKind::While:    compileWhile(s);  break;
        case StmtKind::Print:    compilePrint(s);  break;
        case StmtKind::Block:    compileBlock(s);  break;
        case StmtKind::ExprStmt:
            compileExpr(*s.expr);
            m_chunk->emit(OpCode::POP, s.line);
            break;
    }
}

// let name = expr ;
void Compiler::compileLet(const Stmt& s) {
    compileExpr(*s.expr);
    uint16_t idx = m_chunk->internName(s.name);
    m_chunk->emit(OpCode::DEFINE, s.line);
    m_chunk->emitUint16(idx, s.line);
}

// name = expr ;
void Compiler::compileAssign(const Stmt& s) {
    compileExpr(*s.expr);
    uint16_t idx = m_chunk->internName(s.name);
    m_chunk->emit(OpCode::STORE, s.line);
    m_chunk->emitUint16(idx, s.line);
}

// if ( cond ) thenBlock [ else elseBlock ]
void Compiler::compileIf(const Stmt& s) {
    compileExpr(*s.expr);

    m_chunk->emit(OpCode::JUMP_IF_FALSE, s.line);
    std::size_t jumpIfFalseAddr = m_chunk->emitJumpPlaceholder(s.line);

    compileStmt(*s.thenBranch);

    if (s.elseBranch) {
        m_chunk->emit(OpCode::JUMP, s.line);
        std::size_t jumpOverElseAddr = m_chunk->emitJumpPlaceholder(s.line);
        m_chunk->patchJump(jumpIfFalseAddr);
        compileStmt(*s.elseBranch);
        m_chunk->patchJump(jumpOverElseAddr);
    } else {
        m_chunk->patchJump(jumpIfFalseAddr);
    }
}

// while ( cond ) body
void Compiler::compileWhile(const Stmt& s) {
    std::size_t loopStart = m_chunk->here();

    compileExpr(*s.expr);

    m_chunk->emit(OpCode::JUMP_IF_FALSE, s.line);
    std::size_t exitJumpAddr = m_chunk->emitJumpPlaceholder(s.line);

    compileStmt(*s.body);

    m_chunk->emit(OpCode::JUMP, s.line);
    int16_t backOffset = static_cast<int16_t>(
        static_cast<std::ptrdiff_t>(loopStart) -
        static_cast<std::ptrdiff_t>(m_chunk->here() + 2));
    m_chunk->emitInt16(backOffset, s.line);

    m_chunk->patchJump(exitJumpAddr);
}

// print expr ;
void Compiler::compilePrint(const Stmt& s) {
    compileExpr(*s.expr);
    m_chunk->emit(OpCode::PRINT, s.line);
}

// { stmt* }
void Compiler::compileBlock(const Stmt& s) {
    for (const auto& child : s.stmts) {
        compileStmt(*child);
        if (m_hadError) return;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Expression compiler
// ─────────────────────────────────────────────────────────────────────────────

void Compiler::compileExpr(const Expr& e) {
    switch (e.kind) {

        case ExprKind::NumberLit:
            m_chunk->emit(OpCode::PUSH_INT, e.line);
            m_chunk->emitInt32(static_cast<int32_t>(e.intValue), e.line);
            break;

        case ExprKind::BoolLit:
            m_chunk->emit(OpCode::PUSH_BOOL, e.line);
            m_chunk->code.push_back(e.boolValue ? 1u : 0u);
            m_chunk->lines.push_back(e.line);
            break;

        case ExprKind::Variable: {
            uint16_t idx = m_chunk->internName(e.name);
            m_chunk->emit(OpCode::LOAD, e.line);
            m_chunk->emitUint16(idx, e.line);
            break;
        }

        case ExprKind::Input:
            m_chunk->emit(OpCode::INPUT, e.line);
            break;

        case ExprKind::Grouping:
            compileExpr(*e.lhs);
            break;

        case ExprKind::Unary:
            compileExpr(*e.lhs);
            switch (e.op) {
                case TokenType::MINUS: m_chunk->emit(OpCode::NEG, e.line); break;
                case TokenType::BANG:  m_chunk->emit(OpCode::NOT, e.line); break;
                default: error(e.line, "Unknown unary operator");
            }
            break;

        case ExprKind::Binary:
            // Short-circuit operators are handled via jumps, not as binary ops
            if (e.op == TokenType::AND) {
                // a && b:
                //   eval a
                //   if false → jump to push_false (skip b)
                //   eval b
                //   jump over push_false
                //   push false
                compileExpr(*e.lhs);
                m_chunk->emit(OpCode::JUMP_IF_FALSE, e.line);
                std::size_t skipRight = m_chunk->emitJumpPlaceholder(e.line);
                compileExpr(*e.rhs);
                m_chunk->emit(OpCode::JUMP, e.line);
                std::size_t skipFalse = m_chunk->emitJumpPlaceholder(e.line);
                m_chunk->patchJump(skipRight);
                m_chunk->emit(OpCode::PUSH_BOOL, e.line);
                m_chunk->code.push_back(0u); m_chunk->lines.push_back(e.line);
                m_chunk->patchJump(skipFalse);
                break;
            }
            if (e.op == TokenType::OR) {
                // a || b:
                //   eval a
                //   if true  → jump to push_true (skip b)
                //   eval b
                //   jump over push_true
                //   push true
                compileExpr(*e.lhs);
                // Invert: JUMP_IF_FALSE means "if NOT true, keep going"
                m_chunk->emit(OpCode::JUMP_IF_FALSE, e.line);
                std::size_t evalRight = m_chunk->emitJumpPlaceholder(e.line);
                // lhs was true — push true and skip rhs
                m_chunk->emit(OpCode::JUMP, e.line);
                std::size_t skipTrue = m_chunk->emitJumpPlaceholder(e.line);
                m_chunk->patchJump(evalRight);
                compileExpr(*e.rhs);
                m_chunk->emit(OpCode::JUMP, e.line);
                std::size_t skipFalse = m_chunk->emitJumpPlaceholder(e.line);
                m_chunk->patchJump(skipTrue);
                m_chunk->emit(OpCode::PUSH_BOOL, e.line);
                m_chunk->code.push_back(1u); m_chunk->lines.push_back(e.line);
                m_chunk->patchJump(skipFalse);
                break;
            }

            // Standard binary: push both operands then the op
            compileExpr(*e.lhs);
            compileExpr(*e.rhs);
            switch (e.op) {
                case TokenType::PLUS:    m_chunk->emit(OpCode::ADD,   e.line); break;
                case TokenType::MINUS:   m_chunk->emit(OpCode::SUB,   e.line); break;
                case TokenType::STAR:    m_chunk->emit(OpCode::MUL,   e.line); break;
                case TokenType::SLASH:   m_chunk->emit(OpCode::DIV,   e.line); break;
                case TokenType::PERCENT: m_chunk->emit(OpCode::MOD,   e.line); break;
                case TokenType::EQEQ:    m_chunk->emit(OpCode::EQ,    e.line); break;
                case TokenType::BANG_EQ: m_chunk->emit(OpCode::NEQ,   e.line); break;
                case TokenType::LT:      m_chunk->emit(OpCode::LT,    e.line); break;
                case TokenType::LT_EQ:   m_chunk->emit(OpCode::LT_EQ, e.line); break;
                case TokenType::GT:      m_chunk->emit(OpCode::GT,    e.line); break;
                case TokenType::GT_EQ:   m_chunk->emit(OpCode::GT_EQ, e.line); break;
                default: error(e.line, "Unknown binary operator");
            }
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Error helper
// ─────────────────────────────────────────────────────────────────────────────
void Compiler::error(int line, const std::string& msg) {
    m_hadError = true;
    std::cerr << "[Compiler] Line " << line << ": " << msg << "\n";
}
