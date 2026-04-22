#include "AST.h"
#include "Token.h"

#include <ostream>
#include <stdexcept>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Helper: indentation
// ─────────────────────────────────────────────────────────────────────────────
static std::string ind(int n) { return std::string(static_cast<std::size_t>(n * 2), ' '); }

// ─────────────────────────────────────────────────────────────────────────────
// printExpr
// ─────────────────────────────────────────────────────────────────────────────
void printExpr(std::ostream& os, const Expr& e, int indent) {
    switch (e.kind) {
        case ExprKind::NumberLit:
            os << ind(indent) << "Number(" << e.intValue << ")\n";
            break;

        case ExprKind::BoolLit:
            os << ind(indent) << "Bool(" << (e.boolValue ? "true" : "false") << ")\n";
            break;

        case ExprKind::Variable:
            os << ind(indent) << "Var(" << e.name << ")\n";
            break;

        case ExprKind::Input:
            os << ind(indent) << "Input\n";
            break;

        case ExprKind::Unary:
            os << ind(indent) << "Unary(" << tokenTypeName(e.op) << ")\n";
            printExpr(os, *e.lhs, indent + 1);
            break;

        case ExprKind::Binary:
            os << ind(indent) << "Binary(" << tokenTypeName(e.op) << ")\n";
            printExpr(os, *e.lhs, indent + 1);
            printExpr(os, *e.rhs, indent + 1);
            break;

        case ExprKind::Grouping:
            os << ind(indent) << "Group\n";
            printExpr(os, *e.lhs, indent + 1);
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// printStmt
// ─────────────────────────────────────────────────────────────────────────────
void printStmt(std::ostream& os, const Stmt& s, int indent) {
    switch (s.kind) {
        case StmtKind::Let:
            os << ind(indent) << "Let(" << s.name << ")\n";
            printExpr(os, *s.expr, indent + 1);
            break;

        case StmtKind::Assign:
            os << ind(indent) << "Assign(" << s.name << ")\n";
            printExpr(os, *s.expr, indent + 1);
            break;

        case StmtKind::Print:
            os << ind(indent) << "Print\n";
            printExpr(os, *s.expr, indent + 1);
            break;

        case StmtKind::ExprStmt:
            os << ind(indent) << "ExprStmt\n";
            printExpr(os, *s.expr, indent + 1);
            break;

        case StmtKind::If:
            os << ind(indent) << "If\n";
            os << ind(indent + 1) << "Cond:\n";
            printExpr(os, *s.expr, indent + 2);
            os << ind(indent + 1) << "Then:\n";
            printStmt(os, *s.thenBranch, indent + 2);
            if (s.elseBranch) {
                os << ind(indent + 1) << "Else:\n";
                printStmt(os, *s.elseBranch, indent + 2);
            }
            break;

        case StmtKind::While:
            os << ind(indent) << "While\n";
            os << ind(indent + 1) << "Cond:\n";
            printExpr(os, *s.expr, indent + 2);
            os << ind(indent + 1) << "Body:\n";
            printStmt(os, *s.body, indent + 2);
            break;

        case StmtKind::Block:
            os << ind(indent) << "Block\n";
            for (const auto& child : s.stmts) {
                printStmt(os, *child, indent + 1);
            }
            break;
    }
}
