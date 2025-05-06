#include "matchActionTableMetrics.h"

namespace P4 {

unsigned MatchActionTableMetricsPass::keySize(const IR::KeyElement *keyElement) {
    if (keyElement == nullptr) return 0;

    const IR::Type *currentType = keyElement->expression->type;
    while (currentType != nullptr) {
        if (auto bitType = currentType->to<IR::Type_Bits>())
            return bitType->width_bits();
        else if (currentType->is<IR::Type_Boolean>())
            return 1;
        else if (currentType->to<IR::Type_Error>())
            return 32u;  // Common default
        else if (currentType->to<IR::Type_Enum>())
            return 32u;  // Target-dependent, default to 32 bits
        else if (auto serEnum = currentType->to<IR::Type_SerEnum>())
            return serEnum->type->width_bits();

        // Unwrap the type if it's a type definition wrapper
        if (auto tt = currentType->to<IR::Type_Type>())
            currentType = tt->type;
        else if (auto nt = currentType->to<IR::Type_Newtype>())
            currentType = typeMap->getType(nt->type, true);
        else if (auto td = currentType->to<IR::Type_Typedef>())
            currentType = typeMap->getType(td->type, true);
        else
            break;
    }

    return 0;  // Unsupported type
}

void MatchActionTableMetricsPass::postorder(const IR::P4Table *table) {
    std::string tableName = table->externalName().string();

    if (table->getKey() != nullptr) {
        for (auto keyElement : table->getKey()->keyElements) {
            metrics.keysNum[tableName] += 1;
            metrics.keySizeSum[tableName] += keySize(keyElement);
        }
    }

    if (table->getActionList() != nullptr)
        metrics.actionsNum[tableName] = table->getActionList()->size();

    metrics.numTables += 1;
    metrics.totalKeys += metrics.keysNum[tableName];
    metrics.totalKeySizeSum += metrics.keySizeSum[tableName];
    metrics.totalActions += metrics.actionsNum[tableName];
    metrics.maxKeysPerTable = std::max(metrics.maxKeysPerTable, metrics.keysNum[tableName]);
    metrics.maxActionsPerTable =
        std::max(metrics.maxActionsPerTable, metrics.actionsNum[tableName]);
}

void MatchActionTableMetricsPass::postorder(const IR::P4Program * /*program*/) {
    if (metrics.numTables > 0) {
        metrics.avgKeysPerTable =
            static_cast<double>(metrics.totalKeys) / static_cast<double>(metrics.numTables);
        metrics.avgActionsPerTable =
            static_cast<double>(metrics.totalActions) / static_cast<double>(metrics.numTables);
    }
    if (metrics.totalKeys > 0)
        metrics.avgKeySize =
            static_cast<double>(metrics.totalKeySizeSum) / static_cast<double>(metrics.totalKeys);
}

}  // namespace P4
