#include "backends/p4tools/common/compiler/convert_hs_index.h"

#include <string>

#include "ir/id.h"
#include "ir/irutils.h"
#include "lib/cstring.h"

namespace P4Tools {

const IR::Node *HSIndexToMember::postorder(IR::ArrayIndex *curArrayIndex) {
    if (const auto *arrayConst = curArrayIndex->right->to<IR::Constant>()) {
        return produceStackIndex(curArrayIndex->type, curArrayIndex->left, arrayConst->asInt());
    }
    return curArrayIndex;
}

const IR::ArrayIndex *HSIndexToMember::produceStackIndex(const IR::Type *type,
                                                         const IR::Expression *expression,
                                                         size_t arrayIndex) {
    return new IR::ArrayIndex(type, expression, IR::getConstant(IR::getBitType(32), arrayIndex));
}

}  // namespace P4Tools
