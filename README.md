# CVM++

A lightweight scripting language built from scratch in C++17 — source text flows through a hand-written lexer, recursive-descent parser, single-pass compiler, and stack-based virtual machine, all without external dependencies.

---

## Quick start

```bash
# Build
cmake -S . -B build
cmake --build build

# Run a script
./build/cvm examples/arithmetic.cvm

# Interactive REPL
./build/cvm

# Debug flags (can be combined)
./build/cvm --debug-tokens   examples/arithmetic.cvm   # token stream
./build/cvm --debug-ast      examples/arithmetic.cvm   # AST dump
./build/cvm --debug-bytecode examples/arithmetic.cvm   # disassembly
```

---

## Language reference

### Types

| Type    | Literals              |
|---------|-----------------------|
| Integer | `0`, `42`, `-7`       |
| Boolean | `true`, `false`       |

### Operators

| Category    | Operators                             |
|-------------|---------------------------------------|
| Arithmetic  | `+`  `-`  `*`  `/`                   |
| Comparison  | `==`  `!=`  `<`  `<=`  `>`  `>=`    |
| Unary       | `-` (negate)  `!` (boolean not)      |

Operator precedence follows standard mathematical rules: `*` and `/` bind tighter than `+` and `-`; parentheses override everything.

### Variables

```
let x = 10;       // declare and initialise
x = x + 1;       // reassign
```

### Control flow

```
if (x > 0) {
    print x;
} else if (x == 0) {
    print 0;
} else {
    print -x;
}

while (n > 0) {
    n = n - 1;
}
```

### Built-ins

```
print <expr>;       // print value followed by newline
let n = input;      // read an integer from stdin
```

### Comments

```
// This is a line comment
```

---

## Example programs

### Arithmetic (`examples/arithmetic.cvm`)

```
let a = 10;
let b = 3;
let sum  = a + b;   // 13
let diff = a - b;   // 7
let prod = a * b;   // 30
let quot = a / b;   // 3
print sum;
print diff;
print prod;
print quot;
let isGreater = a > b;
print isGreater;    // true
```

### Factorial

```
let n = 5;
let result = 1;
while (n > 1) {
    result = result * n;
    n = n - 1;
}
print result;       // 120
```

### Fibonacci (iterative)

```
let a = 0;
let b = 1;
let i = 0;
while (i < 10) {
    let tmp = a + b;
    a = b;
    b = tmp;
    i = i + 1;
}
print a;            // 55
```

---

## Architecture

```
Source text
    │
    ▼
┌─────────┐    tokens     ┌────────┐    AST      ┌──────────┐    Chunk    ┌────┐
│  Lexer  │ ──────────▶  │ Parser │ ──────────▶ │ Compiler │ ──────────▶ │ VM │
└─────────┘               └────────┘             └──────────┘             └────┘
```

### Lexer (`include/Token.h`, `src/Lexer.cpp`)

Converts raw source characters into a flat `std::vector<Token>`.  Each `Token` carries its `TokenType`, raw lexeme string, and source line number.  Recognised token kinds: integer literals, boolean literals, string literals, identifiers, all 14 operators, 5 delimiters, 6 keywords, and `EOF`/`ERROR` sentinels.

### Parser (`include/Parser.h`, `src/Parser.cpp`)

A hand-written recursive-descent parser that consumes the token stream and builds a tree of heap-allocated `Stmt` and `Expr` nodes (`include/AST.h`).  Expression precedence is encoded directly in the call chain (equality → comparison → addition → multiply → unary → primary).  Panic-mode error recovery lets the parser report multiple errors per file by synchronising on statement boundaries.

### AST nodes

| Kind | Node | Fields |
|---|---|---|
| Expr | `NumberLit` | `intValue` |
| Expr | `BoolLit` | `boolValue` |
| Expr | `Variable` | `name` |
| Expr | `Unary` | `op`, `lhs` |
| Expr | `Binary` | `op`, `lhs`, `rhs` |
| Expr | `Grouping` | `lhs` |
| Expr | `Input` | — |
| Stmt | `Let` | `name`, `expr` |
| Stmt | `Assign` | `name`, `expr` |
| Stmt | `If` | `expr`, `thenBranch`, `elseBranch?` |
| Stmt | `While` | `expr`, `body` |
| Stmt | `Print` | `expr` |
| Stmt | `Block` | `stmts[]` |

### Compiler (`include/Compiler.h`, `src/Compiler.cpp`)

