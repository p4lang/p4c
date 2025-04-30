#include "headerMetrics.h"

namespace P4 {

void HeaderMetricsPass::postorder(const IR::Type_Header *header) {
    std::string headerName = header->getName().string();
    metrics.numHeaders++;
    size_t numFields = 0;
    size_t sizeSum = 0;

    for (auto field : header->fields) {
        numFields++;
        const IR::Type* currentType = typeMap->getType(field->type, true);

        // Unwrap the field type and get size.
        while (currentType != nullptr) {
            if (auto typeType = currentType->to<IR::Type_Type>()) {
                currentType = typeType->type;
            }
            else if (auto bitsType = currentType->to<IR::Type_Bits>()) {
                sizeSum += bitsType->width_bits();
                break;
            } else if (auto newtype = currentType->to<IR::Type_Newtype>()) {
                currentType = typeMap->getType(newtype->type, true);
            } else if (auto typedefType = currentType->to<IR::Type_Typedef>()) {
                currentType = typeMap->getType(typedefType->type, true);
            } else {
                break;
            }
        }
    }

    metrics.fieldsNum[headerName] = numFields;
    metrics.fieldSizeSum[headerName] = sizeSum;
    totalFieldsNum += numFields;
    totalFieldsSize += sizeSum;
}

void HeaderMetricsPass::postorder(const IR::P4Program* /*program*/){
    if (metrics.numHeaders > 0)
        metrics.avgFieldsNum = static_cast<double>(totalFieldsNum) / static_cast<double>(metrics.numHeaders);
    if (totalFieldsNum > 0)
        metrics.avgFieldSize = static_cast<double>(totalFieldsSize) / static_cast<double>(totalFieldsNum);
}

}  // namespace P4
