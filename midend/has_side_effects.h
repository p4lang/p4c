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

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/methodInstance.h"
#include "ir/ir.h"

namespace P4 {

/* Should this be a method on IR::Expression?  Maybe after the refMap/typeMap go away */

class hasSideEffects : public Inspector, public ResolutionContext {
    P4::TypeMap *typeMap = nullptr;

    bool result = false;
    bool preorder(const IR::AssignmentStatement *) override {
        result = true;
        return false;
    }
    bool preorder(const IR::MethodCallExpression *mc) override {
        if (result) return false;

        /* assume has side effects if we can't look it up */
        if (typeMap) {
            auto *mi = P4::MethodInstance::resolve(mc, this, typeMap, true);
            if (auto *bm = mi->to<P4::BuiltInMethod>()) {
                if (bm->name == IR::Type_Header::isValid) return true;
            } else if (auto *em = mi->to<P4::ExternMethod>()) {
                if (em->method->hasAnnotation(IR::Annotation::noSideEffectsAnnotation)) return true;
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
    hasSideEffects(P4::TypeMap *tm, const IR::Expression *e, const Visitor::Context *ctxt)
        : typeMap(tm) {
        e->apply(*this, ctxt);
    }
    bool operator()(const IR::Expression *e) {
        result = false;
        e->apply(*this);
        return result;
    }
    explicit operator bool() { return result; }
};

}  // namespace P4

#endif /* MIDEND_HAS_SIDE_EFFECTS_H_ */
