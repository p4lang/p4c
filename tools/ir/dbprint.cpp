/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

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

void IR::Annotation::dbprint(std::ostream& out) const {
    out << '@' << name;
    const char *sep = "(";
    for (auto e : expr) {
        out << sep << e;
        sep = ", "; }
    if (*sep != '(') out << ')';
}

void IR::Block::dbprint_recursive(std::ostream& out) const {
    out << dbp(this);
    out << indent;
    for (auto it : constantValue) {
        if (it.second == nullptr) {
            out << endl << dbp(it.first) << " => null";
            continue;
        }
        if (it.second->is<IR::Block>() && it.first->is<IR::IDeclaration>()) {
            auto block = it.second->to<IR::Block>();
            out << endl << dbp(it.first) << " => ";
            block->dbprint_recursive(out);
        }
    }
    out << unindent;
}

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
void dbprint(const IR::Node &n) {
  std::cout << n << std::endl;
}
void dbprint(const std::set<const IR::Expression *> s) {
    std::cout << indent << " {";
    int i = 0;
    for (auto el : s) std::cout << endl << '[' << i++ << "] " << el;
    std::cout << " }" << unindent << std::endl;
}
