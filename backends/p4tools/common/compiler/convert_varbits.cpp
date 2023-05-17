#include "backends/p4tools/common/compiler/convert_varbits.h"

#include "lib/log.h"

namespace P4Tools {

const IR::Node *ConvertVarbits::postorder(IR::Type_Varbits *varbit) {
    // At this point, we do not know the future size of the varbit. We assume 0 for now.
    auto *varbitType = new IR::Extracted_Varbits(varbit->srcInfo, varbit->size, false, 0);
    return varbitType;
}

const IR::Node *ConvertVarbits::postorder(IR::Expression *expr) {
    auto varBits = ConvertVarbits();
    expr->type = expr->type->apply(varBits)->checkedTo<IR::Type>();
    return expr;
}

ConvertVarbits::ConvertVarbits() {
    setName("ConvertVarbits");
    visitDagOnce = false;
}
}  // namespace P4Tools
