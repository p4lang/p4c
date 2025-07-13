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

#include "eliminateActionRun.h"

#include "frontends/p4/methodInstance.h"
#include "ir/irutils.h"

namespace P4 {

void ElimActionRun::ActionTableUse::postorder(const IR::Member *mem) {
    if (mem->member != IR::Type_Table::action_run) return;
    auto *mce = mem->expr->to<IR::MethodCallExpression>();
    if (!mce) return;
    auto *mi = MethodInstance::resolve(mce, this);
    auto *tbl = mi->object->to<IR::P4Table>();
    if (!tbl) return;
    LOG3("found: " << mem);
    auto [it, inserted] = self.actionRunTables.emplace(tbl->name, tbl);
    if (!inserted) {
        LOG3("  (additional apply() of table)");
        return;
    }
    auto *info = &it->second;
    IR::IndexedVector<IR::Declaration_ID> tags;
    for (auto *ale : tbl->getActionList()->actionList) {
        cstring name = ale->getName();
        BUG_CHECK(!self.actionsToModify.count(name),
                  "action %s used in multiple tables (%s and %s) -- LocalizeActions must be run "
                  "before ElimActionRun",
                  name, self.actionsToModify.at(name)->tbl->name, tbl->name);
        self.actionsToModify[name] = info;
        cstring tag = self.nameGen.newName(tbl->name + "_" + name);
        info->actions.emplace(name, tag);
        tags.push_back(new IR::Declaration_ID(tag));
        LOG4("  action " << name << " [" << ale->expression << "] -> " << tag);
    }
    info->action_tags =
        new IR::Type_Enum(self.nameGen.newName(tbl->name + "_action_run_t"), std::move(tags));
    info->action_run = new IR::Declaration_Variable(self.nameGen.newName(tbl->name + "_action_run"),
                                                    new IR::Type_Name(info->action_tags->name));
}

const IR::Expression *ElimActionRun::RewriteActionRun::postorder(IR::Member *mem) {
    if (mem->member != IR::Type_Table::action_run) return mem;
    auto *mce = mem->expr->to<IR::MethodCallExpression>();
    if (!mce) return mem;
    auto *mi = MethodInstance::resolve(mce, this);
    auto *tbl = mi->object->to<IR::P4Table>();
    if (!tbl) return mem;
    auto &info = self.actionRunTables.at(tbl->name);
    auto *parent = findContext<IR::Statement>();
    BUG_CHECK(parent && parent->is<IR::SwitchStatement>(), "action_run not in switch");
    auto &pps = self.prepend_statement[parent];
    pps.push_back(new IR::MethodCallStatement(mce));
    self.add_to_control.push_back(info.action_run);
    return new IR::PathExpression(new IR::Type_Name(info.action_tags->name), info.action_run->name);
}

const IR::P4Action *ElimActionRun::RewriteActionRun::preorder(IR::P4Action *act) {
    auto *info = get(self.actionsToModify, act->name);
    if (!info) return act;
    auto *body = act->body->clone();
    auto *tag_type = new IR::Type_Name(info->action_tags->name);

    body->components.insert(
        body->components.begin(),
        new IR::AssignmentStatement(
            new IR::PathExpression(tag_type->clone(), info->action_run->name),
            new IR::Member(new IR::TypeNameExpression(tag_type), info->actions.at(act->name))));
    act->body = body;
    return act;
}

const IR::SwitchCase *ElimActionRun::RewriteActionRun::postorder(IR::SwitchCase *swCase) {
    auto *swtch = getParent<IR::SwitchStatement>();
    BUG_CHECK(swtch, "case not in switch");
    if (!self.prepend_statement.count(swtch)) return swCase;
    if (swCase->label->is<IR::DefaultExpression>()) return swCase;
    auto *pe = swCase->label->to<IR::PathExpression>();
    BUG_CHECK(pe, "case label is not an action name");
    auto *info = self.actionsToModify.at(pe->path->name);
    auto *tag_type = new IR::Type_Name(info->action_tags->name);
    swCase->label =
        new IR::Member(new IR::TypeNameExpression(tag_type), info->actions.at(pe->path->name));
    return swCase;
}

const IR::Node *ElimActionRun::RewriteActionRun::postorder(IR::Statement *stmt) {
    auto pps = self.prepend_statement.find(stmt);
    if (pps == self.prepend_statement.end()) return stmt;
    pps->second.push_back(stmt);
    auto *rv = inlineBlock(*this, std::move(pps->second));
    self.prepend_statement.erase(pps);
    return rv;
}

const IR::P4Control *ElimActionRun::RewriteActionRun::postorder(IR::P4Control *ctrl) {
    BUG_CHECK(self.prepend_statement.empty(), "orphan action_run in control %s", ctrl->name);
    ctrl->controlLocals.prepend(self.add_to_control);
    if (!self.add_to_control.empty()) LOG4(ctrl);
    self.add_to_control.clear();
    return ctrl;
}

const IR::P4Program *ElimActionRun::RewriteActionRun::postorder(IR::P4Program *prog) {
    auto front = prog->objects.begin();
    for (auto &[_, info] : self.actionRunTables)
        front = std::next(prog->objects.insert(front, info.action_tags));
    return prog;
}

}  // namespace P4
