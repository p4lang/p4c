/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _P4_STRENGTHREDUCTION_H_
#define _P4_STRENGTHREDUCTION_H_

#include "ir/ir.h"
#include "../common/resolveReferences/referenceMap.h"
#include "lib/exceptions.h"
#include "lib/cstring.h"
#include "frontends/p4/substitution.h"
#include "frontends/p4/substitutionVisitor.h"

namespace P4 {

/** Implements a pass that replaces expensive arithmetic and boolean
  * operations with cheaper ones -- i.e., strength reduction
  * 
  * Specifically, it provides:
  * 
  * 1. A collection of helper methods that determine whether a given
  * expression is `0`, `1`, `true`, or `false`, or a power of `2`
  * 
  * 2. A visitor that transforms arithmetic and boolean exprssions
  * 
  * @pre: None
  * 
  * @post: 
  *   - all arithmetic and boolean expressions are simplified
  *   - check for div / mod by `0`
  *       
  * 
  */ 
// TODO:
//   - overlap with constant folding?
//   - store constant values in IR?
class StrengthReduction final : public Transform {
    bool isOne(const IR::Expression* expr) const;
    bool isZero(const IR::Expression* expr) const;
    bool isFalse(const IR::Expression* expr) const;
    bool isTrue(const IR::Expression* expr) const;
    int isPowerOf2(const IR::Expression* expr) const;

 public:
    StrengthReduction() { visitDagOnce = true; setName("StrengthReduction"); }

    using Transform::postorder;

    const IR::Node* postorder(IR::BAnd* expr) override;
    const IR::Node* postorder(IR::BOr* expr) override;
    const IR::Node* postorder(IR::BXor* expr) override;
    const IR::Node* postorder(IR::LAnd* expr) override;
    const IR::Node* postorder(IR::LOr* expr) override;
    const IR::Node* postorder(IR::Sub* expr) override;
    const IR::Node* postorder(IR::Add* expr) override;
    const IR::Node* postorder(IR::Shl* expr) override;
    const IR::Node* postorder(IR::Shr* expr) override;
    const IR::Node* postorder(IR::Mul* expr) override;
    const IR::Node* postorder(IR::Div* expr) override;
    const IR::Node* postorder(IR::Mod* expr) override;
};

}  // namespace P4

#endif /* _P4_STRENGTHREDUCTION_H_ */
