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

#include "bf-p4c/arch/rewrite_action_selector.h"

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/mau/resource_estimate.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/methodInstance.h"

namespace BFN {

const IR::Node *RewriteActionSelector::postorder(IR::Declaration_Instance *di) {
    auto inst = P4::Instantiation::resolve(di, refMap, typeMap);
    if (auto tn = inst->instance->type->to<IR::Type_Name>()) {
        if (tn->path->name == "ActionSelector") {
            if (inst->substitution.lookupByName("size"_cs)) {
                auto instances = new IR::IndexedVector<IR::Declaration>();
                // generate ActionProfile(size) ap;
                auto type = new IR::Type_Name("ActionProfile");
                auto args = new IR::Vector<IR::Argument>();
                args->push_back(inst->substitution.lookupByName("size"_cs));
                auto ap = new IR::Declaration_Instance(di->srcInfo, di->name, di->annotations, type,
                                                       args);
                instances->push_back(ap);

                // generate ActionSelector(ap, hash, mode, gsize, ng);
                type = new IR::Type_Name("ActionSelector");
                auto ap_path = new IR::PathExpression(new IR::Path(IR::ID(di->name)));
                cstring sel = di->name + "_sel"_cs;
                args = new IR::Vector<IR::Argument>();
                args->push_back(new IR::Argument(ap_path));
                args->push_back(inst->substitution.lookupByName("hash"_cs));
                args->push_back(inst->substitution.lookupByName("mode"_cs));
                if (inst->substitution.lookupByName("reg"_cs))
                    args->push_back(inst->substitution.lookupByName("reg"_cs));
                args->push_back(new IR::Argument(new IR::Constant(
                    IR::Type_Bits::get(32), StageUseEstimate::SINGLE_RAMLINE_POOL_SIZE)));
                args->push_back(new IR::Argument(new IR::Constant(
                    IR::Type_Bits::get(32), StageUseEstimate::COMPILER_DEFAULT_SELECTOR_POOLS)));
                const IR::Annotations *annot = nullptr;
                if (auto ann = di->getAnnotation(IR::Annotation::nameAnnotation)) {
                    auto annName = ann->getName();
                    cstring name = annName + "_sel"_cs;
                    annot = di->annotations->addOrReplace(IR::Annotation::nameAnnotation,
                                                          new IR::StringLiteral(name));
                }
                auto as = new IR::Declaration_Instance(di->srcInfo, IR::ID(sel), annot, type, args);
                instances->push_back(as);
                return instances;
            }
        } else if (tn->path->name == "SelectorAction") {
            /**
             * Only apply if --arch is tna
             */
            if (BackendOptions().arch == "v1model") return di;
            auto sel = inst->substitution.lookupByName("sel"_cs);
            if (auto path = sel->expression->to<IR::PathExpression>()) {
                auto decl = refMap->getDeclaration(path->path);
                if (decl == nullptr) return di;
                if (auto selector = decl->to<IR::Declaration_Instance>()) {
                    auto sel_inst = P4::Instantiation::resolve(selector, refMap, typeMap);
                    // if selector was using the old syntax
                    if (sel_inst->substitution.lookupByName("size"_cs)) {
                        auto args = new IR::Vector<IR::Argument>();
                        cstring sel = path->path->name + "_sel"_cs;
                        args->push_back(
                            new IR::Argument(new IR::PathExpression(new IR::Path(IR::ID(sel)))));
                        auto sa =
                            new IR::Declaration_Instance(di->srcInfo, di->name, di->annotations,
                                                         di->type, args, di->initializer);
                        return sa;
                    }
                }
            }
        }
    }
    return di;
}

// rewrite 'implementation = as'
const IR::Node *RewriteActionSelector::postorder(IR::ExpressionValue *ev) {
    auto prop = findContext<IR::Property>();
    if (!prop) return ev;
    if (prop->name != "implementation") return ev;
    auto pval = ev->expression;
    if (auto pe = pval->to<IR::PathExpression>()) {
        auto &d = refMap->getDeclaration(pe->path, true)->as<IR::Declaration_Instance>();
        auto inst = P4::Instantiation::resolve(&d, refMap, typeMap);
        if (auto tn = inst->instance->type->to<IR::Type_Name>()) {
            if (tn->path->name != "ActionSelector") return ev;
            if (inst->substitution.lookupByName("size"_cs)) {
                cstring sel = pe->path->name + "_sel"_cs;
                auto ret =
                    new IR::ExpressionValue(ev->srcInfo, new IR::PathExpression(new IR::Path(sel)));
                return ret;
            }
        }
    }
    return ev;
}

const IR::Node *RewriteActionSelector::postorder(IR::ConstructorCallExpression *cce) {
    auto inst = P4::ExternInstance::resolve(cce, refMap, typeMap);
    if (inst == std::nullopt) return cce;
    if (inst->type->name == "ActionSelector") {
        if (inst->substitution.lookupByName("size"_cs)) {
            // generate ActionProfile(size) ap;
            auto type = new IR::Type_Name("ActionProfile");
            auto args = new IR::Vector<IR::Argument>();
            args->push_back(inst->substitution.lookupByName("size"_cs));
            auto ap = new IR::ConstructorCallExpression(cce->srcInfo, type, args);

            // generate ActionSelector(ap, hash, mode, gsize, ng);
            type = new IR::Type_Name("ActionSelector");
            args = new IR::Vector<IR::Argument>();
            args->push_back(new IR::Argument(ap));
            args->push_back(inst->substitution.lookupByName("hash"_cs));
            args->push_back(inst->substitution.lookupByName("mode"_cs));
            args->push_back(new IR::Argument(new IR::Constant(
                IR::Type_Bits::get(32), StageUseEstimate::SINGLE_RAMLINE_POOL_SIZE)));
            args->push_back(new IR::Argument(new IR::Constant(
                IR::Type_Bits::get(32), StageUseEstimate::COMPILER_DEFAULT_SELECTOR_POOLS)));
            auto as = new IR::ConstructorCallExpression(cce->srcInfo, type, args);
            return as;
        }
    } else if (inst->type->name == "SelectorAction") {
        auto sel = inst->substitution.lookupByName("sel"_cs);
        if (auto path = sel->expression->to<IR::PathExpression>()) {
            auto decl = refMap->getDeclaration(path->path);
            if (decl == nullptr) return cce;
            if (auto selector = decl->to<IR::Declaration_Instance>()) {
                auto sel_inst = P4::Instantiation::resolve(selector, refMap, typeMap);
                // if selector was using the old syntax
                if (sel_inst->substitution.lookupByName("size"_cs)) {
                    auto args = new IR::Vector<IR::Argument>();
                    cstring sel = path->path->name + "_sel"_cs;
                    args->push_back(
                        new IR::Argument(new IR::PathExpression(new IR::Path(IR::ID(sel)))));
                    auto sa = new IR::ConstructorCallExpression(cce->srcInfo, cce->type, args);
                    return sa;
                }
            }
        }
    }
    return cce;
}

}  // namespace BFN
