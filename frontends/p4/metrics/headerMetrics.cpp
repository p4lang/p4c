#include "headerMetrics.h"

namespace P4 {

bool HeaderMetricsPass::preorder(const IR::Type_Header *header){
    std::string headerName = header->getName().string();
    headerMetrics.numHeaders++;

    int numFields = 0;
    int sizeSum = 0;

    for (auto field : header->fields) {
        numFields++;
        sizeSum += field->type->width_bits();
    }

    headerMetrics.fieldsNum[headerName] = numFields;
    headerMetrics.fieldSizeSum[headerName] = sizeSum;

    totalFieldsNum += numFields;
    totalFieldsSize += sizeSum;
    return true;
}

void HeaderMetricsPass::postorder([[maybe_unused]] const IR::P4Program *program){
    if (headerMetrics.numHeaders > 0)
        headerMetrics.avgFieldsNum = static_cast<double>(totalFieldsNum) / headerMetrics.numHeaders;
    if (totalFieldsNum > 0)
        headerMetrics.avgFieldSize = static_cast<double>(totalFieldsSize) / totalFieldsNum;
}

}  // namespace P4
