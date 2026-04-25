#include "VM.h"
#include "Chunk.h"

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
    int line = m_chunk ? m_chunk->lineAt(m_ip > 0 ? m_ip - 1 : 0) : -1;
    if (line > 0)
        std::cerr << "[VM] Line " << line << ": " << msg << "\n";
    else
        std::cerr << "[VM] Runtime error: " << msg << "\n";
    m_stack.clear();
    return VMResult::RUNTIME_ERROR;
}

// ─────────────────────────────────────────────────────────────────────────────
// execute()
//
// Runs the chunk and returns OK or RUNTIME_ERROR.
//
// Variable storage strategy:
//   Each chunk has a names[] table mapping uint16 index -> string name.
//   At runtime we resolve index -> name via the chunk, then look up / store
//   the value in m_globals (string-keyed) so that variables defined in one
//   REPL line are visible to the next.
// ─────────────────────────────────────────────────────────────────────────────
VMResult VM::execute(const Chunk& chunk) {
    m_chunk = &chunk;
    m_ip    = 0;
    m_stack.clear();
    // NOTE: m_globals is intentionally NOT cleared here -- persistence is the point.

    const auto& code  = chunk.code;
    const auto& names = chunk.names;

    // Fetch helpers
    auto readU8 = [&]() -> uint8_t  { return code[m_ip++]; };
    auto readI32 = [&]() -> int32_t {
        int32_t v = readInt32(code, m_ip); m_ip += 4; return v;
    };
    auto readU16 = [&]() -> uint16_t {
        uint16_t v = readUint16(code, m_ip); m_ip += 2; return v;
    };
    auto readI16 = [&]() -> int16_t {
        int16_t v = readInt16(code, m_ip); m_ip += 2; return v;
    };
    auto nameOf = [&](uint16_t idx) -> const std::string& { return names[idx]; };

    // ── Dispatch loop ─────────────────────────────────────────────────────
    while (true) {
        OpCode op = static_cast<OpCode>(readU8());

        switch (op) {

            // ── Literals ─────────────────────────────────────────────────
            case OpCode::PUSH_INT:  push(Value{ readI32() }); break;
            case OpCode::PUSH_BOOL: push(Value{ readU8() != 0 }); break;

            // ── Variables ─────────────────────────────────────────────────
            case OpCode::DEFINE: {
                uint16_t idx = readU16();
                m_globals[nameOf(idx)] = pop();
                break;
            }
            case OpCode::STORE: {
                uint16_t idx = readU16();
                const std::string& name = nameOf(idx);
                if (m_globals.find(name) == m_globals.end())
                    return runtimeError(
                        "Assignment to undefined variable '" + name + "'.\n"
                        "  Hint: declare it first with 'let " + name + " = <value>;'");
                m_globals[name] = pop();
                break;
            }
            case OpCode::LOAD: {
                uint16_t idx = readU16();
                const std::string& name = nameOf(idx);
                auto it = m_globals.find(name);
                if (it == m_globals.end())
                    return runtimeError("Undefined variable '" + name + "'.");
                push(it->second);
                break;
            }

            // ── Arithmetic ────────────────────────────────────────────────
            case OpCode::ADD: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    return runtimeError("'+' requires integer operands, got "
                        + valueStr(a) + " and " + valueStr(b) + ".");
                push(Value{ asInt(a) + asInt(b) });
                break;
            }
            case OpCode::SUB: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    return runtimeError("'-' requires integer operands, got "
                        + valueStr(a) + " and " + valueStr(b) + ".");
                push(Value{ asInt(a) - asInt(b) });
                break;
            }
            case OpCode::MUL: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    return runtimeError("'*' requires integer operands, got "
                        + valueStr(a) + " and " + valueStr(b) + ".");
                push(Value{ asInt(a) * asInt(b) });
                break;
            }
            case OpCode::DIV: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    return runtimeError("'/' requires integer operands, got "
                        + valueStr(a) + " and " + valueStr(b) + ".");
                if (asInt(b) == 0)
                    return runtimeError("Division by zero.");
                push(Value{ asInt(a) / asInt(b) });
                break;
            }
            case OpCode::MOD: {
                Value b = pop(), a = pop();
                if (!isInt(a) || !isInt(b))
                    return runtimeError("'%' requires integer operands, got "
                        + valueStr(a) + " and " + valueStr(b) + ".");
                if (asInt(b) == 0)
                    return runtimeError("Modulo by zero.");
                push(Value{ asInt(a) % asInt(b) });
                break;
            }
            case OpCode::NEG: {
                Value a = pop();
                if (!isInt(a))
                    return runtimeError("Unary '-' requires an integer, got "
                        + valueStr(a) + ".");
                push(Value{ -asInt(a) });
                break;
            }

            // ── Logic ─────────────────────────────────────────────────────
            case OpCode::NOT: {
                Value a = pop();
                if (!isBool(a))
                    return runtimeError("'!' requires a boolean, got "
                        + valueStr(a) + ".");
                push(Value{ !asBool(a) });
                break;
            }

            // ── Comparisons ───────────────────────────────────────────────
            case OpCode::EQ:  { Value b=pop(),a=pop(); push(Value{a==b}); break; }
            case OpCode::NEQ: { Value b=pop(),a=pop(); push(Value{a!=b}); break; }
            case OpCode::LT: {
                Value b=pop(),a=pop();
                if (!isInt(a)||!isInt(b)) return runtimeError(
                    "'<' requires integer operands, got "
                    +valueStr(a)+" and "+valueStr(b)+".");
                push(Value{asInt(a)<asInt(b)}); break;
            }
            case OpCode::LT_EQ: {
                Value b=pop(),a=pop();
                if (!isInt(a)||!isInt(b)) return runtimeError(
                    "'<=' requires integer operands, got "
                    +valueStr(a)+" and "+valueStr(b)+".");
                push(Value{asInt(a)<=asInt(b)}); break;
            }
            case OpCode::GT: {
                Value b=pop(),a=pop();
                if (!isInt(a)||!isInt(b)) return runtimeError(
                    "'>' requires integer operands, got "
                    +valueStr(a)+" and "+valueStr(b)+".");
                push(Value{asInt(a)>asInt(b)}); break;
            }
            case OpCode::GT_EQ: {
                Value b=pop(),a=pop();
                if (!isInt(a)||!isInt(b)) return runtimeError(
                    "'>=' requires integer operands, got "
                    +valueStr(a)+" and "+valueStr(b)+".");
                push(Value{asInt(a)>=asInt(b)}); break;
            }

            // ── I/O ───────────────────────────────────────────────────────
            case OpCode::PRINT: {
                printValue(pop());
                break;
            }
            case OpCode::INPUT: {
                int32_t n = 0;
                if (!(std::cin >> n)) {
                    std::cin.clear();
                    std::cin.ignore(1000, '\n');
                    n = 0;
                }
                push(Value{ n });
                break;
            }

            // ── Stack ─────────────────────────────────────────────────────
            case OpCode::POP: pop(); break;

            // ── Control flow ──────────────────────────────────────────────
            case OpCode::JUMP: {
                int16_t off = readI16();
                m_ip = static_cast<std::size_t>(
                    static_cast<std::ptrdiff_t>(m_ip) + off);
                break;
            }
            case OpCode::JUMP_IF_FALSE: {
                int16_t off = readI16();
                Value cond = pop();
                bool taken = isBool(cond) ? !asBool(cond) : (asInt(cond) == 0);
                if (taken)
                    m_ip = static_cast<std::size_t>(
                        static_cast<std::ptrdiff_t>(m_ip) + off);
                break;
            }

            // ── Termination ───────────────────────────────────────────────
            case OpCode::HALT:
                return VMResult::OK;

            default:
                return runtimeError("Unknown opcode.");
        }
    }
}
