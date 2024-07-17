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

#ifndef FRONTENDS_P4_TYPECHECKING_CONSTANTTYPESUBSTITUTION_H_
#define FRONTENDS_P4_TYPECHECKING_CONSTANTTYPESUBSTITUTION_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/visitor.h"
#include "typeChecker.h"
#include "typeSubstitution.h"

namespace P4 {

// Used to set the type of Constants after type inference
class ConstantTypeSubstitution : public Transform, ResolutionContext {
    TypeVariableSubstitution *subst;
    TypeMap *typeMap;
    TypeInferenceBase *tc;

 public:
    ConstantTypeSubstitution(TypeVariableSubstitution *subst, TypeMap *typeMap,
                             TypeInferenceBase *tc)
        : subst(subst), typeMap(typeMap), tc(tc) {
        CHECK_NULL(subst);
        CHECK_NULL(typeMap);
        CHECK_NULL(tc);
        LOG3("ConstantTypeSubstitution " << subst);
    }

    const IR::Node *postorder(IR::Constant *cst) override {
        auto cstType = typeMap->getType(getOriginal(), true);
        if (!cstType->is<IR::ITypeVar>()) return cst;
        auto repl = cstType;
        while (repl->is<IR::ITypeVar>()) {
            auto next = subst->get(repl->to<IR::ITypeVar>());
            BUG_CHECK(next != repl, "Cycle in substitutions: %1%", next);
            if (!next) break;
            repl = next;
        }
        if (repl != cstType) {
            // We may replace a type variable with another one
            LOG2("Inferred type " << repl << " for " << cst);
            cst = new IR::Constant(cst->srcInfo, repl, cst->value, cst->base);
        } else {
            LOG2("No type inferred for " << cst << " repl is " << repl);
        }
        return cst;
    }

    const IR::Expression *convert(const IR::Expression *expr, const Visitor::Context *ctxt) {
        auto result = expr->apply(*this, ctxt)->to<IR::Expression>();
        if (result != expr && (::P4::errorCount() == 0)) tc->learn(result, this, ctxt);
        return result;
    }
    const IR::Vector<IR::Expression> *convert(const IR::Vector<IR::Expression> *vec,
                                              const Visitor::Context *ctxt) {
        auto result = vec->apply(*this, ctxt)->to<IR::Vector<IR::Expression>>();
        if (result != vec) tc->learn(result, this, ctxt);
        return result;
    }
    const IR::Vector<IR::Argument> *convert(const IR::Vector<IR::Argument> *vec,
                                            const Visitor::Context *ctxt) {
        auto result = vec->apply(*this, ctxt)->to<IR::Vector<IR::Argument>>();
        if (result != vec) tc->learn(result, this, ctxt);
        return result;
    }
};

}  // namespace P4

#endif  // FRONTENDS_P4_TYPECHECKING_CONSTANTTYPESUBSTITUTION_H_
