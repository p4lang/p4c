/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MIDEND_ELIMINATEACTIONRUN_H_
#define MIDEND_ELIMINATEACTIONRUN_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "ir/ir.h"

namespace P4 {

/** Pass to convert table.apply().action_run into a temp metadata.
 *
 * This will take code that looks like
 *
 *              switch (table.apply().action_run) {
 *                  .. cases for actions in the table ..
 *
 * and convert it into:
 *
 *              enum new_meta_t { .. tags for actions in the table ..
 *                              } new_meta;
 *              table.apply();
 *              switch (new_meta) {
 *                  .. cases for tags corresponding to actions ..
 *
 * where all the actions are modified to additionally set `new_meta` and
 * the declaration of `new_meta` is up enough to be in scope for all the actions.
 *
 * This is more or less the reverse of what the EliminateSwitch pass does, so it
 * never makes sense to include this pass along with EliminateSwitch.
 */
class ElimActionRun : public PassManager {
    MinimalNameGenerator nameGen;

    struct atinfo_t {
        const IR::P4Table *tbl;
        const IR::Type_Enum *action_tags;
        const IR::Declaration_Variable *action_run;
        std::map<cstring, cstring> actions;
        explicit atinfo_t(const IR::P4Table *t) : tbl(t) {}
    };

    std::map<cstring, atinfo_t> actionRunTables;
    std::map<cstring, atinfo_t *> actionsToModify;
    std::map<const IR::Statement *, IR::IndexedVector<IR::StatOrDecl>> prepend_statement;
    IR::Vector<IR::Declaration> add_to_control;

    class ActionTableUse : public Inspector, public ResolutionContext {
        ElimActionRun &self;

     public:
        explicit ActionTableUse(ElimActionRun &s) : self(s) {}

        bool preorder(const IR::P4Parser *) override { return false; }
        bool preorder(const IR::P4Action *) override { return false; }
        bool preorder(const IR::Function *) override { return false; }

        void postorder(const IR::Member *) override;
    } actionTableUse;

    class RewriteActionRun : public Transform, public ResolutionContext {
        ElimActionRun &self;

     public:
        explicit RewriteActionRun(ElimActionRun &s) : self(s) {}

        const IR::P4Parser *preorder(IR::P4Parser *p) override {
            prune();
            return p;
        }
        const IR::Function *preorder(IR::Function *f) override {
            prune();
            return f;
        }

        const IR::Expression *postorder(IR::Member *) override;
        const IR::P4Action *preorder(IR::P4Action *) override;
        const IR::SwitchCase *postorder(IR::SwitchCase *) override;
        const IR::Node *postorder(IR::Statement *) override;
        const IR::P4Control *postorder(IR::P4Control *) override;
        const IR::P4Program *postorder(IR::P4Program *) override;
    };

 public:
    ElimActionRun() : actionTableUse(*this) {
        addPasses({&actionTableUse, new PassIf([this]() { return !actionRunTables.empty(); },
                                               {
                                                   &nameGen,
                                                   new RewriteActionRun(*this),
                                                   [this]() { actionRunTables.clear(); },
                                               })});
    }
};

}  // namespace P4

#endif /* MIDEND_ELIMINATEACTIONRUN_H_ */
