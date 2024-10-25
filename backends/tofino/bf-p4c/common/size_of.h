/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BACKENDS_TOFINO_BF_P4C_COMMON_SIZE_OF_H_
#define BACKENDS_TOFINO_BF_P4C_COMMON_SIZE_OF_H_

#include "frontends/p4/strengthReduction.h"
#include "ir/ir.h"

namespace BFN {

using namespace P4;

class CloneConstants : public Transform {
 public:
    CloneConstants() = default;
    const IR::Node *postorder(IR::Constant *constant) override {
        // We clone the constant.  This is necessary because the same
        // the type associated with the constant may participate in
        // type unification, and thus we want to have different type
        // objects for different constant instances.
        const IR::Type *type = constant->type;
        if (type->is<IR::Type_Bits>()) {
            type = constant->type->clone();
        } else if (auto ii = type->to<IR::Type_InfInt>()) {
            // You can't just clone a InfInt value, because
            // you get the same declid.  We want a new declid.
            type = IR::Type_InfInt::get(ii->srcInfo);
        } else {
            BUG("unexpected type %2% for constant %2%", type, constant);
        }
        return new IR::Constant(constant->srcInfo, type, constant->value, constant->base);
    }
    static const IR::Expression *clone(const IR::Expression *expression) {
        return expression->apply(CloneConstants())->to<IR::Expression>();
    }
};

/**
 * This class resolves the value of sizeInBytes and sizeInBits externs into a
 * constant.
 */
class ConvertSizeOfToConstant : public Transform {
 public:
    ConvertSizeOfToConstant() {}
    const IR::Node *preorder(IR::MAU::TypedPrimitive *p) override;
    const IR::Node *preorder(IR::MethodCallExpression *mce) override;
};

/**
 * We cannot reuse the frontend constant folding pass because the frontend pass
 * uses typeMap and refMap which are not available in backend.
 */
class BackendConstantFolding : public Transform {
 public:
    const IR::Expression *getConstant(const IR::Expression *expr) const;
    const IR::Node *postorder(IR::Slice *e) override;
};

class BackendStrengthReduction : public Transform {
    const IR::Node *sub(IR::MAU::Instruction *inst);
    const IR::Node *preorder(IR::MAU::SaluInstruction *inst) override;
    const IR::Node *preorder(IR::MAU::Instruction *inst) override;
};

class ResolveSizeOfOperator : public PassManager {
 public:
    ResolveSizeOfOperator() {
        addPasses({new ConvertSizeOfToConstant(), new BackendConstantFolding(),
                   new BackendStrengthReduction()});
    }
};

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_COMMON_SIZE_OF_H_ */
