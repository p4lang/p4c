#include "dpdkHelpers.h"
#include "ir/dbprint.h"
#include <iostream>

using namespace DBPrint;

void add_space(std::ostream &out, int size) {
  out << std::setfill(' ') << std::setw(size) << " ";
}

namespace DPDK {
// this function takes different subclass of Expression and translate it into
// string in desired format. For example, for PathExpression, it returns
// PathExpression->path->name For Member, it returns
// toStr(Member->expr).Member->member
cstring toStr(const IR::Expression *const);

// this function takes different subclass of Type and translate it into string
// in desired format. For example, for Type_Boolean, it returns bool For
// Type_Bits, it returns bit_<bit_width>
cstring toStr(const IR::Type *const);

// this function takes different subclass of PropertyValue and translate it into
// string in desired format. For example, for ExpressionValue, it returns
// toStr(ExpressionValue->expression)
cstring toStr(const IR::PropertyValue *const);

cstring toStr(const IR::Constant *const c) {
  std::ostringstream out;
  out << c->value;
  return out.str();
}
cstring toStr(const IR::BoolLiteral *const b) {
  std::ostringstream out;
  out << b->value;
  return out.str();
}

cstring toStr(const IR::Member *const m) {
  std::ostringstream out;
  out << m->member;
  return toStr(m->expr) + "." + out.str();
}

cstring toStr(const IR::PathExpression *const p) { return p->path->name; }

cstring toStr(const IR::TypeNameExpression *const p) {
  return p->typeName->path->name;
}

cstring toStr(const IR::MethodCallExpression *const m) {
  if (auto path = m->method->to<IR::PathExpression>()) {
    return path->path->name;
  } else {
    ::error("action's method is not a PathExpression");
  }
  return "";
}

cstring toStr(const IR::Expression *const exp) {
  if (auto e = exp->to<IR::Constant>())
    return toStr(e);
  else if (auto e = exp->to<IR::BoolLiteral>())
    return toStr(e);
  else if (auto e = exp->to<IR::Member>())
    return toStr(e);
  else if (auto e = exp->to<IR::PathExpression>())
    return toStr(e);
  else if (auto e = exp->to<IR::TypeNameExpression>())
    return toStr(e);
  else if (auto e = exp->to<IR::MethodCallExpression>())
    return toStr(e);
  else if (auto e = exp->to<IR::Cast>())
    return toStr(e->expr);
  else {
    BUG("%1% not implemented", exp);
  }
  return "";
}

cstring toStr(const IR::Type *const type) {
  if (type->is<IR::Type_Boolean>())
    return "bool";
  else if (auto b = type->to<IR::Type_Bits>()) {
    std::ostringstream out;
    out << "bit_" << b->width_bits();
    return out.str();
  } else if (auto n = type->to<IR::Type_Name>()) {
    return n->path->name;
  } else if (auto s = type->to<IR::Type_Specialized>()) {
    cstring type_s = s->baseType->path->name;
    for (auto arg : *(s->arguments)) {
      type_s += " " + toStr(arg);
    }
    return type_s;
  } else {
    std::cerr << type->node_type_name() << std::endl;
    BUG("not implemented type");
  }
}
cstring toStr(const IR::PropertyValue *const property) {
  if (auto expr_value = property->to<IR::ExpressionValue>()) {
    return toStr(expr_value->expression);
  } else {
    std::cerr << property->node_type_name() << std::endl;
    BUG("not implemneted property value");
  }
}

} // namespace DPDK

std::ostream &IR::DpdkAsmProgram::toSpec(std::ostream &out) const {
  for (auto l : globals) {
    l->toSpec(out) << std::endl;
  }
  out << std::endl;
  for (auto h : headerType)
    h->toSpec(out) << std::endl;
  for (auto s : structType)
    s->toSpec(out) << std::endl;
  for (auto a : actions) {
    a->toSpec(out) << std::endl << std::endl;
  }
  for (auto t : tables) {
    t->toSpec(out) << std::endl << std::endl;
  }
  for (auto s : statements) {
    s->toSpec(out) << std::endl;
  }
  return out;
}

std::ostream &IR::DpdkAsmStatement::toSpec(std::ostream &out) const {
  BUG("asm statement %1% not implemented", this);
  return out;
}

std::ostream &IR::DpdkDeclaration::toSpec(std::ostream &out) const {
  // TBD
  return out;
}

