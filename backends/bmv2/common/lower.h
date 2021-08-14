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

#ifndef BACKENDS_BMV2_COMMON_LOWER_H_
#define BACKENDS_BMV2_COMMON_LOWER_H_

#include "ir/ir.h"
#include "frontends/p4/typeMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace BMV2 {

/**
  This pass rewrites expressions which are not supported natively on BMv2.
*/
class LowerExpressions : public Transform {
    P4::TypeMap* typeMap;
    // Cannot shift with a value larger than 8 bits
    const int maxShiftWidth = 8;

    const IR::Expression* shift(const IR::Operation_Binary* expression) const;
 public:
    explicit LowerExpressions(P4::TypeMap* typeMap) : typeMap(typeMap)
    { CHECK_NULL(typeMap); setName("LowerExpressions"); }

    const IR::Node* postorder(IR::Expression* expression) override;
    const IR::Node* postorder(IR::Shl* expression) override
    { return shift(expression); }
    const IR::Node* postorder(IR::Shr* expression) override
    { return shift(expression); }
    const IR::Node* postorder(IR::Cast* expression) override;
    const IR::Node* postorder(IR::Neg* expression) override;
    const IR::Node* postorder(IR::Slice* expression) override;
    const IR::Node* postorder(IR::Concat* expression) override;
    const IR::Node* preorder(IR::P4Table* table) override
    { prune(); return table; }  // don't simplify expressions in table
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_COMMON_LOWER_H_ */
