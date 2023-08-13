/*
Copyright 2016 VMware, Inc.

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

#include "copyStructures.h"

#include <ostream>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/number.hpp>

#include "frontends/p4/alias.h"
#include "ir/id.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/log.h"

namespace P4 {

const IR::Node *RemoveAliases::postorder(IR::AssignmentStatement *statement) {
    const auto *type = typeMap->getType(statement->left);
    if (!type->is<IR::Type_StructLike>()) {
        return statement;
    }

    ReadsWrites rw(refMap);
    if (!rw.mayAlias(statement->left, statement->right)) {
        return statement;
    }
    auto tmp = refMap->newName("tmp");
    auto *decl = new IR::Declaration_Variable(IR::ID(tmp), type->getP4Type(), nullptr);
    declarations.push_back(decl);
    auto *result = new IR::IndexedVector<IR::StatOrDecl>();
    result->push_back(new IR::AssignmentStatement(statement->srcInfo, new IR::PathExpression(tmp),
                                                  statement->right));
    result->push_back(new IR::AssignmentStatement(statement->srcInfo, statement->left,
                                                  new IR::PathExpression(tmp)));
    LOG3("Inserted temporary " << decl << " for " << statement);
    return new IR::BlockStatement(statement->srcInfo, *result);
}

const IR::Node *RemoveAliases::postorder(IR::P4Parser *parser) {
    if (!declarations.empty()) {
        parser->parserLocals.append(declarations);
        declarations.clear();
    }
    return parser;
}

const IR::Node *RemoveAliases::postorder(IR::P4Control *control) {
    if (!declarations.empty()) {
        // prepend declarations: they must come before any actions
        // that may have been modified
        control->controlLocals.prepend(declarations);
        declarations.clear();
    }
    return control;
}

const IR::Node *DoCopyStructures::postorder(IR::AssignmentStatement *statement) {
    const auto *ltype = typeMap->getType(statement->left, true);

    // If the left type is not a struct like or a header stack, return.
    if (!(ltype->is<IR::Type_StructLike>() || ltype->is<IR::Type_Stack>())) {
        return statement;
    }
    /*
       FIXME: this is not correct for header unions and should be fixed.
       The fix bellow, commented-out, causes problems elsewhere.
       https://github.com/p4lang/p4c/issues/3842
    if (ltype->is<IR::Type_HeaderUnion>())
        return statement;
    */

    // Do not copy structures for method calls.
    if (statement->right->is<IR::MethodCallExpression>()) {
        if (errorOnMethodCall) {
            ::error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                    "%1%: functions or methods returning structures "
                    "are not supported on this target",
                    statement->right);
        }
        return statement;
    }

    const auto &srcInfo = statement->srcInfo;
    auto *retval = new IR::IndexedVector<IR::StatOrDecl>();
    if (const auto *si = statement->right->to<IR::StructExpression>()) {
        const auto *strct = ltype->checkedTo<IR::Type_StructLike>();
        for (const auto *f : strct->fields) {
            const auto *right = si->components.getDeclaration<IR::NamedExpression>(f->name);
            const auto *left = new IR::Member(statement->left, f->name);
            retval->push_back(
                new IR::AssignmentStatement(statement->srcInfo, left, right->expression));
        }
    } else if (copyHeaders && ltype->is<IR::Type_Header>()) {
        const auto *header = ltype->checkedTo<IR::Type_Header>();
        // Build a "src.isValid()" call.
        const auto *isSrcValidCall = new IR::MethodCallExpression(
            srcInfo, IR::Type::Boolean::get(),
            new IR::Member(srcInfo, statement->right, IR::Type_Header::isValid),
            new IR::Vector<IR::Argument>());
        // Build a "dst.setValid()" call.
        const auto *setDstValidCall = new IR::MethodCallExpression(
            srcInfo, IR::Type::Void::get(),
            new IR::Member(srcInfo, statement->left, IR::Type_Header::setValid),
            new IR::Vector<IR::Argument>());

        // Build a "dst.setInvalid()" call.
        const auto *setDstInvalidCall = new IR::MethodCallExpression(
            srcInfo, IR::Type::Void::get(),
            new IR::Member(srcInfo, statement->left, IR::Type_Header::setInvalid),
            new IR::Vector<IR::Argument>());
        // Build the "then" branch: set the validity bit and copy the fields.
        IR::IndexedVector<IR::StatOrDecl> thenStmts;
        thenStmts.push_back(new IR::MethodCallStatement(srcInfo, setDstValidCall));
        for (const auto *field : header->fields) {
            const auto *left = new IR::Member(field->type, statement->left, field->name);
            const auto *right = new IR::Member(field->type, statement->right, field->name);
            thenStmts.push_back(new IR::AssignmentStatement(srcInfo, left, right));
        }

        // Build the "else" branch: clear the validity bit.
        auto *elseStmt = new IR::MethodCallStatement(srcInfo, setDstInvalidCall);

        // Finish up.
        return new IR::IfStatement(srcInfo, isSrcValidCall,
                                   new IR::BlockStatement(srcInfo, thenStmts), elseStmt);
    } else if (ltype->is<IR::Type_Stack>() &&
               ((!findContext<IR::P4Parser>()) ||
                (statement->right->is<IR::HeaderStackExpression>()))) {
        // no copies in parsers -- copying stacks looses the .next field
        const auto *stack = ltype->checkedTo<IR::Type_Stack>();
        const auto *stackSize = stack->size->to<IR::Constant>();
        BUG_CHECK(stackSize && stackSize->value > 0, "Size of stack %s is not a positive constant",
                  ltype);
        BUG_CHECK(statement->right->is<IR::PathExpression>() ||
                      statement->right->is<IR::HeaderStackExpression>() ||
                      statement->right->is<IR::Member>() || statement->right->is<IR::ArrayIndex>(),
                  "%1%: Unexpected operation encountered while eliminating stack copying",
                  statement->right);

        // Copy each stack element.
        for (int i = 0; i < stackSize->asInt(); ++i) {
            const auto *left =
                new IR::ArrayIndex(stack->elementType, statement->left, new IR::Constant(i));
            const auto *right =
                new IR::ArrayIndex(stack->elementType, statement->right, new IR::Constant(i));
            retval->push_back(new IR::AssignmentStatement(srcInfo, left, right));
        }
    } else {
        if (ltype->is<IR::Type_Header>() || ltype->is<IR::Type_Stack>()) {
            // Leave headers as they are -- copy_header will also copy the valid bit
            return statement;
        }
        const auto *strct = ltype->checkedTo<IR::Type_StructLike>();
        BUG_CHECK(statement->right->is<IR::PathExpression>() ||
                      statement->right->is<IR::Member>() || statement->right->is<IR::ArrayIndex>(),
                  "%1%: Unexpected operation when eliminating struct copying", statement->right);
        for (const auto *f : strct->fields) {
            const auto *right = new IR::Member(statement->right, f->name);
            const auto *left = new IR::Member(statement->left, f->name);
            retval->push_back(new IR::AssignmentStatement(statement->srcInfo, left, right));
        }
    }
    return new IR::BlockStatement(statement->srcInfo, *retval);
}

}  // namespace P4
