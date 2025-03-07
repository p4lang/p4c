#include "parserMetrics.h"

namespace P4 {

bool ParserMetricsPass::preorder(const IR::P4Program *program) {
    std::cout<<"parser metrics pass"<<std::endl;
    parserMetrics.totalStates = 10;
    return false;
}

}  // namespace P4
