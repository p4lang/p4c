/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "persistToReg.h"

#include <map>

#include "frontends/common/resolveReferences/referenceMap.h"  // for MinimalNameGenerator
#include "ir/ir.h"
#include "ir/irutils.h"

namespace P4 {

using namespace literals;

class FindPersist : public Inspector {
 public:
    struct info_t {
        const IR::Declaration_Variable *var;
        const IR::Type *eltype = nullptr;
        const IR::Type::Bits *idx_type = nullptr;
        int regsize = 0;
    };

 private:
    std::map<cstring, info_t> persist;
    profile_t init_apply(const IR::Node *root) {
        persist.clear();
        return Inspector::init_apply(root);
    }
    bool preorder(const IR::Declaration_Variable *decl) {
        if (!decl->hasAnnotation(IR::Annotation::persistentAnnotation)) return false;
        LOG2("FindPersist: " << decl);
        auto &info = persist[decl->name.name];
        info.var = decl;
        if (auto *st = decl->type->to<IR::Type_Array>()) {
            info.eltype = st->elementType;
            auto *size = st->size->to<IR::Constant>();
            BUG_CHECK(size, "Array size not constant: %s", decl);
            info.regsize = size->asInt();
        } else {
            info.eltype = decl->type;
            info.regsize = 1;
        }
        return true;
    }
    bool preorder(const IR::ArrayIndex *ai) {
        auto *pe = ai->left->to<IR::PathExpression>();
        auto *itype = ai->right->type->to<IR::Type::Bits>();
        if (pe && itype) {
            if (auto *info = at(pe->path->name)) {
                if (!info->idx_type && (itype->size > 30 || (1 << itype->size) >= info->regsize))
                    info->idx_type = itype;
            }
        }
        return true;
    }
    void end_apply() {
        for (auto &[_, info] : persist)
            if (!info.idx_type)
                info.idx_type = IR::Type::Bits::get(std::max(ceil_log2(info.regsize), 1));
    }

 public:
    FindPersist() {}
    bool empty() const { return persist.empty(); }
    info_t *at(cstring name) {
        if (auto i = persist.find(name); i != persist.end()) return &i->second;
        return nullptr;
    }
    const info_t *at(cstring name) const {
        if (auto i = persist.find(name); i != persist.end()) return &i->second;
        return nullptr;
    }
    const info_t *at(const IR::Expression *expr) const {
        if (auto *ai = expr->to<IR::ArrayIndex>()) return at(ai->left);
        if (auto *pe = expr->to<IR::PathExpression>()) return at(pe->path->name);
        return nullptr;
    }
};

class RewritePersistAsReg : public Transform, public P4WriteContext {
    cstring regExtern;
    const FindPersist *persist;
    MinimalNameGenerator *nameGen;
    const IR::Type *regSizeType;
    const IR::Method *readMethod, *writeMethod;
    std::map<cstring, const IR::Declaration_Instance *> regDecls;
    IR::IndexedVector<IR::StatOrDecl> prependToCurrentStmt;

    IR::P4Program *preorder(IR::P4Program *top) {
        const IR::Type_Extern *regType = nullptr;
        for (auto *decl : *top->getDeclsByName(regExtern)) {
            BUG_CHECK(!regType, "Multiple %s declarations%s%s", regExtern, regType->srcInfo,
                      decl->getNode()->srcInfo);
            regType = decl->to<IR::Type_Extern>();
        }
        BUG_CHECK(regType, "No %s extern", regExtern);
        for (auto *method : regType->methods) {
            if (method->name == regExtern) {  // constructor
                regSizeType = method->getParameters()->getParameter(0)->type;
            } else if (method->name == "read"_cs) {
                readMethod = method;
            } else if (method->name == "write"_cs) {
                writeMethod = method;
            }
        }
        prependToCurrentStmt.clear();
        return top;
    }

    IR::Declaration *preorder(IR::Declaration_Variable *decl) {
        prune();
        if (!decl->hasAnnotation(IR::Annotation::persistentAnnotation)) return decl;
        auto *info = persist->at(decl->name.name);
        BUG_CHECK(info, "persist mismatch ", decl);
        auto *types = new IR::Vector<IR::Type>({info->eltype, info->idx_type});
        auto *type = new IR::Type_Specialized(new IR::Type_Name(IR::ID(regExtern)), types);
        auto *args = new IR::Vector<IR::Argument>(
            {new IR::Argument(new IR::Constant(regSizeType, info->regsize))});
        auto *rv =
            new IR::Declaration_Instance(decl->srcInfo, decl->name, decl->annotations, type, args);
        bool ok = regDecls.emplace(decl->name.name, rv).second;
        BUG_CHECK(ok, "duplicate regster %s", rv);
        return rv;
    }

