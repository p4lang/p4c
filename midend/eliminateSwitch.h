/*
Copyright 2020 VMware, Inc.

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

#ifndef MIDEND_ELIMINATESWITCH_H_
#define MIDEND_ELIMINATESWITCH_H_

#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

/**
 * Replaces switch statements that operate on arbitrary scalars with
 * switch statements that operate on actions by introducing a new table.

switch (expression) {
  1: { ... }
  2:
  3: { ... }
  4: { ... }
  default: { ... }
}

This generates the following program:

ExpressionType switch1_key;

@hidden action switch1_case_1 () {
    // no statements here, by design
}
@hidden action switch1_case_23 () {}
@hidden action switch1_case_4 () {}
@hidden action switch1_case_default () {}
@hidden table switch1_table {
    key = {
        switch1_key : exact;
    }
    actions = {
        switch1_case_1;
        switch1_case_23;
        switch1_case_4;
        switch1_case_default;
    }
    const entries = {
        1 : switch1_case_1;
        2 : switch1_case_23;
        3 : switch1_case_23;
        4 : switch1_case_4;
    }
    const default_action = switch1_case_default;
}

// later in the control's apply block, where the original switch
// statement appeared:
switch1_key = expression;
switch (switch1_table.apply().action_run) {
switch1_case_1:       { ... }
switch1_case_23:      { ... }
switch1_case_4:       { ... }
switch1_case_default: { ... }
}

 */
class DoEliminateSwitch final : public Transform {
    ReferenceMap *refMap;
    const TypeMap *typeMap;
    std::vector<const IR::Declaration *> toInsert;

 public:
    bool exactNeeded = false;

    DoEliminateSwitch(ReferenceMap *refMap, const TypeMap *typeMap)
        : refMap(refMap), typeMap(typeMap) {
        setName("DoEliminateSwitch");
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
    }
    const IR::Node *postorder(IR::SwitchStatement *statement) override;
    const IR::Node *postorder(IR::P4Control *control) override;
    const IR::Node *postorder(IR::P4Program *program) override;
};

class EliminateSwitch final : public PassManager {
 public:
    EliminateSwitch(ReferenceMap *refMap, TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        if (!typeChecking) typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoEliminateSwitch(refMap, typeMap));
        passes.push_back(new ClearTypeMap(typeMap));
        setName("EliminateSwitch");
    }
};

}  // namespace P4

#endif /* MIDEND_ELIMINATESWITCH_H_ */