Single-pass AST walker that emits bytecode directly into a `Chunk` (`include/Chunk.h`).  Key design points:

- **Variable names** are interned into a `names[]` table; `LOAD`/`STORE`/`DEFINE` reference them by 16-bit index.
- **Forward jumps** (for `if`/`else`) use a two-step emit-then-patch strategy: the compiler emits a `JUMP_IF_FALSE` with a zero placeholder, compiles the branch body, then back-fills the correct signed 16-bit offset.
- **Back jumps** (for `while`) compute the negative offset directly from the known loop-start position.

### Bytecode instruction set

| Opcode | Operand | Stack effect |
|---|---|---|
| `PUSH_INT` | int32 | `() → (n)` |
| `PUSH_BOOL` | uint8 | `() → (b)` |
| `LOAD` | uint16 idx | `() → (val)` |
| `STORE` | uint16 idx | `(val) → ()` |
| `DEFINE` | uint16 idx | `(val) → ()` |
| `ADD` `SUB` `MUL` `DIV` | — | `(a,b) → (result)` |
| `NEG` `NOT` | — | `(a) → (result)` |
| `EQ` `NEQ` `LT` `LT_EQ` `GT` `GT_EQ` | — | `(a,b) → (bool)` |
| `PRINT` | — | `(val) → ()` |
| `INPUT` | — | `() → (int)` |
| `POP` | — | `(val) → ()` |
| `JUMP` | int16 offset | — |
| `JUMP_IF_FALSE` | int16 offset | `(cond) → ()` |
| `HALT` | — | stops execution |

### VM (`include/VM.h`, `src/VM.cpp`)

Stack-based interpreter that executes a `Chunk` in a tight `while(true)` dispatch loop.

- **Value type**: `std::variant<int32_t, bool>` — typed at runtime, not erased.
- **Variable store**: `std::unordered_map<uint16_t, Value>` keyed by name index.
- **Runtime errors**: undefined variables, type mismatches, division by zero — all reported with the faulting bytecode offset.

---

## Project structure

```
CVM++/
├── CMakeLists.txt
├── include/
│   ├── Token.h          ← TokenType enum, Token struct, stream operators
│   ├── Lexer.h          ← Lexer interface
│   ├── AST.h            ← Expr/Stmt nodes, factory functions, AST printer decl
│   ├── Parser.h         ← Recursive-descent parser interface
│   ├── Chunk.h          ← OpCode enum, Chunk (bytecode object), read helpers
│   ├── Compiler.h       ← AST → bytecode compiler interface
│   └── VM.h             ← Value type, VM interface
├── src/
│   ├── Lexer.cpp        ← Lexer implementation
│   ├── ASTPrinter.cpp   ← printExpr / printStmt (--debug-ast)
│   ├── Parser.cpp       ← Parser implementation
│   ├── Chunk.cpp        ← Chunk::disassemble (--debug-bytecode)
│   ├── Compiler.cpp     ← Compiler implementation
│   ├── VM.cpp           ← VM::execute dispatch loop
│   └── main.cpp         ← CLI entry point (REPL + file runner + debug flags)
├── tests/
│   ├── test_lexer.cpp   ← 71 lexer tests
│   ├── test_parser.cpp  ← 99 parser tests
│   ├── test_compiler.cpp← 84 compiler tests (exact bytecode assertions)
│   └── test_vm.cpp      ← 44 end-to-end execution tests
└── examples/
    ├── hello.cvm
    └── arithmetic.cvm
```

---

## Test suite

```bash
./build/test_lexer      # 71  tests — tokenisation
./build/test_parser     # 99  tests — AST structure
./build/test_compiler   # 84  tests — bytecode emission
./build/test_vm         # 44  tests — end-to-end execution
                        # ─────────────────────────────
                        # 298 total, 0 failures
```

---

## Development phases

| Phase | Status | Description |
|---|---|---|
| 1 — Lexer | ✅ Complete | Tokenisation of all language constructs |
| 2 — Parser | ✅ Complete | Recursive-descent parser → typed AST |
| 3 — Compiler | ✅ Complete | AST → bytecode with forward/back-jump patching |
| 4 — VM | ✅ Complete | Stack-based execution engine with runtime error handling |
| 5 — Polish | 🔜 Next | Richer error messages, more example scripts |

---

## Resources

- *Crafting Interpreters* — Robert Nystrom (primary reference)
