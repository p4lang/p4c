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
            std::cout << i->node_type_name() << std::endl;
            std::cout << i << std::endl;
            visit(i);
        }
        return false;
    }

    bool ConvertToDpdkIRHelper::preorder(const IR::IfStatement* s){
        // auto cond = new IR::DpdkMovStatement(new IR::PathExpression(IR::ID(name)), s->condition);
        // add_instr(cond);
        // auto true_label  = Util::printf_format("label_%d", next_label_id++);
        // auto false_label = Util::printf_format("label_%d", next_label_id++);
        // auto end_label = Util::printf_format("label_%d", next_label_id++);
        // add_inst(new IR::DpdkJmpStatement(true_label));
        // add_inst(new IR::DpdkJmpStatement(false_label));
        // add_inst(new IR::DpdkLabelStatement(true_label));
        // visit(stmt->ifTrue);
        // add_inst(new IR::DpdkLabelStatement(false_label));
        // visit(stmt->ifFalse);
    }

    bool ConvertToDpdkIRHelper::preorder(const IR::MethodCallStatement* s){

    }
    
}