std::ostream &IR::DpdkHeaderType::toSpec(std::ostream &out) const {
  out << "struct " << name << " {" << std::endl;
  if (fields.empty()) {
    out << "}" << std::endl;
    return out;
  }
  for (auto it = fields.begin(); it != fields.end(); ++it) {
    if (auto t = (*it)->type->to<IR::Type_Bits>())
      out << "\tbit<" << t->width_bits() << ">";
    else if (auto t = (*it)->type->to<IR::Type_Name>())
      out << "\t" << t->path->name;
    else if ((*it)->type->to<IR::Type_Boolean>())
      out << "\tbool";
    else {
      std::cout << (*it)->type->node_type_name() << std::endl;
      BUG("Unsupported type");
    }
    out << " " << (*it)->name;
    out << std::endl;
  }
  out << "}" << std::endl;
  return out;
}

std::ostream &IR::DpdkStructType::toSpec(std::ostream &out) const {
  if (getAnnotations()->getSingle("__packet_data__")) {
    for (auto it = fields.begin(); it != fields.end(); ++it) {
      if (auto t = (*it)->type->to<IR::Type_Name>())
        out << "header " << (*it)->name << " instanceof " << t->path->name;
      else {
        std::cout << (*it)->type->node_type_name() << std::endl;
        BUG("Unsupported type");
      }
      out << std::endl;
    }
  } else {
    out << "struct " << name << " {" << std::endl;
    if (fields.empty()) {
      out << "}" << std::endl;
      return out;
    }
    for (auto it = fields.begin(); it != fields.end(); ++it) {
      if (auto t = (*it)->type->to<IR::Type_Bits>())
        out << "\tbit<" << t->width_bits() << ">";
      else if (auto t = (*it)->type->to<IR::Type_Name>()) {
        if (t->path->name == "error") {
          out << "\tbit<8>";
        } else {
          out << "\t" << t->path;
        }
      } else if ((*it)->type->to<IR::Type_Error>()) {
        // DPDK implements error type as bit<8>
        out << "\tbit<8>";
      } else if ((*it)->type->to<IR::Type_Boolean>()) {
        // DPDK implements bool as bit<8>
        out << "\tbit<8>";
      } else {
        std::cout << (*it)->type->node_type_name() << std::endl;
        BUG("Unsupported type");
      }
      out << " " << (*it)->name;
      out << std::endl;
    }
    out << "}" << std::endl;
    if (getAnnotations()->getSingle("__metadata__")) {
      out << "metadata instanceof " << name << std::endl;
    }
  }
  return out;
}

std::ostream &IR::DpdkListStatement::toSpec(std::ostream &out) const {
  out << "apply {" << std::endl;
  out << "\trx m.psa_ingress_input_metadata_ingress_port" << std::endl;
  for (auto s : statements) {
    out << "\t";
    s->toSpec(out) << std::endl;
  }
  out << "\ttx m.psa_ingress_output_metadata_egress_port" << std::endl;
  out << "}" << std::endl;
  return out;
}

std::ostream &IR::DpdkMovStatement::toSpec(std::ostream &out) const {
  out << "mov " << DPDK::toStr(left) << " " << DPDK::toStr(right);
  return out;
}

std::ostream &IR::DpdkAddStatement::toSpec(std::ostream &out) const {
  out << "add " << DPDK::toStr(dst) << " " << DPDK::toStr(src2);
  return out;
}

std::ostream &IR::DpdkEquStatement::toSpec(std::ostream &out) const {
  out << "equ " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " "
      << DPDK::toStr(src2);
  return out;
}

std::ostream &IR::DpdkAndStatement::toSpec(std::ostream &out) const {
  out << "and";
  return out;
}

std::ostream &IR::DpdkApplyStatement::toSpec(std::ostream &out) const {
  out << "table " << table;
  return out;
}

std::ostream &IR::DpdkEmitStatement::toSpec(std::ostream &out) const {
  out << "emit " << DPDK::toStr(header);
  return out;
}

std::ostream &IR::DpdkExtractStatement::toSpec(std::ostream &out) const {
  out << "extract " << DPDK::toStr(header);
  return out;
}

std::ostream &IR::DpdkJmpStatement::toSpec(std::ostream &out) const {
  out << "jmp " << label;
  return out;
}

std::ostream &IR::DpdkJmpActionStatement::toSpec(std::ostream &out) const {
  out << "jmpa";
  return out;
}

std::ostream &IR::DpdkJmpMissStatement::toSpec(std::ostream &out) const {
  out << "jmpm";
  return out;
}

