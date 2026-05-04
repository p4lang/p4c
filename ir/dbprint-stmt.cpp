// SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
// Copyright 2013-present Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <ostream>

#include "dbprint.h"
#include "ir/ir.h"
#include "lib/indent.h"
#include "lib/log.h"

namespace P4 {

using namespace DBPrint;
using namespace IndentCtl;

void IR::ReturnStatement::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << "return";
    if (expression) {
        out << " " << Prec_Low << expression << setprec(prec);
    }
    if (!prec) out << ';';
}

void IR::AssignmentStatement::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << Prec_Low << left << " = " << right << setprec(prec);
    if (!prec) out << ';';
}

void IR::OpAssignmentStatement::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << Prec_Low << left << " " << getStringOp() << "= " << right << setprec(prec);
    if (!prec) out << ';';
}

void IR::IfStatement::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << Prec_Low << "if (" << condition << ") {" << indent << setprec(0) << Log::endl << ifTrue;
    if (ifFalse) {
        out << unindent << Log::endl << "} else ";
        if (ifFalse->is<IR::IfStatement>()) {
            out << ifFalse << setprec(prec);
            return;
        }
        out << "{" << indent << Log::endl << ifFalse;
    }
    out << " }" << unindent << setprec(prec);
}

void IR::MethodCallStatement::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << Prec_Low << methodCall << setprec(prec);
    if (!prec) out << ';';
}

void IR::Function::dbprint(std::ostream &out) const {
    out << annotations;
    if (type->returnType) out << type->returnType << ' ';
    out << name;
    if (type->typeParameters && !type->typeParameters->empty()) out << type->typeParameters;
    out << "(" << type->parameters << ") {" << indent;
    for (auto s : body->components) out << Log::endl << s;
    out << unindent << " }";
}

void IR::SwitchStatement::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << Prec_Low << "switch (" << expression << ") {" << indent;
    bool fallthrough = false;
    for (auto c : cases) {
        if (!fallthrough) out << Log::endl;
        out << Prec_Low << c->label << ": " << setprec(0);
        if (c->statement) {
            out << c->statement;
            fallthrough = false;
        } else {
            fallthrough = true;
        }
    }
    out << unindent << " }" << setprec(prec);
}

void IR::ForStatement::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << annotations << Prec_Low << "for (";
    bool first = true;
    for (auto *sd : init) {
        if (!first) out << ", ";
        out << sd;
        first = false;
    }
    out << "; " << condition << "; ";
    first = true;
    for (auto *sd : updates) {
        if (!first) out << ", ";
        out << sd;
        first = false;
    }
    out << ") {" << indent << Log::endl << body << " }" << unindent << setprec(prec);
}

void IR::ForInStatement::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << annotations << Prec_Low << "for (";
    if (decl) {
        out << decl;
    } else {
        out << ref;
    }
    out << " in " << collection << ") {" << indent << Log::endl
        << body << " }" << unindent << setprec(prec);
}

}  // namespace P4
