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

#include "implicitReadWrite.h"

#include <map>

#include "frontends/common/resolveReferences/referenceMap.h"  // for MinimalNameGenerator
#include "frontends/p4/cloner.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"
#include "ir/irutils.h"

namespace P4 {

using namespace literals;

class RewriteImplicitReadWrite : public Transform, public P4WriteContext {
    TypeMap *typeMap;
    MinimalNameGenerator *nameGen;
    IR::IndexedVector<IR::StatOrDecl> prependToCurrentStmt, appendToCurrentStmt;

    IR::P4Program *preorder(IR::P4Program *top) {
        prependToCurrentStmt.clear();
        appendToCurrentStmt.clear();
        return top;
    }

    const IR::Type *getType(const IR::Expression *e) {
        auto *type = typeMap ? typeMap->getType(e, false) : nullptr;
        if (!type) type = e->type;
        if (auto *tsc = type->to<IR::Type_SpecializedCanonical>()) type = tsc->substituted;
        if (auto *ts = type->to<IR::Type_Specialized>()) type = ts->baseType;
        return type;
    }

    const IR::Method *readMethod(const IR::Type_Extern *ext) {
        for (auto *m : ext->methods) {
            if (!m->hasAnnotation("implicit"_cs)) continue;
            if (m->getParameters()->size() != 0) continue;
            if (m->type->returnType->is<IR::Type_Void>()) continue;
            return m;
        }
        return nullptr;
    }

    const IR::Method *writeMethod(const IR::Type_Extern *ext) {
        for (auto *m : ext->methods) {
            if (!m->hasAnnotation("implicit"_cs)) continue;
            if (m->getParameters()->size() != 1) continue;
            if (!m->type->returnType->is<IR::Type_Void>()) continue;
            return m;
        }
        return nullptr;
    }

    const IR::Expression *deepClone(const IR::Expression *e) {
        CloneExpressions cloner;
        return cloner.clone<IR::Expression>(e);
    }

    const IR::MethodCallExpression *makeCall(const Util::SourceInfo &srcInfo,
                                             const IR::Expression *receiver,
                                             const IR::Method *method,
                                             const IR::Expression *value = nullptr) {
        auto *args = new IR::Vector<IR::Argument>;
        BUG_CHECK(method->getParameters()->size() == (value != nullptr), "wrong number of args");
        if (value) {
            auto *param = method->getParameters()->getParameter(0);
            if (!param->type->is<IR::Type_Name>() && value->type != param->type)
                value = new IR::Cast(param->type, value);
            args->emplace_back(param->name, value);
        }
        auto *rtype = method->type->returnType;
        if (rtype->is<IR::Type_Name>() || rtype->is<IR::Type_Var>())
            rtype = typeMap->getTypeType(rtype, true);
        if (auto tt = rtype->to<IR::Type_Type>()) rtype = tt->type;
        return new IR::MethodCallExpression(
            srcInfo, rtype, new IR::Member(method->type, receiver, method->name), args);
    }

    cstring tempName(const IR::Expression *e, cstring base) {
        while (auto ai = e->to<IR::ArrayIndex>()) e = ai->left;
        if (auto pe = e->to<IR::PathExpression>()) base = pe->path->name.name;
        return nameGen->newName(base + "_tmp");
    }

    // rebuild the LHS of an assignment by taking the expression parts from lhs that are 'above'
    // old and relayer them on rv and return that
    const IR::Expression *makeLHS(const IR::Expression *rv, const IR::Expression *old,
                                  const IR::Expression *lhs) {
        if (old == lhs)
            return rv;
        else if (auto *sl = lhs->to<IR::AbstractSlice>()) {
            auto *n = sl->clone();
            n->e0 = makeLHS(rv, old, n->e0);
            return n;
        } else if (auto *m = lhs->to<IR::Member>()) {
            auto *n = m->clone();
            n->expr = makeLHS(rv, old, n->expr);
            return n;
        } else if (auto *ai = lhs->to<IR::ArrayIndex>()) {
            auto *n = ai->clone();
            n->left = makeLHS(rv, old, n->left);
            return n;
        }
        BUG("unexpected lhs %s", lhs);
    }

    // From an expression that accesses into data accessed implicitly via an extern, find
    // the child expression that refers to the extern and the extern type.
    std::pair<const IR::Expression *, const IR::Type_Extern *> findExt(const IR::Expression *exp) {
        const IR::Type_Extern *ext;
        while (!(ext = getType(exp)->to<IR::Type_Extern>())) {
            if (auto *sl = exp->to<IR::AbstractSlice>())
                exp = sl->e0;
            else if (auto *m = exp->to<IR::Member>())
                exp = m->expr;
            else if (auto *ai = exp->to<IR::ArrayIndex>())
                exp = ai->left;
            else
                break;
        }
        return std::make_pair(exp, ext);
    }

