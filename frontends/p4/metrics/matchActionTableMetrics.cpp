#include "matchActionTableMetrics.h"

namespace P4 {

bool MatchActionTableMetricsPass::preorder(const IR::P4Program *program) {
    std::cout<<"Match-action table pass"<<std::endl;
    matchActionTableMetrics.numTables = 4;
    return false;
}

}  // namespace P4
