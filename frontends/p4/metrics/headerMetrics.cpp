#include "headerMetrics.h"

namespace P4 {

bool HeaderMetricsPass::preorder(const IR::Type_Header *header){
    std::string headerName = header->getName().string();
    metrics.headerMetrics.numHeaders++;

    int numFields = 0;
    int sizeSum = 0;

    for (auto field : header->fields) {
        numFields++;
        sizeSum += field->type->width_bits();
    }

    metrics.headerMetrics.fieldsNum[headerName] = numFields;
    metrics.headerMetrics.fieldSizeSum[headerName] = sizeSum;

    totalFieldsNum += numFields;
    totalFieldsSize += sizeSum;
    return true;
}

void HeaderMetricsPass::postorder(const IR::P4Program* /*program*/){
    if (metrics.headerMetrics.numHeaders > 0)
        metrics.headerMetrics.avgFieldsNum = static_cast<double>(totalFieldsNum) / metrics.headerMetrics.numHeaders;
    if (totalFieldsNum > 0)
        metrics.headerMetrics.avgFieldSize = static_cast<double>(totalFieldsSize) / totalFieldsNum;
}

}  // namespace P4
