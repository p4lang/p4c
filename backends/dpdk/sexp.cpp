#include "sexp.h"
#include "ConvertToDpdkHelper.h"
#include <iostream>
#include "ir/dbprint.h"

using namespace DBPrint;

static void print(std::ostream& out, const IR::DpdkAsmStatement* s) {
    s->toSexp(out) << std::endl;
}

void add_space(std::ostream& out, int size) {
    out << std::setfill(' ') << std::setw(size) << " ";
}

namespace DPDK {
// this function takes different subclass of Expression and translate it into string in desired format.
// For example, for PathExpression, it returns PathExpression->path->name
// For Member, it returns toStr(Member->expr).Member->member
cstring toStr(const IR::Expression *const);

// this function takes different subclass of Type and translate it into string in desired format.
// For example, for Type_Boolean, it returns bool
// For Type_Bits, it returns bit_<bit_width>
cstring toStr(const IR::Type *const);

// this function takes different subclass of PropertyValue and translate it into string in desired format.
// For example, for ExpressionValue, it returns toStr(ExpressionValue->expression)
cstring toStr(const IR::PropertyValue* const);


cstring toStr(const IR::Constant* const c){
    std::ostringstream out;
    out << c->value;
    return out.str();
}
cstring toStr(const IR::BoolLiteral* const b){
    std::ostringstream out;
    out << b->value;
    return out.str();
}

cstring toStr(const IR::Member* const m){
    std::ostringstream out;
    out << m->member;
    return toStr(m->expr) + "." + out.str();
}

cstring toStr(const IR::PathExpression* const p){
    return p->path->name;
}

cstring toStr(const IR::TypeNameExpression* const p){
    return p->typeName->path->name;
}

cstring toStr(const IR::MethodCallExpression* const m){
    if(auto path = m->method->to<IR::PathExpression>()){
        return path->path->name;
    }
    else{
        ::error("action's method is not a PathExpression");
    }
    return "";
}

cstring toStr(const IR::Expression* const exp){
    if (auto e = exp->to<IR::Constant>()) return toStr(e);
    else if (auto e = exp->to<IR::BoolLiteral>()) return toStr(e);
    else if (auto e = exp->to<IR::Member>()) return toStr(e);
    else if (auto e = exp->to<IR::PathExpression>()) return toStr(e);
    else if (auto e = exp->to<IR::TypeNameExpression>()) return toStr(e);
    else if (auto e = exp->to<IR::MethodCallExpression>()) return toStr(e);
    else if (auto e = exp->to<IR::Cast>()) return toStr(e->expr);
    else{
        BUG("%1% not implemented", exp);
    }
    return "";
}

cstring toStr(const IR::Type* const type){
    if(type->is<IR::Type_Boolean>()) return "bool";
    else if(auto b = type->to<IR::Type_Bits>()) {
        std::ostringstream out;
        out << "bit_" << b->width_bits();
        return out.str();
    }
    else if(auto n = type->to<IR::Type_Name>()) {
        return n->path->name;
    }
    else if(auto s = type->to<IR::Type_Specialized>()) {
        cstring type_s = s->baseType->path->name;
        for(auto arg: *(s->arguments)) {
            type_s += " " + toStr(arg);
        }
        return type_s;
    }
    else{
        std::cerr << type->node_type_name() << std::endl;
        BUG("not implemented type");
    }
}
cstring toStr(const IR::PropertyValue* const property){
    if(auto expr_value = property->to<IR::ExpressionValue>()){
        return toStr(expr_value->expression);
    }
    else{
        std::cerr << property->node_type_name() << std::endl;
        BUG("not implemneted property value");
    }
}

}  // namespace DPDK

