#include "halsteadMetrics.h"

namespace P4 {

bool HalsteadMetricsPass::preorder(const IR::P4Program *program) {
    std::cout<<"Halstead pass"<<std::endl;
    halsteadMetrics.uniqueOperators = 10;
    halsteadMetrics.uniqueOperands = 20;
    return false;
}

}  // namespace P4
