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

#include "lower.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "lib/gmputil.h"

namespace BMV2 {

// We make an effort to update the typeMap as we proceed
// since parent expression trees may need the information
// when processing in post-order.

const IR::Expression* LowerExpressions::shift(const IR::Operation_Binary* expression) const {
    auto rhs = expression->right;
    auto rhstype = typeMap->getType(rhs, true);
    if (rhstype->is<IR::Type_InfInt>()) {
        auto cst = rhs->to<IR::Constant>();
        mpz_class maxShift = Util::shift_left(1, LowerExpressions::maxShiftWidth);
        if (cst->value > maxShift)
            ::error(ErrorType::ERR_OVERLIMIT, "%1%: shift amount limited to %2% on this target",
                    expression, maxShift);
    } else {
        BUG_CHECK(rhstype->is<IR::Type_Bits>(), "%1%: expected a bit<> type", rhstype);
        auto bs = rhstype->to<IR::Type_Bits>();
        if (bs->size > LowerExpressions::maxShiftWidth)
            ::error(ErrorType::ERR_OVERLIMIT,
                    "%1%: shift amount limited to %2% bits on this target",
                    expression, LowerExpressions::maxShiftWidth);
    }
    auto ltype = typeMap->getType(getOriginal(), true);
    typeMap->setType(expression, ltype);
    return expression;
}

const IR::Node* LowerExpressions::postorder(IR::Neg* expression) {
    auto type = typeMap->getType(getOriginal(), true);
    auto zero = new IR::Constant(type, 0);
    auto sub = new IR::Sub(expression->srcInfo, zero, expression->expr);
    typeMap->setType(zero, type);
    typeMap->setType(sub, type);
    LOG3("Replaced " << expression << " with " << sub);
    return sub;
}

const IR::Node* LowerExpressions::postorder(IR::Cast* expression) {
    // handle bool <-> bit casts
    auto destType = typeMap->getType(getOriginal(), true);
    auto srcType = typeMap->getType(expression->expr, true);
    if (destType->is<IR::Type_Boolean>() && srcType->is<IR::Type_Bits>()) {
        auto zero = new IR::Constant(srcType, 0);
        auto cmp = new IR::Neq(expression->srcInfo, expression->expr, zero);
        typeMap->setType(cmp, destType);
        LOG3("Replaced " << expression << " with " << cmp);
        return cmp;
    } else if (destType->is<IR::Type_Bits>() && srcType->is<IR::Type_Boolean>()) {
        auto mux = new IR::Mux(expression->srcInfo, expression->expr,
                               new IR::Constant(destType, 1),
                               new IR::Constant(destType, 0));
        typeMap->setType(mux, destType);
        LOG3("Replaced " << expression << " with " << mux);
        return mux;
    }
    // This may be a new expression
    typeMap->setType(expression, destType);
    return expression;
}

const IR::Node* LowerExpressions::postorder(IR::Expression* expression) {
    // Just update the typeMap incrementally.
    auto type = typeMap->getType(getOriginal(), true);
    typeMap->setType(expression, type);
    return expression;
}

const IR::Node* LowerExpressions::postorder(IR::Slice* expression) {
    // This is in a RHS expression a[m:l]  ->  (cast)(a >> l)
    int h = expression->getH();
    int l = expression->getL();
    auto e0type = typeMap->getType(expression->e0, true);
    BUG_CHECK(e0type->is<IR::Type_Bits>(), "%1%: expected a bit<> type", e0type);
    const IR::Expression* expr;
    if (l != 0) {
        expr = new IR::Shr(expression->e0->srcInfo, expression->e0, new IR::Constant(l));
        typeMap->setType(expr, e0type);
    } else {
        expr = expression->e0;
    }

    // Narrowing cast.
    auto type = IR::Type_Bits::get(h - l + 1, e0type->to<IR::Type_Bits>()->isSigned);
    auto result = new IR::Cast(expression->srcInfo, type, expr);
    typeMap->setType(result, type);

    // Signedness conversion.
    type = IR::Type_Bits::get(h - l + 1, false);
    result = new IR::Cast(expression->srcInfo, type, result);
    typeMap->setType(result, type);

    LOG3("Replaced " << expression << " with " << result);
    return result;
}

const IR::Node* LowerExpressions::postorder(IR::Concat* expression) {
    // a ++ b  -> ((cast)a << sizeof(b)) | ((cast)b & mask)
    auto type = typeMap->getType(expression->right, true);
    auto resulttype = typeMap->getType(getOriginal(), true);
    BUG_CHECK(type->is<IR::Type_Bits>(), "%1%: expected a bitstring got a %2%",
              expression->right, type);
    BUG_CHECK(resulttype->is<IR::Type_Bits>(), "%1%: expected a bitstring got a %2%",
              expression->right, type);
    unsigned sizeofb = type->to<IR::Type_Bits>()->size;
    unsigned sizeofresult = resulttype->to<IR::Type_Bits>()->size;
    auto cast0 = new IR::Cast(expression->left->srcInfo, resulttype, expression->left);
    auto cast1 = new IR::Cast(expression->right->srcInfo, resulttype, expression->right);

    auto sh = new IR::Shl(cast0->srcInfo, cast0, new IR::Constant(sizeofb));
    mpz_class m = Util::maskFromSlice(sizeofb, 0);
    auto mask = new IR::Constant(expression->right->srcInfo,
                                 IR::Type_Bits::get(sizeofresult), m, 16);
    auto and0 = new IR::BAnd(expression->right->srcInfo, cast1, mask);
    auto result = new IR::BOr(expression->srcInfo, sh, and0);
    typeMap->setType(cast0, resulttype);
    typeMap->setType(cast1, resulttype);
    typeMap->setType(result, resulttype);
    typeMap->setType(sh, resulttype);
    typeMap->setType(and0, resulttype);
    LOG3("Replaced " << expression << " with " << result);
    return result;
}

/////////////////////////////////////////////////////////////

namespace {
/**
Detect whether a Select expression is too complicated for BMv2.
Also used to detect complex expressions that are arguments
to method calls.
*/
class ComplexExpression : public Inspector {
 public:
    bool isComplex = false;
    ComplexExpression() { setName("ComplexExpression"); }