std::ostream &IR::DpdkJmpHitStatement::toSpec(std::ostream &out) const {
  out << "jmph";
  return out;
}

std::ostream &IR::DpdkRxStatement::toSpec(std::ostream &out) const {
  out << "rx ";
  return out;
}

std::ostream &IR::DpdkTxStatement::toSpec(std::ostream &out) const {
  out << "tx ";
  return out;
}

std::ostream &IR::DpdkShlStatement::toSpec(std::ostream &out) const {
  out << "shl ";
  return out;
}

std::ostream &IR::DpdkShrStatement::toSpec(std::ostream &out) const {
  out << "shr ";
  return out;
}

std::ostream &IR::DpdkSubStatement::toSpec(std::ostream &out) const {
  out << "sub ";
  return out;
}

std::ostream &IR::DpdkOrStatement::toSpec(std::ostream &out) const {
  out << "or ";
  return out;
}

std::ostream &IR::DpdkXorStatement::toSpec(std::ostream &out) const {
  out << "xor ";
  return out;
}

std::ostream &IR::DpdkInvalidateStatement::toSpec(std::ostream &out) const {
  out << "jiv " << DPDK::toStr(header) << " " << label;
  return out;
}

std::ostream &IR::DpdkExternObjStatement::toSpec(std::ostream &out) const {
  out << "extern_obj ";
  return out;
}

std::ostream &IR::DpdkExternFuncStatement::toSpec(std::ostream &out) const {
  out << "extern_func ";
  return out;
}

std::ostream &IR::DpdkReturnStatement::toSpec(std::ostream &out) const {
  out << "return ";
  return out;
}

std::ostream &IR::DpdkLabelStatement::toSpec(std::ostream &out) const {
  out << "label " << label;
  return out;
}
std::ostream &IR::DpdkTable::toSpec(std::ostream &out) const {
  out << "table " << name << " {" << std::endl;
  if (match_keys) {
    out << "\tkey {" << std::endl;
    for (auto key : match_keys->keyElements) {
      out << "\t\t" << DPDK::toStr(key->expression) << " "
          << DPDK::toStr(key->matchType) << std::endl;
    }
    out << "\t}" << std::endl;
  }
  out << "\tactions {" << std::endl;
  for (auto action : actions->actionList) {
    out << "\t\t" << DPDK::toStr(action->expression) << std::endl;
  }
  out << "\t}" << std::endl;

  out << "\tdefault_action " << DPDK::toStr(default_action);
  if (default_action->to<IR::MethodCallExpression>()->arguments->size() == 0) {
    out << " args none ";
  } else {
    BUG("non-zero default action arguments not supported yet");
  }
  out << std::endl;
  if (auto psa_implementation = properties->getProperty("psa_implementation")) {
    out << "\taction_selector " << DPDK::toStr(psa_implementation->value)
        << std::endl;
  }
  if (auto size = properties->getProperty("size")) {
    out << "\tsize " << DPDK::toStr(size->value) << "" << std::endl;
  } else {
    out << "\tsize 0" << std::endl;
  }
  out << "}" << std::endl;
  return out;
}
std::ostream &IR::DpdkAction::toSpec(std::ostream &out) const {
  out << "action " << name << " args ";

  if (para.parameters.size() == 0)
    out << "none ";

  for (auto p : para.parameters) {
    out << "instanceof " << p->type << " ";
    if (p != para.parameters.back())
      out << " ";
  }
  out << "{" << std::endl;
  for (auto i : statements) {
    out << "\t";
    i->toSpec(out) << std::endl;
  }
  out << "\treturn" << std::endl;
  out << "}";

  return out;
}

std::ostream &IR::DpdkChecksumAddStatement::toSpec(std::ostream &out) const {
  out << "ckadd "
      << "h.cksum_state." << intermediate_value << " " << DPDK::toStr(field);
  return out;
}

std::ostream &IR::DpdkGetHashStatement::toSpec(std::ostream &out) const {
  out << "hash_get " << DPDK::toStr(dst) << " " << hash << " (";
  if (auto l = fields->to<IR::ListExpression>()) {
    for (auto c : l->components) {
      out << " " << DPDK::toStr(c);
    }
  } else {
    ::error("get_hash's arg is not a ListExpression.");
  }
  out << ")";
  return out;
}

std::ostream &IR::DpdkValidateStatement::toSpec(std::ostream &out) const {
  out << "jv " << DPDK::toStr(header) << " " << label;
  return out;
}