    const IR::MethodCallExpression *makeCall(const Util::SourceInfo &srcInfo,
                                             const FindPersist::info_t *info,
                                             const IR::Method *method, const IR::Expression *index,
                                             const IR::Expression *value) {
        auto *reg = regDecls.at(info->var->name);
        auto *args = new IR::Vector<IR::Argument>;
        for (auto *param : method->getParameters()->parameters) {
            if (param->name == "index"_cs) {
                if (!index) index = new IR::Constant(info->idx_type, 0);
                if (param->type->is<IR::Type::Bits>() && index->type != param->type)
                    index = new IR::Cast(param->type, index);
                args->emplace_back(param->name, index);
            } else if (param->name == "value"_cs || param->name == "result"_cs) {
                BUG_CHECK(value, "Value not set");
                if (!param->type->is<IR::Type_Name>() && value->type != param->type)
                    value = new IR::Cast(param->type, value);
                args->emplace_back(param->name, value);
            } else {
                BUG("unknown parameter %s to %s.%s", param->name, regExtern, method->name);
            }
        }
        auto *rtype = method->type->returnType;
        if (rtype->is<IR::Type_Name>() || rtype->is<IR::Type_Var>()) rtype = info->eltype;
        return new IR::MethodCallExpression(
            srcInfo, rtype,
            new IR::Member(method->type, new IR::PathExpression(reg->type, info->var->name),
                           method->name),
            args);
    }

    cstring tempName(cstring reg) { return nameGen->newName(reg + "_tmp"); }

    // rebuild the LHS of an assignment by taking the expression parts from lhs that are 'above'
    // old and relayer them on rv and return that
    const IR::Expression *makeLHS(const IR::Expression *rv, const IR::Expression *old,
                                  const IR::Expression *lhs) {
        if (old == lhs) return rv;
        if (auto *sl = lhs->to<IR::AbstractSlice>()) {
            auto *n = sl->clone();
            n->e0 = makeLHS(rv, old, n->e0);
            return n;
        }
        if (auto *m = lhs->to<IR::Member>()) {
            auto *n = m->clone();
            n->expr = makeLHS(rv, old, n->expr);
            return n;
        }
        BUG("unexpected lhs %s", lhs);
    }

    const IR::Node *preorder(IR::BaseAssignmentStatement *assign) {
        auto *exp = assign->left;
        while (1) {
            if (auto *sl = exp->to<IR::AbstractSlice>())
                exp = sl->e0;
            else if (auto *m = exp->to<IR::Member>())
                exp = m->expr;
            else
                break;
        }
        auto *info = persist->at(exp);
        if (!info) return assign;
        bool needCopy = exp != assign->left || assign->is<IR::OpAssignmentStatement>();
        auto *val = assign->right;
        const IR::Expression *index = nullptr;
        if (auto *ai = exp->to<IR::ArrayIndex>()) index = ai->right;
        IR::IndexedVector<IR::StatOrDecl> rv;
        if (needCopy) {
            auto tmpvar = tempName(info->var->name.toString());
            rv.push_back(new IR::Declaration_Variable(tmpvar, info->eltype));
            if (readMethod->type->returnType->is<IR::Type_Void>()) {
                rv.push_back(new IR::MethodCallStatement(
                    makeCall(exp->srcInfo, info, readMethod, index,
                             new IR::PathExpression(info->eltype, tmpvar))));
            } else {
                rv.push_back(new IR::AssignmentStatement(
                    exp->srcInfo, new IR::PathExpression(info->eltype, tmpvar),
                    makeCall(exp->srcInfo, info, readMethod, index, 0)));
            }
            assign->left = makeLHS(new IR::PathExpression(info->eltype, tmpvar), exp, assign->left);
            rv.push_back(assign);
            val = new IR::PathExpression(info->eltype, tmpvar);
        }
        auto *mcs =
            new IR::MethodCallStatement(makeCall(assign->srcInfo, info, writeMethod, index, val));
        rv.push_back(mcs);
        return inlineBlock(*this, std::move(rv));
    }

    const IR::Expression *doRead(const IR::Expression *exp, const IR::Expression *index) {
        if (auto *info = persist->at(exp)) {
            if (readMethod->type->returnType->is<IR::Type_Void>()) {
                auto tmpvar = tempName(info->var->name.toString());
                prependToCurrentStmt.push_back(new IR::Declaration_Variable(tmpvar, info->eltype));
                prependToCurrentStmt.push_back(new IR::MethodCallStatement(
                    makeCall(exp->srcInfo, info, readMethod, index,
                             new IR::PathExpression(info->eltype, tmpvar))));
                return new IR::PathExpression(info->eltype, tmpvar);
            } else {
                return makeCall(exp->srcInfo, info, readMethod, index, nullptr);
            }
        }
        return exp;
    }

    const IR::Expression *preorder(IR::ArrayIndex *ai) { return doRead(ai, ai->right); }
    const IR::Expression *preorder(IR::PathExpression *pe) { return doRead(pe, nullptr); }
    const IR::Member *preorder(IR::Member *m) {
        // need to avoid recursing into the member function calls created by makeCall
        if (m->type->is<IR::Type_Method>()) prune();
        return m;
    }

    const IR::Node *postorder(IR::Statement *stmt) {
        if (prependToCurrentStmt.empty()) return stmt;
        prependToCurrentStmt.push_back(stmt);
        auto *rv = inlineBlock(*this, std::move(prependToCurrentStmt));
        prependToCurrentStmt.clear();
        return rv;
    }

 public:
    RewritePersistAsReg(cstring re, const FindPersist *p, MinimalNameGenerator *ng)
        : regExtern(re), persist(p), nameGen(ng) {}
};

PersistToReg::PersistToReg(cstring regExtern) {
    auto *findPersist = new FindPersist;
    auto *nameGen = new MinimalNameGenerator;
    addPasses(
        {findPersist, PassIf([=]() { return !findPersist->empty(); },
                             {nameGen, new RewritePersistAsReg(regExtern, findPersist, nameGen)})});
}

}  // namespace P4
