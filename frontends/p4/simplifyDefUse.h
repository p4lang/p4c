/*
Copyright 2016 VMware, Inc.

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

#ifndef FRONTENDS_P4_SIMPLIFYDEFUSE_H_
#define FRONTENDS_P4_SIMPLIFYDEFUSE_H_

#include "frontends/p4/cloner.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace P4 {

class DoSimplifyDefUse : public Transform {
    ReferenceMap *refMap;
    TypeMap *typeMap;

    const IR::Node *process(const IR::Node *node);

 public:
    DoSimplifyDefUse(ReferenceMap *refMap, TypeMap *typeMap) : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        setName("DoSimplifyDefUse");
    }

    const IR::Node *postorder(IR::Function *function) override {
        if (findContext<IR::Declaration_Instance>() == nullptr)
            // not an abstract function implementation: these
            // are processed as part of the control body
            return process(function);
        return function;
    }
    const IR::Node *postorder(IR::P4Parser *parser) override { return process(parser); }
    const IR::Node *postorder(IR::P4Control *control) override { return process(control); }
};

/// The Cloner pass below may insert @hidden annotations
/// on empty control blocks; remove them
class RemoveHidden : public Transform {
    const IR::Node *postorder(IR::BlockStatement *stat) override {
        if (!stat->components.empty()) return stat;
        if (stat->annotations->size() != 1) return stat;
        auto anno = stat->annotations->getSingle(IR::Annotation::hiddenAnnotation);
        if (!anno) return stat;
        // Lose the annotation.
        return new IR::BlockStatement(stat->srcInfo);
    }
};

class SimplifyDefUse : public PassManager {
    class Cloner : public ClonePathExpressions {
     public:
        Cloner() { setName("Cloner"); }
        const IR::Node *postorder(IR::EmptyStatement *stat) override {
            // You cannot clone an empty statement, since
            // the visitor claims it's equal to the original one.
            // So we convert it to an empty block.
            return new IR::BlockStatement(stat->srcInfo);
        }
        const IR::Node *postorder(IR::BlockStatement *stat) override {
            // If the block statement is empty then we need to clone
            // it and add an new annotation to force it to be
            // different from the original one.
            if (stat->components.empty()) {
                auto annos = new IR::Annotations();
                // We are losing the original annotations, but hopefully these don't
                // matter on an empty block.
                annos->add(new IR::Annotation(IR::Annotation::hiddenAnnotation, {}));
                auto result = new IR::BlockStatement(stat->srcInfo, annos);
                LOG2("Cloning " << getOriginal()->id << " into " << result->id);
                return result;
            }
            // Ideally we'd like ClonePathExpressions::postorder(stat),
            // but that doesn't work.
            return Transform::postorder(stat);
        }
    };

 public:
    SimplifyDefUse(ReferenceMap *refMap, TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);

        // SimplifyDefUse needs the expression tree *not* to be a DAG,
        // because it keeps state in hash-maps indexed with PathExpressions.
        // This is achieved by Cloner.
        passes.push_back(new Cloner());
        if (!typeChecking) typeChecking = new TypeChecking(refMap, typeMap);

        auto repeated = new PassRepeated({typeChecking, new DoSimplifyDefUse(refMap, typeMap)});
        passes.push_back(repeated);
        passes.push_back(new RemoveHidden());
        setName("SimplifyDefUse");
    }
};

}  // namespace P4

#endif /* FRONTENDS_P4_SIMPLIFYDEFUSE_H_ */
