#include "DpdkAsmOptimization.h"

namespace DPDK{
const IR::Node *RemoveRedundantEmit::preorder(IR::DpdkListStatement *l){
    std::cout << "here" << std::endl;
    for(auto s: l->statements){
        std::cout << s->node_type_name() << std::endl;
    }
    return l;
}
} // namespace DPDK