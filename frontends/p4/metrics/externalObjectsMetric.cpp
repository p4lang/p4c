#include "externalObjectsMetric.h"

namespace P4 {

bool ExternalObjectsMetricPass::preorder(const IR::P4Program *program) {
    std::cout<<"External objects pass"<<std::endl;
    externalObjectsNum = 4;
    return false;
}

}  // namespace P4
