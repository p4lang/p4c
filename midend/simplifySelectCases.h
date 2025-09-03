/*
Copyright 2016 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef MIDEND_SIMPLIFYSELECTCASES_H_
#define MIDEND_SIMPLIFYSELECTCASES_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
 * This pass analyzes reachability of select cases.
 * - If there is just one case label, the select statement is eliminated.
 * - If a case label appears after the default label, the case is
 *   unreachable and therefore eliminated.
 * - If all select case labels are compile-time constants, and if multiple different
 *   states match a given case label, then all but the first select case with this
 *   label are eliminated.
 * - If all select case labels are compile-time constants, then all non-default transitions
 *   to state s are eliminated if the select statement contains a default transition to state s.
 *
 * If requireConstants is true this pass requires that
 * all select labels evaluate to constants.
 *
 * @pre Assumes that there are no side-effects in the evaluation of select labels.
 * @post Unreachable case labels are removed. Case statement with
 *       a single label is replaced with a direct transition
 */
class DoSimplifySelectCases : public Transform, ResolutionContext {
    const TypeMap *typeMap;
    bool requireConstants;

    void checkSimpleConstant(const IR::Expression *expr) const;

 public:
    DoSimplifySelectCases(const TypeMap *typeMap, bool requireConstants)
        : typeMap(typeMap), requireConstants(requireConstants) {
        setName("DoSimplifySelectCases");
    }
    const IR::Node *preorder(IR::SelectExpression *expression) override;
};

class SimplifySelectCases : public PassManager {
 public:
    SimplifySelectCases(TypeMap *typeMap, bool requireConstants,
                        TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(nullptr, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoSimplifySelectCases(typeMap, requireConstants));
        setName("SimplifySelectCases");
    }
};

}  // namespace P4

#endif /* MIDEND_SIMPLIFYSELECTCASES_H_ */