    const IR::Node *preorder(IR::BaseAssignmentStatement *assign) {
        auto [exp, ext] = findExt(assign->left);
        if (!ext) return assign;
        if (exp != assign->left || assign->is<IR::OpAssignmentStatement>()) {
            auto rmethod = readMethod(ext);
            auto rtype = rmethod->type->returnType;
            auto tmpvar = tempName(exp, ext->name);
            prependToCurrentStmt.push_back(new IR::Declaration_Variable(tmpvar, rtype));
            prependToCurrentStmt.push_back(
                new IR::AssignmentStatement(exp->srcInfo, new IR::PathExpression(exp->type, tmpvar),
                                            makeCall(exp->srcInfo, deepClone(exp), rmethod)));
            assign->left = makeLHS(new IR::PathExpression(rtype, tmpvar), exp, assign->left);
            appendToCurrentStmt.push_back(new IR::MethodCallStatement(makeCall(
                assign->srcInfo, exp, writeMethod(ext), new IR::PathExpression(rtype, tmpvar))));
            return assign;
        } else {
            return new IR::MethodCallStatement(
                makeCall(assign->srcInfo, exp, writeMethod(ext), assign->right));
        }
    }

    // this is doing almost the same thing as the assignment preorder above -- should be possible
    // to factor them better
    const IR::Expression *makeRMW(const IR::Expression *lhs, bool forceRMW) {
        auto [exp, ext] = findExt(lhs);
        if (!ext) return nullptr;
        if (exp != lhs || forceRMW) {
            auto rmethod = readMethod(ext);
            auto rtype = rmethod->type->returnType;
            auto tmpvar = tempName(exp, ext->name);
            prependToCurrentStmt.push_back(new IR::Declaration_Variable(tmpvar, rtype));
            prependToCurrentStmt.push_back(
                new IR::AssignmentStatement(exp->srcInfo, new IR::PathExpression(exp->type, tmpvar),
                                            makeCall(exp->srcInfo, deepClone(exp), rmethod)));
            lhs = makeLHS(new IR::PathExpression(rtype, tmpvar), exp, lhs);
            appendToCurrentStmt.push_back(new IR::MethodCallStatement(makeCall(
                lhs->srcInfo, exp, writeMethod(ext), new IR::PathExpression(rtype, tmpvar))));
            return lhs;
        } else {
            auto wmethod = writeMethod(ext);
            auto wtype = wmethod->getParameters()->getParameter(0)->type;
            auto tmpvar = tempName(exp, ext->name);
            prependToCurrentStmt.push_back(new IR::Declaration_Variable(tmpvar, wtype));
            appendToCurrentStmt.push_back(new IR::MethodCallStatement(
                makeCall(lhs->srcInfo, lhs, wmethod, new IR::PathExpression(wtype, tmpvar))));
            return new IR::PathExpression(wtype, tmpvar);
        }
    }

    const IR::Parameter *getParameter(const IR::Node *n, int idx) {
        if (auto *mce = n->to<IR::MethodCallExpression>()) {
            auto mt = getType(mce->method)->to<IR::Type_Method>();
            BUG_CHECK(mt, "calling non method?");
            return mt->parameters->getParameter(idx);
        } else {
            BUG("%s is not something callable", n);
        }
    }

    const IR::Expression *doRead(const IR::Expression *exp) {
        if (auto ext = getType(getOriginal<IR::Expression>())->to<IR::Type_Extern>()) {
            const Visitor::Context *ctxt = nullptr;
            if (findContext<IR::Argument>(ctxt)) {
                auto *param = getParameter(ctxt->parent->parent->node, ctxt->parent->child_index);
                BUG_CHECK(param, "param not found");
                if (param->direction == IR::Direction::None) return exp;
                if (param->direction != IR::Direction::In) {
                    if (auto *rv = makeRMW(exp, param->direction == IR::Direction::InOut))
                        return rv;
                }
            }
            prune();
            return makeCall(exp->srcInfo, exp, readMethod(ext));
        }
        return exp;
    }

    const IR::MethodCallExpression *preorder(IR::MethodCallExpression *mc) {
        // skip the method
        visit(mc->typeArguments, "typeArguments", 1);  // noop?
        visit(mc->arguments, "arguments", 2);
        prune();
        return mc;
    }
    const IR::Expression *preorder(IR::ArrayIndex *ai) { return doRead(ai); }
    const IR::Expression *preorder(IR::PathExpression *pe) { return doRead(pe); }

    const IR::Node *postorder(IR::Statement *stmt) {
        if (prependToCurrentStmt.empty() && appendToCurrentStmt.empty()) return stmt;
        prependToCurrentStmt.push_back(stmt);
        prependToCurrentStmt.append(appendToCurrentStmt);
        auto *rv = inlineBlock(*this, std::move(prependToCurrentStmt));
        prependToCurrentStmt.clear();
        appendToCurrentStmt.clear();
        return rv;
    }

    // these may contain references to externs (as PathExpression), but will never be "read"s
    // so we prune them here so doRead doesn't need to worry about them
    const IR::Declaration_Instance *preorder(IR::Declaration_Instance *di) {
        prune();
        return di;
    }
    const IR::ConstructorCallExpression *preorder(IR::ConstructorCallExpression *cc) {
        prune();
        return cc;
    }
    const IR::P4Table *preorder(IR::P4Table *tbl) {
        prune();
        return tbl;
    }

 public:
    RewriteImplicitReadWrite(TypeMap *tm, MinimalNameGenerator *ng) : typeMap(tm), nameGen(ng) {}
};

ImplicitReadWrite::ImplicitReadWrite(TypeMap *typeMap) {
    auto *nameGen = new MinimalNameGenerator;
    addPasses({typeMap ? new P4::TypeChecking(nullptr, typeMap) : nullptr, nameGen,
               new RewriteImplicitReadWrite(typeMap, nameGen)});
}

}  // namespace P4
