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
#include "lib/map.h"

using namespace DBPrint;
using namespace IndentCtl;

void IR::HeaderStackItemRef::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << setprec(Prec_Postfix) << *base_ << "[" << setprec(Prec_Low) << *index_
        << "]" << setprec(prec);
    if (prec == 0) out << ';';
}

void IR::FieldList::dbprint(std::ostream &out) const {
    out << "field_list " << name << " {" << indent;
    for (auto f : fields)
        out << endl << f;
    if (payload)
        out << endl << "payload;";
    out << unindent << " }";
}
void IR::FieldListCalculation::dbprint(std::ostream &out) const {
    out << "field_list_calculation" << name << '(' << algorithm << ", " << output_width << ')';
}
void IR::CalculatedField::dbprint(std::ostream &out) const {
    out << "calculated_field ";
    if (field) {
        out << *field;
    } else {
        out << "(null)";
    }
    out << indent;
    for (auto &spec : specs) {
        out << endl << (spec.update ? "update " : "verify ") << spec.name;
        if (spec.cond) out << " if " << spec.cond; }
    out << unindent;
}
void IR::CaseEntry::dbprint(std::ostream &out) const {
    const char *sep = "";
    int prec = getprec(out);
    out << setprec(Prec_Low);
    for (auto &val : values) {
        if (val.first->is<IR::Constant>()) {
            if (val.second->asLong() == -1)
                out << sep << *val.first;
            else if (val.second->asLong() == 0)
                out << sep << "default";
            else
                out << sep << *val.first << " &&& " << *val.second;
        } else if (val.first->is<IR::PathExpression>()) {
            out << sep << *val.first->to<IR::PathExpression>();
        }
        sep = ", "; }
    out << ':' << setprec(prec) << " " << action;
}
void IR::V1Parser::dbprint(std::ostream &out) const {
    out << "parser " << name << " {" << indent;
    for (auto &stmt : stmts)
        out << endl << *stmt;
    if (select) {
        int prec = getprec(out);
        const char *sep = "";
        out << endl << "select (" << setprec(Prec_Low);
        for (auto e : *select) {
            out << sep << *e;
            sep = ", "; }
        out << ") {" << indent << setprec(prec); }
    if (cases)
        for (auto c : *cases)
            out << endl << *c;
    if (select)
        out << " }" << unindent;
    if (default_return)
        out << endl << "return " << default_return << ";";
    if (parse_error)
        out << endl << "error " << parse_error << ";";
    if (drop)
        out << endl << "drop;";
    out << " }" << unindent;
}
void IR::ParserException::dbprint(std::ostream &out) const { out << "IR::ParserException"; }
void IR::ParserState::dbprint(std::ostream &out) const {
    out << "state " << name << " " << annotations << "{" << indent;
    for (auto s : components)
        out << endl << s;
    if (selectExpression)
        out << endl << selectExpression;
    out << " }" << unindent;
}
void IR::P4Parser::dbprint(std::ostream &out) const {
    out << "parser " << name;
    if (type->typeParameters && !type->typeParameters->empty())
        out << type->typeParameters;
    out << '(' << type->applyParams << ')';
    if (constructorParams)
        out << '(' << constructorParams << ')';
    out << " " << type->annotations << "{" << indent;
    for (auto d : parserLocals)
        out << endl << d;
    for (auto s : states)
        out << endl << s;
    out << " }" << unindent;
}

void IR::Counter::dbprint(std::ostream &out) const { IR::Attached::dbprint(out); }
void IR::Meter::dbprint(std::ostream &out) const { IR::Attached::dbprint(out); }
void IR::Register::dbprint(std::ostream &out) const { IR::Attached::dbprint(out); }
void IR::PrimitiveAction::dbprint(std::ostream &out) const { out << "IR::PrimitiveAction"; }
void IR::NameList::dbprint(std::ostream &out) const { out << "IR::NameList"; }

void IR::ActionFunction::dbprint(std::ostream &out) const {
    out << "action " << name << "(";
    const char *sep = "";
    for (auto &arg : args) {
        out << sep << *arg->type << ' ' << arg->name;
        sep = ", "; }
    out << ") {" << indent;
    for (auto &p : action)
        out << endl << p;
    out << unindent << " }";
}

