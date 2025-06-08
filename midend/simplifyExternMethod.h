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

#ifndef MIDEND_SIMPLIFYEXTERNMETHOD_H_
#define MIDEND_SIMPLIFYEXTERNMETHOD_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {

/** SimplifyExternMethodCalls
 *
 * the BMV2 backend cannot handle extern method calls that are part of more
 * complex expressions -- every method call must either be the immediate RHS
 * of an AssignmentStatement or a stand-alone MethodCallStatment.
 *
 * This pass rewrites MethodCallExpressions that are not either of those things
 * into an assignment to a temp var just before the containing Statement and a
 * reference to that temp var in the more complex expression
 *
 * eg, if you have something like:
 *    header.field = header.field + reg.read(x);
 * that will be transformed into
 *    tmp_4 = reg.read(x);
 *    header.field + header.field + tmp_4;
 *
 * In practice this only matters for @noSideEffects methods, as all calls with side
 * effects will have already been moved by DoSimplifyExpressions
 */

class SimplifyExternMethodCalls : public Transform, public ResolutionContext {
    // FIXME -- would prefer not to require the typemap, but the type in
    // MethodCallExpressions has generally not been updated properly
    TypeMap *typeMap;
    MinimalNameGenerator nameGen;
    std::vector<const IR::Declaration_Variable *> newTemps;
    std::vector<const IR::AssignmentStatement *> newCopies;

    Visitor::profile_t init_apply(const IR::Node *root) override {
        Visitor::profile_t rv = Transform::init_apply(root);
        root->apply(nameGen);
        return rv;
    }
    void end_apply() override {
        BUG_CHECK(newTemps.empty() && newCopies.empty(),
                  "Not all newly created statements found a home");
    }

    const IR::Expression *preorder(IR::MethodCallExpression *mce) override;
    const IR::Node *postorder(IR::Statement *stmt) override;
    const IR::BlockStatement *postorder(IR::BlockStatement *block) override;

    // Don't touch parsers -- they have their own rules
    const IR::P4Parser *preorder(IR::P4Parser *p) override {
        prune();
        return p;
    }

 public:
    explicit SimplifyExternMethodCalls(TypeMap *tm) : typeMap(tm) {}
};

}  // namespace P4

#endif /* MIDEND_SIMPLIFYEXTERNMETHOD_H_ */
