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

#include "removeComplexExpressions.h"

#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/methodInstance.h"

namespace P4 {

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

    void postorder(const IR::ArrayIndex *) override {}
    void postorder(const IR::TypeNameExpression *) override {}
    void postorder(const IR::PathExpression *) override {}
    void postorder(const IR::Member *) override {}
    void postorder(const IR::Literal *) override {}
    // all other expressions are complex
    void postorder(const IR::Expression *) override { isComplex = true; }
};

}  // namespace

const IR::PathExpression *RemoveComplexExpressions::createTemporary(
    const IR::Expression *expression) {
    auto type = typeMap->getType(expression, true);
    auto name = refMap->newName("tmp");
    auto decl = new IR::Declaration_Variable(IR::ID(name), type->getP4Type());
    newDecls.push_back(decl);
    typeMap->setType(decl, type);
    auto assign =
        new IR::AssignmentStatement(expression->srcInfo, new IR::PathExpression(name), expression);
    assignments.push_back(assign);
    return new IR::PathExpression(expression->srcInfo, new IR::Path(name));
}

const IR::Vector<IR::Argument> *RemoveComplexExpressions::simplifyExpressions(
    const IR::Vector<IR::Argument> *args) {
    bool changes = false;
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
    if (changes) return result;
    return args;
}

const IR::Expression *RemoveComplexExpressions::simplifyExpression(const IR::Expression *expression,
                                                                   bool force) {
    // Note that 'force' is not applied recursively
    if (auto list = expression->to<IR::ListExpression>()) {
        auto simpl = simplifyExpressions(&list->components);
        if (simpl != &list->components) return new IR::ListExpression(expression->srcInfo, *simpl);
        return expression;
    } else if (auto si = expression->to<IR::StructExpression>()) {
        auto simpl = simplifyExpressions(&si->components);
        if (simpl != &si->components)
            return new IR::StructExpression(si->srcInfo, si->structType, si->structType, *simpl);
        return expression;
    } else {
        ComplexExpression ce;
        cd.setCalledBy(this);
        (void)expression->apply(ce);
        if (force || ce.isComplex) {
            LOG3("Moved into temporary " << dbp(expression));
            return createTemporary(expression);
        }
        return expression;
    }
}

const IR::Vector<IR::Expression> *RemoveComplexExpressions::simplifyExpressions(
    const IR::Vector<IR::Expression> *vec, bool force) {
    // This is more complicated than I'd like.  If an expression is
    // a list expression, then we actually simplify the elements
    // of the list.  Otherwise we simplify the argument itself.
    // This is mostly for functions that take FieldLists - these
    // should still take a list as argument.
    bool changes = true;
    auto result = new IR::Vector<IR::Expression>();
    for (auto e : *vec) {
        auto r = simplifyExpression(e, force);
        if (r != e) changes = true;
        result->push_back(r);
    }
    if (changes) return result;
    return vec;
}

const IR::IndexedVector<IR::NamedExpression> *RemoveComplexExpressions::simplifyExpressions(
    const IR::IndexedVector<IR::NamedExpression> *vec) {
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
    if (changes) return result;
    return vec;
}

const IR::Node *RemoveComplexExpressions::postorder(IR::SelectExpression *expression) {
    auto vec = simplifyExpressions(&expression->select->components);
    if (vec != &expression->select->components)
        expression->select = new IR::ListExpression(expression->select->srcInfo, *vec);
    return expression;
}

const IR::Node *RemoveComplexExpressions::preorder(IR::P4Control *control) {
    if (policy != nullptr && !policy->convert(control)) {
        prune();
        return control;
    }
    newDecls.clear();
    return control;
}

const IR::Node *RemoveComplexExpressions::postorder(IR::MethodCallExpression *expression) {
    if (expression->arguments->size() == 0) return expression;
    auto mi = P4::MethodInstance::resolve(expression, refMap, typeMap);
    if (mi->isApply() || mi->is<P4::BuiltInMethod>()) return expression;

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
                ::error(ErrorType::ERR_EXPECTED, "%1%: expected 2 arguments", expression);
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
            } else if (auto si = arg1->to<IR::StructExpression>()) {
                auto list = simplifyExpressions(&si->components);
                arg1 = new IR::StructExpression(si->srcInfo, si->structType, si->structType, *list);
                vec->push_back(new IR::Argument(arg1));
            } else {
                auto tmp =
                    new IR::Argument(expression->arguments->at(1)->srcInfo,
                                     createTemporary(expression->arguments->at(1)->expression));
                vec->push_back(tmp);
            }
            expression->arguments = vec;
            return expression;
        }
    }

    auto vec = simplifyExpressions(expression->arguments);
    if (vec != expression->arguments) expression->arguments = vec;
    return expression;
}

const IR::Node *RemoveComplexExpressions::simpleStatement(IR::Statement *statement) {
    if (assignments.empty()) return statement;
    auto block = new IR::BlockStatement(assignments);
    block->push_back(statement);
    assignments.clear();
    return block;
}

const IR::Node *RemoveComplexExpressions::postorder(IR::Statement *statement) {
    return simpleStatement(statement);
}

const IR::Node *RemoveComplexExpressions::postorder(IR::MethodCallStatement *statement) {
    auto mi = P4::MethodInstance::resolve(statement, refMap, typeMap);
    if (auto em = mi->to<P4::ExternMethod>()) {
        if (em->originalExternType->name != P4::P4CoreLibrary::instance().packetIn.name ||
            em->method->name != P4::P4CoreLibrary::instance().packetIn.lookahead.name)
            return simpleStatement(statement);
        auto type = em->actualMethodType->returnType;
        auto name = refMap->newName("tmp");
        LOG3("Adding variable for lookahead " << name);
        auto decl = new IR::Declaration_Variable(IR::ID(name), type->getP4Type());
        newDecls.push_back(decl);
        typeMap->setType(decl, typeMap->getTypeType(type, true));
        auto assign = new IR::AssignmentStatement(statement->srcInfo, new IR::PathExpression(name),
                                                  statement->methodCall);
        return simpleStatement(assign);
    }
    return simpleStatement(statement);
}

}  // namespace P4
