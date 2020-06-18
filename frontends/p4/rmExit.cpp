/*
Copyright 2020 MNK Labs & Consulting, LLC
* https://mnkcg.com

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

#include "frontends/p4/rmExit.h"

namespace P4 {

/*
 *  If an action is found, check if action has an exit statement.
 *  If so, check if any action parameter is hasOut.  Only on both
 *  conditions satisfied, the action is processed by this method.
 *
 *  If no action processing is performed, check if the control includes
 *  an exit statements.
 */
const IR::Node* DoRmExits::preorder(IR::P4Control* control) {
    LOG1("#############################################################################");
    LOG1("Visiting control cpp " << std::hex << control->name);
    bool hasExit = false;
    bool hasOut = false;
    bool ctrlExit = false;
    bool ctrlOut = false;

    auto applyParams = control->getApplyParameters();
    for (auto p : *applyParams) {
        if (p->hasOut()) {
            ctrlOut = true;
            LOG1("CtrlOut found cpp");
            break;
        }
    }
    // Collect actions list with exit or not.
    std::map<cstring, std::pair<int, bool>> actions;
    FindExitActions fea(refMap, &actions);
    (void)control->apply(fea);
    LOG1("Action list size cpp: " << actions.size());
    for (auto it : actions) {
        LOG1("action loop cpp: " << it.first << " exit " << it.second.first <<
             " hasOut " << it.second.second);
    }
    LOG1("");
    for (auto it : actions) {
        if (it.second.first) {
            LOG1("action cpp has : " << it.first << " exit "
                 << it.second.first);
            hasExit = true;
            break;
        }
    }
    for (auto it : actions) {
        if (it.second.second) {
            LOG1("action cpp has : " << it.first << " hasOut "
                 << it.second.second);
            hasOut = true;
            break;
        }
    }
    // End Collect actions list with exit or not.

    // Check if control has any exit.
    auto newComponents = new IR::IndexedVector<IR::StatOrDecl>();
    for (auto c : control->body->components) {
        if (c->is<IR::ExitStatement>()) {
            ctrlExit = true;
            break;
        }
        newComponents->push_back(c);
    }
    // End check if control has any exit.

    if (!hasExit && !ctrlExit && !ctrlOut) return control;
    if (ctrlExit) {
        auto bs = control->body->clone();
        bs->components = *newComponents;
        auto result = new IR::P4Control(control->srcInfo, control->name,
                                        control->type,
                                        control->constructorParams,
                                        control->controlLocals, bs);
        return result;
    }

    if (!hasOut && !ctrlOut) return control;
    if (!hasExit) return control;

    // Now update the IR due exit statements.
    EditCtrlIR ecr(refMap, typeMap, &actions);
    auto result = control->apply(ecr);
    LOG1("Exiting control cpp " << control->name << std::endl);
    return result;
}

}  // namespace P4
