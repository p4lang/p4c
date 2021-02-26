/*
Copyright 2020 VMware, Inc.

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

#include "eliminateUnion.h"

namespace P4 {

const IR::Node* DoEliminateUnion::postorder(IR::Type_Union* type) {
    // Create a new enum with the same fields as the struct
    auto result = new IR::Vector<IR::Node>();
    cstring enumName = refMap->newName(type->getName().name + "_Tag");
    auto enumId = IR::ID(type->srcInfo, enumName);
    cstring name = type->getName().name;
    tagTypeName.emplace(name, enumId);

    // Populate the enum with the field names
    IR::IndexedVector<IR::Declaration_ID> members;
    for (auto f : type->fields)
        members.push_back(new IR::Declaration_ID(f->name));
    MinimalNameGenerator none(&members);
    // Add a new field called 'None' for uninitalized unions
    cstring noneName = none.newName("None");
    noneTagName.emplace(name, noneName);
    members.push_back(new IR::Declaration_ID(noneName));
    auto typeEnum = new IR::Type_Enum(enumId, members);
    result->push_back(typeEnum);

    // Convert union to a struct and add a field named 'tag'
    MinimalNameGenerator mg(type);
    auto tagName = mg.newName("tag");
    tagFieldName.emplace(name, tagName);
    IR::IndexedVector<IR::StructField> fields = type->fields;
    fields.push_back(new IR::StructField(tagName, new IR::Type_Name(enumId)));
    auto newType = new IR::Type_Struct(
        type->srcInfo, type->name, type->annotations, type->typeParameters, fields);
    result->push_back(newType);
    // The result contains both the enum and the struct
    return result;
}

const IR::Node* DoEliminateUnion::postorder(IR::SwitchCase* sc) {
    // Rewrite `u.b as var: {}` into `Tag.b: { FieldType var = u.b; {}}`
    auto sw = findContext<IR::SwitchStatement>();
    CHECK_NULL(sw);
    auto swtype = typeMap->getType(sw->expression, true);
    auto tu = swtype->to<IR::Type_Union>();
    if (!tu)
        return sc;
    if (sc->label->is<IR::DefaultExpression>())
        return sc;

    auto mem = sc->label->to<IR::Member>();
    BUG_CHECK(mem, "%1%: expected a member", sc->label);
    auto it = tagTypeName.find(tu->name.name);
    BUG_CHECK(it != tagTypeName.end(), "%1%: no tag type found", tu);
    CHECK_NULL(sc->statement);
    auto fieldType = typeMap->getType(sc->label, true);
    auto decl = new IR::Declaration_Variable(
        sc->srcInfo, sc->alias->name, fieldType->getP4Type(), sc->label);
    auto stat = new IR::BlockStatement;
    stat->push_back(decl);
    stat->push_back(sc->statement);
    return new IR::SwitchCase(
        sc->srcInfo,
        new IR::Member(sc->srcInfo, new IR::TypeNameExpression(it->second), mem->member),
        stat);
}

const IR::Node* DoEliminateUnion::postorder(IR::SwitchStatement* statement) {
    // Rewrite the switch expression of the form switch(u)
    // into switch (u.tag)
    auto type = typeMap->getType(statement->expression, true);
    if (auto tu = type->to<IR::Type_Union>()) {
        auto name = tu->getName().name;
        auto field = tagFieldName.find(name);
        if (field != tagFieldName.end())
            statement->expression = new IR::Member(
                statement->expression->srcInfo, statement->expression, field->second);
    }
    return statement;
}

const IR::Node* DoEliminateUnion::postorder(IR::Declaration_Variable* decl) {
    // Initialize all union variables to have a tag of "None".
    // This may be a bit conservative.  Create the initializers
    // in the toInsert list.
    auto type = typeMap->getType(getOriginal(), true);
    if (auto tu = type->to<IR::Type_Union>()) {
        if (findContext<IR::P4Parser>()) {
            ::error(ErrorType::ERR_UNSUPPORTED,
                    "%1%: union-typed variables currently only supported in controls", decl);
            return decl;
        }
        BUG_CHECK(!decl->initializer, "%1%: didn't expect an initializer", decl);
        auto name = tu->getName().name;
        auto none = noneTagName.find(name);
        BUG_CHECK(none != noneTagName.end(), "Could not find tag for %1%", type);
        auto enumType = tagTypeName.find(name);
        BUG_CHECK(enumType != tagTypeName.end(), "Could not find enum for %1%", type);
        auto tagName = tagFieldName.find(name);
        BUG_CHECK(tagName != tagFieldName.end(), "Could not find field %1%", type);
        auto initializer = new IR::Member(
            new IR::TypeNameExpression(enumType->second), none->second);
        auto dest = new IR::Member(
            decl->srcInfo, new IR::PathExpression(decl->getName()), tagName->second);
        auto assign = new IR::AssignmentStatement(decl->srcInfo, dest, initializer);
        toInsert.push_back(assign);
    }
    return decl;
}

const IR::Node* DoEliminateUnion::postorder(IR::P4Action* action) {
    // Insert the intializers that were created.
    if (!toInsert.empty()) {
        toInsert.append(action->body->components);
        action->body = new IR::BlockStatement(
            action->body->srcInfo, action->body->annotations, toInsert);
        toInsert.clear();
    }
    return action;
}

const IR::Node* DoEliminateUnion::postorder(IR::P4Control* control) {
    // Insert the intializers that were created.
    if (!toInsert.empty()) {
        toInsert.append(control->body->components);
        control->body = new IR::BlockStatement(
            control->body->srcInfo, control->body->annotations, toInsert);
        toInsert.clear();
    }
    return control;
}

const IR::Node* DoEliminateUnion::postorder(IR::AssignmentStatement* statement) {
    // look only for
    auto mem = statement->left->to<IR::Member>();
    if (!mem)
        return statement;
    auto btype = typeMap->getType(mem->expr, true);
    auto tu = btype->to<IR::Type_Union>();
    if (!tu)
        return statement;
    // We have an assignment of the form e.m = x, where e has type union.
    // Convert it to a pair of assignments, where the second one assigns
    // the tag of e.
    auto name = tu->getName().name;
    auto tagField = tagFieldName.find(name);
    BUG_CHECK(tagField != tagFieldName.end(), "Could not find field for %1%", tu);
    auto enumName = tagTypeName.find(name);
    BUG_CHECK(enumName != tagTypeName.end(), "Could not find enum for %1%", tu);
    auto assign = new IR::AssignmentStatement(
        statement->srcInfo, new IR::Member(
            mem->srcInfo, mem->expr, tagField->second),
        new IR::Member(new IR::TypeNameExpression(enumName->second), mem->member));
    IR::IndexedVector<IR::StatOrDecl> body;
    body.push_back(statement);
    body.push_back(assign);
    return new IR::BlockStatement(body);
}

}  // namespace P4