void IR::P4Action::dbprint(std::ostream &out) const {
    out << "action " << name << "(";
    const char *sep = "";
    for (auto arg : parameters->parameters) {
        out << sep << arg->direction << ' ' << arg->type << ' ' << arg->name;
        sep = ", "; }
    out << ") {" << indent;
    if (body)
        for (auto p : body->components)
            out << endl << p;
    out << unindent << " }";
}

void IR::BlockStatement::dbprint(std::ostream &out) const {
    out << "{" << indent;
    bool first = true;
    for (auto p : components) {
        if (first) {
            out << ' ' << p;
            first = false;
        } else {
            out << endl << p; } }
    out << unindent << " }";
}

void IR::ActionProfile::dbprint(std::ostream &out) const { out << "IR::ActionProfile"; }
void IR::ActionSelector::dbprint(std::ostream &out) const { out << "IR::ActionSelector"; }
void IR::V1Table::dbprint(std::ostream &out) const { out << "IR::V1Table " << name; }

void IR::ActionList::dbprint(std::ostream &out) const {
    out << "{" << indent;
    bool first = true;
    for (auto el : actionList) {
        if (first)
            out << ' ' << el;
        else
            out << endl << el;
        first = false; }
    out << unindent << " }";
}
void IR::KeyElement::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << annotations << Prec_Low << expression << ": " << matchType << setprec(prec);
    if (!prec) out << ';';
}
void IR::Key::dbprint(std::ostream &out) const {
    out << "{" << indent;
    bool first = true;
    for (auto el : keyElements) {
        if (first)
            out << ' ' << el;
        else
            out << endl << el;
        first = false; }
    out << unindent << " }";
}
void IR::P4Table::dbprint(std::ostream &out) const {
    out << "table " << name;
    out << " " << annotations << "{" << indent;
    for (auto p : properties->properties)
        out << endl << p;
    out << " }" << unindent;
}

void IR::P4ValueSet::dbprint(std::ostream &out) const {
    out << "value_set<" << elementType << "> " << name;
    out << " " << annotations << "(" << size << ")";
}

void IR::V1Control::dbprint(std::ostream &out) const {
    out << "control " << name << " {" << indent << code << unindent << " }";
}
void IR::P4Control::dbprint(std::ostream &out) const {
    out << "control " << name;
    if (type->typeParameters && !type->typeParameters->empty())
        out << type->typeParameters;
    if (type->applyParams)
        out << '(' << type->applyParams << ')';
    if (constructorParams)
        out << '(' << constructorParams << ')';
    out << " " << type->annotations << "{" << indent;
    for (auto d : controlLocals)
        out << endl << d;
    for (auto s : body->components)
        out << endl << s;
    out << " }" << unindent;
}

void IR::V1Program::dbprint(std::ostream &out) const {
    for (auto &obj : Values(scope))
        out << obj << endl;
}

void IR::P4Program::dbprint(std::ostream &out) const {
    for (auto obj : objects)
        out << obj << endl;
}

void IR::Type_Error::dbprint(std::ostream &out) const {
    out << "error {";
    const char *sep = " ";
    for (auto id : members) {
        out << sep << id->name;
        sep = ", "; }
    out << (sep+1) << "}";
}

void IR::Declaration_MatchKind::dbprint(std::ostream &out) const {
    out << "match_kind {";
    const char *sep = " ";
    for (auto id : members) {
        out << sep << id->name;
        sep = ", "; }
    out << (sep+1) << "}";
}

void IR::Declaration_Instance::dbprint(std::ostream &out) const {
    int prec = getprec(out);
    out << annotations << type << ' ' << name << '(' << Prec_Low;
    const char *sep = "";
    for (auto e : *arguments) {
        out << sep << e;
        sep = ", "; }
    out << ')' << setprec(prec);
    if (initializer)
        out << " {" << indent << initializer << " }" << unindent;
    if (!properties.empty()) {
        out << " {" << indent;
        for (auto &obj : properties)
            out << endl << obj.second;
        out << " }" << unindent; }
}
