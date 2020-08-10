/*
Copyright 2020 Intel Corp.

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
#include "ConvertToDpdkHelper.h"
#include "ir/ir.h"
#include <iostream>

namespace DPDK{

bool ConvertStatementToDpdk::preorder(const IR::AssignmentStatement* a){
    auto left = a->left;
    auto right = a->right;
    IR::DpdkAsmStatement *i;
    // handle Binary Operation
    if(auto r = right->to<IR::Operation_Binary>()){
        if(right->is<IR::Add>()){
            i = new IR::DpdkAddStatement(left, r->left, r->right);
        }
        else if(right->is<IR::Sub>()){
            i = new IR::DpdkSubStatement(left, r->left, r->right);
        }
        else if(right->is<IR::Shl>()){
            i = new IR::DpdkShlStatement(left, r->left, r->right);
        }
        else if(right->is<IR::Shr>()){
            i = new IR::DpdkShrStatement(left, r->left, r->right);
        }
        else if(right->is<IR::Equ>()){
            i = new IR::DpdkEquStatement(left, r->left, r->right);
        }
        else if(right->is<IR::LAnd>()){
            i = new IR::DpdkLAndStatement(left, r->left, r->right);
        }
        else if(right->is<IR::Leq>()){
            i = new IR::DpdkLeqStatement(left, r->left, r->right);
        }
        else {
            std::cerr << right->node_type_name() << std::endl;
            BUG("not implemented.");
        }
    }
    // handle method call
    else if(auto m = right->to<IR::MethodCallExpression>()){
        auto mi = P4::MethodInstance::resolve(m, refmap, typemap);
        // std::cerr << m << std::endl;
        if(auto e = mi->to<P4::ExternMethod>()){
            if(e->originalExternType->getName().name == "Hash"){
                if(e->expr->arguments->size() == 1){
                    auto field = (*e->expr->arguments)[0];
                    i = new IR::DpdkGetHashStatement(e->object->getName(), field->expression, left);
                }
            }
            else if(e->originalExternType->getName().name == "InternetChecksum"){
                if(e->method->getName().name == "get"){
                    auto res = csum_map->find(e->object->to<IR::Declaration_Instance>());
                    cstring intermediate;
                    if(res != csum_map->end()){
                        intermediate = res->second;
                    }
                    i = new IR::DpdkGetChecksumStatement(left, e->object->getName(), intermediate);
                }
            }
            else if(e->originalExternType->getName().name == "Register"){
                if(e->method->getName().name == "get"){
                    auto index = (*e->expr->arguments)[0]->expression;
                    i = new IR::DpdkRegisterReadStatement(left, e->object->getName(), index);
                }
            }
            else{
                std::cerr << e->originalExternType->getName() << std::endl;
                BUG("ExternMethod Not implemented");
            }
        }
        else if(auto b = mi->to<P4::BuiltInMethod>()){
            std::cerr << b->name.name << std::endl;
            BUG("BuiltInMethod Not Implemented");
        }
        else {
            BUG("MethodInstance Not implemented");
        }
    }
    else if(right->is<IR::Operation_Unary>() and not right->is<IR::Member>()){
        if(auto ca = right->to<IR::Cast>()){
            // std::cerr << ca << std::endl;
            // std::cerr << ca->expr->is<IR::Member>() << std::endl;
            i = new IR::DpdkCastStatement(left, ca->expr, ca->destType);
        }
        else if(auto n = right->to<IR::Neg>()){
            i = new IR::DpdkNegStatement(left, n->expr);
        }
        else if(auto c = right->to<IR::Cmpl>()){
            i = new IR::DpdkCmplStatement(left, c->expr);
        }
        else if(auto ln = right->to<IR::LNot>()){
            i = new IR::DpdkLNotStatement(left, ln->expr);
        }
        else{
            std::cerr << right->node_type_name() << std::endl;
            BUG("Not implemented.");
        }
    }
    else if(right->is<IR::PathExpression>() or
            right->is<IR::Member>() or
            right->is<IR::BoolLiteral>() or
            right->is<IR::Constant>()){
        i = new IR::DpdkMovStatement(a->left, a->right);
    }
    else{
        std::cerr<< right->node_type_name() << std::endl;
        BUG("Not implemented.");
    }
    add_instr(i);
    return false;
}
bool ConvertStatementToDpdk::preorder(const IR::BlockStatement* b){
    for(auto i:b->components){
        visit(i);
    }
    return false;
}

bool BranchingInstructionGeneration::generate(const IR::Expression *expr, cstring true_label, cstring false_label, bool is_and){
        if(auto land = expr->to<IR::LAnd>()){
            if(nested(land->left) and nested(land->right)){
                generate(land->left, true_label + "half", false_label, true);
                instructions.push_back(new IR::DpdkJmpStatement(false_label));
                instructions.push_back(new IR::DpdkLabelStatement(true_label + "half"));
                generate(land->right, true_label, false_label, true);
                instructions.push_back(new IR::DpdkJmpStatement(false_label));
                return false;
            }
            else if(not nested(land->left) and nested(land->right)){
                generate(land->left, true_label, false_label, true);
                generate(land->right, true_label, false_label, true);
                instructions.push_back(new IR::DpdkJmpStatement(false_label));
                return false;
            }
            else if(not nested(land->left) and not nested(land->right)){
                generate(land->left, true_label, false_label, true);
                generate(land->right, true_label, false_label, true);
                instructions.push_back(new IR::DpdkJmpStatement(true_label));
                return true;
            }
            else{
                BUG("Previous simple expression lifting pass failed");
            }
        }
        else if(auto lor = expr->to<IR::LOr>()){
            if(nested(lor->left) and nested(lor->right)){
                generate(lor->left, true_label, false_label + "half", false);
                instructions.push_back(new IR::DpdkJmpStatement(true_label));
                instructions.push_back(new IR::DpdkLabelStatement(false_label + "half"));
                generate(lor->right, true_label, false_label, false);
                instructions.push_back(new IR::DpdkJmpStatement(true_label));
                return true;
            }
            else if(not nested(lor->left) and nested(lor->right)){
                generate(lor->left, true_label, false_label, false);
                generate(lor->right, true_label, false_label, false);
                instructions.push_back(new IR::DpdkJmpStatement(true_label));
                return true;
            }
            else if(not nested(lor->left) and not nested(lor->right)){
                generate(lor->left, true_label, false_label, false);
                generate(lor->right, true_label, false_label, false);
                instructions.push_back(new IR::DpdkJmpStatement(false_label));
                return false;
            }
            else{
                BUG("Previous simple expression lifting pass failed");
            }
        }
        else if(auto equ = expr->to<IR::Equ>()){
            if(is_and) instructions.push_back(new IR::DpdkJmpNotEqualStatement(false_label, equ->left, equ->right));
            else instructions.push_back(new IR::DpdkJmpEqualStatement(true_label, equ->left, equ->right));
            return is_and;
        }
        else if(auto neq = expr->to<IR::Neq>()){
            if(is_and) instructions.push_back(new IR::DpdkJmpEqualStatement(false_label, neq->left, neq->right));
            else instructions.push_back(new IR::DpdkJmpNotEqualStatement(true_label, neq->left, neq->right));
            return is_and;
        }
        else if(auto lss = expr->to<IR::Lss>()){
            if(is_and) instructions.push_back(new IR::DpdkJmpGreaterEqualStatement(false_label, lss->left, lss->right));
            else instructions.push_back(new IR::DpdkJmpLessorStatement(true_label, lss->left, lss->right));
            return is_and;
        }
        else if(auto grt = expr->to<IR::Grt>()){
            if(is_and) instructions.push_back(new IR::DpdkJmpLessorEqualStatement(false_label, grt->left, grt->right));
            else instructions.push_back(new IR::DpdkJmpGreaterStatement(true_label, grt->left, grt->right));
            return is_and;
        }
        else if(auto mce = expr->to<IR::MethodCallExpression>()){
            auto mi = P4::MethodInstance::resolve(mce, refMap, typeMap);
            if(auto a = mi->to<P4::BuiltInMethod>()){
                if(a->name == "isValid"){
                    if(is_and) instructions.push_back(new IR::DpdkInvalidateStatement(false_label, a->appliedTo));
                    else instructions.push_back(new IR::DpdkValidateStatement(true_label, a->appliedTo));
                    return is_and;
                }
                else{
                    std::cerr << a->name << std::endl;
                    BUG("Not implemented");
                }
            }
            else {
                BUG("not implemented method instance");
            }
        }
        else if(expr->is<IR::PathExpression>() or expr->is<IR::Member>()){
            if(is_and) instructions.push_back(new IR::DpdkJmpNotEqualStatement(false_label, expr, new IR::Constant(1)));
            else instructions.push_back(new IR::DpdkJmpEqualStatement(true_label, expr, new IR::Constant(1)));
            return is_and;
        }
        else{
            std::cerr << expr->node_type_name() << std::endl;
            BUG("Not implemented");
        }
    }

bool ConvertStatementToDpdk::preorder(const IR::IfStatement* s){
    auto true_label  = Util::printf_format("label_%dtrue", next_label_id);
    auto false_label  = Util::printf_format("label_%dfalse", next_label_id);
    auto end_label = Util::printf_format("label_%dend", next_label_id++);
    auto gen = new BranchingInstructionGeneration(&next_label_id, refmap, typemap);
    bool res = gen->generate(s->condition, true_label, false_label, true);
    
    instructions.append(gen->instructions);
    if(res == true){
        add_instr(new IR::DpdkLabelStatement(true_label));
        visit(s->ifTrue);
        add_instr(new IR::DpdkJmpStatement(end_label));
        add_instr(new IR::DpdkLabelStatement(false_label));
        visit(s->ifFalse);
        add_instr(new IR::DpdkLabelStatement(end_label));
    }
    else{
        add_instr(new IR::DpdkLabelStatement(false_label));
        visit(s->ifFalse);
        add_instr(new IR::DpdkJmpStatement(end_label));
        add_instr(new IR::DpdkLabelStatement(true_label));
        visit(s->ifTrue);
        add_instr(new IR::DpdkLabelStatement(end_label));
    }
    return false;
}

bool ConvertStatementToDpdk::preorder(const IR::MethodCallStatement* s){
    auto mi = P4::MethodInstance::resolve(s->methodCall, refmap, typemap);
    if(auto a = mi->to<P4::ApplyMethod>()){
        if(a->isTableApply()){
            auto table = a->object->to<IR::P4Table>();
            add_instr(new IR::DpdkApplyStatement(table->name));
        }
        else BUG("not implemented for `apply` other than table");
    }
    else if(auto a = mi->to<P4::ExternMethod>()){
        // std::cerr << a->originalExternType->getName() << std::endl;
        // Checksum function call
        if(a->originalExternType->getName().name == "InternetChecksum"){
            if(a->method->getName().name == "add") {
                auto res = csum_map->find(a->object->to<IR::Declaration_Instance>());
                cstring intermediate;
                if(res != csum_map->end()){
                    intermediate = res->second;
                }
                else {
                    BUG("checksum map does not collect all checksum def.");
                }
                auto args = a->expr->arguments;
                const IR::Argument *arg = (*args)[0];
                if(auto l = arg->expression->to<IR::ListExpression>()){
                    for(auto field : l->components){
                        add_instr(new IR::DpdkChecksumAddStatement(a->object->getName(), intermediate, field));
                    }
                }
                else ::error("The argument of InternetCheckSum.add is not a list.");
            }
        }
        // Packet emit function call
        else if(a->originalExternType->getName().name == "packet_out"){
            if(a->method->getName().name == "emit"){
                auto args = a->expr->arguments;
                auto header = (*args)[0];
                if(auto m = header->expression->to<IR::Member>()){
                    add_instr(new IR::DpdkEmitStatement(m));
                }
                else if (auto path = header->expression->to<IR::PathExpression>()){
                    add_instr(new IR::DpdkEmitStatement(path));
                }
                else ::error("One emit does not like this packet.emit(header.xxx)");
            }
        }
        else if(a->originalExternType->getName().name == "packet_in"){
            if(a->method->getName().name == "extract"){
                auto args = a->expr->arguments;
                auto header = (*args)[0];
                if(auto m = header->expression->to<IR::Member>()){
                    add_instr(new IR::DpdkExtractStatement(m));
                }
                else if(auto path = header->expression->to<IR::PathExpression>()){
                    add_instr(new IR::DpdkExtractStatement(path));
                }
                else ::error("Extract format does not like this packet.extract(header.xxx)");
            }
        }
        else if(a->originalExternType->getName().name == "Meter"){
            if(a->method->getName().name == "execute"){
                auto args = a->expr->arguments;
                auto index = (*args)[0]->expression;
                auto color = (*args)[1]->expression;
                auto meter = a->object->getName();
                add_instr(new IR::DpdkMeterExecuteStatement(meter, index, color));
            }
            else BUG("Meter function not implemented.");
        }
        else if(a->originalExternType->getName().name == "Counter"){
            if(a->method->getName().name == "count"){
                auto args = a->expr->arguments;
                auto index = (*args)[0]->expression;
                auto counter = a->object->getName();
                add_instr(new IR::DpdkCounterCountStatement(counter, index));
            }
            else BUG("Counter function not implemented");
        }
        else if(a->originalExternType->getName().name == "Register"){
            if(a->method->getName().name == "read"){
                std::cerr << "ignore this register read, because the return value is optimized." << std::endl;
            }
            else if(a->method->getName().name == "write"){
                auto args = a->expr->arguments;
                auto index = (*args)[0]->expression;
                auto src = (*args)[1]->expression;
                auto reg = a->object->getName();
                add_instr(new IR::DpdkRegisterWriteStatement(reg, index, src));
            }
        }
        else{
            std::cerr << a->originalExternType->getName() << std::endl;
            BUG("Unknown extern function.");
        }
    }
    else if(auto a = mi->to<P4::ExternFunction>()){
        if(a->method->name == "verify"){
            auto args = a->expr->arguments;
            auto condition = (*args)[0];
            auto error = (*args)[1];
            add_instr(new IR::DpdkVerifyStatement(condition->expression, error->expression));
        }
    }
    else {
        BUG("function not implemented.");
    }
    return false;
}



} // namespace DPDK
