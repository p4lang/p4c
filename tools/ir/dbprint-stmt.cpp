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

using namespace DBPrint;
using namespace IndentCtl;

void IR::ReturnStatement::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << "return";
    if (expression) {
        out << " " << Prec_Low << expression << setprec(prec); }
    if (!prec) out << ';';
}


void IR::AssignmentStatement::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << Prec_Low << left << " = " << right << setprec(prec);
    if (!prec) out << ';';
}

void IR::IfStatement::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << Prec_Low << "if (" << condition << ") {" << indent << setprec(0) << endl << ifTrue;
    if (ifFalse)
        out << unindent << endl << "} else {" << indent << endl << ifFalse;
    out << " }" << unindent << setprec(prec);
}

void IR::MethodCallStatement::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << Prec_Low << methodCall << setprec(prec);
    if (!prec) out << ';';
}

void IR::Function::dbprint(std::ostream &out) const {
    if (type->returnType) out << type->returnType << ' ';
    out << name;
    if (type->typeParameters && !type->typeParameters->empty())
        out << type->typeParameters;
    out << "(" << type->parameters << ") {" << indent;
    for (auto s : body->components)
        out << endl << s;
    out << unindent << " }";
}

void IR::SwitchStatement::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << Prec_Low << "switch (" << expression << ") {" << indent;
    bool fallthrough = false;
    for (auto c : cases) {
        if (!fallthrough) out << endl;
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
