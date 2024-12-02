/*
Copyright 2024 Cisco Systems, Inc.

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

#ifndef FRONTENDS_P4_DUPLICATEACTIONCONTROLPLANENAMECHECK_H_
#define FRONTENDS_P4_DUPLICATEACTIONCONTROLPLANENAMECHECK_H_

#include <vector>
#include "ir/ir.h"
#include "ir/visitor.h"

namespace P4 {

/**
 * This pass does not change anything in the IR.  It only gives an
 * error if it finds two actions with the same hierarchical name.  I
 * am not certain, but it might be that at this point in the front
 * end, the only way such a duplicate can happen is due to @name
 * annotations.  @name annotations are definitely taken into account
 * in this duplicate check.
 *
 * See also the pass HierarchicalNames, on which the implementation of
 * this pass was based.
 *
 * This pass should be run before pass LocalizeAllActions, because
 * LocalizeAllActions can create actions with duplicate names, by
 * design, that were not created by the P4 developer.  We should not
 * issue an error if that pass creates duplicate hierarchical names.
 */
class DuplicateActionControlPlaneNameCheck : public Transform {
    std::vector<cstring> stack;
    /// Used for detection of conflicting control plane names among actions.
    absl::flat_hash_map<cstring, const IR::Node *> actions;

 public:
    cstring getName(const IR::IDeclaration *decl);

    DuplicateActionControlPlaneNameCheck() {
        setName("DuplicateActionControlPlaneNameCheck");
        visitDagOnce = false;
    }
    const IR::Node *preorder(IR::P4Parser *parser) override {
        // There cannot be any action definitions inside a parser, so
        // do not traverse through its nodes.
        prune();
        return parser;
    }

    const IR::Node *preorder(IR::P4Control *control) override {
        stack.push_back(getName(control));
        return control;
    }
    const IR::Node *postorder(IR::P4Control *control) override {
        stack.pop_back();
        return control;
    }

    const IR::Node *preorder(IR::P4Table *table) override {
        // There cannot be any action definitions inside a table, so
        // do not traverse through its nodes.
        prune();
        return table;
    }

    void checkForDuplicateName(cstring name, const IR::Node *node);

    const IR::Node *postorder(IR::P4Action *action) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_DUPLICATEACTIONCONTROLPLANENAMECHECK_H_ */
