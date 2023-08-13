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

#ifndef MIDEND_HAS_SIDE_EFFECTS_H_
#define MIDEND_HAS_SIDE_EFFECTS_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

/* Should this be a method on IR::Expression?  Maybe after the refMap/typeMap go away */

class hasSideEffects : public Inspector {
    P4::ReferenceMap *refMap = nullptr;
    P4::TypeMap *typeMap = nullptr;

    bool result = false;
    bool preorder(const IR::AssignmentStatement *) override {
        result = true;
        return false;
    }
    bool preorder(const IR::MethodCallExpression *mc) override {
        if (result) {
            return false;
        }
        /* assume has side effects if we can't look it up */
        if (refMap && typeMap) {
            auto *mi = P4::MethodInstance::resolve(mc, refMap, typeMap, true);
            if (auto *bm = mi->to<P4::BuiltInMethod>()) {
                if (bm->name == "isValid") {
                    return true;
                }
            }
            if (auto *em = mi->to<P4::ExternMethod>()) {
                if (em->method->getAnnotation(IR::Annotation::noSideEffectsAnnotation)) return true;
            }
        }
        result = true;
        return false;
    }

    bool preorder(const IR::Primitive *) override {
        result = true;
        return false;
    }
    bool preorder(const IR::Expression *) override { return !result; }

 public:
    explicit hasSideEffects(const IR::Expression *e) { e->apply(*this); }
    hasSideEffects(P4::ReferenceMap *rm, P4::TypeMap *tm) : refMap(rm), typeMap(tm) {}
    hasSideEffects(P4::ReferenceMap *rm, P4::TypeMap *tm, const IR::Expression *e)
        : refMap(rm), typeMap(tm) {
        e->apply(*this);
    }
    bool operator()(const IR::Expression *e) {
        result = false;
        e->apply(*this);
        return result;
    }
    explicit operator bool() { return result; }
};

#endif /* MIDEND_HAS_SIDE_EFFECTS_H_ */
