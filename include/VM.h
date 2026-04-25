#pragma once

#include "Chunk.h"

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Value
//
// CVM++ is dynamically typed at the value level: a variable can hold either
// an integer or a boolean.  We use std::variant<int32_t, bool> so the
// compiler can track the type statically via the active alternative.
// ─────────────────────────────────────────────────────────────────────────────
using Value = std::variant<int32_t, bool>;

// Helpers
inline bool isInt (const Value& v) { return std::holds_alternative<int32_t>(v); }
inline bool isBool(const Value& v) { return std::holds_alternative<bool>(v); }
inline int32_t asInt (const Value& v) { return std::get<int32_t>(v); }
inline bool    asBool(const Value& v) { return std::get<bool>(v); }

// Print a Value to stdout.
void printValue(const Value& v);

// Return a human-readable string for a Value (for error messages).
std::string valueStr(const Value& v);

// ─────────────────────────────────────────────────────────────────────────────
// VM result codes
// ─────────────────────────────────────────────────────────────────────────────
enum class VMResult { OK, RUNTIME_ERROR };

// ─────────────────────────────────────────────────────────────────────────────
// VM
//
// A simple stack-based interpreter for the bytecode emitted by the Compiler.
//
// Two usage modes:
//
//   Stateless (file runner / one-shot):
//       VM vm;
//       VMResult result = vm.execute(chunk);
//
//   Stateful (REPL — variables persist across calls):
//       VM vm;
//       vm.execute(chunk1);   // defines 'x'
//       vm.execute(chunk2);   // can still read 'x'
//
// Variables are stored by name (string key) rather than by chunk-local index
// so that they survive across independently compiled chunks.  Each execute()
// call builds a local index→name map from the incoming chunk's name table and
// uses it to look up/store values in the persistent m_globals store.
// ─────────────────────────────────────────────────────────────────────────────
class VM {
public:
    VM() = default;

    // Execute a compiled chunk.  Returns OK or RUNTIME_ERROR.
    VMResult execute(const Chunk& chunk);

    // Clear all persistent variable state (used between REPL sessions if needed).
    void resetState() { m_globals.clear(); }

    // Read-only access to the global store (for REPL introspection).
    const std::unordered_map<std::string, Value>& globals() const { return m_globals; }

private:
    // ── Stack operations ─────────────────────────────────────────────────
    void  push(Value v);
    Value pop();
    Value peek() const;

    // ── Error reporting ──────────────────────────────────────────────────
    VMResult runtimeError(const std::string& msg);

    // ── State ─────────────────────────────────────────────────────────────
    std::vector<Value>                        m_stack;

    // Persistent variable store: name string → value.
    // Survives across execute() calls so the REPL keeps variables alive.
    std::unordered_map<std::string, Value>    m_globals;

    const Chunk*                              m_chunk = nullptr;
    std::size_t                               m_ip    = 0;
};
