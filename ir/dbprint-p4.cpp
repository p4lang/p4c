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

void IR::FieldList::dbprint(std::ostream &out) const { out << "IR::FieldList"; }
void IR::FieldListCalculation::dbprint(std::ostream &out) const {
  out << "IR::FieldListCalculation"; }
void IR::CalculatedField::dbprint(std::ostream &out) const { out << "IR::CalculatedField"; }
void IR::CaseEntry::dbprint(std::ostream &out) const { out << "IR::CaseEntry"; }
void IR::V1Parser::dbprint(std::ostream &out) const { out << "IR::V1Parser"; }
void IR::ParserException::dbprint(std::ostream &out) const { out << "IR::ParserException"; }
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
  for (auto &arg : *parameters->parameters) {
    out << sep << arg->direction << ' ' << arg->type << ' ' << arg->name;
    sep = ", "; }
  out << ") {" << indent;
  if (body)
    for (auto &p : *body->components)
      out << endl << p;
  out << unindent << " }";
}

void IR::BlockStatement::dbprint(std::ostream &out) const {
  out << "{" << indent;
  if (components) {
    bool first = true;
    for (auto &p : *components) {
      if (first) {
        out << ' ' << p;
        first = false;
      } else {
        out << endl << p; } } }
  out << unindent << " }";
}

void IR::ActionProfile::dbprint(std::ostream &out) const { out << "IR::ActionProfile"; }
void IR::ActionSelector::dbprint(std::ostream &out) const { out << "IR::ActionSelector"; }
void IR::V1Table::dbprint(std::ostream &out) const { out << "IR::V1Table " << name; }

void IR::V1Control::dbprint(std::ostream &out) const {
  out << "control " << name << " {" << indent << code << unindent << " }";
}

void IR::V1Program::dbprint(std::ostream &out) const {
  for (auto &obj : Values(scope))
    out << obj << endl;
}
