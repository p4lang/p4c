/*
 * Copyright 2013-present Barefoot Networks, Inc.
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
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
    bool preorder(const IR::BaseAssignmentStatement *) override {
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
