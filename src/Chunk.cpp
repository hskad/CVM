#include "Chunk.h"

#include <iomanip>
#include <iostream>

// ─────────────────────────────────────────────────────────────────────────────
// Chunk::disassemble
//
// Prints a human-readable listing of the bytecode to the given stream.
// Format:
//   0000  PUSH_INT      42
//   0005  DEFINE        [0] 'x'
//   0008  LOAD          [0] 'x'
//   0011  JUMP_IF_FALSE +7
//   ...
// ─────────────────────────────────────────────────────────────────────────────
void Chunk::disassemble(std::ostream& os, const std::string& title) const {
    if (!title.empty()) {
        os << "\n── Bytecode: " << title << " ──────────────────────────\n";
    } else {
        os << "\n── Bytecode ───────────────────────────────────────────\n";
    }

    std::size_t ip = 0;
    while (ip < code.size()) {
        // Print offset — use snprintf to guarantee zero-padded 4 digits
        char offsetBuf[8];
        std::snprintf(offsetBuf, sizeof(offsetBuf), "%04zu", ip);
        os << offsetBuf << "  ";

        OpCode op = static_cast<OpCode>(code[ip]);
        ++ip;

        // Helper to print opname left-padded to 20 chars
        auto name = [&](const char* s) {
            os << std::left << std::setw(18) << std::setfill(' ') << s;
        };

        switch (op) {
            case OpCode::PUSH_INT: {
                int32_t v = readInt32(code, ip); ip += 4;
                name("PUSH_INT"); os << v << "\n";
                break;
            }
            case OpCode::PUSH_BOOL: {
                uint8_t b = code[ip++];
                name("PUSH_BOOL"); os << (b ? "true" : "false") << "\n";
                break;
            }
            case OpCode::LOAD: {
                uint16_t idx = readUint16(code, ip); ip += 2;
                name("LOAD");
                os << "[" << idx << "] '" << names[idx] << "'\n";
                break;
            }
            case OpCode::STORE: {
                uint16_t idx = readUint16(code, ip); ip += 2;
                name("STORE");
                os << "[" << idx << "] '" << names[idx] << "'\n";
                break;
            }
            case OpCode::DEFINE: {
                uint16_t idx = readUint16(code, ip); ip += 2;
                name("DEFINE");
                os << "[" << idx << "] '" << names[idx] << "'\n";
                break;
            }
            case OpCode::JUMP: {
                int16_t off = readInt16(code, ip); ip += 2;
                std::size_t dest = static_cast<std::size_t>(
                    static_cast<std::ptrdiff_t>(ip) + off);
                name("JUMP");
                os << (off >= 0 ? "+" : "") << off << "  -> " << dest << "\n";
                break;
            }
            case OpCode::JUMP_IF_FALSE: {
                int16_t off = readInt16(code, ip); ip += 2;
                std::size_t dest = static_cast<std::size_t>(
                    static_cast<std::ptrdiff_t>(ip) + off);
                name("JUMP_IF_FALSE");
                os << (off >= 0 ? "+" : "") << off << "  -> " << dest << "\n";
                break;
            }
            // All zero-operand instructions
            default:
                os << opName(op) << "\n";
                break;
        }
    }
    os << "───────────────────────────────────────────────────────\n\n";
}
