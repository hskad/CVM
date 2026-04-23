#include "Compiler.h"
#include "AST.h"
#include "Chunk.h"
#include "Token.h"

#include <iostream>
#include <stdexcept>
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

    chunk.emit(OpCode::HALT);
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
            m_chunk->emit(OpCode::POP);  // discard result
            break;
    }
}

// let name = expr ;
void Compiler::compileLet(const Stmt& s) {
    compileExpr(*s.expr);
    uint16_t idx = m_chunk->internName(s.name);
    m_chunk->emit(OpCode::DEFINE);
    m_chunk->emitUint16(idx);
}

// name = expr ;
void Compiler::compileAssign(const Stmt& s) {
    compileExpr(*s.expr);
    uint16_t idx = m_chunk->internName(s.name);
    m_chunk->emit(OpCode::STORE);
    m_chunk->emitUint16(idx);
}

// if ( cond ) thenBlock [ else elseBlock ]
//
// Bytecode layout:
//   <cond>
//   JUMP_IF_FALSE  -> [after_then | before_else_block]
//   <then_body>
//   [JUMP -> after_else]        (only when else branch exists)
//   <else_body>                 (optional)
//
void Compiler::compileIf(const Stmt& s) {
    // Compile condition
    compileExpr(*s.expr);

    // Emit conditional jump; save placeholder address for patching
    m_chunk->emit(OpCode::JUMP_IF_FALSE);
    std::size_t jumpIfFalseAddr = m_chunk->emitJumpPlaceholder();

    // Compile then-branch
    compileStmt(*s.thenBranch);

    if (s.elseBranch) {
        // Need an unconditional jump to skip the else block
        m_chunk->emit(OpCode::JUMP);
        std::size_t jumpOverElseAddr = m_chunk->emitJumpPlaceholder();

        // Patch the JUMP_IF_FALSE to land here (start of else)
        m_chunk->patchJump(jumpIfFalseAddr);

        // Compile else-branch
        compileStmt(*s.elseBranch);

        // Patch the unconditional jump to land here (after else)
        m_chunk->patchJump(jumpOverElseAddr);
    } else {
        // No else: patch JUMP_IF_FALSE to land here (after then)
        m_chunk->patchJump(jumpIfFalseAddr);
    }
}

// while ( cond ) body
//
// Bytecode layout:
//   [loop_start]:
//   <cond>
//   JUMP_IF_FALSE  -> after_body
//   <body>
//   JUMP           -> loop_start
//   [after_body]:
//
void Compiler::compileWhile(const Stmt& s) {
    std::size_t loopStart = m_chunk->here();

    // Compile condition
    compileExpr(*s.expr);

    // Conditional exit jump
    m_chunk->emit(OpCode::JUMP_IF_FALSE);
    std::size_t exitJumpAddr = m_chunk->emitJumpPlaceholder();

    // Compile body
    compileStmt(*s.body);

    // Unconditional back-jump to loop start
    m_chunk->emit(OpCode::JUMP);
    // offset = loopStart - (here + 2)   [the +2 is past the operand bytes]
    int16_t backOffset = static_cast<int16_t>(
        static_cast<std::ptrdiff_t>(loopStart) -
        static_cast<std::ptrdiff_t>(m_chunk->here() + 2));
    m_chunk->emitInt16(backOffset);

    // Patch exit jump to land here (after loop body)
    m_chunk->patchJump(exitJumpAddr);
}

// print expr ;
void Compiler::compilePrint(const Stmt& s) {
    compileExpr(*s.expr);
    m_chunk->emit(OpCode::PRINT);
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
            m_chunk->emit(OpCode::PUSH_INT);
            m_chunk->emitInt32(static_cast<int32_t>(e.intValue));
            break;

        case ExprKind::BoolLit:
            m_chunk->emit(OpCode::PUSH_BOOL);
            m_chunk->code.push_back(e.boolValue ? 1u : 0u);
            break;

        case ExprKind::Variable: {
            uint16_t idx = m_chunk->internName(e.name);
            m_chunk->emit(OpCode::LOAD);
            m_chunk->emitUint16(idx);
            break;
        }

        case ExprKind::Input:
            m_chunk->emit(OpCode::INPUT);
            break;

        case ExprKind::Grouping:
            compileExpr(*e.lhs);
            break;

        case ExprKind::Unary:
            compileExpr(*e.lhs);
            switch (e.op) {
                case TokenType::MINUS: m_chunk->emit(OpCode::NEG); break;
                case TokenType::BANG:  m_chunk->emit(OpCode::NOT); break;
                default:
                    error(e.line, "Unknown unary operator");
            }
            break;

        case ExprKind::Binary:
            // Push left operand, then right, then operation
            compileExpr(*e.lhs);
            compileExpr(*e.rhs);
            switch (e.op) {
                case TokenType::PLUS:    m_chunk->emit(OpCode::ADD);   break;
                case TokenType::MINUS:   m_chunk->emit(OpCode::SUB);   break;
                case TokenType::STAR:    m_chunk->emit(OpCode::MUL);   break;
                case TokenType::SLASH:   m_chunk->emit(OpCode::DIV);   break;
                case TokenType::EQEQ:    m_chunk->emit(OpCode::EQ);    break;
                case TokenType::BANG_EQ: m_chunk->emit(OpCode::NEQ);   break;
                case TokenType::LT:      m_chunk->emit(OpCode::LT);    break;
                case TokenType::LT_EQ:   m_chunk->emit(OpCode::LT_EQ); break;
                case TokenType::GT:      m_chunk->emit(OpCode::GT);    break;
                case TokenType::GT_EQ:   m_chunk->emit(OpCode::GT_EQ); break;
                default:
                    error(e.line, "Unknown binary operator");
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
