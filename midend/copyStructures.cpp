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
#include "frontends/p4/alias.h"

namespace P4 {

const IR::Node* RemoveAliases::postorder(IR::AssignmentStatement* statement) {
    auto type = typeMap->getType(statement->left);
    if (!type->is<IR::Type_StructLike>())
        return statement;

    ReadsWrites rw(refMap, false);
    if (!rw.mayAlias(statement->left, statement->right))
        return statement;
    auto tmp = refMap->newName("tmp");
    auto decl = new IR::Declaration_Variable(IR::ID(tmp), type->getP4Type(), nullptr);
    declarations.push_back(decl);
    auto result = new IR::Vector<IR::StatOrDecl>();
    result->push_back(new IR::AssignmentStatement(
        statement->srcInfo, new IR::PathExpression(tmp), statement->right));
    result->push_back(new IR::AssignmentStatement(
        statement->srcInfo, statement->left, new IR::PathExpression(tmp)));
    LOG3("Inserted temporary " << decl << " for " << statement);
    return result;
}

const IR::Node* RemoveAliases::postorder(IR::P4Parser* parser) {
    if (!declarations.empty()) {
        parser->parserLocals.append(declarations);
        declarations.clear();
    }
    return parser;
}

const IR::Node* RemoveAliases::postorder(IR::P4Control* control) {
    if (!declarations.empty()) {
        // prepend declarations: they must come before any actions
        // that may have been modified
        control->controlLocals.prepend(declarations);
        declarations.clear();
    }
    return control;
}

const IR::Node* DoCopyStructures::postorder(IR::AssignmentStatement* statement) {
    auto ltype = typeMap->getType(statement->left, true);
    if (ltype->is<IR::Type_StructLike>()) {
        if (statement->right->is<IR::MethodCallExpression>()) {
            if (errorOnMethodCall)
                ::error("%1%: functions or methods returning structures "
                        "are not supported on this target",
                        statement->right);
            return statement;
        }

        auto retval = new IR::IndexedVector<IR::StatOrDecl>();
        auto strct = ltype->to<IR::Type_StructLike>();
        if (auto list = statement->right->to<IR::ListExpression>()) {
            unsigned index = 0;
            for (auto f : strct->fields) {
                auto right = list->components.at(index);
                auto left = new IR::Member(statement->left, f->name);
                retval->push_back(new IR::AssignmentStatement(statement->srcInfo, left, right));
                index++;
            }
        } else if (auto si = statement->right->to<IR::StructInitializerExpression>()) {
            for (auto f : strct->fields) {
                auto right = si->components.getDeclaration<IR::NamedExpression>(f->name);
                auto left = new IR::Member(statement->left, f->name);
                retval->push_back(new IR::AssignmentStatement(
                    statement->srcInfo, left, right->expression));
            }
        } else {
            if (ltype->is<IR::Type_Header>())
                // Leave headers as they are -- copy_header will also copy the valid bit
                return statement;

            BUG_CHECK(statement->right->is<IR::PathExpression>() ||
                      statement->right->is<IR::Member>() ||
                      statement->right->is<IR::ArrayIndex>(),
                      "%1%: Unexpected operation when eliminating struct copying",
                      statement->right);
            for (auto f : strct->fields) {
                auto right = new IR::Member(statement->right, f->name);
                auto left = new IR::Member(statement->left, f->name);
                retval->push_back(new IR::AssignmentStatement(statement->srcInfo, left, right));
            }
        }
        return new IR::BlockStatement(statement->srcInfo, *retval);
    }

    return statement;
}

}  // namespace P4
