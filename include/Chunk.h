#pragma once

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// OpCode
//
// Every instruction the CVM++ virtual machine can execute.
//
// Stack convention (before → after):
//   PUSH_INT n        ()           → (n)
//   PUSH_BOOL b       ()           → (b)
//   LOAD name         ()           → (value of var)
//   STORE name        (value)      → ()
//   DEFINE name       (value)      → ()          [first assignment, let]
//   ADD               (a, b)       → (a+b)
//   SUB               (a, b)       → (a-b)
//   MUL               (a, b)       → (a*b)
//   DIV               (a, b)       → (a/b)
//   NEG               (a)          → (-a)
//   NOT               (a)          → (!a)
//   EQ                (a, b)       → (a==b)
//   NEQ               (a, b)       → (a!=b)
//   LT                (a, b)       → (a<b)
//   LT_EQ             (a, b)       → (a<=b)
//   GT                (a, b)       → (a>b)
//   GT_EQ             (a, b)       → (a>=b)
//   PRINT             (value)      → ()          [prints value + newline]
//   INPUT             ()           → (value)     [reads integer from stdin]
//   POP               (value)      → ()          [discard top of stack]
//   JUMP offset       ()           → ()          [unconditional relative jump]
//   JUMP_IF_FALSE off (cond)       → ()          [pops; jumps if cond==false]
//   HALT              ()           → ()          [stop execution]
// ─────────────────────────────────────────────────────────────────────────────
enum class OpCode : uint8_t {
    // ── Literals ──────────────────────────────────────────────────────────
    PUSH_INT,       // operand: int32 (4 bytes, little-endian)
    PUSH_BOOL,      // operand: uint8 (0=false, 1=true)

    // ── Variables ─────────────────────────────────────────────────────────
    LOAD,           // operand: uint16 index into name table
    STORE,          // operand: uint16 index
    DEFINE,         // operand: uint16 index  (let — creates slot on first use)

    // ── Arithmetic ────────────────────────────────────────────────────────
    ADD,
    SUB,
    MUL,
    DIV,
    NEG,            // unary negate

    // ── Logic ─────────────────────────────────────────────────────────────
    NOT,            // boolean not

    // ── Comparisons ───────────────────────────────────────────────────────
    EQ,
    NEQ,
    LT,
    LT_EQ,
    GT,
    GT_EQ,

    // ── I/O ───────────────────────────────────────────────────────────────
    PRINT,
    INPUT,

    // ── Stack ─────────────────────────────────────────────────────────────
    POP,

    // ── Control flow ──────────────────────────────────────────────────────
    JUMP,           // operand: int16 relative offset (signed, from next instr)
    JUMP_IF_FALSE,  // operand: int16 relative offset

    // ── Termination ───────────────────────────────────────────────────────
    HALT,
};

// Return the human-readable name of an opcode.
inline const char* opName(OpCode op) {
    switch (op) {
        case OpCode::PUSH_INT:       return "PUSH_INT";
        case OpCode::PUSH_BOOL:      return "PUSH_BOOL";
        case OpCode::LOAD:           return "LOAD";
        case OpCode::STORE:          return "STORE";
        case OpCode::DEFINE:         return "DEFINE";
        case OpCode::ADD:            return "ADD";
        case OpCode::SUB:            return "SUB";
        case OpCode::MUL:            return "MUL";
        case OpCode::DIV:            return "DIV";
        case OpCode::NEG:            return "NEG";
        case OpCode::NOT:            return "NOT";
        case OpCode::EQ:             return "EQ";
        case OpCode::NEQ:            return "NEQ";
        case OpCode::LT:             return "LT";
        case OpCode::LT_EQ:         return "LT_EQ";
        case OpCode::GT:             return "GT";
        case OpCode::GT_EQ:          return "GT_EQ";
        case OpCode::PRINT:          return "PRINT";
        case OpCode::INPUT:          return "INPUT";
        case OpCode::POP:            return "POP";
        case OpCode::JUMP:           return "JUMP";
        case OpCode::JUMP_IF_FALSE:  return "JUMP_IF_FALSE";
        case OpCode::HALT:           return "HALT";
    }
    return "UNKNOWN";
}

