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

#include "validateParsedProgram.h"

namespace P4 {

void ValidateParsedProgram::postorder(const IR::Constant* c) {
    if (c->type != nullptr &&
        !c->type->is<IR::Type_Unknown>() &&
        !c->type->is<IR::Type_Bits>() &&
        !c->type->is<IR::Type_InfInt>())
        BUG("Invalid type %1% for constant %2%", c->type, c);
}

void ValidateParsedProgram::postorder(const IR::Method* m) {
    if (m->name.isDontCare())
        ::error("%1%: Illegal method/function name", m->name);
    if (auto ext = findContext<IR::Type_Extern>()) {
        if (m->name == ext->name && m->type->returnType != nullptr)
            ::error("%1%: Constructor can't have return type", m); }
}

void ValidateParsedProgram::postorder(const IR::StructField* f) {
    if (f->name.isDontCare())
        ::error("%1%: Illegal field name", f->name);
}

void ValidateParsedProgram::postorder(const IR::Type_Union* type) {
    if (type->fields->size() == 0)
        ::error("%1%: empty union", type);
}

void ValidateParsedProgram::postorder(const IR::Type_Bits* type) {
    if (type->size <= 0)
        ::error("%1%: Illegal bit size", type);
    if (type->size == 1 && type->isSigned)
        ::error("%1%: Signed types cannot be 1-bit wide", type);
}

void ValidateParsedProgram::postorder(const IR::ParserState* s) {
    if (s->name == IR::ParserState::accept ||
        s->name == IR::ParserState::reject)
        ::error("%1%: parser state should not be implemented, it is built-in", s->name);
}

void ValidateParsedProgram::container(const IR::IContainer* type) {
    for (auto p : *type->getConstructorParameters()->parameters)
        if (p->direction != IR::Direction::None)
            ::error("%1%: constructor parameters cannot have a direction", p);
}

void ValidateParsedProgram::postorder(const IR::P4Table* t) {
    auto ac = t->properties->getProperty(IR::TableProperties::actionsPropertyName);
    if (ac == nullptr)
        ::error("Table %1% does not have an `%2%' property",
                t->name, IR::TableProperties::actionsPropertyName);
    auto da = t->properties->getProperty(IR::TableProperties::defaultActionPropertyName);
    if (!isv1 && da == nullptr)
        ::warning("Table %1% does not have an `%2%' property",
                t->name, IR::TableProperties::defaultActionPropertyName);
    for (auto p : *t->parameters->parameters) {
        if (p->direction == IR::Direction::None)
            ::error("%1%: Table parameters cannot be direction-less", p);
    }
}

void ValidateParsedProgram::postorder(const IR::ConstructorCallExpression* expression) {
    auto inAction = findContext<IR::P4Action>();
    if (inAction != nullptr)
        ::error("%1%: Constructor calls not allowed in actions", expression);
}

void ValidateParsedProgram::postorder(const IR::Declaration_Variable* decl) {
    if (decl->name.isDontCare())
        ::error("%1%: illegal variable name", decl);
}

void ValidateParsedProgram::postorder(const IR::Declaration_Constant* decl) {
    if (decl->name.isDontCare())
        ::error("%1%: illegal constant name", decl);
}

void ValidateParsedProgram::postorder(const IR::SwitchStatement* statement) {
    auto inAction = findContext<IR::P4Action>();
    if (inAction != nullptr)
        ::error("%1%: switch statements not allowed in actions", statement);
}

}  // namespace P4