    void postorder(const IR::ArrayIndex*) override {}
    void postorder(const IR::TypeNameExpression*) override {}
    void postorder(const IR::PathExpression*) override {}
    void postorder(const IR::Member*) override {}
    void postorder(const IR::Literal*) override {}
    // all other expressions are complex
    void postorder(const IR::Expression*) override { isComplex = true; }
};

}  // namespace

const IR::PathExpression*
RemoveComplexExpressions::createTemporary(const IR::Expression* expression) {
    auto type = typeMap->getType(expression, true);
    auto name = refMap->newName("tmp");
    auto decl = new IR::Declaration_Variable(IR::ID(name), type->getP4Type());
    newDecls.push_back(decl);
    typeMap->setType(decl, type);
    auto assign = new IR::AssignmentStatement(
        expression->srcInfo, new IR::PathExpression(name), expression);
    assignments.push_back(assign);
    return new IR::PathExpression(expression->srcInfo, new IR::Path(name));
}

const IR::Vector<IR::Argument>*
RemoveComplexExpressions::simplifyExpressions(const IR::Vector<IR::Argument>* args) {
    bool changes = true;
    auto result = new IR::Vector<IR::Argument>();
    for (auto arg : *args) {
        auto r = simplifyExpression(arg->expression, false);
        if (r != arg->expression) {
            changes = true;
            result->push_back(new IR::Argument(arg->srcInfo, arg->name, r));
        } else {
            result->push_back(arg);
        }
    }
    if (changes)
        return result;
    return args;
}

const IR::Expression*
RemoveComplexExpressions::simplifyExpression(const IR::Expression* expression, bool force) {
    // Note that 'force' is not applied recursively
    if (auto list = expression->to<IR::ListExpression>()) {
        auto simpl = simplifyExpressions(&list->components);
        if (simpl != &list->components)
            return new IR::ListExpression(expression->srcInfo, *simpl);
        return expression;
    } else if (auto si = expression->to<IR::StructInitializerExpression>()) {
        auto simpl = simplifyExpressions(&si->components);
        if (simpl != &si->components)
            return new IR::StructInitializerExpression(
                si->srcInfo, si->typeName, si->typeName, *simpl);
        return expression;
    } else {
        ComplexExpression ce;
        (void)expression->apply(ce);
        if (force || ce.isComplex) {
            LOG3("Moved into temporary " << dbp(expression));
            return createTemporary(expression);
        }
        return expression;
    }
}

const IR::Vector<IR::Expression>*
RemoveComplexExpressions::simplifyExpressions(const IR::Vector<IR::Expression>* vec, bool force) {
    // This is more complicated than I'd like.  If an expression is
    // a list expression, then we actually simplify the elements
    // of the list.  Otherwise we simplify the argument itself.
    // This is mostly for functions that take FieldLists - these
    // should still take a list as argument.
    bool changes = true;
    auto result = new IR::Vector<IR::Expression>();
    for (auto e : *vec) {
        auto r = simplifyExpression(e, force);
        if (r != e)
            changes = true;
        result->push_back(r);
    }
    if (changes)
        return result;
    return vec;
}

const IR::IndexedVector<IR::NamedExpression>*
RemoveComplexExpressions::simplifyExpressions(
    const IR::IndexedVector<IR::NamedExpression>* vec) {
    auto result = new IR::IndexedVector<IR::NamedExpression>();
    bool changes = false;
    for (auto e : *vec) {
        auto r = simplifyExpression(e->expression, true);
        if (r != e->expression) {
            changes = true;
            result->push_back(new IR::NamedExpression(e->srcInfo, e->name, r));
        } else {
            result->push_back(e);
        }
    }
    if (changes)
        return result;
    return vec;
}

const IR::Node*
RemoveComplexExpressions::postorder(IR::SelectExpression* expression) {
    auto vec = simplifyExpressions(&expression->select->components);
    if (vec != &expression->select->components)
        expression->select = new IR::ListExpression(expression->select->srcInfo, *vec);
    return expression;
}

const IR::Node*
RemoveComplexExpressions::preorder(IR::P4Control* control) {
    if (policy != nullptr && !policy->convert(control)) {
        prune();
        return control;
    }
    newDecls.clear();
    return control;
}

const IR::Node*
RemoveComplexExpressions::postorder(IR::MethodCallExpression* expression) {
    if (expression->arguments->size() == 0)
        return expression;
    auto mi = P4::MethodInstance::resolve(expression, refMap, typeMap);
    if (mi->isApply() || mi->is<P4::BuiltInMethod>())
        return expression;

    if (auto ef = mi->to<P4::ExternFunction>()) {
        if (ef->method->name == P4V1::V1Model::instance.digest_receiver.name) {
            // Special handling for digest; the semantics on bmv2 is to
            // execute the digest at the very end of the pipeline, and to
            // pass a reference to the fields, so fields can be modified
            // and the latest value is part of the digest.  We want the
            // digest to appear as if it is executed instantly, so we copy
            // the data to temporaries.  This could be a problem for P4-14
            // programs that depend on this semantics, but we hope that no
            // one knew of this feature, since it was not very clearly
            // documented.
            if (expression->arguments->size() != 2) {
                ::error(ErrorType::ERR_EXPECTED, "2 arguments", expression);
                return expression;
            }
            auto vec = new IR::Vector<IR::Argument>();
            // Digest has two arguments, we have to save the second one.
            // It should be a list expression.
            vec->push_back(expression->arguments->at(0));
            auto arg1 = expression->arguments->at(1)->expression;
            if (auto list = arg1->to<IR::ListExpression>()) {
                auto simplified = simplifyExpressions(&list->components, true);
                arg1 = new IR::ListExpression(arg1->srcInfo, *simplified);
                vec->push_back(new IR::Argument(arg1));
            } else if (auto si = arg1->to<IR::StructInitializerExpression>()) {
                auto list = simplifyExpressions(&si->components);
                arg1 = new IR::StructInitializerExpression(
                    si->srcInfo, si->typeName, si->typeName, *list);
                vec->push_back(new IR::Argument(arg1));
            } else {
                auto tmp = new IR::Argument(
                    expression->arguments->at(1)->srcInfo,
                    createTemporary(expression->arguments->at(1)->expression));
                vec->push_back(tmp);
            }
            expression->arguments = vec;
            return expression;
        }
    }

    auto vec = simplifyExpressions(expression->arguments);
    if (vec != expression->arguments)
        expression->arguments = vec;
    return expression;
}

const IR::Node*
RemoveComplexExpressions::simpleStatement(IR::Statement* statement) {
    if (assignments.empty())
        return statement;
    auto block = new IR::BlockStatement(assignments);
    block->push_back(statement);
    assignments.clear();
    return block;
}

const IR::Node*
RemoveComplexExpressions::postorder(IR::Statement* statement) {
    return simpleStatement(statement);
}

const IR::Node*
RemoveComplexExpressions::postorder(IR::MethodCallStatement* statement) {
    auto mi = P4::MethodInstance::resolve(statement, refMap, typeMap);
    if (auto em = mi->to<P4::ExternMethod>()) {
        if (em->originalExternType->name != P4::P4CoreLibrary::instance.packetIn.name ||
            em->method->name != P4::P4CoreLibrary::instance.packetIn.lookahead.name)
            return simpleStatement(statement);
        auto type = em->actualMethodType->returnType;
        auto name = refMap->newName("tmp");
        LOG3("Adding variable for lookahead " << name);
        auto decl = new IR::Declaration_Variable(IR::ID(name), type->getP4Type());
        newDecls.push_back(decl);
        typeMap->setType(decl, typeMap->getTypeType(type, true));
        auto assign = new IR::AssignmentStatement(
            statement->srcInfo, new IR::PathExpression(name), statement->methodCall);
        return simpleStatement(assign);
    }
    return simpleStatement(statement);
}

}  // namespace BMV2
