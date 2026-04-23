#include "VM.h"
#include "Chunk.h"

#include <cstdio>
#include <iostream>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Value helpers
// ─────────────────────────────────────────────────────────────────────────────
void printValue(const Value& v) {
    if (isInt(v))       std::cout << asInt(v);
    else if (isBool(v)) std::cout << (asBool(v) ? "true" : "false");
    std::cout << "\n";
}

std::string valueStr(const Value& v) {
    if (isInt(v))  return std::to_string(asInt(v));
    if (isBool(v)) return asBool(v) ? "true" : "false";
    return "?";
}

// ─────────────────────────────────────────────────────────────────────────────
// Stack helpers
// ─────────────────────────────────────────────────────────────────────────────
void VM::push(Value v) { m_stack.push_back(std::move(v)); }

Value VM::pop() {
    Value v = std::move(m_stack.back());
    m_stack.pop_back();
    return v;
}

Value VM::peek() const { return m_stack.back(); }

VMResult VM::runtimeError(const std::string& msg) {
    std::cerr << "[VM] Runtime error at offset "
              << (m_ip > 0 ? m_ip - 1 : 0)
              << ": " << msg << "\n";
    m_stack.clear();
    m_vars.clear();
    return VMResult::RUNTIME_ERROR;
}

// ─────────────────────────────────────────────────────────────────────────────
// execute()
// ─────────────────────────────────────────────────────────────────────────────
VMResult VM::execute(const Chunk& chunk) {
    m_chunk = &chunk;
    m_ip    = 0;
    m_stack.clear();
    m_vars.clear();

    const auto& code  = chunk.code;
    const auto& names = chunk.names;

    // ── Fetch helpers ─────────────────────────────────────────────────────
    auto readU8 = [&]() -> uint8_t {
        return code[m_ip++];
    };
    auto readI32 = [&]() -> int32_t {
        int32_t v = readInt32(code, m_ip);
        m_ip += 4;
        return v;
    };
    auto readU16 = [&]() -> uint16_t {
        uint16_t v = readUint16(code, m_ip);
        m_ip += 2;
        return v;
    };
    auto readI16 = [&]() -> int16_t {
        int16_t v = readInt16(code, m_ip);
        m_ip += 2;
        return v;
    };

    // ── Dispatch loop ─────────────────────────────────────────────────────
    while (true) {
        OpCode op = static_cast<OpCode>(readU8());

        switch (op) {

            // ── Literals ─────────────────────────────────────────────────
            case OpCode::PUSH_INT: {
                int32_t v = readI32();
                push(Value{v});
                break;
            }
            case OpCode::PUSH_BOOL: {
                bool b = (readU8() != 0);
                push(Value{b});
                break;
            }

            // ── Variables ─────────────────────────────────────────────────
            case OpCode::DEFINE: {
                uint16_t idx = readU16();
                m_vars[idx] = pop();
                break;
            }
            case OpCode::STORE: {
                uint16_t idx = readU16();
                if (m_vars.find(idx) == m_vars.end()) {
                    return runtimeError(
                        "Assignment to undefined variable '" + names[idx] + "'");
                }
                m_vars[idx] = pop();
                break;
            }
            case OpCode::LOAD: {
                uint16_t idx = readU16();
                auto it = m_vars.find(idx);
                if (it == m_vars.end()) {
                    return runtimeError(
                        "Undefined variable '" + names[idx] + "'");
                }
                push(it->second);
                break;
            }

            // ── Arithmetic ────────────────────────────────────────────────
            case OpCode::ADD: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    return runtimeError("'+' requires integer operands");
                push(Value{asInt(a) + asInt(b)});
                break;
            }
            case OpCode::SUB: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    return runtimeError("'-' requires integer operands");
                push(Value{asInt(a) - asInt(b)});
                break;
            }
            case OpCode::MUL: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    return runtimeError("'*' requires integer operands");
                push(Value{asInt(a) * asInt(b)});
                break;
            }
            case OpCode::DIV: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    return runtimeError("'/' requires integer operands");
                if (asInt(b) == 0)
                    return runtimeError("Division by zero");
                push(Value{asInt(a) / asInt(b)});
                break;
            }
            case OpCode::NEG: {
                Value a = pop();
                if (!isInt(a))
                    return runtimeError("Unary '-' requires an integer");
                push(Value{-asInt(a)});
                break;
            }

            // ── Logic ─────────────────────────────────────────────────────
            case OpCode::NOT: {
                Value a = pop();
                if (!isBool(a))
                    return runtimeError("'!' requires a boolean");
                push(Value{!asBool(a)});
                break;
            }

            // ── Comparisons ───────────────────────────────────────────────
            case OpCode::EQ: {
                Value b = pop(), a = pop();
                // Allow cross-type == (always false if types differ)
                push(Value{a == b});
                break;
            }
            case OpCode::NEQ: {
                Value b = pop(), a = pop();
                push(Value{a != b});
                break;
            }
            case OpCode::LT: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    return runtimeError("'<' requires integer operands");
                push(Value{asInt(a) < asInt(b)});
                break;
            }
            case OpCode::LT_EQ: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    return runtimeError("'<=' requires integer operands");
                push(Value{asInt(a) <= asInt(b)});
                break;
            }
            case OpCode::GT: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    return runtimeError("'>' requires integer operands");
                push(Value{asInt(a) > asInt(b)});
                break;
            }
            case OpCode::GT_EQ: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    return runtimeError("'>=' requires integer operands");
                push(Value{asInt(a) >= asInt(b)});
                break;
            }

            // ── I/O ───────────────────────────────────────────────────────
            case OpCode::PRINT: {
                Value v = pop();
                printValue(v);
                break;
            }
            case OpCode::INPUT: {
                int32_t n = 0;
                if (!(std::cin >> n)) {
                    // On bad input, push 0 and clear error state
                    std::cin.clear();
                    std::cin.ignore(1000, '\n');
                    n = 0;
                }
                push(Value{n});
                break;
            }

            // ── Stack ─────────────────────────────────────────────────────
            case OpCode::POP:
                pop();
                break;

            // ── Control flow ──────────────────────────────────────────────
            case OpCode::JUMP: {
                int16_t offset = readI16();
                m_ip = static_cast<std::size_t>(
                    static_cast<std::ptrdiff_t>(m_ip) + offset);
                break;
            }
            case OpCode::JUMP_IF_FALSE: {
                int16_t offset = readI16();
                Value cond = pop();
                bool taken = false;
                if (isBool(cond))       taken = !asBool(cond);
                else if (isInt(cond))   taken = (asInt(cond) == 0);
                if (taken) {
                    m_ip = static_cast<std::size_t>(
                        static_cast<std::ptrdiff_t>(m_ip) + offset);
                }
                break;
            }

            // ── Termination ───────────────────────────────────────────────
            case OpCode::HALT:
                return VMResult::OK;

            default:
                return runtimeError("Unknown opcode");
        }
    }
}
