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
            auto left_unroller = new TreeUnroller(next_tmp_id, refmap, typemap);
            auto right_unroller = new TreeUnroller(next_tmp_id, refmap, typemap);
            r->left->apply(*left_unroller);
            for(auto i: left_unroller->get_instr()){
                add_instr(i);
            }
            r->right->apply(*right_unroller);
            for(auto i: right_unroller->get_instr()){
                add_instr(i);
            }
            const IR::Expression* left_tmp = left_unroller->get_tmp();
            const IR::Expression* right_tmp = right_unroller->get_tmp();
            if(not left_tmp) left_tmp = r->left;
            if(not right_tmp) right_tmp = r->right;

            if(right->is<IR::Add>()){
                i = new IR::DpdkAddStatement(left, left_tmp, right_tmp);
            }
            else if(right->is<IR::Sub>()){
                i = new IR::DpdkSubStatement(left, left_tmp, right_tmp);
            }
            else if(right->is<IR::Shl>()){
                i = new IR::DpdkShlStatement(left, left_tmp, right_tmp);
            }
            else if(right->is<IR::Shr>()){
                i = new IR::DpdkShrStatement(left, left_tmp, right_tmp);
            }
            else if(right->is<IR::Equ>()){
                i = new IR::DpdkEquStatement(left, left_tmp, right_tmp);
            }
            else {
                BUG("not implemented.");
            }
        }
        // handle method call
        else if(auto m = right->to<IR::MethodCallExpression>()){
            auto mi = P4::MethodInstance::resolve(m, refmap, typemap);
            // std::cout << m << std::endl;
            if(auto e = mi->to<P4::ExternMethod>()){
                if(e->originalExternType->getName().name == "Hash"){
                    if(e->method->getName().name == "get_hash"){
                        if(e->expr->arguments->size() == 1){
                            auto field = (*e->expr->arguments)[0];
                            i = new IR::DpdkGetHashStatement(e->object->getName(), field->expression, left);
                        }
                        else{
                            BUG("get_hash function has 0 or 2 more args.");
                        }
                    }
                }
                else if(e->originalExternType->getName().name == "InternetChecksum"){
                    if(e->method->getName().name == "get"){
                        if(e->expr->arguments->size() == 0){
                            i = new IR::DpdkGetChecksumStatement(left, e->object->getName());
                        }
                        else{
                            BUG("checksum.get() has args");
                        }
                    }
                }
                else{
                    BUG("ExternMethod Not implemented");
                } 
            }
            else if(auto b = mi->to<P4::BuiltInMethod>()){
                if(b->name.name == "isValid"){
                    if(auto member = b->appliedTo->to<IR::Member>()){
                        i = new IR::DpdkValidateStatement(left, member->member);
                        // std::cout << member->member << std::endl;
                    }
                    else{
                        BUG("Does match header.xxx.isValid() format");
                    }
                }
                else{
                    BUG("BuiltInMethod Not Implemented");
                }
            }
            else {
                BUG("MethodInstance Not implemented");
            }
        }
        else if(right->is<IR::Operation_Unary>() and not right->is<IR::Member>()){
            if(auto ca = right->to<IR::Cast>()){
                // std::cout << ca << std::endl;
                // std::cout << ca->expr->is<IR::Member>() << std::endl;
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
                // std::cout << right->node_type_name() << std::endl;
            }
        }
        else{
            // std::cout << right->node_type_name() << std::endl;
            i = new IR::DpdkMovStatement(a->left, a->right);
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

    bool ConvertStatementToDpdk::preorder(const IR::IfStatement* s){
        auto true_label  = Util::printf_format("label_%d", next_label_id++);
        auto end_label = Util::printf_format("label_%d", next_label_id++);

        auto converter = new TreeUnroller(next_tmp_id, refmap, typemap);
        s->condition->apply(*converter);
        for(auto i:converter->get_instr()){
            add_instr(i);
        }
        const IR::Expression * tmp = converter->get_tmp();
        if(not tmp) tmp = s->condition;

        add_instr(new IR::DpdkCmpStatement(tmp, new IR::Constant(0)));
        add_instr(new IR::DpdkJmpNotEqualStatement(true_label));
        visit(s->ifFalse);
        add_instr(new IR::DpdkJmpStatement(end_label));
        add_instr(new IR::DpdkLabelStatement(true_label));
        visit(s->ifTrue);
        add_instr(new IR::DpdkLabelStatement(end_label));
        return false;
    }

    bool ConvertStatementToDpdk::preorder(const IR::MethodCallStatement* s){
        auto mi = P4::MethodInstance::resolve(s->methodCall, refmap, typemap);
        if(auto a = mi->to<P4::ApplyMethod>()){
            if(a->isTableApply()){
                auto table = a->object->to<IR::P4Table>();
                add_instr(new IR::DpdkApplyStatement(table->name));
            }
            else{
                BUG("not implemented for `apply` other than table");
            }
        }
        else if(auto a = mi->to<P4::ExternMethod>()){
            // std::cout << a->originalExternType->getName() << std::endl;
            // Checksum function call
            if(a->originalExternType->getName().name == "InternetChecksum"){
                if(a->method->getName().name == "add") {
                    auto args = a->expr->arguments;
                    if(args->size() == 1){
                        const IR::Argument *arg = (*args)[0];
                        if(auto l = arg->expression->to<IR::ListExpression>()){
                            for(auto field : l->components){
                                add_instr(new IR::DpdkChecksumAddStatement(a->object->getName(), field));
                            }
                        }
                        else{
                            BUG("The argument of InternetCheckSum.add is not a list.");
                        }
                    }
                    else{
                        BUG("InternetChecksum.add has 0 or 2 more args");
                    }
                }
            }
            // Packet emit function call
            else if(a->originalExternType->getName().name == "packet_out"){
                if(a->method->getName().name == "emit"){
                    auto args = a->expr->arguments;
                    if(args->size() == 1){
                        auto header = (*args)[0];
                        if(auto m = header->expression->to<IR::Member>()){
                            add_instr(new IR::DpdkEmitStatement(m->member));
                        }
                        else{
                            BUG("One emit does not like this packet.emit(header.xxx)");
                        }
                    }
                    else{
                        BUG("emit function has 0 or 2 more args");
                    }
                }
            }
            else if(a->originalExternType->getName().name == "packet_in"){
                if(a->method->getName().name == "extract"){
                    auto args = a->expr->arguments;
                    if(args->size() == 1){
                        auto header = (*args)[0];
                        if(auto m = header->expression->to<IR::Member>()){
                            add_instr(new IR::DpdkExtractStatement(m));
                        }
                        else{
                            BUG("Extract does not like this packet.extract(header.xxx)");
                        }
                    }
                    else{
                        BUG("extraction has 0 or 2 more args");
                    }
                }
            }

            else{    
                BUG("Unknown extern function.");
            }
        }
        // else if(auto a = mi->to<P4::ExternFunction>()){
        //     if(a->method->name == "verify"){
        //         auto args = a->expr->arguments;
        //         if(args->size() == 2){
        //             auto condition = (*args)[0];
        //             auto error = (*args)[1];
        //             add_instr(new IR::DpdkVerifyStatement(condition->expression, error->expression));
        //         }
        //         else{
        //             BUG("verify has 0 or 2 more args");
        //         }
        //     }
        // }

        // else {
        //     BUG("function not implemented.");
        // }
        return false;
    }


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

    cstring toStr(const IR::Expression* const exp){
        if(auto e = exp->to<IR::Constant>()) return toStr(e);
        else if(auto e = exp->to<IR::BoolLiteral>()) return toStr(e);
        else if(auto e = exp->to<IR::Member>()) return toStr(e);
        else if(auto e = exp->to<IR::PathExpression>()) return toStr(e);
        else{
            std::cout << exp << std::endl;
            std::cout << exp->node_type_name() << std::endl;
            BUG("not implemented");
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
        else{
            BUG("not implemented type");
        }
    }


    bool TreeUnroller::preorder(const IR::Operation_Binary *bin){
        visit(bin->left);
        const IR::Expression* left_tmp = tmp;
        visit(bin->right);
        const IR::Expression* right_tmp = tmp;
        std::ostringstream out;
        out << "backend_tmp" << (*next_tmp_id)++;
        tmp = new IR::PathExpression(IR::ID(cstring(out.str())));
        if(not left_tmp) left_tmp = bin->left;
        if(not right_tmp) right_tmp = bin->right;
        IR::DpdkAsmStatement *i;
        if(bin->is<IR::Add>()) i = new IR::DpdkAddStatement(tmp, left_tmp, right_tmp);
        else if(bin->is<IR::Sub>()) i = new IR::DpdkSubStatement(tmp, left_tmp, right_tmp);
        else if(bin->is<IR::Shl>()) i = new IR::DpdkShlStatement(tmp, left_tmp, right_tmp);
        else if(bin->is<IR::Shr>()) i = new IR::DpdkShrStatement(tmp, left_tmp, right_tmp);
        else if(bin->is<IR::Equ>()) i = new IR::DpdkEquStatement(tmp, left_tmp, right_tmp);
        else if(bin->is<IR::LAnd>()) i = new IR::DpdkLAndStatement(tmp, left_tmp, right_tmp);
        else if(bin->is<IR::Leq>()) i = new IR::DpdkLeqStatement(tmp, left_tmp, right_tmp);
        else if(bin->is<IR::Lss>()) i = new IR::DpdkLssStatement(tmp, left_tmp, right_tmp);
        else if(bin->is<IR::Geq>()) i = new IR::DpdkGeqStatement(tmp, left_tmp, right_tmp);
        else if(bin->is<IR::Grt>()) i = new IR::DpdkGrtStatement(tmp, left_tmp, right_tmp);
        else if(bin->is<IR::Neq>()) i = new IR::DpdkNeqStatement(tmp, left_tmp, right_tmp);
        else{
            std::cout << bin->node_type_name() << std::endl;
            BUG("not implemented");
        }
        add_instr(i);
        return false;
    }

    bool TreeUnroller::preorder(const IR::MethodCallExpression *method){
        std::ostringstream out;
        out << "backend_tmp" << (*next_tmp_id)++;
        tmp = new IR::PathExpression(IR::ID(cstring(out.str())));
        auto assignment = new IR::AssignmentStatement(tmp, method);
        auto convertor = new ConvertStatementToDpdk(refmap, typemap, 0, next_tmp_id);
        assignment->apply(*convertor);
        for(auto i : convertor->get_instr()){
            add_instr(i);
        }
        return false;
    }

    bool TreeUnroller::preorder(const IR::Member *member){
        tmp = nullptr;
        return false;
    }

    bool TreeUnroller::preorder(const IR::PathExpression *path){
        tmp = nullptr;
        return false;
    }

    bool TreeUnroller::preorder(const IR::Constant *path){
        tmp = nullptr;
        return false;
    }


    


} // namespace DPDK