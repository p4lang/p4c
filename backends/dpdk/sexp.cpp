#include "sexp.h"
#include <iostream>
#include "ir/dbprint.h"

using namespace DBPrint;

static void print(std::ostream& out, const IR::DpdkAsmStatement* s) {
    s->toSexp(out) << std::endl;
    // if (s->is<IR::DpdkJmpStatement>())
    //     out << s->to<IR::DpdkJmpStatement>() << std::endl;
    // else if (s->is<IR::DpdkLabelStatement>())
    //     out << s->to<IR::DpdkLabelStatement>() << std::endl;
    // else if (s->is<IR::DpdkMovStatement>())
    //     out << s->to<IR::DpdkMovStatement>() << std::endl;
    // else if (s->is<IR::DpdkExternObjStatement>()) {
    //     LOG1("print extern ");
    //     out << s->to<IR::DpdkExternObjStatement>() << std::endl; }
    // else if (s->is<IR::DpdkListStatement>())
    //     out << s->to<IR::DpdkListStatement>() << std::endl;
    // else
    //     BUG("Statement %s unsupported", s);
}

std::ostream& IR::DpdkAsmProgram::toSexp(std::ostream& out) const {
    for (auto h : headerType)
        h->toSexp(out) << std::endl;
        // out << h << std::endl;
    for (auto s : structType)
        s->toSexp(out) << std::endl;
    for (auto s : statements) {
        s->toSexp(out) << std::endl;
    }
    for (auto a: actions){
        a->toSexp(out) << std::endl;
    }
    for (auto t: tables){
        t->toSexp(out) << std::endl;
    }
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
    for (auto s : statements) {
        out << "  ";
        s->toSexp(out) << std::endl;
        // if (s->is<IR::DpdkJmpStatement>())
        //     out << s->to<IR::DpdkJmpStatement>() << std::endl;
        // else if (s->is<IR::DpdkLabelStatement>())
        //     out << s->to<IR::DpdkLabelStatement>() << std::endl;
        // else if (s->is<IR::DpdkMovStatement>())
        //     out << s->to<IR::DpdkMovStatement>() << std::endl;
        // else if (s->is<IR::DpdkAddStatement>())
        //     out << s->to<IR::DpdkAddStatement>() << std::endl;
        // else if (s->is<IR::DpdkExternObjStatement>()) {
        //     out << s->to<IR::DpdkExternObjStatement>() << std::endl; }
        // else if (s->is<IR::DpdkListStatement>())
        //     out << s->to<IR::DpdkListStatement>() << std::endl;
        // else if (s->is<IR::DpdkExtractStatement>())
        //     out << s->to<IR::DpdkExtractStatement>() << std::endl;
        // else
        //     BUG("Statement %s unsupported", s);
    }
    out << ")" << std::endl;
    return out;
}

std::ostream& IR::DpdkMovStatement::toSexp(std::ostream& out) const {
    out << "(mov " << left << " " << right << ")";
    return out;
}

std::ostream& IR::DpdkAddStatement::toSexp(std::ostream& out) const {
    out << "(add " << dst << " " << src1 << " " << src2 << ")";
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
    out << "(extract " << header << ")";
    return out;
}

std::ostream& IR::DpdkJmpStatement::toSexp(std::ostream& out) const {
    LOG1("print jmp");
    if(condition.isNullOrEmpty())
        out << "(jmp " << label << ")";
    else
        out << "(jmp " << label << " " << condition << ")";
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
std::ostream& IR::DpdkTable::toSexp(std::ostream& out) const {
    out << "(deft " << name << std::endl;
    if(match_keys){
        out << "(key " << std::endl;
        for(auto key : match_keys->keyElements){
            out << "(" << key->expression << " " << key->matchType << ")" << std::endl;
        }
        out << ")" << std::endl; 
    }
    out << "(action " << std::endl;
    for(auto action: actions->actionList){
        out << "(" << action->expression << ")" << std::endl;
    }
    out << ")" << std::endl;

    out << ")";
    return out;
}
std::ostream& IR::DpdkAction::toSexp(std::ostream& out) const {
    out << "(defa " << name;
    out << " (";

    for(auto p : para.parameters){
        out << p->type << " ";
        out << p->name;
        if(p != para.parameters.back())
            out << ", ";
    }
    out << ")" << std::endl << "(" << std::endl;
    for(auto i: statements){
        i->toSexp(out) << std::endl;
    }
    out << "))";

    return out;
}