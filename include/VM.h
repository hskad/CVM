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
// Usage:
//     VM vm;
//     VMResult result = vm.execute(chunk);
// ─────────────────────────────────────────────────────────────────────────────
class VM {
public:
    VM() = default;

    VMResult execute(const Chunk& chunk);

private:
    // ── Stack operations ─────────────────────────────────────────────────
    void  push(Value v);
    Value pop();
    Value peek() const;

    // ── Error reporting ──────────────────────────────────────────────────
    VMResult runtimeError(const std::string& msg);

    // ── State ─────────────────────────────────────────────────────────────
    std::vector<Value>                        m_stack;
    std::unordered_map<uint16_t, Value>       m_vars;   // name-index → value
    const Chunk*                              m_chunk = nullptr;
    std::size_t                               m_ip    = 0;
};
