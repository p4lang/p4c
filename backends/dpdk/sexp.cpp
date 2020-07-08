#include "sexp.h"
#include "ConvertToDpdkHelper.h"
#include <iostream>
#include "ir/dbprint.h"

using namespace DBPrint;

static void print(std::ostream& out, const IR::DpdkAsmStatement* s) {
    s->toSexp(out) << std::endl;
}

std::ostream& IR::DpdkAsmProgram::toSexp(std::ostream& out) const {
    for (auto h : headerType)
        h->toSexp(out) << std::endl;
    for (auto s : structType)
        s->toSexp(out) << std::endl;
    for (auto a: actions){
        a->toSexp(out) << std::endl;
        out << std::endl;
    }
    for (auto t: tables){
        t->toSexp(out) << std::endl;
        out << std::endl;
    }
    for (auto s : statements) {
        s->toSexp(out) << std::endl;
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
    }
    out << ")" << std::endl;
    return out;
}

std::ostream& IR::DpdkMovStatement::toSexp(std::ostream& out) const {
    out << "(mov " << DPDK::toStr(left) << " " << DPDK::toStr(right) << ")";
    return out;
}

std::ostream& IR::DpdkAddStatement::toSexp(std::ostream& out) const {
    out << "(add " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkEquStatement::toSexp(std::ostream& out) const {
    out << "(Equ " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkAndStatement::toSexp(std::ostream& out) const {
    out << "(and";
    return out;
}

std::ostream& IR::DpdkApplyStatement::toSexp(std::ostream& out) const {
    out << "(apply " << table << ")";
    return out;
}

std::ostream& IR::DpdkEmitStatement::toSexp(std::ostream& out) const {
    out << "(emit " << DPDK::toStr(header) << ")";
    return out;
}

std::ostream& IR::DpdkExtractStatement::toSexp(std::ostream& out) const {
    out << "(extract " << DPDK::toStr(header) << ")";
    return out;
}

std::ostream& IR::DpdkJmpStatement::toSexp(std::ostream& out) const {
    LOG1("print jmp");
    if(!condition)
        out << "(jmp " << label << ")";
    else
        out << "(jmp " << label << " " << DPDK::toStr(condition) << ")";
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
        for(auto key : match_keys->keyElements){
            out << "(key (" << DPDK::toStr(key->expression) << " " << DPDK::toStr(key->matchType) << "))" << std::endl;
        }
    }
    out << "(action " << std::endl;
    for(auto action: actions->actionList){
        out << "(" << DPDK::toStr(action->expression) << ")" << std::endl;
    }
    out << ")" << std::endl;

    out << "(default_action " << DPDK::toStr(default_action) << " )" << std::endl;
    

    out << ")";
    return out;
}
std::ostream& IR::DpdkAction::toSexp(std::ostream& out) const {
    out << "(defa " << name;
    out << " (";

    for(auto p : para.parameters){
        out << p->type << " ";
        out << p->getName();
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

std::ostream& IR::DpdkChecksumAddStatement::toSexp(std::ostream& out) const{
    out << "(csum_add " << csum << " " << DPDK::toStr(field) << ")";
    return out;
}

std::ostream& IR::DpdkGetHashStatement::toSexp(std::ostream& out) const{
    out << "(get_hash " << DPDK::toStr(dst) << " " << hash  << " (";
    if(auto l = fields->to<IR::ListExpression>()){
        for(auto c : l->components) {
            out << " " << DPDK::toStr(c);
        }
    }
    else{
        BUG("get_hash's arg is not a ListExpression.");
    }
    out << "))";
    return out;
}

std::ostream& IR::DpdkValidateStatement::toSexp(std::ostream& out) const{
    out << "(validate " << DPDK::toStr(dst) << " " << header << ")";
    return out;
}

std::ostream& IR::DpdkGetChecksumStatement::toSexp(std::ostream& out) const{
    out << "(csum_get " << DPDK::toStr(dst) << " " << checksum << ")";
    return out;
}

std::ostream& IR::DpdkNegStatement::toSexp(std::ostream& out) const{
    out << "(neg " << DPDK::toStr(dst) << " " << DPDK::toStr(src) << ")";
    return out;
}

std::ostream& IR::DpdkCmplStatement::toSexp(std::ostream& out) const{
    out << "(cmpl " << DPDK::toStr(dst) << " " << DPDK::toStr(src) << ")";
    return out;
}

std::ostream& IR::DpdkLNotStatement::toSexp(std::ostream& out) const{
    out << "(lnot " << DPDK::toStr(dst) << " " << DPDK::toStr(src) << ")";
    return out;
}

std::ostream& IR::DpdkCastStatement::toSexp(std::ostream& out) const{
    out << "(cast " << " " << DPDK::toStr(dst) << DPDK::toStr(type) << " " << DPDK::toStr(src) <<  ")";
    return out;
}


std::ostream& IR::DpdkCmpStatement::toSexp(std::ostream& out) const{
    out << "(cmp " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkJmpEqualStatement::toSexp(std::ostream& out) const{
    out << "(je " << label << ")";
    return out;
}

std::ostream& IR::DpdkJmpNotEqualStatement::toSexp(std::ostream& out) const{
    out << "(je " << label << ")";
    return out;
}

std::ostream& IR::DpdkJmpGreaterEqualStatement::toSexp(std::ostream& out) const{
    out << "(jge " << label << ")";
    return out;
}

std::ostream& IR::DpdkJmpGreaterStatement::toSexp(std::ostream& out) const{
    out << "(jg " << label << ")";
    return out;
}

std::ostream& IR::DpdkJmpLessorEqualStatement::toSexp(std::ostream& out) const{
    out << "(jle " << label << ")";
    return out;
}

std::ostream& IR::DpdkJmpLessorStatement::toSexp(std::ostream& out) const{
    out << "(jl " << label << ")";
    return out;
}

std::ostream& IR::DpdkLAndStatement::toSexp(std::ostream& out) const{
    out << "(LAnd " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkLeqStatement::toSexp(std::ostream& out) const{
    out << "(Leq " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkLssStatement::toSexp(std::ostream& out) const{
    out << "(Lss " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkGeqStatement::toSexp(std::ostream& out) const{
    out << "(Geq " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkGrtStatement::toSexp(std::ostream& out) const{
    out << "(Grt " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkNeqStatement::toSexp(std::ostream& out) const{
    out << "(Neq " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkVerifyStatement::toSexp(std::ostream& out) const{
    out << "(verify " << DPDK::toStr(condition) << " " << DPDK::toStr(error) << ")";
    return out;
}

std::ostream& IR::DpdkMeterExecuteStatement::toSexp(std::ostream& out) const{
    out << "(meter_execute " << meter << " " << DPDK::toStr(index) << " " << DPDK::toStr(color) << ")";
    return out;
}

std::ostream& IR::DpdkCounterCountStatement::toSexp(std::ostream& out) const{
    out << "(counter_count " << counter << " " << DPDK::toStr(index) << ")";
    return out;
}

std::ostream& IR::DpdkRegisterReadStatement::toSexp(std::ostream& out) const{
    out << "(register_read " << DPDK::toStr(dst) << " " << reg << " " << DPDK::toStr(index) << ")";
    return out;
}

std::ostream& IR::DpdkRegisterWriteStatement::toSexp(std::ostream& out) const{
    out << "(register_write " << reg << " " << DPDK::toStr(index) << " " << DPDK::toStr(src) << ")";
    return out;
}
