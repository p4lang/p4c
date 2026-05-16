// Copyright 2020 Barefoot Networks, Inc.
// SPDX-FileCopyrightText: 2020 Barefoot Networks, Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "switchAddDefault.h"

#include "lib/ordered_set.h"

namespace P4 {

void SwitchAddDefault::postorder(IR::SwitchStatement *sw) {
    ordered_set<cstring> case_tags;
    for (auto sc : sw->cases) {
        if (sc->label->is<IR::DefaultExpression>()) {
            return;
        } else if (auto pe = sc->label->to<IR::PathExpression>()) {
            case_tags.insert(pe->path->name);
        }
    }
    bool need_default = false;
    if (auto actenum = sw->expression->type->to<IR::Type_ActionEnum>()) {
        for (auto act : actenum->actionList->actionList) {
            if (auto mce = act->expression->to<IR::MethodCallExpression>()) {
                if (auto pe = mce->method->to<IR::PathExpression>()) {
                    if (!case_tags.count(pe->path->name)) {
                        need_default = true;
                    }
                } else {
                    need_default = true;
                }
            } else {
                need_default = true;
            }
            if (need_default) break;
        }
    }
    if (need_default) {
        sw->cases.push_back(new IR::SwitchCase(new IR::DefaultExpression(IR::Type_Dontcare::get()),
                                               new IR::BlockStatement));
    }
}

}  // namespace P4
