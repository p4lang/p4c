#include "ir.h"
#include "dbprint.h"
#include "lib/hex.h"

#define ALL_UNARY_OPS(M, ...)           \
    M(Neg, -, ##__VA_ARGS__) M(Cmpl, ~, ##__VA_ARGS__) M(LNot, !, ##__VA_ARGS__)
#define ALL_BINARY_OPS(M, ...)                                                  \
    M(Mul, Binary, *, ##__VA_ARGS__)                                            \
    M(Div, Binary, /, ##__VA_ARGS__) M(Mod, Binary, %, ##__VA_ARGS__)           \
    M(Add, Binary, +, ##__VA_ARGS__) M(Sub, Binary, -, ##__VA_ARGS__)           \
    M(Shl, Binary, <<, ##__VA_ARGS__) M(Shr, Binary, >>, ##__VA_ARGS__)         \
    M(Equ, Relation, ==, ##__VA_ARGS__) M(Neq, Relation, !=, ##__VA_ARGS__)     \
    M(Lss, Relation, <, ##__VA_ARGS__) M(Leq, Relation, <=, ##__VA_ARGS__)      \
    M(Grt, Relation, >, ##__VA_ARGS__) M(Geq, Relation, >=, ##__VA_ARGS__)      \
    M(BAnd, Binary, &, ##__VA_ARGS__) M(BOr, Binary, |, ##__VA_ARGS__)          \
    M(BXor, Binary, ^, ##__VA_ARGS__)                                           \
    M(LAnd, Binary, &&, ##__VA_ARGS__) M(LOr, Binary, ||, ##__VA_ARGS__)

using namespace DBPrint;
using namespace IndentCtl;

#define UNOP_DBPRINT(NAME, OP)                                                  \
void IR::NAME::dbprint(std::ostream &out) const {                               \
    int prec = getprec(out);                                                    \
    if (prec > Prec_Prefix) out << '(';                                         \
    out << #OP << setprec(Prec_Prefix) << expr << setprec(prec);                \
    if (prec > Prec_Prefix) out << ')';                                         \
    if (prec == 0) out << ';';                                                  \
}
ALL_UNARY_OPS(UNOP_DBPRINT)

#define BINOP_DBPRINT(NAME, BASE, OP)                                           \
void IR::NAME::dbprint(std::ostream &out) const {                               \
    int prec = getprec(out);                                                    \
    if (prec > Prec_##NAME) out << '(';                                         \
    out << setprec(Prec_##NAME) << left << " "#OP" "                            \
        << setprec(Prec_##NAME+1) << right << setprec(prec);                    \
    if (prec > Prec_##NAME) out << ')';                                         \
    if (prec == 0) out << ';';                                                  \
}
ALL_BINARY_OPS(BINOP_DBPRINT)

void IR::Slice::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << setprec(Prec_Postfix) << e0 << "["
        << setprec(Prec_Low) << e1 << ":"
        << setprec(Prec_Low) << e2 << setprec(prec) << ']';
    if (prec == 0) out << ';';
}

void IR::Primitive::dbprint(std::ostream &out) const {
    const char *sep = "";
    int prec = getprec(out);
    out << name << '(' << setprec(Prec_Low);
    for (auto a : operands) {
        out << sep << a;
        sep = ", "; }
    out << setprec(prec) << ')';
    if (prec == 0) out << ';';
}

void IR::Constant::dbprint(std::ostream &out) const {
    // Node::dbprint(out);
    out << value;
    // if (getprec(out) == 0) out << ';';
}

void IR::NamedRef::dbprint(std::ostream &out) const {
    out << name;
    if (getprec(out) == 0) out << ';';
}
void IR::Member::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << setprec(Prec_Postfix) << expr << setprec(prec) << '.' << member;
    if (prec == 0) out << ';';
}

void IR::If::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    if (prec) {
        if (prec > Prec_Cond) out << '(';
        out << setprec(Prec_Cond+1) << pred << " ? " << setprec(Prec_Low) << ifTrue
            << " : " << setprec(Prec_Cond) << ifFalse << setprec(prec);
        if (prec > Prec_Cond) out << ')';
    } else {
        out << "if (" << setprec(Prec_Low) << pred << ") {" << indent << setprec(0) << ifTrue;
        if (ifFalse) {
            if (!ifFalse->empty())
                out << unindent << endl << "} else {" << indent;
            out << ifFalse; }
        out << " }" << unindent;
    }
}

void IR::Apply::dbprint(std::ostream &out) const {
    out << "apply(" << name << ")";
    int prec = getprec(out);
    if (!actions.empty()) {
        out << " {" << indent << setprec(0);
        for (auto act : actions)
            out << endl << act.first << " {" << indent << act.second << unindent << " }";
        out << setprec(prec) << " }" << unindent;
    } else if (prec == 0) {
        out << ';'; }
}

void IR::BoolLiteral::dbprint(std::ostream &out) const {
    out << value;
    if (getprec(out) == 0) out << ';';
}

void IR::PathExpression::dbprint(std::ostream &out) const {
    out << path;
    if (getprec(out) == 0) out << ';';
}

void IR::Mux::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    if (prec > Prec_Cond) out << '(';
    out << setprec(Prec_Cond+1) << e0 << " ? "
        << setprec(Prec_Low) << e1 << " : "
        << setprec(Prec_Cond) << e2 << setprec(prec);
    if (prec > Prec_Cond) out << ')';
    if (prec == 0) out << ';';
}

void IR::Concat::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    if (prec > Prec_Add) out << '(';
    out << setprec(Prec_Add) << left << " ++ " << setprec(Prec_Add+1)
        << right << setprec(prec);
    if (prec > Prec_Add) out << ')';
    if (prec == 0) out << ';';
}

void IR::ArrayIndex::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << setprec(Prec_Postfix) << left << "[" << setprec(Prec_Low)
        << right << "]" << setprec(prec);
    if (prec == 0) out << ';';
}

void IR::Range::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    if (prec > Prec_Low) out << '(';
    out << setprec(Prec_Low) << left << " .. " << setprec(Prec_Low+1)
        << right << setprec(prec);
    if (prec > Prec_Low) out << ')';
    if (prec == 0) out << ';';
}

void IR::Mask::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    if (prec > Prec_Low) out << '(';
    out << setprec(Prec_Low) << left << " &&& " << setprec(Prec_Low+1)
        << right << setprec(prec);
    if (prec > Prec_Low) out << ')';
    if (prec == 0) out << ';';
}

void IR::Cast::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    if (prec > Prec_Prefix) out << '(';
    out << setprec(Prec_Prefix) << "(" << type << ")" << setprec(Prec_Prefix)
        << expr << setprec(prec);
    if (prec > Prec_Prefix) out << ')';
    if (prec == 0) out << ';';
}

void IR::MethodCallExpression::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    if (prec > Prec_Postfix) out << '(';
    out << setprec(Prec_Postfix) << method;
    if (typeArguments->size() > 0) {
        out << "<";
        bool first = true;
        for (auto a : *typeArguments) {
            if (first) out << ", ";
            first = false;
            out << a;
        }
        out << ">";
    }
    out << "(" << setprec(Prec_Low);
    {
        bool first = true;
        for (auto a : *arguments) {
            if (!first) out << ", ";
            first = false;
            out << setprec(Prec_Low) << a;
        }
    }
    out << ")" << setprec(prec);
    if (prec > Prec_Postfix) out << ')';
    if (prec == 0) out << ';';
}

void IR::ConstructorCallExpression::dbprint(std::ostream &out) const {
    Node::dbprint(out);
    out << type << "(" << "..." << ")";
}

void IR::ListExpression::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    if (prec > Prec_Postfix) out << '(';
    out << setprec(Prec_Postfix) << "{" << setprec(Prec_Low);
    bool first = true;
    for (auto a : *components) {
        if (!first) out << ", ";
        out << setprec(Prec_Low) << a;
        first = false;
    }
    out << "}" << setprec(prec);
    if (prec > Prec_Postfix) out << ')';
    if (prec == 0) out << ';';
}

void IR::DefaultExpression::dbprint(std::ostream &out) const { out << "default"; }

void IR::StringLiteral::dbprint(std::ostream &out) const { out << toString(); }