// ─────────────────────────────────────────────────────────────────────────────
// Chunk
//
// A compiled code object: a flat byte array of instructions plus a table of
// variable names (strings are interned by index so LOAD/STORE are cheap).
//
// Encoding summary:
//   PUSH_INT       op(1) + int32(4)   = 5 bytes
//   PUSH_BOOL      op(1) + uint8(1)   = 2 bytes
//   LOAD/STORE/DEFINE  op(1) + uint16(2) = 3 bytes
//   JUMP/JUMP_IF_FALSE op(1) + int16(2)  = 3 bytes
//   all others     op(1)              = 1 byte
// ─────────────────────────────────────────────────────────────────────────────
struct Chunk {
    std::vector<uint8_t>     code;     // raw bytecode
    std::vector<std::string> names;    // variable name table

    // ── Emit helpers ─────────────────────────────────────────────────────
    void emit(OpCode op)  { code.push_back(static_cast<uint8_t>(op)); }

    // Emit a 4-byte signed integer (little-endian).
    void emitInt32(int32_t v) {
        code.push_back(static_cast<uint8_t>( v        & 0xFF));
        code.push_back(static_cast<uint8_t>((v >>  8) & 0xFF));
        code.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
        code.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    }

    // Emit a 2-byte unsigned integer (little-endian).
    void emitUint16(uint16_t v) {
        code.push_back(static_cast<uint8_t>( v       & 0xFF));
        code.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    }

    // Emit a 2-byte signed integer (little-endian) — used for jump offsets.
    void emitInt16(int16_t v) {
        emitUint16(static_cast<uint16_t>(v));
    }

    // Emit a placeholder int16 and return its address for later patching.
    std::size_t emitJumpPlaceholder() {
        std::size_t addr = code.size();
        emitInt16(0);   // placeholder
        return addr;
    }

    // Patch a previously-emitted int16 at addr with the correct offset.
    // The offset is relative to the instruction *after* the placeholder.
    void patchJump(std::size_t placeholderAddr) {
        // current position = first byte after the placeholder's parent opcode
        // and the 2-byte operand.  The placeholder itself is at placeholderAddr.
        std::size_t target = code.size();
        // offset = target - (placeholderAddr + 2)   (past the operand bytes)
        int16_t offset = static_cast<int16_t>(
            static_cast<std::ptrdiff_t>(target) -
            static_cast<std::ptrdiff_t>(placeholderAddr + 2));
        code[placeholderAddr]     = static_cast<uint8_t>( offset       & 0xFF);
        code[placeholderAddr + 1] = static_cast<uint8_t>((offset >> 8) & 0xFF);
    }

    // Intern a variable name; return its index.
    uint16_t internName(const std::string& name) {
        for (std::size_t i = 0; i < names.size(); ++i) {
            if (names[i] == name) return static_cast<uint16_t>(i);
        }
        names.push_back(name);
        return static_cast<uint16_t>(names.size() - 1);
    }

    // Current write position (= next instruction offset).
    std::size_t here() const { return code.size(); }

    // ── Disassembler ─────────────────────────────────────────────────────
    void disassemble(std::ostream& os, const std::string& title = "") const;
};

// Read a little-endian int32 from a byte slice at offset.
inline int32_t readInt32(const std::vector<uint8_t>& code, std::size_t off) {
    return static_cast<int32_t>(
        (static_cast<uint32_t>(code[off    ])      ) |
        (static_cast<uint32_t>(code[off + 1]) <<  8) |
        (static_cast<uint32_t>(code[off + 2]) << 16) |
        (static_cast<uint32_t>(code[off + 3]) << 24));
}

// Read a little-endian uint16 from a byte slice at offset.
inline uint16_t readUint16(const std::vector<uint8_t>& code, std::size_t off) {
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(code[off    ])     ) |
        (static_cast<uint16_t>(code[off + 1]) << 8));
}

// Read a little-endian int16 from a byte slice at offset.
inline int16_t readInt16(const std::vector<uint8_t>& code, std::size_t off) {
    return static_cast<int16_t>(readUint16(code, off));
}
