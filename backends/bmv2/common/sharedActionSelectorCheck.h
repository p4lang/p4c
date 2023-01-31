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

#include <algorithm>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/frontend.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "helpers.h"
#include "ir/ir.h"
#include "lib/error.h"
#include "lib/json.h"

namespace BMV2 {

using SelectorInput = std::vector<const IR::Expression *>;

// This pass makes sure that when several match tables share a selector, they use the same input for
// the selection algorithm. This is because bmv2 considers that the selection key is part of the
// action_selector while v1model.p4 considers that it belongs to the table match key definition.
template <Standard::Arch arch>
class SharedActionSelectorCheck : public Inspector {
    BMV2::ConversionContext *ctxt;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;

    static bool checkSameKeyExpr(const IR::Expression *expr0, const IR::Expression *expr1) {
        if (expr0->node_type_name() != expr1->node_type_name()) return false;
        if (auto pe0 = expr0->to<IR::PathExpression>()) {
            auto pe1 = expr1->to<IR::PathExpression>();
            return pe0->path->name == pe1->path->name && pe0->path->absolute == pe1->path->absolute;
        } else if (auto mem0 = expr0->to<IR::Member>()) {
            auto mem1 = expr1->to<IR::Member>();
            return checkSameKeyExpr(mem0->expr, mem1->expr) && mem0->member == mem1->member;
        } else if (auto l0 = expr0->to<IR::Literal>()) {
            auto l1 = expr1->to<IR::Literal>();
            return *l0 == *l1;
        } else if (auto ai0 = expr0->to<IR::ArrayIndex>()) {
            auto ai1 = expr1->to<IR::ArrayIndex>();
            return checkSameKeyExpr(ai0->left, ai1->left) &&
                   checkSameKeyExpr(ai0->right, ai1->right);
        }
        return false;
    }

 public:
    explicit SharedActionSelectorCheck(BMV2::ConversionContext *ctxt) : ctxt(ctxt) {
        refMap = ctxt->refMap;
        typeMap = ctxt->typeMap;
    }

    bool preorder(const IR::P4Table *table) override {
        auto implementation = table->properties->getProperty("implementation");
        if (implementation == nullptr) return false;
        if (!implementation->value->is<IR::ExpressionValue>()) {
            ::error(ErrorType::ERR_EXPECTED, "%1%: expected expression for property",
                    implementation);
            return false;
        }
        auto propv = implementation->value->to<IR::ExpressionValue>();
        if (!propv->expression->is<IR::PathExpression>()) return false;
        auto pathe = propv->expression->to<IR::PathExpression>();
        auto decl = refMap->getDeclaration(pathe->path, true);
        if (!decl->is<IR::Declaration_Instance>()) {
            ::error(ErrorType::ERR_EXPECTED, "%1%: expected a reference to an instance", pathe);
            return false;
        }
        auto dcltype = typeMap->getType(pathe, true);
        if (!dcltype->is<IR::Type_Extern>()) {
            ::error(ErrorType::ERR_UNEXPECTED, "%1%: unexpected type for implementation", dcltype);
            return false;
        }
        auto type_extern_name = dcltype->to<IR::Type_Extern>()->name;
        auto actionSelectorName = Standard::ActionSelectorTraits<arch>::typeName();
        if (type_extern_name != actionSelectorName) return false;

        auto key = table->getKey();
        SelectorInput input;
        for (auto ke : key->keyElements) {
            auto mt = refMap->getDeclaration(ke->matchType->path, true)->to<IR::Declaration_ID>();
            BUG_CHECK(mt != nullptr, "%1%: could not find declaration", ke->matchType);
            if (mt->name.name != BMV2::MatchImplementation::selectorMatchTypeName) continue;
            input.push_back(ke->expression);
        }
        auto decl_instance = decl->to<IR::Declaration_Instance>();
        auto it = ctxt->selector_input_map.find(decl_instance);
        if (it == ctxt->selector_input_map.end()) {
            ctxt->selector_input_map[decl_instance] = input;
            return false;
        }
        // returns true if inputs are the same, false otherwise
        auto cmp_inputs = [](const SelectorInput &i1, const SelectorInput &i2) {
            if (i1.size() != i2.size()) return false;
            return std::equal(i1.begin(), i1.end(), i2.begin(), checkSameKeyExpr);
        };

        if (!cmp_inputs(it->second, input)) {
            ::error(ErrorType::ERR_INVALID,
                    "Action selector %1% is used by multiple tables with different selector inputs",
                    decl);
        }

        return false;
    }
};

}  // namespace BMV2

#endif /* BACKENDS_BMV2_COMMON_SHAREDACTIONSELECTORCHECK_H_ */
