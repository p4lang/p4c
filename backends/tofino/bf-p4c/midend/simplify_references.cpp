/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "simplify_references.h"

#include "bf-p4c/common/utils.h"
#include "bf-p4c/midend/copy_header.h"  // ENABLE_P4C3251
#include "bf-p4c/midend/param_binding.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeMap.h"

namespace {

class ApplyParamBindings : public Transform {
    ParamBinding *bindings;
    const P4::ReferenceMap *refMap;
    const IR::Node *preorder(IR::Type_Parser *n) override {
        prune();
        return n;
    }
    const IR::Node *preorder(IR::Type_Control *n) override {
        prune();
        return n;
    }
    const IR::Expression *postorder(IR::PathExpression *pe) override;
    const IR::Expression *postorder(IR::Member *mem) override;
    const IR::Node *postorder(IR::MAU::SaluAction *salu) override;

 public:
    ApplyParamBindings(ParamBinding *bindings, const P4::ReferenceMap *refMap)
        : bindings(bindings), refMap(refMap) {}
};

const IR::Expression *ApplyParamBindings::postorder(IR::PathExpression *pe) {
    if (auto decl = refMap->getDeclaration(pe->path)) {
        if (auto param = decl->to<IR::Parameter>()) {
            if (auto ref = bindings->get(param)) {
                LOG2("binding " << pe << " to " << ref);
                return ref;
            } else {
                LOG3("no binding for " << param->name);
            }
        } else if (auto var = decl->to<IR::Declaration_Variable>()) {
            if (auto ref = bindings->get(var)) {
                LOG3("return existing instance for var " << var->name);
                return ref;
            } else {
                LOG2("creating instance for var " << var->name);
                bindings->bind(var);
                return bindings->get(var);
            }
        } else {
            LOG4(decl << " is not a parameter");
        }
    } else {
        LOG3("nothing in blockMap for " << pe);
    }
    return pe;
}

const IR::Expression *ApplyParamBindings::postorder(IR::Member *mem) {
    if (auto iref = mem->expr->to<IR::V1InstanceRef>()) {
        if ((iref = iref->nested.get<IR::V1InstanceRef>(mem->member))) {
            LOG2("collapsing " << mem << " to " << iref);
            return iref;
        }
        LOG3("not collapsing " << mem << " (no nested iref)");
    } else if (auto iref = mem->expr->to<IR::InstanceRef>()) {
        if ((iref = iref->nested.get<IR::InstanceRef>(iref->name + "." + mem->member))) {
            LOG2("collapsing " << mem << " to " << iref);
            return iref;
        }
        LOG3("not collapsing " << mem << " (no nested iref)");
    } else {
        LOG3("not collapsing " << mem << " (not an iref)");
    }
    return mem;
}

const IR::Node *ApplyParamBindings::postorder(IR::MAU::SaluAction *salu) {
    // Also transform p4func for the benefit of p4v.
    visit(salu->p4func, "p4func");

    return Transform::postorder(salu);
}

class SplitComplexInstanceRef : public Transform {
    profile_t init_apply(const IR::Node *root) override { return Transform::init_apply(root); }
    const IR::Node *preorder(IR::MethodCallStatement *prim) override;
};

/**
 * Split extern method calls that refer to a complex objects (non-header structs or stacks)
 * such calls need to be split to refer to each field or element of the stack.
 * These are (currently) just 'extract' and 'emit' methods.  They need to be split to
 * only extract a single header at a time.
 * We do this as a perorder function so after splitting, we'll recursively visit the split
 * children, splitting further as needed.
 */

const IR::Node *SplitComplexInstanceRef::preorder(IR::MethodCallStatement *mc) {
    if (mc->methodCall->arguments->size() == 0) return mc;
    auto dest = mc->methodCall->arguments->at(0)->expression->to<IR::InstanceRef>();
    if (!dest) return mc;
    if (dest->obj != nullptr && dest->obj->is<IR::HeaderStack>()) {
        auto hs = dest->obj->to<IR::HeaderStack>();
        auto *rv = new IR::BlockStatement;
        for (int idx = 0; idx < hs->size; ++idx) {
            auto *split = mc->methodCall->clone();
            auto *args = split->arguments->clone();
            split->arguments = args;
            for (auto &op : *args)
                if (auto ir = op->expression->to<IR::InstanceRef>())
                    if (ir->obj == hs)
                        op =
                            new IR::Argument(new IR::HeaderStackItemRef(ir, new IR::Constant(idx)));
            rv->components.push_back(new IR::MethodCallStatement(mc->srcInfo, split));
        }
        return rv;
    } else if (!dest->nested.empty()) {
        auto *rv = new IR::BlockStatement;
        for (auto nest : dest->nested) {
            auto *split = mc->methodCall->clone();
            auto *args = split->arguments->clone();
            split->arguments = args;
            for (auto &op : *args)
                if (auto ir = op->expression->to<IR::InstanceRef>())
                    op = new IR::Argument(ir->nested.at(nest.first));
            rv->components.push_back(new IR::MethodCallStatement(mc->srcInfo, split));
        }
        return rv;
    }
    return mc;
}

class RemoveInstanceRef : public Transform {
    std::map<cstring, const IR::Expression *> created_tempvars;

