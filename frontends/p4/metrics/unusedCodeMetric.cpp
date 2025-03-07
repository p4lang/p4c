#include "unusedCodeMetric.h"

namespace P4 {

bool UnusedCodeMetricPass::preorder(const IR::P4Program *program) {
    std::cout<<"unused code pass"<<std::endl;
    unusedCodeInstances = 3;  // Mock value
    return false;
}

}  // namespace P4
