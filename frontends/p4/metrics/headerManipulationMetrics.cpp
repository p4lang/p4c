#include "headerManipulationMetrics.h"

namespace P4 {

bool HeaderManipulationMetricsPass::preorder(const IR::P4Program *program) {
    std::cout<<"Header manipulation pass"<<std::endl;
    headerManipulationMetrics.totalManipulations.numOperations = 7;
    headerManipulationMetrics.totalManipulations.totalSize = 200;
    return false;
}

}  // namespace P4
