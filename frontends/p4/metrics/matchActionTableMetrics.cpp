#include "matchActionTableMetrics.h"

namespace P4 {

MatchActionTableMetrics &matM = matchActionTableMetrics;

unsigned MatchActionTableMetricsPass::keySize(const IR::KeyElement *keyElement){
    if (keyElement == nullptr)
        return 0;

    auto keyType = keyElement->expression->type;

    if (auto bitType = keyType->to<IR::Type_Bits>())
        return bitType->width_bits();
    else if (keyType->is<IR::Type_Boolean>())
        return 1;
    // Enum is dependent on the backend/target, default to 32
    else if (auto enumType = keyType->to<IR::Type_Enum>())
        return 32u;
    else if (auto intType = keyType->to<IR::Type_SerEnum>())
        return intType->type->width_bits();
    else
        return 0;
}

bool MatchActionTableMetricsPass::preorder(const IR::P4Table *table) {
    std::string tableName = table->externalName().string();

    if (table->getKey() != nullptr) {
        for (auto keyElement : table->getKey()->keyElements) {
            matM.keysNum[tableName] += 1;
            matM.keySizeSum[tableName] += keySize(keyElement);
        }
    }

    if (table->getActionList() != nullptr) {
        matM.actionsNum[tableName] = table->getActionList()->size();
    }

    matM.numTables += 1;
    matM.totalKeys += matM.keysNum[tableName];
    matM.totalKeySizeSum += matM.keySizeSum[tableName];
    matM.totalActions += matM.actionsNum[tableName];
    matM.maxKeysPerTable = std::max(matM.maxKeysPerTable, matM.keysNum[tableName]);
    matM.maxActionsPerTable= std::max(matM.maxActionsPerTable, matM.actionsNum[tableName]);
    return true;
}

void MatchActionTableMetricsPass::postorder([[maybe_unused]] const IR::P4Program *program){
    if(matM.numTables > 0){
        matM.avgKeysPerTable = static_cast<double>(matM.totalKeys) / matM.numTables;
        matM.avgActionsPerTable = static_cast<double>(matM.totalActions) / matM.numTables;
    }
    if (matM.totalKeys > 0)
        matM.avgKeySize = static_cast<double>(matM.totalKeySizeSum) / matM.totalKeys;
}

}  // namespace P4