 public:
    RemoveInstanceRef() : RemoveInstanceRef(std::map<cstring, const IR::Expression *>()) {}
    explicit RemoveInstanceRef(std::map<cstring, const IR::Expression *> created_tempvars)
        : created_tempvars(created_tempvars) {
        dontForwardChildrenBeforePreorder = true;
    }
    const IR::Expression *preorder(IR::InstanceRef *ir) override {
        if (!ir->obj) {
            const IR::Expression *rv = nullptr;
            if (created_tempvars.count(ir->name.name)) {
                rv = created_tempvars.at(ir->name.name);
                LOG2("RemoveInstanceRef existing TempVar " << ir->name.name);
            } else {
                created_tempvars[ir->name.name] = rv = new IR::TempVar(ir->type, ir->name.name);
                LOG2("RemoveInstanceRef new TempVar " << ir->name.name);
            }
            return rv;
        } else if (!ir->obj->is<IR::HeaderStack>()) {
            LOG2("RemoveInstanceRef new ConcreteheaderRef " << ir->obj);
            return new IR::ConcreteHeaderRef(ir->obj);
        } else {
            LOG2("RemoveInstanceRef not removing " << ir->name.name);
            return ir;
        }
    }

    const IR::Node *postorder(IR::MAU::SaluAction *salu) override {
        // Also transform p4func for the benefit of p4v.
        visit(salu->p4func, "p4func");

        return Transform::postorder(salu);
    }
};

#if ENABLE_P4C3251
// Implemention consolidate in bf-p4c/midend/copy_header.cpp
#else
/**
 * Remove calls to the `isValid()`, `setValid()`, and `setInvalid()` methods on
 * headers and replace them with references to the header's `$valid` POV bit
 * field.
 *
 * TODO: It would be nicer to deal with this in terms of the
 * frontend/midend IR, because then we could rerun type checking and resolve
 * references to make sure everything is still correct. Doing it here feels a
 * bit hacky by comparison. This is considerably simpler, though, so it makes
 * sense as a first step.
 *
 * FIXME(CTD) -- this functionality is duplicated in ConvertMethodCall in extract_maupipe.cpp
 * because that runs before this pass for action bodies, and it converts them to backend
 * form (IR::MAU::TypedPrimitives), which this code cannot handle (and ignores).
 */
struct SimplifyHeaderValidMethods : public Transform {
    const IR::Expression *preorder(IR::MethodCallExpression *call) override {
        auto *method = call->method->to<IR::Member>();
        if (!method) return call;
        if (method->member != "isValid") return call;
        return replaceWithPOVRead(call, method);
    }

