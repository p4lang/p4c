#include "sexp.h"
#include <iostream>
#include "ir/dbprint.h"

using namespace DBPrint;

static void print(std::ostream& out, const IR::DpdkAsmStatement* s) {
    //if (s->is<IR::DpdkJmpStatement>())
    //    out << s->to<IR::DpdkJmpStatement>() << std::endl;
    //else if (s->is<IR::DpdkLabelStatement>())
    //    out << s->to<IR::DpdkLabelStatement>() << std::endl;
    if (s->is<IR::DpdkMovStatement>())
        out << s->to<IR::DpdkMovStatement>() << std::endl;
    //else if (s->is<IR::DpdkExternObjStatement>())
    //    out << s->to<IR::DpdkExternObjStatement>() << std::endl;
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

#if 0
std::ostream& operator<<(std::ostream& out, const IR::DpdkAddStatement* statement) {
    out << "(add";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkAndStatement* statement) {
    out << "(and";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkApplyStatement* statement) {
    out << "(apply";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkEmitStatement* statement) {
    out << "(emit";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkExtractStatement* statement) {
    out << "(extract";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkJmpStatement* statement) {
    out << "(jmp " << statement->label << ")";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkJmpActionStatement* statement) {
    out << "(jmpa";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkJmpHitStatement* statement) {
    out << "(jmph";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkJmpMissStatement* statement) {
    out << "(jmpm";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkRxStatement* statement) {
    out << "(rx";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkTxStatement* statement) {
    out << "(tx";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkShlStatement* statement) {
    out << "(shl";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkShrStatement* statement) {
    out << "(shr";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkSubStatement* statement) {
    out << "(sub";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkOrStatement* statement) {
    out << "(or";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkXorStatement* statement) {
    out << "(xor";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkValidateStatement* statement) {
    out << "(valid";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkInvalidateStatement* statement) {
    out << "(invalid";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkExternObjStatement* statement) {
    out << "(extern " << statement->methodCall << ")";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkExternFuncStatement* statement) {
    out << "(extern";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkReturnStatement* statement) {
    out << "(return";
    return out;
}
std::ostream& operator<<(std::ostream& out, const IR::DpdkLabelStatement* statement) {
    out << "(label " << statement->label << ")";
    return out;
}
#endif

