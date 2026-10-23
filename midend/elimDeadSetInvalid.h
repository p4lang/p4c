// SPDX-FileCopyrightText: 2026 Abhishek Agarwal
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MIDEND_ELIMDEADSETINVALID_H_
#define MIDEND_ELIMDEADSETINVALID_H_

#include <set>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "midend/def_use.h"

namespace P4 {

/// Copies parsers so that no select case is shared between states. ParsersUnroll can
/// leave one shared by several states, which ComputeDefUse then reports as an IR loop.
class UnshareParserSelects : public Transform {
 public:
    UnshareParserSelects() {
        visitDagOnce = false;
        forceClone = true;
        setName("UnshareParserSelects");
    }
    const IR::Node *preorder(IR::P4Parser *parser) override { return parser; }
    const IR::Node *preorder(IR::P4Control *control) override {
        prune();
        return control;
    }
    const IR::Node *preorder(IR::Type *type) override {
        prune();
        return type;
    }
};

/// Locals used in ways ComputeDefUse does not link to the valid bit: whole reads,
/// non-constant indexes, stack operations, and methods on structs or unions.
/// Also all locals of parsers with loops, as ComputeDefUse does not follow parser back edges.
class FindUnsafeLocals : public Inspector {
    ReferenceMap *refMap;
    TypeMap *typeMap;
    std::set<const IR::IDeclaration *> &unsafe;

 public:
    FindUnsafeLocals(ReferenceMap *refMap, TypeMap *typeMap,
                     std::set<const IR::IDeclaration *> &unsafe)
        : refMap(refMap), typeMap(typeMap), unsafe(unsafe) {
        visitDagOnce = false;
        setName("FindUnsafeLocals");
    }
    Visitor::profile_t init_apply(const IR::Node *node) override;
    void postorder(const IR::PathExpression *pe) override;
    void postorder(const IR::P4Parser *parser) override;
};

class DoElimDeadSetInvalid : public Transform {
    ReferenceMap *refMap;
    TypeMap *typeMap;
    const ComputeDefUse *defuse;
    const std::set<const IR::IDeclaration *> &unsafe;

 public:
    DoElimDeadSetInvalid(ReferenceMap *refMap, TypeMap *typeMap, const ComputeDefUse *defuse,
                         const std::set<const IR::IDeclaration *> &unsafe)
        : refMap(refMap), typeMap(typeMap), defuse(defuse), unsafe(unsafe) {
        setName("DoElimDeadSetInvalid");
    }
    const IR::Node *postorder(IR::MethodCallStatement *mcs) override;
};

/// Removes setInvalid() calls on local headers whose valid bit is never read afterwards.
/// Only locals are handled, as their validity is not visible once the parser or control
/// ends, including after a failed extract.
class ElimDeadSetInvalid : public PassManager {
    std::set<const IR::IDeclaration *> unsafe;

 public:
    ElimDeadSetInvalid(ReferenceMap *refMap, TypeMap *typeMap) {
        auto *defuse = new ComputeDefUse;
        passes.push_back(new UnshareParserSelects);
        passes.push_back(new TypeChecking(refMap, typeMap, true));
        passes.push_back(defuse);
        passes.push_back(new FindUnsafeLocals(refMap, typeMap, unsafe));
        passes.push_back(new DoElimDeadSetInvalid(refMap, typeMap, defuse, unsafe));
        setName("ElimDeadSetInvalid");
    }
};

}  // namespace P4

#endif /* MIDEND_ELIMDEADSETINVALID_H_ */
