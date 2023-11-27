#include "backends/p4tools/common/compiler/convert_struct_expr.h"

#include "ir/irutils.h"

namespace P4Tools {

const IR::Node *ConvertStructExpr::postorder(IR::StructExpression *structExpr) {
    auto structType = structExpr->type;
    bool resolved = false;
    if (structType->is<IR::Type_Name>()) {
        structType = typeMap->getTypeType(structType, true);
        resolved = true;
    }
    if (resolved) {
        return new IR::StructExpression(structExpr->srcInfo, structType, structExpr->type,
                                        structExpr->components);
    }
    return structExpr;
}

ConvertStructExpr::ConvertStructExpr(const P4::TypeMap *typeMap) : typeMap(typeMap) {
    setName("ConvertStructExpr");
    visitDagOnce = false;
}
}  // namespace P4Tools