std::ostream &IR::DpdkGetChecksumStatement::toSpec(std::ostream &out) const {
  out << "mov " << DPDK::toStr(dst) << " "
      << "h.cksum_state." << intermediate_value;
  return out;
}

std::ostream &IR::DpdkNegStatement::toSpec(std::ostream &out) const {
  out << "neg " << DPDK::toStr(dst) << " " << DPDK::toStr(src);
  return out;
}

std::ostream &IR::DpdkCmplStatement::toSpec(std::ostream &out) const {
  out << "cmpl " << DPDK::toStr(dst) << " " << DPDK::toStr(src);
  return out;
}

std::ostream &IR::DpdkLNotStatement::toSpec(std::ostream &out) const {
  out << "lnot " << DPDK::toStr(dst) << " " << DPDK::toStr(src);
  return out;
}

std::ostream &IR::DpdkCastStatement::toSpec(std::ostream &out) const {
  out << "cast "
      << " " << DPDK::toStr(dst) << " " << DPDK::toStr(type) << " "
      << DPDK::toStr(src);
  return out;
}

std::ostream &IR::DpdkCmpStatement::toSpec(std::ostream &out) const {
  out << "cmp " << DPDK::toStr(src1) << " " << DPDK::toStr(src2);
  return out;
}

std::ostream &IR::DpdkJmpEqualStatement::toSpec(std::ostream &out) const {
  out << "je " << label << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2);
  return out;
}

std::ostream &IR::DpdkJmpNotEqualStatement::toSpec(std::ostream &out) const {
  out << "jne " << label << " " << DPDK::toStr(src1) << " "
      << DPDK::toStr(src2);
  return out;
}

std::ostream &
IR::DpdkJmpGreaterEqualStatement::toSpec(std::ostream &out) const {
  out << "jge " << label << " " << DPDK::toStr(src1) << " "
      << DPDK::toStr(src2);
  return out;
}

std::ostream &IR::DpdkJmpGreaterStatement::toSpec(std::ostream &out) const {
  out << "jg " << label << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2);
  return out;
}

std::ostream &IR::DpdkJmpLessorEqualStatement::toSpec(std::ostream &out) const {
  out << "jle " << label;
  return out;
}

std::ostream &IR::DpdkJmpLessorStatement::toSpec(std::ostream &out) const {
  out << "jl " << label << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2);
  return out;
}

std::ostream &IR::DpdkLAndStatement::toSpec(std::ostream &out) const {
  out << "land " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " "
      << DPDK::toStr(src2);
  return out;
}

std::ostream &IR::DpdkLeqStatement::toSpec(std::ostream &out) const {
  out << "leq " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " "
      << DPDK::toStr(src2);
  return out;
}

std::ostream &IR::DpdkLssStatement::toSpec(std::ostream &out) const {
  out << "lss " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " "
      << DPDK::toStr(src2);
  return out;
}

std::ostream &IR::DpdkGeqStatement::toSpec(std::ostream &out) const {
  out << "geq " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " "
      << DPDK::toStr(src2);
  return out;
}

std::ostream &IR::DpdkGrtStatement::toSpec(std::ostream &out) const {
  out << "grt " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " "
      << DPDK::toStr(src2);
  return out;
}

std::ostream &IR::DpdkNeqStatement::toSpec(std::ostream &out) const {
  out << "neq " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " "
      << DPDK::toStr(src2);
  return out;
}

std::ostream &IR::DpdkVerifyStatement::toSpec(std::ostream &out) const {
  out << "verify " << DPDK::toStr(condition) << " " << DPDK::toStr(error);
  return out;
}

std::ostream &IR::DpdkMeterExecuteStatement::toSpec(std::ostream &out) const {
  out << "meter_execute " << meter << " " << DPDK::toStr(index) << " "
      << DPDK::toStr(color);
  return out;
}

std::ostream &IR::DpdkCounterCountStatement::toSpec(std::ostream &out) const {
  out << "counter_count " << counter << " " << DPDK::toStr(index);
  return out;
}

std::ostream &IR::DpdkRegisterReadStatement::toSpec(std::ostream &out) const {
  out << "register_read " << DPDK::toStr(dst) << " " << reg << " "
      << DPDK::toStr(index);
  return out;
}

std::ostream &IR::DpdkRegisterWriteStatement::toSpec(std::ostream &out) const {
  out << "register_write " << reg << " " << DPDK::toStr(index) << " "
      << DPDK::toStr(src);
  return out;
}
