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

void IR::ParameterList::dbprint(std::ostream &out) const {
    int flags = dbgetflags(out);
    const char *sep = "";
    out << Brief;
    for (auto param : parameters) {
        out << sep << param;
        sep = ", "; }
    dbsetflags(out, flags);
}

void IR::Type_MethodBase::dbprint(std::ostream& out) const {
    int flags = dbgetflags(out);
    out << Brief;
    if (returnType != nullptr)
        out << returnType << " ";
    out << "_";
    if (typeParameters != nullptr)
        out << typeParameters;
    out << "(" << parameters << ")";
    dbsetflags(out, flags);
}

void IR::Method::dbprint(std::ostream& out) const {
    int flags = dbgetflags(out);
    out << annotations;
    if (isAbstract) out << "abstract ";
    out << Brief << type->returnType << " " << name;
    if (type->typeParameters != nullptr)
        out << type->typeParameters;
    out << "(" << type->parameters << ")";
    dbsetflags(out, flags);
}

void IR::Type_Parser::dbprint(std::ostream& out) const {
    if (dbgetflags(out) & Brief) {
        out << name;
        return; }
    out << Brief << "parser " << name;
    if (typeParameters != nullptr)
        out << typeParameters;
    out << "(" << applyParams << ")" << annotations << ';' << clrflag(Brief);
}

void IR::Type_Control::dbprint(std::ostream& out) const {
    if (dbgetflags(out) & Brief) {
        out << name;
        return; }
    out << Brief << "control " << name;
    if (typeParameters != nullptr)
        out << typeParameters;
    out << "(" << applyParams << ")" << annotations << ';' << clrflag(Brief);
}

void IR::Type_Specialized::dbprint(std::ostream& out) const {
    int flags = dbgetflags(out);
    out << Brief << baseType << "<";
    const char *sep = "";
    for (auto *arg : *arguments) {
        out << sep << arg;
        sep = ", "; }
    out << ">";
    dbsetflags(out, flags);
}

void IR::Type_Package::dbprint(std::ostream& out) const {
    if (dbgetflags(out) & Brief) {
        out << name;
        return; }
    out << Brief << "package " << name;
    if (typeParameters != nullptr)
        out << typeParameters;
    out << "(" << constructorParams << ")" << annotations << ';' << clrflag(Brief);
}

void IR::Type_Tuple::dbprint(std::ostream& out) const {
    int flags = dbgetflags(out);
    out << Brief << "tuple<";
    const char *sep = "";
    for (auto t : components) {
        out << sep << t;
        sep = ", "; }
    out << ">";
    dbsetflags(out, flags);
}

void IR::Type_Extern::dbprint(std::ostream& out) const {
    if (dbgetflags(out) & Brief) {
        out << name;
        return; }
    out << Brief << "extern " << name;
    if (typeParameters != nullptr)
        out << typeParameters;
    out << " {" << indent << clrflag(Brief);
    for (auto &method : methods)
        out << endl << method << ';';
    out << " }" << unindent;
}

void IR::TypeParameters::dbprint(std::ostream& out) const {
    int flags = dbgetflags(out);
    out << Brief << "<";
    const char *sep = "";
    for (auto p : parameters) {
        out << sep << p;
        sep = ", "; }
    out << ">";
    dbsetflags(out, flags);
}

void IR::StructField::dbprint(std::ostream &out) const {
    int flags = dbgetflags(out);
    out << Brief << annotations << type << ' ' << name;
    dbsetflags(out, flags);
}

void IR::Type_StructLike::dbprint(std::ostream &out) const {
    if (dbgetflags(out) & Brief) {
        out << name;
        return; }
    out << toString() << " " << annotations << "{" << indent;
    for (auto &field : fields)
        out << endl << field << ';';
    out << " }" << unindent;
}

void IR::Type_MethodCall::dbprint(std::ostream& out) const {
    int flags = dbgetflags(out);
    out << Brief << returnType << " _" << typeArguments << "(" << arguments << ")";
    dbsetflags(out, flags);
}

void IR::Type_ActionEnum::dbprint(std::ostream& out) const {
    Node::dbprint(out);
}