std::ostream& IR::DpdkAsmProgram::toSexp(std::ostream& out) const {
    for(auto l: globals){
        l->toSexp(out) << std::endl;
    }
    out << std::endl;
    for (auto h : headerType)
        h->toSexp(out) << std::endl;
    for (auto s : structType)
        s->toSexp(out) << std::endl;
    for (auto a: actions){
        a->toSexp(out) << std::endl << std::endl;
    }
    for (auto t: tables){
        t->toSexp(out) << std::endl << std::endl;
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

std::ostream& IR::DpdkDeclaration::toSexp(std::ostream& out) const{
    if(auto var = global->to<IR::Declaration_Variable>()){
        out << "(Var " << DPDK::toStr(var->type) << " " << global->name << ")";
    }
    else if(auto ins = global->to<IR::Declaration_Instance>()){
        out << "(" << DPDK::toStr(ins->type);
        for(auto arg : *ins->arguments)
            out << " " << DPDK::toStr(arg->expression);
        out << " " << global->name << ")";
    }
    else{
        std::cout << global->node_type_name() << std::endl;
        BUG("Delcaration type not implementation");
    }
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
        else if(auto t = (*it)->type->to<IR::Type_Name>())
            out << " " << t->path->name << ")";
        else if(auto t = (*it)->type->to<IR::Type_Boolean>())
            out << " bool)";
        else{
            std::cout << (*it)->type->node_type_name() << std::endl;
            BUG("Unsupported type");
        }
        if (std::next(it) != fields.end())
            out << std::endl;
    }
    out << ")" << std::endl;
    return out;
}

std::ostream& IR::DpdkStructType::toSexp(std::ostream& out) const {
    if(this->is<IR::DpdkArgStructType>()) out << "(arg_struct " << name << std::endl;
    else if(this->is<IR::DpdkHeaderStructType>()) out << "(header_struct " << name << std::endl;
    else out << "(struct " << name << std::endl;
    if (fields.empty()) {
        out << "  ()";
        return out; }
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        out << "  (field " << (*it)->name;
        if (auto t = (*it)->type->to<IR::Type_Bits>())
            out << " (bit " << t->width_bits() << "))";
        else if (auto t = (*it)->type->to<IR::Type_Name>()){
            if(t->path->name == "error"){
                out << " (bit 8))";
            }
            else {
                out << " " << t->path << ")";
            }
        }
        else if(auto t = (*it)->type->to<IR::Type_Error>()){
            out << " (bit 8))";
        }
            // out << " " << t->error << ")";
        else if(auto t = (*it)->type->to<IR::Type_Boolean>())
            out << " bool)";
        else{
            std::cout << (*it)->type->node_type_name() << std::endl;
            BUG("Unsupported type");
        }
        if (std::next(it) != fields.end())
            out << std::endl;
    }
    out << ")" << std::endl;
    return out;
}

std::ostream& IR::DpdkListStatement::toSexp(std::ostream& out) const {
    out << "(main " << std::endl << "(" << std::endl;
    out << "  (rx m.psa_ingress_input_metadata_ingress_port)" << std::endl;
    for (auto s : statements) {
        out << "  ";
        s->toSexp(out) << std::endl;
    }
    out << "  (tx m.psa_ingress_output_metadata_egress_port)" << std::endl;
    out << ")" << std::endl << ")" << std::endl;
    return out;
}

std::ostream& IR::DpdkMovStatement::toSexp(std::ostream& out) const {
    out << "(mov " << DPDK::toStr(left) << " " << DPDK::toStr(right) << ")";
    return out;
}

