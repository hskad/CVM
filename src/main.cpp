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
// compile()
//
// Lex + parse + compile source into a Chunk.
// Returns false and sets hadError if any phase fails.
// ─────────────────────────────────────────────────────────────────────────────
static bool compile(const std::string& source,
                    Chunk&             chunkOut,
                    bool               debugTokens,
                    bool               debugAst,
                    bool               debugBytecode)
{
    // ── Phase 1: Lex ─────────────────────────────────────────────────────
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();

    if (debugTokens) {
        std::cout << "\n-- Token stream -------------------------------------------\n";
        for (const auto& tok : tokens)
            std::cout << "  " << tok << "\n";
        std::cout << "-----------------------------------------------------------\n\n";
    }

    if (lexer.hadError()) return false;

    // ── Phase 2: Parse ───────────────────────────────────────────────────
    Parser parser(std::move(tokens));
    std::vector<StmtPtr> program = parser.parse();

    if (debugAst) {
        std::cout << "\n-- AST ----------------------------------------------------\n";
        for (const auto& stmt : program)
            printStmt(std::cout, *stmt);
        std::cout << "-----------------------------------------------------------\n\n";
    }

    if (parser.hadError()) return false;

    // ── Phase 3: Compile ─────────────────────────────────────────────────
    Compiler compiler;
    chunkOut = compiler.compile(program);

    if (debugBytecode)
        chunkOut.disassemble(std::cout);

    return !compiler.hadError();
}

// ─────────────────────────────────────────────────────────────────────────────
// runFile
// ─────────────────────────────────────────────────────────────────────────────
static int runFile(const std::string& path,
                   bool debugTokens, bool debugAst, bool debugBytecode)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[cvm] Cannot open file: " << path << "\n";
        return 1;
    }
    std::ostringstream buf;
    buf << file.rdbuf();

    Chunk chunk;
    if (!compile(buf.str(), chunk, debugTokens, debugAst, debugBytecode))
        return 1;

    VM vm;
    VMResult result = vm.execute(chunk);
    return (result == VMResult::OK) ? 0 : 1;
}

// ─────────────────────────────────────────────────────────────────────────────
// runREPL
//
// Persistent REPL: a single VM instance lives for the whole session so that
// variables declared on one line are visible on the next.
//
// Special commands (no semicolon needed):
//   exit | quit   — leave the REPL
//   vars          — print all currently defined variables
//   reset         — clear all variables
//   help          — show command list
// ─────────────────────────────────────────────────────────────────────────────
// Returns true if `src` has at least one more '{' than '}' — i.e. the user
// has started a block and hasn't closed it yet.
static bool isIncomplete(const std::string& src) {
    int depth = 0;
    bool inString = false;
    for (std::size_t i = 0; i < src.size(); ++i) {
        char c = src[i];
        if (inString) {
            if (c == '\\') { ++i; continue; }   // skip escape
            if (c == '"')  inString = false;
            continue;
        }
        if (c == '"')  { inString = true; continue; }
        if (c == '/' && i + 1 < src.size() && src[i+1] == '/') {
            // skip rest of line
            while (i < src.size() && src[i] != '\n') ++i;
            continue;
        }
        if (c == '{') ++depth;
        if (c == '}') --depth;
    }
    return depth > 0;
}

static void runREPL(bool debugTokens, bool debugAst, bool debugBytecode)
{
    std::cout << "CVM++ 0.2.0  --  interactive REPL\n"
              << "Type 'help' for commands, 'exit' to quit.\n\n";

    VM vm;
    std::string accumulated;

    auto prompt = [&]() {
        std::cout << (accumulated.empty() ? "cvm> " : "  .. ");
        std::cout.flush();
    };

    std::string line;
    while (true) {
        prompt();
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            break;
        }

        // Trim
        auto s = line.find_first_not_of(" \t\r\n");
        auto e = line.find_last_not_of (" \t\r\n");
        if (s == std::string::npos) {
            if (accumulated.empty()) continue;
            line = "";
        } else {
            line = line.substr(s, e - s + 1);
        }

        // Built-in REPL commands (only at the top level)
        if (accumulated.empty()) {
            if (line == "exit" || line == "quit") break;

            if (line == "help") {
                std::cout <<
                    "  exit | quit   — leave the REPL\n"
                    "  vars          — list all defined variables\n"
                    "  reset         — clear all variables\n"
                    "  help          — show this message\n"
                    "  Any CVM++ statement ending with ';' or a full block '{ ... }'\n\n";
                continue;
            }
            if (line == "vars") {
                const auto& g = vm.globals();
                if (g.empty()) {
                    std::cout << "  (no variables defined)\n\n";
                } else {
                    for (const auto& [name, val] : g)
                        std::cout << "  " << name << " = " << valueStr(val) << "\n";
                    std::cout << "\n";
                }
                continue;
            }
            if (line == "reset") {
                vm.resetState();
                std::cout << "  (all variables cleared)\n\n";
                continue;
            }
        }

        // Accumulate input
        if (!accumulated.empty()) accumulated += '\n';
        accumulated += line;

        // Keep reading if the block isn't closed yet
        if (isIncomplete(accumulated)) continue;

        // We have a complete statement (or the user finished their block)
        std::string src = accumulated;
        accumulated.clear();

        if (src.empty()) continue;

        Chunk chunk;
        if (!compile(src, chunk, debugTokens, debugAst, debugBytecode))
            continue;

        vm.execute(chunk);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// printUsage
// ─────────────────────────────────────────────────────────────────────────────
static void printUsage(const char* prog) {
    std::cerr
        << "Usage:\n"
        << "  " << prog << "                          start interactive REPL\n"
        << "  " << prog << " <script.cvm>             run a script file\n"
        << "  " << prog << " [flags] [file]\n"
        << "\nFlags:\n"
        << "  --debug-tokens    print the token stream\n"
        << "  --debug-ast       print the parsed AST\n"
        << "  --debug-bytecode  print the compiled bytecode disassembly\n"
        << "  --help, -h        show this message\n";
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
            if (!scriptPath.empty()) {
                std::cerr << "[cvm] Only one script file allowed.\n";
                return 1;
            }
            scriptPath = arg;
        }
    }

    if (!scriptPath.empty())
        return runFile(scriptPath, debugTokens, debugAst, debugBytecode);

    runREPL(debugTokens, debugAst, debugBytecode);
    return 0;
}
