#include "inlinedActionsMetric.h"

namespace P4 {

bool InlinedActionsMetricPass::preorder(const IR::P4Program *program) {
    std::cout<<"Inlined actions pass"<<std::endl;
    inlinedActionsNum = 4;
    return false;
}

}  // namespace P4
