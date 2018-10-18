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

#ifndef _FRONTENDS_P4_FUNCTIONSINLINING_H_
#define _FRONTENDS_P4_FUNCTIONSINLINING_H_

#include "ir/ir.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/unusedDeclarations.h"
#include "commonInlining.h"

namespace P4 {

typedef SimpleCallInfo<IR::Node, IR::Statement> FunctionCallInfo;
typedef SimpleInlineWorkList<
    IR::Node, IR::Statement, FunctionCallInfo> FunctionsInlineWorkList;
typedef SimpleInlineList<
    IR::Node, FunctionCallInfo, FunctionsInlineWorkList> FunctionsInlineList;

class DiscoverFunctionsInlining : public Inspector {
    FunctionsInlineList* toInline;  // output
    P4::ReferenceMap*  refMap;    // input
    P4::TypeMap*       typeMap;   // input

 public:
    DiscoverFunctionsInlining(FunctionsInlineList* toInline,
                              P4::ReferenceMap* refMap,
                              P4::TypeMap* typeMap) :
            toInline(toInline), refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(toInline); CHECK_NULL(refMap); CHECK_NULL(typeMap);
        setName("DiscoverFunctionsInlining"); }
    void postorder(const IR::MethodCallExpression* mce) override;
};

/**
  Inline functions.
  This must be executed after SideEffectOrdering and RemoveReturns.
*/
class FunctionsInliner : public AbstractInliner<FunctionsInlineList, FunctionsInlineWorkList> {
    P4::ReferenceMap* refMap;
    // All elements in the replacement map are actually IR::Function objects
    typedef std::map<const IR::Statement*, const IR::Node*> ReplacementMap;
    /// A stack of replacement maps
    std::vector<ReplacementMap*> replacementStack;

    /// Clones the body of a function without the return statement;
    /// returns the expression of a return statement.  This assumes
    /// that there is only one return statement within the function
    /// body.  This is ensured by the RemoveReturns pass.
    const IR::Expression* cloneBody(const IR::IndexedVector<IR::StatOrDecl>& src,
                                    IR::IndexedVector<IR::StatOrDecl>& dest);
    /// Returns a block statement that will replace the statement
    const IR::Node* inlineBefore(
        const IR::Node* calleeNode, const IR::MethodCallExpression* call,
        const IR::Statement* before);
    bool preCaller();
    const IR::Node* postCaller(const IR::Node* caller);
    const ReplacementMap* getReplacementMap() const;
    void dumpReplacementMap() const;

 public:
    explicit FunctionsInliner(bool isv1) : refMap(new P4::ReferenceMap())
    { refMap->setIsV1(isv1); }
    Visitor::profile_t init_apply(const IR::Node* node) override;
    void end_apply(const IR::Node* node) override;
    const IR::Node* preorder(IR::Function* function) override;
    const IR::Node* preorder(IR::P4Control* control) override;
    const IR::Node* preorder(IR::P4Parser* parser) override;
    const IR::Node* preorder(IR::P4Action* action) override;
    const IR::Node* preorder(IR::MethodCallStatement* statement) override;
    const IR::Node* preorder(IR::AssignmentStatement* statement) override;
};

typedef InlineDriver<FunctionsInlineList, FunctionsInlineWorkList> InlineFunctionsDriver;

class InlineFunctions : public PassManager {
    FunctionsInlineList functionsToInline;

 public:
    InlineFunctions(ReferenceMap* refMap, TypeMap* typeMap) {
        passes.push_back(new TypeChecking(refMap, typeMap));
        passes.push_back(new DiscoverFunctionsInlining(&functionsToInline, refMap, typeMap));
        passes.push_back(new InlineFunctionsDriver(&functionsToInline,
                                                   new FunctionsInliner(refMap->isV1())));
        passes.push_back(new RemoveAllUnusedDeclarations(refMap));
        setName("InlineFunctions");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_FUNCTIONSINLINING_H_ */
