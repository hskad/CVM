#pragma once

#include "AST.h"
#include "Chunk.h"

#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Compiler
//
// Responsibility: walk the AST produced by the Parser and emit bytecode into
// a Chunk.  The output Chunk can then be handed directly to the VM (Phase 4).
//
// Usage:
//     Compiler compiler;
//     Chunk chunk = compiler.compile(program);
//     if (compiler.hadError()) { /* handle */ }
//
// Design notes:
//   - Single-pass, no intermediate representation.
//   - Variables live in a name table inside the Chunk; the VM uses a hash map
//     at runtime to look them up by index.
//   - Control flow (if/while) uses forward-jump patching: the compiler emits a
//     JUMP_IF_FALSE with a placeholder offset, compiles the body, then patches
//     the offset to the correct destination.
// ─────────────────────────────────────────────────────────────────────────────
class Compiler {
public:
    Compiler() = default;

    // Compile a full program (list of top-level statements) into a Chunk.
    // Appends a HALT instruction at the end.
    Chunk compile(const std::vector<StmtPtr>& program);

    bool hadError() const { return m_hadError; }

private:
    // ── Statement compilers ──────────────────────────────────────────────
    void compileStmt(const Stmt& s);
    void compileLet(const Stmt& s);
    void compileAssign(const Stmt& s);
    void compileIf(const Stmt& s);
    void compileWhile(const Stmt& s);
    void compilePrint(const Stmt& s);
    void compileBlock(const Stmt& s);

    // ── Expression compilers ─────────────────────────────────────────────
    void compileExpr(const Expr& e);

    // ── Error reporting ──────────────────────────────────────────────────
    void error(int line, const std::string& msg);

    // ── State ────────────────────────────────────────────────────────────
    Chunk* m_chunk    = nullptr;
    bool   m_hadError = false;
};
