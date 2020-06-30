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
    bool ConvertToDpdkIRHelper::preorder(const IR::AssignmentStatement* a){
        auto left = a->left;
        auto right = a->right;
        IR::DpdkAsmStatement *i;
        if(auto r = right->to<IR::Operation_Binary>()){
            if(auto r = right->to<IR::Add>()){
                i = new IR::DpdkAddStatement(left, r->left, r->right);
            }
            else if(auto r = right->to<IR::Sub>()){
                i = new IR::DpdkSubStatement(left, r->left, r->right);
            }
            else if(auto r = right->to<IR::Shl>()){
                i = new IR::DpdkShlStatement(left, r->left, r->right);
            }
            else if(auto r = right->to<IR::Shr>()){
                i = new IR::DpdkShrStatement(left, r->left, r->right);
            }
            else {
                BUG("no implemented.");
            }
        }
        else i = new IR::DpdkMovStatement(a->left, a->right);
        add_instr(i);
        return false;
    }
    bool ConvertToDpdkIRHelper::preorder(const IR::BlockStatement* b){
        for(auto i:b->components){
            visit(i);
        }
        return false;
    }

    bool ConvertToDpdkIRHelper::preorder(const IR::IfStatement* s){
        auto true_label  = Util::printf_format("label_%d", next_label_id++);
        auto end_label = Util::printf_format("label_%d", next_label_id++);
        add_instr(new IR::DpdkJmpStatement(true_label, s->condition));
        visit(s->ifFalse);
        add_instr(new IR::DpdkJmpStatement(end_label));
        add_instr(new IR::DpdkLabelStatement(true_label));
        visit(s->ifTrue);
        add_instr(new IR::DpdkLabelStatement(end_label));
    }

    bool ConvertToDpdkIRHelper::preorder(const IR::MethodCallStatement* s){
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
            if(a->method->getName() == "emit"){
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
                    BUG("emit function has 0 or 2 more params");
                }
            }
            else{
                BUG("Unknown extern function.");
            }
        }
        return false;
    }

}
