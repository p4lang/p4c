#include "externalObjectsMetric.h"

namespace P4 {

bool ExternalObjectsMetricPass::preorder(const IR::P4Program* /*program*/) {
    metrics.externalObjectsNum = 4;
    return false;
}

}  // namespace P4
