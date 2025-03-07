#include "headerModificationMetrics.h"

namespace P4 {

bool HeaderModificationMetricsPass::preorder(const IR::P4Program *program) {
    std::cout<<"Header modification pass"<<std::endl;
    headerModificationMetrics.totalModifications.numOperations = 4;
    headerModificationMetrics.totalModifications.numOperations = 150;
    return false;
}

}  // namespace P4
