#include "sexp.h"
#include <iostream>
#include "ir/dbprint.h"

using namespace DBPrint;

static void print(std::ostream& out, const IR::DpdkAsmStatement* s) {
    if (s->is<IR::DpdkJmpStatement>())
        out << s->to<IR::DpdkJmpStatement>() << std::endl;
    else if (s->is<IR::DpdkLabelStatement>())
        out << s->to<IR::DpdkLabelStatement>() << std::endl;
    if (s->is<IR::DpdkMovStatement>())
        out << s->to<IR::DpdkMovStatement>() << std::endl;
    else if (s->is<IR::DpdkExternObjStatement>())
        out << s->to<IR::DpdkExternObjStatement>() << std::endl;
    else if (s->is<IR::DpdkListStatement>())
        out << s->to<IR::DpdkListStatement>() << std::endl;
    else
        BUG("Statement %s unsupported", s);
}

std::ostream& IR::DpdkAsmProgram::toSexp(std::ostream& out) const {
    for (auto h : headerType)
        out << h << std::endl;
    for (auto s : structType)
        out << s << std::endl;
    for (auto s : statements) {
        out << "  ";
        ::print(out, s);
        out << std::endl;
    }
    return out;
}

std::ostream& IR::DpdkAction::toSexp(std::ostream& out) const {
    out << " Action";
    return out;
}

std::ostream& IR::DpdkAsmStatement::toSexp(std::ostream& out) const {
    out << "asm";
    return out;
}

std::ostream& IR::DpdkHeaderType::toSexp(std::ostream& out) const {
    out << "(header " << name << std::endl;
    if (fields.empty()) {
        out << "  ()";
        return out; }
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        out << "  (field " << (*it)->name;
        if (auto t = (*it)->type->to<IR::Type_Bits>())
            out << " (bit " << t->width_bits() << "))";
        else if (auto t = (*it)->type->to<IR::Type_Name>())
            out << " " << t->path << ")";
        else if (auto t = (*it)->type->to<IR::Type_Boolean>())
            out << " bool)";
        if (std::next(it) != fields.end())
            out << std::endl;
    }
    out << ")" << std::endl;
    return out;
}

std::ostream& IR::DpdkStructType::toSexp(std::ostream& out) const {
    out << "(struct " << name << std::endl;
    if (fields.empty()) {
        out << "  ()";
        return out; }
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        out << "  (field " << (*it)->name;
        if (auto t = (*it)->type->to<IR::Type_Bits>())
            out << " (bit " << t->width_bits() << "))";
        else if (auto t = (*it)->type->to<IR::Type_Name>())
            out << " " << t->path << ")";
        if (std::next(it) != fields.end())
            out << std::endl;
    }
    out << ")" << std::endl;
    return out;
}

std::ostream& IR::DpdkListStatement::toSexp(std::ostream& out) const {
    out << "(def_process " << name << std::endl;
    for (auto h : statements) {
        out << "  ";
        ::print(out, h);
    }
    out << ")" << std::endl;
    return out;
}

std::ostream& IR::DpdkMovStatement::toSexp(std::ostream& out) const {
    out << "(mov " << left << " " << right << ")";
    return out;
}

std::ostream& IR::DpdkAddStatement::toSexp(std::ostream& out) const {
    out << "(add";
    return out;
}

std::ostream& IR::DpdkAndStatement::toSexp(std::ostream& out) const {
    out << "(and";
    return out;
}

std::ostream& IR::DpdkApplyStatement::toSexp(std::ostream& out) const {
    out << "(apply";
    return out;
}

std::ostream& IR::DpdkEmitStatement::toSexp(std::ostream& out) const {
    out << "(emit";
    return out;
}

std::ostream& IR::DpdkExtractStatement::toSexp(std::ostream& out) const {
    out << "(extract";
    return out;
}

std::ostream& IR::DpdkJmpStatement::toSexp(std::ostream& out) const {
    out << "(jmp" << label << ")";
    return out;
}

std::ostream& IR::DpdkJmpActionStatement::toSexp(std::ostream& out) const {
    out << "(jmpa" << ")";
    return out;
}

std::ostream& IR::DpdkJmpMissStatement::toSexp(std::ostream& out) const {
    out << "(jmpm" << ")";
    return out;
}

std::ostream& IR::DpdkJmpHitStatement::toSexp(std::ostream& out) const {
    out << "(jmph" << ")";
    return out;
}

std::ostream& IR::DpdkRxStatement::toSexp(std::ostream& out) const {
    out << "(rx " << ")";
    return out;
}

std::ostream& IR::DpdkTxStatement::toSexp(std::ostream& out) const {
    out << "(tx " << ")";
    return out;
}

std::ostream& IR::DpdkShlStatement::toSexp(std::ostream& out) const {
    out << "(shl " << ")";
    return out;
}

std::ostream& IR::DpdkShrStatement::toSexp(std::ostream& out) const {
    out << "(shr " << ")";
    return out;
}

std::ostream& IR::DpdkSubStatement::toSexp(std::ostream& out) const {
    out << "(sub " << ")";
    return out;
}

std::ostream& IR::DpdkOrStatement::toSexp(std::ostream& out) const {
    out << "(or " << ")";
    return out;
}

std::ostream& IR::DpdkXorStatement::toSexp(std::ostream& out) const {
    out << "(xor " << ")";
    return out;
}

std::ostream& IR::DpdkValidateStatement::toSexp(std::ostream& out) const {
    out << "(valid " << ")";
    return out;
}

std::ostream& IR::DpdkInvalidateStatement::toSexp(std::ostream& out) const {
    out << "(invalid " << ")";
    return out;
}

std::ostream& IR::DpdkExternObjStatement::toSexp(std::ostream& out) const {
    out << "(extern_obj " << ")";
    return out;
}

std::ostream& IR::DpdkExternFuncStatement::toSexp(std::ostream& out) const {
    out << "(extern_func " << ")";
    return out;
}

std::ostream& IR::DpdkReturnStatement::toSexp(std::ostream& out) const {
    out << "(return " << ")";
    return out;
}

std::ostream& IR::DpdkLabelStatement::toSexp(std::ostream& out) const {
    out << "(label " << label << ")";
    return out;
}

std::ostream& operator<<(std::ostream& out, const IR::DpdkAsmProgram* p) {
    return p->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkHeaderType* h) {
    return h->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkStructType* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkAsmStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkListStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkMovStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkAddStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkAndStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkApplyStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkEmitStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkExtractStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkJmpStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkJmpActionStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkJmpHitStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkJmpMissStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkRxStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkTxStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkShlStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkShrStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkSubStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkOrStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkXorStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkValidateStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkInvalidateStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkExternObjStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkExternFuncStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkReturnStatement* s) {
    return s->toSexp(out); }
std::ostream& operator<<(std::ostream& out, const IR::DpdkLabelStatement* s) {
    return s->toSexp(out); }