    const IR::Statement *preorder(IR::MethodCallStatement *statement) override {
        auto *call = statement->methodCall;
        auto *method = call->method->to<IR::Member>();
        if (!method) return statement;

        if (method->member == "setValid")
            return replaceWithPOVWrite(statement, method, 1);
        else if (method->member == "setInvalid")
            return replaceWithPOVWrite(statement, method, 0);
        else if (method->member == "isValid")
            // "bare" call to isValid (ignoring return value) is a noop
            return nullptr;
        return statement;
    }

    const IR::Expression *replaceWithPOVRead(const IR::MethodCallExpression *call,
                                             const IR::Member *method) {
        BUG_CHECK(call->arguments->size() == 0, "Wrong number of arguments for method call: %1%",
                  call);
        auto *target = method->expr;
        BUG_CHECK(target != nullptr, "Method has no target: %1%", call);
        BUG_CHECK(target->type->is<IR::Type_Header>(), "Invoking isValid() on unexpected type %1%",
                  target->type);

        // On Tofino, calling a header's `isValid()` method is implemented by
        // reading the header's POV bit, which is a simple bit<1> value.
        const cstring validField = "$valid"_cs;
        auto *member = new IR::Member(call->srcInfo, target, validField);
        member->type = IR::Type::Bits::get(1);

        // If isValid() is being used as a table key element, it already behaves
        // like a bit<1> value; just replace it with a reference to the valid bit.
        if (getParent<IR::MAU::TableKey>() != nullptr) return member;

        // In other contexts, rewrite isValid() into a comparison with a constant.
        // This maintains a boolean type for the overall expression.
        auto *constant = new IR::Constant(IR::Type::Bits::get(1), 1);
        auto *result = new IR::Equ(call->srcInfo, member, constant);
        result->type = IR::Type::Boolean::get();
        return result;
    }

    const IR::Statement *replaceWithPOVWrite(const IR::MethodCallStatement *statement,
                                             const IR::Member *method, unsigned value) {
        BUG_CHECK(statement->methodCall->arguments->size() == 0,
                  "Wrong number of arguments for method call: %1%", statement);
        auto *target = method->expr;
        BUG_CHECK(target != nullptr, "Method has no target: %1%", statement);
        BUG_CHECK(target->type->is<IR::Type_Header>(), "Invoking isValid() on unexpected type %1%",
                  target->type);

        // On Barefoot architectures, calling a header's `setValid()` and
        // `setInvalid()` methods is implemented by write the header's POV bit,
        // which is a simple bit<1> value.
        const cstring validField = "$valid"_cs;
        auto *member = new IR::Member(statement->srcInfo, target, validField);
        member->type = IR::Type::Bits::get(1);

        // Rewrite the method call into an assignment with the same effect.
        auto *constant = new IR::Constant(IR::Type::Bits::get(1), value);
        return new IR::AssignmentStatement(statement->srcInfo, member, constant);
    }
};
#endif

class ConvertIndexToHeaderStackItemRef : public Transform {
    const IR::Expression *preorder(IR::ArrayIndex *idx) override {
        auto type = idx->type->to<IR::Type_Header>();
        if (!type) BUG("%1% is not a header stack ref", idx->type);
        return new IR::HeaderStackItemRef(idx->srcInfo, type, idx->left, idx->right);
    }

    const IR::Expression *preorder(IR::Member *member) override {
        auto type = member->type->to<IR::Type_Header>();
        if (!type) return member;
        if (member->member == "next" || member->member == "last")
            return new IR::HeaderStackItemRef(
                member->srcInfo, type, member->expr,
                new IR::BFN::UnresolvedHeaderStackIndex(member->member));
        return member;
    }
};

}  // namespace

SimplifyReferences::SimplifyReferences(ParamBinding *bindings, P4::ReferenceMap *refMap,
                                       P4::TypeMap *) {
    addPasses({new ApplyParamBindings(bindings, refMap), new SplitComplexInstanceRef(),
               new RemoveInstanceRef,
#if !ENABLE_P4C3251
               new SimplifyHeaderValidMethods,
#endif
               new ConvertIndexToHeaderStackItemRef});
}
