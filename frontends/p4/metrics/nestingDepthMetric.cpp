#include "nestingDepthMetric.h"

namespace P4 {

bool NestingDepthMetricPass::preorder(const IR::P4Program *program) {
    std::cout<<"nesting depth pass"<<std::endl;
    nestingDepth.avgNestingDepth = 2.5;
    nestingDepth.maxNestingDepth = 4;
    return false;
}

}  // namespace P4
