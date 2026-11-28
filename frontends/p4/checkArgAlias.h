/*
Copyright 2025 Nvidia, Inc.

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

#ifndef FRONTENDS_P4_CHECKARGALIAS_H_
#define FRONTENDS_P4_CHECKARGALIAS_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "typeChecking/typeChecker.h"
#include "typeMap.h"

namespace P4 {

class CheckArgAlias : public PassManager {
    TypeMap *typeMap;
    class FindCaptures : public Inspector, public ResolutionContext {
        std::vector<std::pair<const IR::IDeclaration *, const IR::Node *>> visiting;
        profile_t init_apply(const IR::Node *root) {
            auto rv = Inspector::init_apply(root);
            // initialize visiting stack with (just) sentinel value;
            visiting.resize(1);
            visiting.back().first = nullptr;
            visiting.back().second = nullptr;
            return rv;
        }

        const IR::Expression *getRefCtxt();

        bool preorder(const IR::PathExpression *);
        bool preorder(const IR::Type_Name *) { return false; }

        bool preorder(const IR::IApply *);
        bool preorder(const IR::IFunctional *);

        // FIXME -- would be nice if dynamic dispatch could handle this automatically
        bool preorder(const IR::Node *n) {
            if (auto *ia = n->to<IR::IApply>()) return preorder(ia);
            if (auto *fn = n->to<IR::IFunctional>()) return preorder(fn);
            return true;
        }
        void postorder(const IR::Node *n) {
            if (!visiting.empty() && visiting.back().second == n) visiting.pop_back();
        }

        void end_apply(const IR::Node *) { BUG_CHECK(visiting.size() == 1, "failed to finish?"); }
        void end_apply() { visiting.clear(); }

        using result_t = std::vector<const IR::Expression *>;
        std::map<const IR::IDeclaration *, std::map<cstring, result_t>> result;

     public:
        const result_t &operator()(const IR::IDeclaration *d, cstring name) const {
            static result_t empty;
            auto i1 = result.find(d);
            if (i1 == result.end()) return empty;
            auto i2 = i1->second.find(name);
            if (i2 == i1->second.end()) return empty;
            return i2->second;
        }
        bool any(const IR::IDeclaration *d) const { return result.find(d) != result.end(); }
    } captures;

    class Check : public Inspector, public ResolutionContext {
        const CheckArgAlias &self;
        const IR::Expression *baseExpr(const IR::Expression *, int * = nullptr);
        int baseExprDepth(const IR::Expression *e);
        bool overlaps(const IR::Expression *e1, const IR::Expression *e2);
        bool preorder(const IR::MethodCallExpression *);

     public:
        explicit Check(const CheckArgAlias &s) : self(s) {}
    };

 public:
    explicit CheckArgAlias(TypeMap *tm) : typeMap(tm) {
        addPasses({&captures, new ReadOnlyTypeInference(typeMap), new Check(*this)});
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_CHECKARGALIAS_H_ */
