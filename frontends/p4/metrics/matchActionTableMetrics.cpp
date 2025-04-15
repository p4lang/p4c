#include "matchActionTableMetrics.h"

namespace P4 {

unsigned MatchActionTableMetricsPass::keySize(const IR::KeyElement *keyElement){
    if (keyElement == nullptr)
        return 0;

    auto keyType = keyElement->expression->type;

    if (auto bitType = keyType->to<IR::Type_Bits>())
        return bitType->width_bits();
    else if (keyType->is<IR::Type_Boolean>())
        return 1;
    // Enum is dependent on the backend/target, default to 32
    else if (keyType->to<IR::Type_Enum>())
        return 32u;
    else if (auto intType = keyType->to<IR::Type_SerEnum>())
        return intType->type->width_bits();
    else
        return 0;
}

void MatchActionTableMetricsPass::postorder(const IR::P4Table *table) {
    std::string tableName = table->externalName().string();

    if (table->getKey() != nullptr) {
        for (auto keyElement : table->getKey()->keyElements) {
            metrics.matchActionTableMetrics.keysNum[tableName] += 1;
            metrics.matchActionTableMetrics.keySizeSum[tableName] += keySize(keyElement);
        }
    }

    if (table->getActionList() != nullptr) {
        metrics.matchActionTableMetrics.actionsNum[tableName] = table->getActionList()->size();
    }

    metrics.matchActionTableMetrics.numTables += 1;
    metrics.matchActionTableMetrics.totalKeys += metrics.matchActionTableMetrics.keysNum[tableName];
    metrics.matchActionTableMetrics.totalKeySizeSum += metrics.matchActionTableMetrics.keySizeSum[tableName];
    metrics.matchActionTableMetrics.totalActions += metrics.matchActionTableMetrics.actionsNum[tableName];
    metrics.matchActionTableMetrics.maxKeysPerTable = std::max(metrics.matchActionTableMetrics.maxKeysPerTable, metrics.matchActionTableMetrics.keysNum[tableName]);
    metrics.matchActionTableMetrics.maxActionsPerTable= std::max(metrics.matchActionTableMetrics.maxActionsPerTable, metrics.matchActionTableMetrics.actionsNum[tableName]);
}

void MatchActionTableMetricsPass::postorder(const IR::P4Program* /*program*/){
    if(metrics.matchActionTableMetrics.numTables > 0){
        metrics.matchActionTableMetrics.avgKeysPerTable = static_cast<double>(metrics.matchActionTableMetrics.totalKeys) / metrics.matchActionTableMetrics.numTables;
        metrics.matchActionTableMetrics.avgActionsPerTable = static_cast<double>(metrics.matchActionTableMetrics.totalActions) / metrics.matchActionTableMetrics.numTables;
    }
    if (metrics.matchActionTableMetrics.totalKeys > 0)
        metrics.matchActionTableMetrics.avgKeySize = static_cast<double>(metrics.matchActionTableMetrics.totalKeySizeSum) / metrics.matchActionTableMetrics.totalKeys;
}

}  // namespace P4
