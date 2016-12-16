/*
Copyright 2013-present Barefoot Networks, Inc. 

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

#ifndef FRONTENDS_P4_14_INLINE_CONTROL_FLOW_H_
#define FRONTENDS_P4_14_INLINE_CONTROL_FLOW_H_

#include "ir/ir.h"
#include "frontends/p4/evaluator/evaluator.h"

class InlineControlFlow : public Transform {
 public:
    explicit InlineControlFlow(const IR::V1Program *gl) : global(gl) {
        setName("InlineControlFlow"); }

 private:
    const IR::V1Program *global;

    const IR::Node *preorder(IR::Apply *a) override {
        if (global && !global->get<IR::V1Table>(a->name))
            error("%s: No table named %s", a->srcInfo, a->name);
        return a;
    }
    const IR::Node *preorder(IR::Primitive *p) override {
        if (auto cf = global ? global->get<IR::V1Control>(p->name) : 0) {
            const IR::V1Control *control;
            if (auto act = findContext<IR::ActionFunction>())
                error("%s: Trying to call control flow %s in action %s", p->srcInfo,
                      p->name, act->name);
            else if (auto table = findContext<IR::V1Table>())
                error("%s: Trying to call control flow %s in table %s", p->srcInfo,
                      p->name, table->name);
            else if ((control = findContext<IR::V1Control>()) && control->name == p->name)
                error("%s: Recursive call to control flow %s", p->srcInfo, p->name);
            else
                return cf->code; }
        return p;
    }
};

#endif /* FRONTENDS_P4_14_INLINE_CONTROL_FLOW_H_ */
