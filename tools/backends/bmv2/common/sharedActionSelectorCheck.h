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

#ifndef BACKENDS_BMV2_COMMON_SHAREDACTIONSELECTORCHECK_H_
#define BACKENDS_BMV2_COMMON_SHAREDACTIONSELECTORCHECK_H_

#include "ir/ir.h"
#include "lib/json.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/frontend.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "helpers.h"

namespace BMV2 {

using SelectorInput = std::vector<const IR::Expression *>;

// This pass makes sure that when several match tables share a selector, they use the same input for
// the selection algorithm. This is because bmv2 considers that the selection key is part of the
// action_selector while v1model.p4 considers that it belongs to the table match key definition.
class SharedActionSelectorCheck : public Inspector {
    P4::ReferenceMap* refMap;
    P4::TypeMap*      typeMap;
    std::map<const IR::Declaration_Instance *, SelectorInput> selector_input_map{};

  static bool checkSameKeyExpr(const IR::Expression* expr0, const IR::Expression* expr1) {
      if (expr0->node_type_name() != expr1->node_type_name())
          return false;
      if (auto pe0 = expr0->to<IR::PathExpression>()) {
          auto pe1 = expr1->to<IR::PathExpression>();
          return pe0->path->name == pe1->path->name &&
              pe0->path->absolute == pe1->path->absolute;
      } else if (auto mem0 = expr0->to<IR::Member>()) {
          auto mem1 = expr1->to<IR::Member>();
          return checkSameKeyExpr(mem0->expr, mem1->expr) && mem0->member == mem1->member;
      } else if (auto l0 = expr0->to<IR::Literal>()) {
          auto l1 = expr1->to<IR::Literal>();
          return *l0 == *l1;
      } else if (auto ai0 = expr0->to<IR::ArrayIndex>()) {
          auto ai1 = expr1->to<IR::ArrayIndex>();
          return checkSameKeyExpr(ai0->left, ai1->left) && checkSameKeyExpr(ai0->right, ai1->right);
      }
      return false;
  }

 public:
    explicit SharedActionSelectorCheck(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) :
        refMap(refMap), typeMap(typeMap) {}

    const SelectorInput* get_selector_input(const IR::Declaration_Instance* selector);
    bool preorder(const IR::P4Table* table) override;
};

}  // namespace BMV2

#endif  /* BACKENDS_BMV2_COMMON_SHAREDACTIONSELECTORCHECK_H_ */
