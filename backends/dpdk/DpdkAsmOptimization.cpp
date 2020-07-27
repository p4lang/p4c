#include "DpdkAsmOptimization.h"

namespace DPDK{
// One assumption of this piece of program is that DPDK never produces jmp statements that
// that jump back to previous statements.
const IR::Node *RemoveRedundantLabel::postorder(IR::DpdkListStatement *l){
    IR::IndexedVector<IR::DpdkAsmStatement> used_labels;
    for(auto stmt: l->statements){
        if(auto jmp = stmt->to<IR::DpdkJmpStatement>()){
            bool found = false;
            for(auto label: used_labels){
                if(label->to<IR::DpdkJmpStatement>()->label == jmp->label){
                    found = true;
                    break;
                }
            }
            if(not found){
                used_labels.push_back(stmt);
            }
        }
    }
    auto new_l = new IR::IndexedVector<IR::DpdkAsmStatement>;
    for(auto stmt: l->statements){
        if(auto label = stmt->to<IR::DpdkLabelStatement>()){
            bool found = false;
            for(auto jmp_label: used_labels){
                if(jmp_label->to<IR::DpdkJmpStatement>()->label == label->label){
                    found = true;
                    break;
                }
            }
            if(found){
                new_l->push_back(stmt);
            }
        }
        else{
            new_l->push_back(stmt);
        }
    }
    l->statements = *new_l;
    return l;
}
} // namespace DPDK