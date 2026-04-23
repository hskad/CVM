#include "Lexer.h"
#include "Parser.h"
#include "AST.h"
#include "Chunk.h"
#include "Compiler.h"
#include "VM.h"
#include "Token.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// runSource
//
// Runs the lexer on a source string, prints the token list, and reports
// any errors.  Later phases (parser, compiler, VM) will be plugged in here.
// ─────────────────────────────────────────────────────────────────────────────
static void runSource(const std::string& source, bool debugTokens, bool debugAst, bool debugBytecode) {
    // ── Phase 1: Lex ─────────────────────────────────────────────────────
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    if (debugTokens) {
        std::cout << "\n── Token stream ───────────────────────────────\n";
        for (const auto& tok : tokens) {
            std::cout << "  " << tok << "\n";
        }
        std::cout << "───────────────────────────────────────────────\n\n";
    }

    if (lexer.hadError()) {
        std::cerr << "[cvm] Lexer reported errors. Aborting.\n";
        return;
    }

    // ── Phase 2: Parse ───────────────────────────────────────────────────
    Parser parser(std::move(tokens));
    std::vector<StmtPtr> program = parser.parse();

    if (debugAst) {
        std::cout << "\n── AST ────────────────────────────────────────\n";
        for (const auto& stmt : program) {
            printStmt(std::cout, *stmt);
        }
        std::cout << "───────────────────────────────────────────────\n\n";
    }

    if (parser.hadError()) {
        std::cerr << "[cvm] Parser reported errors. Aborting.\n";
        return;
    }

    // ── Phase 3: Compile ─────────────────────────────────────────────────
    Compiler compiler;
    Chunk chunk = compiler.compile(program);

    if (debugBytecode) {
        chunk.disassemble(std::cout);
    }

    if (compiler.hadError()) {
        std::cerr << "[cvm] Compiler reported errors. Aborting.\n";
        return;
    }

    // ── Phase 4: Execute ─────────────────────────────────────────────────
    VM vm;
    vm.execute(chunk);
}

// ─────────────────────────────────────────────────────────────────────────────
// runFile
//
// Reads an entire .cvm file into a string and passes it to runSource.
// ─────────────────────────────────────────────────────────────────────────────
static int runFile(const std::string& path, bool debugTokens, bool debugAst, bool debugBytecode) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[cvm] Could not open file: " << path << "\n";
        return 1;
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    runSource(buf.str(), debugTokens, debugAst, debugBytecode);
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// runREPL
//
// Interactive Read-Eval-Print Loop.  Each line is lexed independently.
// ─────────────────────────────────────────────────────────────────────────────
static void runREPL(bool debugTokens, bool debugAst, bool debugBytecode) {
    std::string line;
    std::cout << "CVM++ 0.1.0  (type 'exit' or 'quit' to leave)\n";

    while (true) {
        std::cout << "cvm> ";
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }
        if (line == "exit" || line == "quit") break;
        if (line.empty()) continue;

        runSource(line, debugTokens, debugAst, debugBytecode);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// printUsage
// ─────────────────────────────────────────────────────────────────────────────
static void printUsage(const char* prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << "                         — start interactive REPL\n"
              << "  " << prog << " <script.cvm>            — run a script file\n"
              << "  " << prog << " --debug-tokens   [file] — show token stream\n"
              << "  " << prog << " --debug-ast      [file] — show parsed AST\n"
              << "  " << prog << " --debug-bytecode [file] — show compiled bytecode\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    bool        debugTokens   = false;
    bool        debugAst      = false;
    bool        debugBytecode = false;
    std::string scriptPath;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if      (arg == "--debug-tokens")   debugTokens   = true;
        else if (arg == "--debug-ast")      debugAst      = true;
        else if (arg == "--debug-bytecode") debugBytecode = true;
        else if (arg == "--help" || arg == "-h") { printUsage(argv[0]); return 0; }
        else if (arg[0] == '-') {
            std::cerr << "[cvm] Unknown flag: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        } else {
            scriptPath = arg;
        }
    }

    if (!scriptPath.empty()) return runFile(scriptPath, debugTokens, debugAst, debugBytecode);
    runREPL(debugTokens, debugAst, debugBytecode);
    return 0;
}