std::ostream& IR::DpdkAddStatement::toSexp(std::ostream& out) const {
    // out << "(add " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";

    out << "(add " << DPDK::toStr(dst) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkEquStatement::toSexp(std::ostream& out) const {
    out << "(equ " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
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
    out << "(jmp " << label << ")";
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
    out << "(table " << name << std::endl;
    if(match_keys){
        for(auto key : match_keys->keyElements){
            add_space(out, 2);
            out << "(key " << DPDK::toStr(key->expression) << " "
                << DPDK::toStr(key->matchType) << ")" << std::endl;
        }
    }
    add_space(out, 2);
    out << "(actions (";
    for(auto action: actions->actionList){
        out << DPDK::toStr(action->expression);
        if (action != actions->actionList.back())
            out << " ";
    }
    out << "))" << std::endl;

    add_space(out, 2);
    out << "(default_action " << DPDK::toStr(default_action) << ")" << std::endl;
    if(auto psa_implementation = properties->getProperty("psa_implementation")){
        add_space(out, 2);
        out << "(action_selector " << DPDK::toStr(psa_implementation->value) << ")" << std::endl;
    }
    add_space(out, 2);
    if(auto size = properties->getProperty("size")) {
        out << "(table_size " << DPDK::toStr(size->value) << ")" << std::endl;
    }
    else {
        out << "(table_size 0)" << std::endl;
    }
    out << ")";
    return out;
}
std::ostream& IR::DpdkAction::toSexp(std::ostream& out) const {
    out << "(action " << name;
    out << " (";

    for(auto p : para.parameters){
        out << "(param " << p->type << " ";
        out << p->getName() << ")";
        if(p != para.parameters.back())
            out << " ";
    }
    out << ")" << std::endl << "(";
    for(auto i: statements){
        add_space(out, 2);
        i->toSexp(out) << std::endl;
    }
    out << "\t(return ) " << std::endl;
    out << "))";

    return out;
}

std::ostream& IR::DpdkChecksumAddStatement::toSexp(std::ostream& out) const{
    out << "(csum_add " << "h.cksum_state." << intermediate_value << " " << DPDK::toStr(field) << ")";
    return out;
}

std::ostream& IR::DpdkGetHashStatement::toSexp(std::ostream& out) const{
    out << "(hash_get " << DPDK::toStr(dst) << " " << hash  << " (";
    if(auto l = fields->to<IR::ListExpression>()){
        for(auto c : l->components) {
            out << " " << DPDK::toStr(c);
        }
    }
    else{
        ::error("get_hash's arg is not a ListExpression.");
    }
    out << "))";
    return out;
}

std::ostream& IR::DpdkValidateStatement::toSexp(std::ostream& out) const{
    out << "(jv " << DPDK::toStr(header) << " " << label << ")";
    return out;
}

std::ostream& IR::DpdkGetChecksumStatement::toSexp(std::ostream& out) const{
    out << "(mov " << DPDK::toStr(dst) << " " << "h.cksum_state." << intermediate_value << ")";
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
    out << "(cast " << " " << DPDK::toStr(dst) << " " << DPDK::toStr(type) << " " << DPDK::toStr(src) <<  ")";
    return out;
}


std::ostream& IR::DpdkCmpStatement::toSexp(std::ostream& out) const{
    out << "(cmp " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkJmpEqualStatement::toSexp(std::ostream& out) const{
    out << "(je " << label << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkJmpNotEqualStatement::toSexp(std::ostream& out) const{
    out << "(jne " << label << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkJmpGreaterEqualStatement::toSexp(std::ostream& out) const{
    out << "(jge " << label << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkJmpGreaterStatement::toSexp(std::ostream& out) const{
    out << "(jg " << label << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkJmpLessorEqualStatement::toSexp(std::ostream& out) const{
    out << "(jle " << label << ")";
    return out;
}

std::ostream& IR::DpdkJmpLessorStatement::toSexp(std::ostream& out) const{
    out << "(jl " << label << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkLAndStatement::toSexp(std::ostream& out) const{
    out << "(land " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkLeqStatement::toSexp(std::ostream& out) const{
    out << "(leq " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkLssStatement::toSexp(std::ostream& out) const{
    out << "(lss " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkGeqStatement::toSexp(std::ostream& out) const{
    out << "(geq " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkGrtStatement::toSexp(std::ostream& out) const{
    out << "(grt " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
    return out;
}

std::ostream& IR::DpdkNeqStatement::toSexp(std::ostream& out) const{
    out << "(neq " << DPDK::toStr(dst) << " " << DPDK::toStr(src1) << " " << DPDK::toStr(src2) << ")";
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
