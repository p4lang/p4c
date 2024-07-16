/*
Copyright 2018 VMware, Inc.

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

#ifndef FRONTENDS_P4_FUNCTIONSINLINING_H_
#define FRONTENDS_P4_FUNCTIONSINLINING_H_

#include "commonInlining.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/unusedDeclarations.h"
#include "ir/ir.h"

namespace P4 {
using FunctionCallInfo = SimpleCallInfo<IR::Node, IR::Statement>;
using FunctionsInlineWorkList = SimpleInlineWorkList<FunctionCallInfo>;
using FunctionsInlineList = SimpleInlineList<IR::Node, FunctionCallInfo, FunctionsInlineWorkList>;

class DiscoverFunctionsInlining : public Inspector, public ResolutionContext {
    FunctionsInlineList *toInline;  // output
    P4::TypeMap *typeMap;           // input

 public:
    DiscoverFunctionsInlining(FunctionsInlineList *toInline, P4::TypeMap *typeMap)
        : toInline(toInline), typeMap(typeMap) {
        CHECK_NULL(toInline);
        CHECK_NULL(typeMap);
        setName("DiscoverFunctionsInlining");
    }
    Visitor::profile_t init_apply(const IR::Node *node) override;
    void postorder(const IR::MethodCallExpression *mce) override;
};

/**
  Inline functions.
  This must be executed after SideEffectOrdering and RemoveReturns.
*/
class FunctionsInliner : public AbstractInliner<FunctionsInlineList, FunctionsInlineWorkList> {
    std::unique_ptr<MinimalNameGenerator> nameGen;

    // All elements in the replacement map are actually IR::Function objects
    using ReplacementMap = FunctionsInlineWorkList::ReplacementMap;
    /// A stack of replacement maps
    std::vector<ReplacementMap *> replacementStack;
    /// Pending replacements for subexpressions
    std::unordered_map<const IR::MethodCallExpression *, const IR::Expression *>
        pendingReplacements;

    /// Clones the body of a function without the return statement;
    /// returns the expression of a return statement.  This assumes
    /// that there is only one return statement within the function
    /// body.  This is ensured by the RemoveReturns pass.
    const IR::Expression *cloneBody(const IR::IndexedVector<IR::StatOrDecl> &src,
                                    IR::IndexedVector<IR::StatOrDecl> &dest);
    /// Returns a block statement that will replace the statement
    const IR::Statement *inlineBefore(const IR::Node *calleeNode,
                                      const IR::MethodCallExpression *call,
                                      const IR::Statement *before);
    bool preCaller();
    const IR::Node *postCaller(const IR::Node *caller);
    const ReplacementMap *getReplacementMap() const;
    void dumpReplacementMap() const;

 public:
    FunctionsInliner() = default;
    Visitor::profile_t init_apply(const IR::Node *node) override;
    void end_apply(const IR::Node *node) override;
    const IR::Node *preorder(IR::Function *function) override;
    const IR::Node *preorder(IR::P4Control *control) override;
    const IR::Node *preorder(IR::P4Parser *parser) override;
    const IR::Node *preorder(IR::P4Action *action) override;
    const IR::Node *preorder(IR::MethodCallStatement *statement) override;
    const IR::Node *preorder(IR::MethodCallExpression *expr) override;
    const IR::Node *preorder(IR::AssignmentStatement *statement) override;
};

typedef InlineDriver<FunctionsInlineList, FunctionsInlineWorkList> InlineFunctionsDriver;

/// Convert distinct variable declarations into distinct nodes in the
/// IR tree.
class CloneVariableDeclarations : public Transform {
 public:
    CloneVariableDeclarations() {
        setName("CloneVariableDeclarations");
        visitDagOnce = false;
    }
    const IR::Node *postorder(IR::Declaration_Variable *declaration) override {
        // this will have a different declid
        auto result = new IR::Declaration_Variable(declaration->srcInfo, declaration->getName(),
                                                   declaration->annotations, declaration->type,
                                                   declaration->initializer);
        LOG3("Cloned " << dbp(result));
        return result;
    }
};

class InlineFunctions : public PassManager {
    FunctionsInlineList functionsToInline;

 public:
    InlineFunctions(ReferenceMap *refMap, TypeMap *typeMap, const RemoveUnusedPolicy &policy) {
        passes.push_back(new PassRepeated(
            {new TypeChecking(nullptr, typeMap),
             new DiscoverFunctionsInlining(&functionsToInline, typeMap),
             new InlineFunctionsDriver(&functionsToInline, new FunctionsInliner()),
             new ResolveReferences(refMap), new RemoveAllUnusedDeclarations(refMap, policy)}));
        passes.push_back(new CloneVariableDeclarations());
        setName("InlineFunctions");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_FUNCTIONSINLINING_H_ */
