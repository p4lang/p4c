#include "ir.h"
#include "dbprint.h"
#include "lib/hex.h"

using namespace DBPrint;
using namespace IndentCtl;

static int dbprint_index = -1;

int DBPrint::dbgetflags(std::ostream &out) {
    if (dbprint_index < 0) dbprint_index = out.xalloc();
    return out.iword(dbprint_index);
}

int DBPrint::dbsetflags(std::ostream &out, int val, int mask) {
    if (dbprint_index < 0) dbprint_index = out.xalloc();
    auto &flags = out.iword(dbprint_index);
    int rv = flags;
    flags = (flags & ~mask) | val;
    return rv;
}

void IR::Node::dbprint(std::ostream &out) const {
    out << "<" << node_type_name() << ">(" << id << ")";
}

void IR::Block::dbprint(std::ostream& out) const {
    IR::Node::dbprint(out);
    out << " " << node;
}

void IR::InstantiatedBlock::dbprint(std::ostream& out) const {
    IR::Node::dbprint(out);
    out << " " << node << " instance type=" << instanceType;
}

void IR::Block::dbprint_recursive(std::ostream& out) const {
    dbprint(out);
    out << indent;
    for (auto it : constantValue) {
        if (it.second->is<IR::Block>() && it.first->is<IR::IDeclaration>()) {
            auto block = it.second->to<IR::Block>();
            out << endl << it.first << " => ";
            block->dbprint_recursive(out);
        }
    }
    out << unindent;
}

void IR::Type_MethodBase::dbprint(std::ostream& out) const {
    Node::dbprint(out);
    if (returnType != nullptr)
        out << returnType << " ";
    out << "_";
    if (typeParameters != nullptr)
        out << typeParameters;
    out << "(" << parameters << ")";
}

void IR::Type_Parser::dbprint(std::ostream& out) const {
    Type_Declaration::dbprint(out);
    if (typeParams != nullptr)
        out << typeParams;
    out << "(" << applyParams << ")";
}

void IR::Type_Control::dbprint(std::ostream& out) const {
    Type_Declaration::dbprint(out);
    if (typeParams != nullptr)
        out << typeParams;
    out << "(" << applyParams << ")";
}

void IR::Type_Package::dbprint(std::ostream& out) const {
    Type_Declaration::dbprint(out);
    if (typeParams != nullptr)
        out << typeParams;
    out << "(" << constructorParams << ")";
}

void IR::Method::dbprint(std::ostream& out) const {
    Declaration::dbprint(out);
    out << " " << getName() << " " << type;
}

void IR::Type_Extern::dbprint(std::ostream& out) const {
    Node::dbprint(out);
    if (typeParameters != nullptr)
        out << typeParameters;
}

void IR::Type_Var::dbprint(std::ostream& out) const {
    Node::dbprint(out);
    out << ":" << toString();
}

void IR::Parameter::dbprint(std::ostream& out) const {
    Node::dbprint(out);
    out << direction << " " << type << " " << name;
}

void IR::TypeParameters::dbprint(std::ostream& out) const {
    Node::dbprint(out);
    out << "<";
    bool first = true;
    for (auto p : *getEnumerator()) {
        if (!first)
            out << ", ";
        out << p;
        first = false;
    }
    out << ">";
}

void IR::Type_Table::dbprint(std::ostream& out) const
{ Node::dbprint(out); out << container->name; }

std::ostream &operator<<(std::ostream &out, const IR::Vector<IR::Expression> &v) {
    int prec = getprec(out);
    if (prec) {
        if (v.size() == 1) {
            out << v[0];
            return out; }
        out << "{"; }
    for (auto e : v)
        out << endl << setprec(0) << e << setprec(prec);
    if (prec)
        out << " }";
    return out;
}

void dbprint(const IR::Node *n) {
  std::cout << n << std::endl;
}

void IR::ArgumentInfo::dbprint(std::ostream& out) const {
    Node::dbprint(out);
}

void IR::MethodCallStatement::dbprint(std::ostream& out) const {
    Node::dbprint(out);
    out << " " << methodCall->method;
}

void IR::Type_MethodCall::dbprint(std::ostream& out) const {
    Node::dbprint(out);
    out << returnType << " _";
    out << typeArguments;
    out << "(" << arguments << ")";
}

void IR::Type_Bits::dbprint(std::ostream& out) const {
    Node::dbprint(out);
    out << toString();
}

void IR::Type_ActionEnum::dbprint(std::ostream& out) const {
    Node::dbprint(out);
}
