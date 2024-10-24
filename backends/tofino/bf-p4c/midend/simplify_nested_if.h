/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * \defgroup SimplifyNestedIf P4::SimplifyNestedIf
 * \ingroup midend
 * \ingroup parde
 * \brief Set of passes that simplify nested if statements in the
 *        deparser control block.
 */
#include <stack>

#include "bf-p4c/device.h"
#include "bf-p4c/midend/path_linearizer.h"
#include "bf-p4c/midend/type_categories.h"
#include "bf-p4c/midend/type_checker.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/pattern.h"

#ifndef _BACKENDS_TOFINO_BF_P4C_MIDEND_SIMPLIFY_NESTED_IF_H_
#define _BACKENDS_TOFINO_BF_P4C_MIDEND_SIMPLIFY_NESTED_IF_H_

namespace P4 {

/**
 * \ingroup SimplifyNestedIf
 */
class SkipControlPolicy {
 public:
    virtual ~SkipControlPolicy() {}
    /**
       If the policy returns true the control block is processed,
       otherwise it is left unchanged.
    */
    virtual bool convert(const IR::P4Control *control) const = 0;
};

/**
 * \ingroup SimplifyNestedIf
 */
class ProcessDeparser : public SkipControlPolicy {
 public:
    ProcessDeparser() {}
    bool convert(const IR::P4Control *control) const override {
        return control->is<IR::BFN::TnaDeparser>();
    }
};

/**
 * \ingroup SimplifyNestedIf
 */
class DoSimplifyNestedIf : public Transform {
    SkipControlPolicy *policy;
    ordered_map<const IR::Statement *, std::vector<const IR::Expression *>> predicates;
    std::vector<const IR::Expression *> stack_;
    std::vector<int> *mirrorType, *resubmitType, *digestType;
    std::map<const IR::Statement *, std::vector<const IR::Expression *>> extraStmts;

 public:
    explicit DoSimplifyNestedIf(SkipControlPolicy *policy) : policy(policy) {
        int size = 16;
        if (Device::currentDevice() == Device::TOFINO) {
            size = 8;
        }
        mirrorType = new std::vector<int>(size);
        resubmitType = new std::vector<int>(size);
        digestType = new std::vector<int>(size);
        CHECK_NULL(policy);
    }

    const IR::Node *preorder(IR::IfStatement *statement) override;
    const IR::Node *preorder(IR::P4Control *control) override;
    void setExtraStmts(std::vector<int> *typeVec, const IR::Statement *stmt,
                       const IR::Expression *condition);
    std::vector<int> *checkTypeAndGetVec(const IR::Statement *stmt);
    void addInArray(std::vector<int> *typeVec, const IR::Expression *condition,
                    const IR::IfStatement *ifstmt);
};

/**
 * \ingroup SimplifyNestedIf
 */
class SimplifyComplexConditionPolicy {
 public:
    virtual ~SimplifyComplexConditionPolicy() {}
    virtual void reset() = 0;
    virtual bool check(const IR::Expression *) = 0;
};

/**
 * \ingroup SimplifyNestedIf
 */
class UniqueAndValidDest : public SimplifyComplexConditionPolicy {
    ReferenceMap *refMap;
    TypeMap *typeMap;
    const std::set<cstring> *valid_fields;
    std::set<cstring> unique_fields;

 public:
    UniqueAndValidDest(ReferenceMap *refMap, TypeMap *typeMap,
                       const std::set<cstring> *valid_fields)
        : refMap(refMap), typeMap(typeMap), valid_fields(valid_fields) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        CHECK_NULL(valid_fields);
    }

    void reset() override { unique_fields.clear(); }
    bool check(const IR::Expression *dest) override {
        BFN::PathLinearizer path;
        dest->apply(path);
        if (!path.linearPath) {
            error("Destination %1% is too complex ", dest);
            return false;
        }

        auto *param = BFN::getContainingParameter(*path.linearPath, refMap);
        auto *paramType = typeMap->getType(param);
        if (!BFN::isIntrinsicMetadataType(paramType)) {
            error("Destination %1% must be intrinsic metadata ", dest);
            return false;
        }

        if (auto mem = path.linearPath->components[0]->to<IR::Member>()) {
            if (!valid_fields->count(mem->member.name)) {
                error(
                    "Invalid field name %1%, the valid fields to use are "
                    "digest_type, resubmit_type and mirror_type");
            }
        }

        unique_fields.insert(path.linearPath->to_cstring());

        if (unique_fields.size() != 1) return false;

        return true;
    }
};

/**
 * \ingroup SimplifyNestedIf
 * \brief Pass that tries to simplify the conditions to a simple comparison
 *        of constants.
 *
 * Tofino does not support complex condition on if statements in
 * deparser. This pass tries to simplify the conditions to a
 * simple comparison to constant, e.g.:
 *
 *     if (intrinsic_md.mirror_type == 1) {}
 *
 */
class DoSimplifyComplexCondition : public Transform {
    SimplifyComplexConditionPolicy *policy;
    SkipControlPolicy *skip;

    bitvec constants;
    std::stack<const IR::Expression *> stack_;
    const IR::Expression *unique_dest = nullptr;

 public:
    DoSimplifyComplexCondition(SimplifyComplexConditionPolicy *policy, SkipControlPolicy *skip)
        : policy(policy), skip(skip) {
        CHECK_NULL(policy);
        CHECK_NULL(skip);
    }

    void do_equ(bitvec &val, const IR::Equ *eq);
    void do_neq(bitvec &val, const IR::Neq *neq);
    const IR::Node *preorder(IR::LAnd *expr) override;
    const IR::Node *preorder(IR::P4Control *control) override;
};

/**
 * \ingroup SimplifyNestedIf
 * \brief Top level PassManager that governs simplification of
 *        nested if statements in the deparser control block.
 */
class SimplifyNestedIf : public PassManager {
 public:
    SimplifyNestedIf(ReferenceMap *refMap, TypeMap *typeMap, TypeChecking *typeChecking = nullptr) {
        std::set<cstring> valid_fields;
        valid_fields = {"digest_type"_cs, "resubmit_type"_cs, "mirror_type"_cs};
        auto policy = new UniqueAndValidDest(refMap, typeMap, &valid_fields);
        auto skip = new ProcessDeparser();
        if (!typeChecking) typeChecking = new TypeChecking(refMap, typeMap);
        passes.push_back(typeChecking);
        passes.push_back(new DoSimplifyNestedIf(skip));
        passes.push_back(new StrengthReduction(typeMap, typeChecking));
        passes.push_back(new SimplifyControlFlow(typeMap, typeChecking));
        passes.push_back(new DoSimplifyComplexCondition(policy, skip));
        passes.push_back(new BFN::TypeChecking(refMap, typeMap, true));
        setName("SimplifyNestedIf");
    }
};

}  // namespace P4

#endif /* _BACKENDS_TOFINO_BF_P4C_MIDEND_SIMPLIFY_NESTED_IF_H_ */
