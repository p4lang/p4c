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

#include "ir/ir.h"

namespace IR {

const ID ParserState::accept = ID("accept");
const ID ParserState::reject = ID("reject");
const ID ParserState::start = ID("start");
const cstring TableProperties::actionsPropertyName = "actions";
const cstring TableProperties::keyPropertyName = "key";
const cstring TableProperties::defaultActionPropertyName = "default_action";
const cstring IApply::applyMethodName = "apply";
const cstring P4Program::main = "main";

int IR::Declaration::nextId = 0;

const Type_Method* P4Control::getConstructorMethodType() const {
    return new Type_Method(Util::SourceInfo(), getTypeParameters(), type, constructorParams);
}

const Type_Method* P4Parser::getConstructorMethodType() const {
    return new Type_Method(Util::SourceInfo(), getTypeParameters(), type, constructorParams);
}

const Type_Method* Type_Package::getConstructorMethodType() const {
    return new Type_Method(Util::SourceInfo(), getTypeParameters(), this, constructorParams);
}

Util::Enumerator<const IR::IDeclaration*>* IGeneralNamespace::getDeclsByName(cstring name) const {
    std::function<bool(const IDeclaration*)> filter = [name](const IDeclaration* d)
            { return name == d->getName().name; };
    return getDeclarations()->where(filter);
}

void IGeneralNamespace::checkDuplicateDeclarations() const {
    std::unordered_map<cstring, ID> seen;
    auto decls = getDeclarations();
    for (auto decl : *decls) {
        IR::ID name = decl->getName();
        auto f = seen.find(name.name);
        if (f != seen.end()) {
            ::error("Duplicate declaration of %1%: previous declaration at %2%",
                    name, f->second.srcInfo);
        }
        seen.emplace(name.name, name);
    }
}

void P4Parser::checkDuplicates() const {
    for (auto decl : *states) {
        auto prev = stateful->getDeclaration(decl->getName().name);
        if (prev != nullptr)
            ::error("State %1% has same name as %2%", decl, prev);
    }
}

bool Type_Stack::sizeKnown() const { return size->is<Constant>(); }

int Type_Stack::getSize() const {
    if (!sizeKnown())
        BUG("%1%: Size not yet known", size);
    auto cst = size->to<IR::Constant>();
    if (!cst->fitsInt()) {
        ::error("Index too large: %1%", cst);
        return 0;
    }
    return cst->asInt();
}

const Method* Type_Extern::lookupMethod(cstring name, int paramCount) const {
    const Method* result = nullptr;
    size_t upc = paramCount;

    bool reported = false;
    for (auto m : *methods) {
        if (m->name == name && m->getParameterCount() == upc) {
            if (result == nullptr) {
                result = m;
            } else {
                ::error("Ambiguous method %1% with %2% parameters", name, paramCount);
                if (!reported) {
                    ::error("Candidate is %1%", result);
                    reported = true;
                }
                ::error("Candidate is %1%", m);
                return nullptr;
            }
        }
    }
    return result;
}

const Type_Method*
Type_Parser::getApplyMethodType() const {
    return new Type_Method(Util::SourceInfo(), typeParams, nullptr, applyParams);
}

const Type_Method*
Type_Control::getApplyMethodType() const {
    return new Type_Method(Util::SourceInfo(), typeParams, nullptr, applyParams);
}

const IR::Path* ActionListElement::getPath() const {
    auto expr = expression;
    if (expr->is<IR::MethodCallExpression>())
        expr = expr->to<IR::MethodCallExpression>()->method;
    if (expr->is<IR::PathExpression>())
        return expr->to<IR::PathExpression>()->path;
    BUG("%1%: unexpected expression", expression);
}

const Type_Method*
P4Table::getApplyMethodType() const {
    // Synthesize a new type for the return
    auto actions = properties->getProperty(IR::TableProperties::actionsPropertyName);
    if (actions == nullptr) {
        ::error("Table %1% does not contain a list of actions", this);
        return nullptr;
    }
    if (!actions->value->is<IR::ActionList>())
        BUG("Action property is not an IR::ActionList, but %1%",
            actions);
    auto alv = actions->value->to<IR::ActionList>();
    auto fields = new IR::IndexedVector<IR::StructField>();
    auto hit = new IR::StructField(Util::SourceInfo(), IR::Type_Table::hit,
                                   IR::Annotations::empty, IR::Type_Boolean::get());
    fields->push_back(hit);
    auto label = new IR::StructField(Util::SourceInfo(), IR::Type_Table::action_run,
                                     IR::Annotations::empty,
                                     new IR::Type_ActionEnum(Util::SourceInfo(), alv));
    fields->push_back(label);
    auto rettype = new IR::Type_Struct(Util::SourceInfo(), ID(name),
                                       IR::Annotations::empty, fields);
    auto applyMethod = new IR::Type_Method(Util::SourceInfo(), new TypeParameters(),
                                           rettype, parameters);
    return applyMethod;
}

const Type_Method* Type_Table::getApplyMethodType() const
{ return table->getApplyMethodType(); }

void InstantiatedBlock::instantiate(std::vector<const CompileTimeValue*> *args) {
    CHECK_NULL(args);
    auto it = args->begin();
    for (auto p : *getConstructorParameters()->getEnumerator()) {
        LOG1("Set " << p << " to " << *it << " in " << id);
        setValue(p, *it);
        ++it;
    }
}

const IR::CompileTimeValue* InstantiatedBlock::getParameterValue(cstring paramName) const {
    auto param = getConstructorParameters()->getDeclByName(paramName);
    BUG_CHECK(param != nullptr, "No parameter named %1%", paramName);
    BUG_CHECK(param->is<IR::Parameter>(), "No parameter named %1%", paramName);
    return getValue(param->getNode());
}

Util::Enumerator<const IDeclaration*>* P4Action::getDeclarations() const
{ return body->getDeclarations(); }

const IDeclaration* P4Action::getDeclByName(cstring name) const
{ return body->components->getDeclaration(name); }

const IR::PackageBlock* ToplevelBlock::getMain() const {
    auto program = getProgram();
    auto main = program->getDeclByName(IR::P4Program::main);
    if (main == nullptr) {
        ::warning("Program does not contain a `%s' module", IR::P4Program::main);
        return nullptr;
    }
    if (!main->is<IR::Declaration_Instance>()) {
        ::error("%s must be a package declaration", main->getNode());
        return nullptr;
    }
    auto block = getValue(main->getNode());
    if (block == nullptr)
        return nullptr;
    BUG_CHECK(block->is<IR::PackageBlock>(), "%1%: toplevel block is not a package", block);
    return block->to<IR::PackageBlock>();
}

}  // namespace IR
