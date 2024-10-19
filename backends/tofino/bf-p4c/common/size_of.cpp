/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "bf-p4c/common/size_of.h"

namespace BFN {

const IR::Node *ConvertSizeOfToConstant::preorder(IR::MAU::TypedPrimitive *p) {
    if (p->name != "sizeInBytes" && p->name != "sizeInBits") return p;
    ERROR_CHECK(p->operands.size() != 0, "sizeInBytes must have at least one argument");
    auto widthBits = p->operands.at(0)->type->width_bits();
    if (p->name == "sizeInBytes") widthBits = widthBits / 8;
    return new IR::Constant(p->type, widthBits);
}

const IR::Node *ConvertSizeOfToConstant::preorder(IR::MethodCallExpression *mce) {
    if (!mce->method) return mce;
    if (!mce->method->type) return mce;
    if (!mce->method->to<IR::PathExpression>()) return mce;
    auto path = mce->method->to<IR::PathExpression>()->path;
    if (path->name.name != "sizeInBytes" && path->name.name != "sizeInBits") return mce;

    auto width = mce->arguments->at(0)->expression->type->width_bits();
    if (path->name.name == "sizeInBytes") width = width / 8;
    LOG3(" Converted Method Call Expression  " << mce << " to constant width " << width);
    return new IR::Constant(mce->type, width);
}

const IR::Expression *BackendConstantFolding::getConstant(const IR::Expression *expr) const {
    CHECK_NULL(expr);
    if (expr->is<IR::Constant>()) return expr;
    if (expr->is<IR::BoolLiteral>()) return expr;
    if (auto list = expr->to<IR::ListExpression>()) {
        for (auto e : list->components)
            if (getConstant(e) == nullptr) return nullptr;
        return expr;
    } else if (auto si = expr->to<IR::StructExpression>()) {
        for (auto e : si->components)
            if (getConstant(e->expression) == nullptr) return nullptr;
        return expr;
    } else if (auto cast = expr->to<IR::Cast>()) {
        // Casts of a constant to a value with type Type_Newtype
        // are constants, but we cannot fold them.
        if (getConstant(cast->expr)) return CloneConstants::clone(expr);
        return nullptr;
    }
    return nullptr;
}

const IR::Node *BackendConstantFolding::postorder(IR::Slice *e) {
    const IR::Expression *msb = getConstant(e->e1);
    const IR::Expression *lsb = getConstant(e->e2);
    if (msb == nullptr || lsb == nullptr) {
        error("%1%: bit indexes must be compile-time constants", e);
        return e;
    }

    auto e0 = getConstant(e->e0);
    if (e0 == nullptr) return e;

    auto cmsb = msb->to<IR::Constant>();
    if (cmsb == nullptr) {
        error(ErrorType::ERR_EXPECTED, "%1%: an integer value", msb);
        return e;
    }
    auto clsb = lsb->to<IR::Constant>();
    if (clsb == nullptr) {
        error(ErrorType::ERR_EXPECTED, "%1%: an integer value", lsb);
        return e;
    }
    auto cbase = e0->to<IR::Constant>();
    if (cbase == nullptr) {
        error(ErrorType::ERR_EXPECTED, "%1%: an integer value", e->e0);
        return e;
    }

    int m = cmsb->asInt();
    int l = clsb->asInt();
    if (m < l) {
        error("%1%: bit slicing should be specified as [msb:lsb]", e);
        return e;
    }

    big_int value = cbase->value >> l;
    big_int mask = 1;
    mask = (mask << (m - l + 1)) - 1;
    value = value & mask;
    return new IR::Constant(e->srcInfo, e->type, value, cbase->base, true);
}

const IR::Node *BackendStrengthReduction::sub(IR::MAU::Instruction *inst) {
    // Replace `a - constant` with `a + (-constant)`
    if (inst->operands.at(2)->is<IR::Constant>()) {
        auto cst = inst->operands.at(2)->to<IR::Constant>();
        auto neg = new IR::Constant(cst->srcInfo, cst->type, -cst->value, cst->base, true);
        auto result = new IR::MAU::Instruction(inst->srcInfo, "add"_cs,
                                               {inst->operands.at(0), inst->operands.at(1), neg});
        return result;
    }
    return inst;
}

const IR::Node *BackendStrengthReduction::preorder(IR::MAU::SaluInstruction *inst) {
    // Don't go through these.
    LOG3("leaving alone SALU inst " << inst);
    prune();
    return inst;
}

const IR::Node *BackendStrengthReduction::preorder(IR::MAU::Instruction *inst) {
    LOG3("replacing inst " << inst);
    if (inst->name == "sub") return sub(inst);
    return inst;
}

}  // namespace BFN
