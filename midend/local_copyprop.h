/* Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef MIDEND_LOCAL_COPYPROP_H_
#define MIDEND_LOCAL_COPYPROP_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "has_side_effects.h"
#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {
using LocalCopyPropPolicyCallbackFn = std::function<bool(
    const Visitor::Context *, const IR::Expression *, const DeclarationLookup *)>;
/**
Local copy propagation and dead code elimination within a single pass.
This pass is designed to be run after all declarations have received unique
internal names.  This is important because the locals map uses only the
declaration name, and not the full path.

@pre
Requires expression types be stored inline in the expression
(obtained by running Typechecking(updateProgram = true)).

Requires that all declaration names be globally unique
(obtained by running UniqueNames).

Requires that all variable declarations are at the top-level control scope
(obtained using MoveDeclarations).

@param policy

This predicate function will be called for any expression that could be copy-propagated
to determine if it should be.  It will only be called for expressions that are legal to
propagate (so no side effects, or dependencies that would change the meaning), so the
policy should only evaluate the potential cost of propagating, as propagated expressions
may be evaluated mulitple times.  The default policy just returns true -- always propagate
if legal to do so.

 */
class DoLocalCopyPropagation : public ControlFlowVisitor,
                               Transform,
                               P4WriteContext,
                               ResolutionContext {
    TypeMap *typeMap;
    bool working = false;
    struct VarInfo {
        bool local = false;
        bool live = false;
        const IR::Expression *val = nullptr;
    };
    struct TableInfo {
        std::set<cstring> keyreads, actions;
        int apply_count = 0;
        std::map<cstring, const IR::Expression *> key_remap;
    };
    struct FuncInfo {
        std::set<cstring> reads, writes;
        int apply_count = 0;

        /// This field is used in assignments. If is_first_write_insert is true, then the
        /// last inserted expression into the writes set was not in that set before. In
        /// that case, that expression will be removed from it if, after propagation of the
        /// values on the left and the right side, the assignment becomes a self-assignment
        bool is_first_write_insert = false;
    };
    std::map<cstring, VarInfo> available;
    std::shared_ptr<std::map<cstring, TableInfo>> tables;
    std::shared_ptr<std::map<cstring, FuncInfo>> actions;
    std::shared_ptr<std::map<cstring, FuncInfo>> methods;
    std::shared_ptr<std::map<cstring, FuncInfo>> states;
    TableInfo *inferForTable = nullptr;
    FuncInfo *inferForFunc = nullptr;
    bool need_key_rewrite = false;
    LocalCopyPropPolicyCallbackFn policy;
    bool elimUnusedTables = false;
    int uid = -1;
    static int uid_ctr;

    DoLocalCopyPropagation *clone() const override {
        auto *rv = new DoLocalCopyPropagation(*this);
        rv->uid = ++uid_ctr;
        LOG8("flow_clone(" << uid << ") = " << rv->uid);
        return rv;
    }
    void flow_merge(Visitor &) override;
    void flow_copy(ControlFlowVisitor &) override;
    bool operator==(const ControlFlowVisitor &) const override;
    bool name_overlap(cstring, cstring);
    void forOverlapAvail(cstring, std::function<void(cstring, VarInfo *)>);
    void dropValuesUsing(cstring);
    bool hasSideEffects(const IR::Expression *e, const Visitor::Context *ctxt) {
        return bool(::P4::hasSideEffects(typeMap, e, ctxt));
    }
    bool isHeaderUnionIsValid(const IR::Expression *e);

    class LoopPrepass : public Inspector {
        DoLocalCopyPropagation &self;
        void postorder(const IR::BaseAssignmentStatement *) override;
        void postorder(const IR::MethodCallExpression *) override;
        void apply_table(TableInfo *tbl);
        void apply_function(FuncInfo *tbl);

     public:
        explicit LoopPrepass(DoLocalCopyPropagation &s) : self(s) {}
    };

    void visit_local_decl(const IR::Declaration_Variable *);
    const IR::Node *postorder(IR::Declaration_Variable *) override;
    IR::Expression *preorder(IR::Expression *m) override;
    const IR::Expression *copyprop_name(cstring name, const Util::SourceInfo &srcInfo);
    const IR::Expression *postorder(IR::PathExpression *) override;
    const IR::Expression *preorder(IR::Member *) override;
    const IR::Expression *preorder(IR::ArrayIndex *) override;
    IR::Statement *preorder(IR::Statement *) override;
    const IR::Node *preorder(IR::BaseAssignmentStatement *) override;
    IR::AssignmentStatement *postorder(IR::AssignmentStatement *) override;
    IR::IfStatement *postorder(IR::IfStatement *) override;
    IR::ForStatement *preorder(IR::ForStatement *) override;
    IR::ForInStatement *preorder(IR::ForInStatement *) override;
    IR::MethodCallExpression *postorder(IR::MethodCallExpression *) override;
    IR::P4Action *preorder(IR::P4Action *) override;
    IR::P4Action *postorder(IR::P4Action *) override;
    IR::Function *preorder(IR::Function *) override;
    IR::Function *postorder(IR::Function *) override;
    IR::P4Control *preorder(IR::P4Control *) override;
    void apply_table(TableInfo *tbl);
    void apply_function(FuncInfo *tbl);
    IR::P4Table *preorder(IR::P4Table *) override;
    IR::P4Table *postorder(IR::P4Table *) override;
    const IR::P4Parser *postorder(IR::P4Parser *) override;
    IR::ParserState *preorder(IR::ParserState *) override;
    IR::ParserState *postorder(IR::ParserState *) override;
    void end_apply(const IR::Node *node) override;
    class ElimDead;
    class RewriteTableKeys;

    DoLocalCopyPropagation(const DoLocalCopyPropagation &) = default;

 public:
    DoLocalCopyPropagation(TypeMap *typeMap, LocalCopyPropPolicyCallbackFn policy, bool eut)
        : typeMap(typeMap),
          tables(std::make_shared<std::map<cstring, TableInfo>>()),
          actions(std::make_shared<std::map<cstring, FuncInfo>>()),
          methods(std::make_shared<std::map<cstring, FuncInfo>>()),
          states(std::make_shared<std::map<cstring, FuncInfo>>()),
          policy(policy),
          elimUnusedTables(eut) {}
};

class LocalCopyPropagation : public PassManager {
 public:
    LocalCopyPropagation(
        TypeMap *typeMap, TypeChecking *typeChecking = nullptr,
        LocalCopyPropPolicyCallbackFn policy = [](const Context *, const IR::Expression *,
                                                  const DeclarationLookup *) -> bool {
            return true;
        },
        bool elimUnusedTables = false) {
        if (!typeChecking) typeChecking = new TypeChecking(nullptr, typeMap, true);
        passes.push_back(typeChecking);
        passes.push_back(new DoLocalCopyPropagation(typeMap, policy, elimUnusedTables));
    }
    LocalCopyPropagation(TypeMap *typeMap, LocalCopyPropPolicyCallbackFn policy)
        : LocalCopyPropagation(typeMap, nullptr, policy) {}
};

}  // namespace P4

#endif /* MIDEND_LOCAL_COPYPROP_H_ */
