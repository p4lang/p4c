#include "headerMetrics.h"

namespace P4 {

bool HeaderMetricsPass::preorder(const IR::P4Program *program) {
    std::cout<<"Header metrics pass"<<std::endl;
    headerMetrics.numHeaders = 5; 
    return false;
}

}  // namespace P4
