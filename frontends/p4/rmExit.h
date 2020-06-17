/*
Copyright 2020 MNK Labs & Consulting
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

#ifndef _FRONTENDS_P4_RMEXIT_H_
#define _FRONTENDS_P4_RMEXIT_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/methodInstance.h"
#include "unusedDeclarations.h"

namespace P4 {

/** @brief Builds a list of Action nodes with exit or not flag and
 *  Parameter hasOut flag.
  */
class FindExitActions : public Inspector {
    const ReferenceMap* refMap;
    // std::pair has exit type (0,1,2) and hasOut for boolean.
    std::map<cstring, std::pair<int, bool>>* actions;

 public:
    FindExitActions(const ReferenceMap* refMap,
                    /* out */std::map<cstring, std::pair<int, bool>>* actions) :
            refMap(refMap), actions(actions) {
        CHECK_NULL(refMap); CHECK_NULL(actions);
        setName("FindExitActions");
    }
    bool ActionHasOut(const IR::P4Action* action) {
        auto params = action->getParameters();
        for (auto p : *params) {
            if (p->hasOut())
                return true;
        }
        return false;
    }
    bool preorder(const IR::PathExpression* expression) override {
        int isExit = 0;
        auto decl = refMap->getDeclaration(expression->path);
        if (decl != nullptr && decl->is<IR::P4Action>()) {
            auto action = decl->to<IR::P4Action>();
            bool anyOut = ActionHasOut(action);
            int i = 0;
            for (auto c :  action->body->components) {
                if (c->is<IR::ExitStatement>()) {
                    auto result = std::make_pair((!i ? 1 : 2), anyOut);
                    LOG1("FindActionExit " << action->name << ", ("
                         << result.first << "," << result.second << ")");
                    actions->emplace(action->name, result);
                    isExit = false;
                }
                i++;
            }
            if (!isExit) {
                auto result = std::make_pair(0, anyOut);
                actions->emplace(action->name, result);
            }
        }
        return false;
    }
};

/*
walk control body
any statement, keep it
action encountered, walk action body.
     action hasOut,
     hit AS, keep
     hit action, is action exit? delete rest of statements
*/    
class EditCtrlIR : public Transform {
    const ReferenceMap*      refMap;
    const TypeMap*           typeMap;

    std::map<cstring, std::pair<int, bool>>* actions;

 public:
    EditCtrlIR(const ReferenceMap* refMap,
               const TypeMap* typeMap,
               std::map<cstring, std::pair<int, bool>>* actions) :
        refMap(refMap), typeMap(typeMap), actions(actions) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap); CHECK_NULL(actions);
        setName("EditCtrlIR");
    }

    const IR::Node* preorder(IR::BlockStatement* block) override {
        auto act = findContext<IR::P4Action>();
        if (act == nullptr) {
            LOG1("not in action" << block);
            return block;
        }
        LOG1("Action: " << act->name);
        const IR::MethodCallStatement* mcs = nullptr;
        auto newComponents = new IR::IndexedVector<IR::StatOrDecl>();
        for (auto sd : block->components) {
            if ((mcs = sd->to<IR::MethodCallStatement>())) {
                auto mc = mcs->methodCall;
                auto pe = mc->method->to<IR::PathExpression>();
                CHECK_NULL(pe);
                cstring name = pe->path->name;
                auto it = actions->find(name);
                if (it != actions->end()) {
                    if (it->second.first == 1) {
                        LOG1("edit action: " << std::hex << name);
                        break;  // breaks out of for loop.
                    }
                }
            }
            if (sd->is<IR::ExitStatement>()) {
                continue;
            }
            newComponents->push_back(sd);
        }
        auto bs = block->clone();
        bs->components = *newComponents;
        LOG1("Dumping bs" << std::hex);
        // dump(bs);
        LOG1("End Dump");
        return bs;
    }
};

class DoRmExits : public Transform {
    ReferenceMap*        refMap;
    TypeMap*             typeMap;

 public:
    DoRmExits(ReferenceMap* refMap, TypeMap* typeMap)
            : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap); CHECK_NULL(typeMap);
        setName("DoRmExits");
    }

    const IR::Node* preorder(IR::P4Control* control) override;
};

// RemoveAllUnusedDeclarations is called before rmExits so that
// any global action is folded in first into a control before exits
// are checked.
class RmExits : public PassManager {
 public:
    RmExits(ReferenceMap* refMap, TypeMap* typeMap) {
            passes.push_back(new TypeChecking(refMap, typeMap));
            passes.push_back(new RemoveAllUnusedDeclarations(refMap, true));
            passes.push_back(new TypeChecking(refMap, typeMap));
            passes.push_back(new DoRmExits(refMap, typeMap));
        setName("RmExits");
    }
};

}  // namespace P4

#endif /* _FRONTENDS_P4_RMEXIT_H_ */
