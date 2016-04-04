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
void IR::Parser::dbprint(std::ostream &out) const { out << "IR::Parser"; }
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

void IR::ActionProfile::dbprint(std::ostream &out) const { out << "IR::ActionProfile"; }
void IR::ActionSelector::dbprint(std::ostream &out) const { out << "IR::ActionSelector"; }
void IR::Table::dbprint(std::ostream &out) const { out << "IR::Table " << name; }

void IR::Control::dbprint(std::ostream &out) const {
  out << "control " << name << " {" << indent << code << unindent << " }";
}

void IR::Global::dbprint(std::ostream &out) const {
  for (auto &obj : Values(scope))
    out << obj << endl;
}
