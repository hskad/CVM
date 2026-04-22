# CVM++

A lightweight, custom scripting language that compiles down to proprietary bytecode executed by a custom stack-based Virtual Machine — all written from scratch in C++17.

## Project goals

| Component | Purpose |
|---|---|
| **Lexer** | Converts raw source text into a flat stream of Tokens |
| **Parser** | Arranges Tokens into an Abstract Syntax Tree (AST) |
| **Compiler** | Flattens the AST into bytecode instructions (Opcodes) |
| **VM** | Executes bytecode on a stack-based runtime engine |
| **REPL / File Runner** | Interactive shell and file-based script runner |

## Language features (in scope)

- Data types: **integers**, **booleans**
- Operators: `+`, `-`, `*`, `/`, `==`, `!=`, `<`, `<=`, `>`, `>=`
- Variables: `let x = 10;`
- Control flow: `if / else`, `while`
- Built-ins: `print <expr>;`, `let x = input;`
- Comments: `// single-line`

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
# Interactive REPL
./build/cvm

# Run a script file
./build/cvm examples/hello.cvm

# Show token stream (Phase 1 debug)
./build/cvm --debug-tokens examples/arithmetic.cvm

# Run lexer tests
./build/test_lexer
```

## Project structure

```
CVM++/
├── CMakeLists.txt
├── include/
│   ├── Token.h          ← TokenType enum + Token struct
│   └── Lexer.h          ← Lexer interface
├── src/
│   ├── Lexer.cpp        ← Lexer implementation
│   └── main.cpp         ← CLI entry point (REPL + file runner)
├── tests/
│   └── test_lexer.cpp   ← Self-contained lexer test suite
└── examples/
    ├── hello.cvm
    └── arithmetic.cvm
```

## Development phases

| Phase | Status | Description |
|---|---|---|
| 1 — Lexer | ✅ Complete | Tokenisation of all language constructs |
| 2 — Parser | ✅ Complete | Recursive descent parser → AST |
| 3 — Compiler | 🔜 Next | AST → bytecode with backpatching |
| 4 — VM | 🔜 | Stack-based execution engine |
| 5 — Control flow & REPL | 🔜 | `if`, `while`, interactive shell |
| 6 — Polish | 🔜 | Error recovery, debug flags, example scripts |

## Resources

- *Crafting Interpreters* — Robert Nystrom (primary reference)
