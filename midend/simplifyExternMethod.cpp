/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "simplifyExternMethod.h"

#include "frontends/p4/methodInstance.h"
#include "ir/irutils.h"

namespace P4 {

const IR::Expression *SimplifyExternMethodCalls::preorder(IR::MethodCallExpression *mce) {
    if (getParent<IR::Statement>()) return mce;
    auto *mi = MethodInstance::resolve(mce, this, typeMap);
    if (mi && mi->is<P4::ExternCall>()) {
        // could do P4::FunctionCall as well, but I don't think bmv2 supports those at all
        auto tmp = nameGen.newName("tmp");
        newTemps.push_back(new IR::Declaration_Variable(IR::ID(tmp, nullptr), mce->type));
        auto tmpref = new IR::Path(IR::ID(tmp, nullptr));
        newCopies.push_back(
            new IR::AssignmentStatement(new IR::PathExpression(mce->type, tmpref), mce));
        return new IR::PathExpression(mce->type, tmpref->clone());
    }
    return mce;
}

const IR::Node *SimplifyExternMethodCalls::postorder(IR::Statement *stmt) {
    if (newCopies.empty()) return stmt;
    IR::IndexedVector<IR::StatOrDecl> rv(newCopies.begin(), newCopies.end());
    rv.push_back(stmt);
    newCopies.clear();
    return inlineBlock(*this, std::move(rv));
}

const IR::BlockStatement *SimplifyExternMethodCalls::postorder(IR::BlockStatement *block) {
    if (!newCopies.empty()) {
        block->components.insert(block->components.begin(), newCopies.begin(), newCopies.end());
        newCopies.clear();
    }
    if (newTemps.empty() || getParent<IR::Statement>() || getParent<IR::SwitchCase>()) return block;
    block->components.insert(block->components.begin(), newTemps.begin(), newTemps.end());
    newTemps.clear();
    return block;
}

}  // namespace P4
