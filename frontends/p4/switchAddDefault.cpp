/*
Copyright 2020 Barefoot Networks, Inc.

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

#include "switchAddDefault.h"

#include <string>
#include <vector>

#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/vector.h"
#include "lib/cstring.h"
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
