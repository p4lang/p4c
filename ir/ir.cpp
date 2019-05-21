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

const cstring ParserState::accept = "accept";
const cstring ParserState::reject = "reject";
const cstring ParserState::start = "start";
const cstring ParserState::verify = "verify";

const cstring TableProperties::actionsPropertyName = "actions";
const cstring TableProperties::keyPropertyName = "key";
const cstring TableProperties::defaultActionPropertyName = "default_action";
const cstring TableProperties::entriesPropertyName = "entries";
const cstring TableProperties::sizePropertyName = "size";
const cstring IApply::applyMethodName = "apply";
const cstring P4Program::main = "main";
const cstring Type_Error::error = "error";

int IR::Declaration::nextId = 0;
int IR::This::nextId = 0;

const Type_Method* P4Control::getConstructorMethodType() const {
    return new Type_Method(getTypeParameters(), type, constructorParams);
}

const Type_Method* P4Parser::getConstructorMethodType() const {
    return new Type_Method(getTypeParameters(), type, constructorParams);
}

const Type_Method* Type_Package::getConstructorMethodType() const {
    return new Type_Method(getTypeParameters(), this, constructorParams);
}

Util::Enumerator<const IR::IDeclaration*>* IGeneralNamespace::getDeclsByName(cstring name) const {
    std::function<bool(const IDeclaration*)> filter =
            [name](const IDeclaration* d)
            { CHECK_NULL(d); return name == d->getName().name; };
    return getDeclarations()->where(filter);
}

bool IFunctional::callMatches(const Vector<Argument> *arguments) const {
    auto paramList = getParameters()->parameters;
    std::map<cstring, const IR::Parameter*> paramNames;
    for (auto param : paramList)
        paramNames.emplace(param->name.name, param);

    size_t index = 0;
    for (auto arg : *arguments) {
        if (paramNames.size() == 0)
            // Too many arguments
            return false;
        cstring argName = arg->name;
        if (argName.isNullOrEmpty())
            argName = paramList.at(index)->name.name;

        auto it = paramNames.find(argName);
        if (it == paramNames.end())
            // Argument name does not match a parameter
            return false;
        else
            paramNames.erase(it);
        index++;
    }
    // Check if all remaining parameters have default values
    // or are optional.
    for (auto it : paramNames) {
        auto param = it.second;
        if (!param->isOptional() && !param->defaultValue)
            return false;
    }
    return true;
}

void IGeneralNamespace::checkDuplicateDeclarations() const {
    std::unordered_map<cstring, ID> seen;
    for (auto decl : *getDeclarations()) {
        IR::ID name = decl->getName();
        auto f = seen.find(name.name);
        if (f != seen.end()) {
            ::error(ErrorType::ERR_DUPLICATE,
                    "Duplicate declaration of %1%: previous declaration at %2%",
                    name, f->second.srcInfo);
        }
        seen.emplace(name.name, name);
    }
}

void P4Parser::checkDuplicates() const {
    for (auto decl : states) {
        auto prev = parserLocals.getDeclaration(decl->getName().name);
        if (prev != nullptr)
            ::error(ErrorType::ERR_DUPLICATE,
                    "State %1% has same name as %2%", decl, prev);
    }
}

bool Type_Stack::sizeKnown() const { return size->is<Constant>(); }

unsigned Type_Stack::getSize() const {
    if (!sizeKnown())
        BUG("%1%: Size not yet known", size);
    auto cst = size->to<IR::Constant>();
    if (!cst->fitsInt()) {
        ::error(ErrorType::ERR_OVERLIMIT, "Index too large: %1%", cst);
        return 0;
    }
    int size = cst->asInt();
    if (size <= 0)
        ::error(ErrorType::ERR_OVERLIMIT, "Illegal array size: %1%", cst);
    return static_cast<unsigned>(size);
}

const Method* Type_Extern::lookupMethod(IR::ID name, const Vector<Argument>* arguments) const {
    const Method* result = nullptr;
    bool reported = false;
    for (auto m : methods) {
        if (m->name != name) continue;
        if (m->callMatches(arguments)) {
            if (result == nullptr) {
                result = m;
            } else {
                ::error(ErrorType::ERR_DUPLICATE, "Ambiguous method %1%", name);
                if (!reported) {
                    ::error(ErrorType::ERR_DUPLICATE, "Candidate is %1%", result);
                    reported = true;
                }
                ::error(ErrorType::ERR_DUPLICATE, "Candidate is %1%", m);
                return nullptr;
            }
        }
    }
    return result;
}

const Type_Method*
Type_Parser::getApplyMethodType() const {
    return new Type_Method(applyParams);
}

const Type_Method*
Type_Control::getApplyMethodType() const {
    return new Type_Method(applyParams);
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
        ::error(ErrorType::ERR_INVALID, "table does not contain a list of actions", this);
        return nullptr;
    }
    if (!actions->value->is<IR::ActionList>())
        BUG("Action property is not an IR::ActionList, but %1%",
            actions);
    auto alv = actions->value->to<IR::ActionList>();
    auto hit = new IR::StructField(IR::Type_Table::hit, IR::Type_Boolean::get());
    auto label = new IR::StructField(IR::Type_Table::action_run, new IR::Type_ActionEnum(alv));
    auto rettype = new IR::Type_Struct(ID(name), { hit, label });
    auto applyMethod = new IR::Type_Method(rettype, new IR::ParameterList());
    return applyMethod;
}

const Type_Method* Type_Table::getApplyMethodType() const
{ return table->getApplyMethodType(); }

void Block::setValue(const Node* node, const CompileTimeValue* value) {
    CHECK_NULL(node);
    auto it = constantValue.find(node);
    if (it != constantValue.end())
        BUG_CHECK(value == constantValue[node],
                      "%1% already set in %2% to %3%, not %4%",
                  node, this, value, constantValue[node]);
    else
        constantValue[node] = value;
}

void InstantiatedBlock::instantiate(std::vector<const CompileTimeValue*> *args) {
    CHECK_NULL(args);
    auto it = args->begin();
    for (auto p : *getConstructorParameters()->getEnumerator()) {
        if (it == args->end()) {
            BUG_CHECK(p->isOptional(), "Missing nonoptional arg %s", p);
            continue; }
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

const IR::CompileTimeValue*
InstantiatedBlock::findParameterValue(cstring paramName) const {
    auto* param = getConstructorParameters()->getDeclByName(paramName);
    if (!param) return nullptr;
    if (!param->is<IR::Parameter>()) return nullptr;
    return getValue(param->getNode());
}

Util::Enumerator<const IDeclaration*>* P4Action::getDeclarations() const
{ return body->getDeclarations(); }

const IDeclaration* P4Action::getDeclByName(cstring name) const
{ return body->components.getDeclaration(name); }

Util::Enumerator<const IDeclaration*>* P4Program::getDeclarations() const {
    return objects.getEnumerator()
            ->as<const IDeclaration*>()
            ->where([](const IDeclaration* d) { return d != nullptr; });
}

const IR::PackageBlock* ToplevelBlock::getMain() const {
    auto program = getProgram();
    auto mainDecls = program->getDeclsByName(IR::P4Program::main)->toVector();
    if (mainDecls->size() == 0) {
        ::warning("Program does not contain a `%s' module", IR::P4Program::main);
        return nullptr;
    }
    auto main = mainDecls->at(0);
    if (mainDecls->size() > 1) {
        ::error(ErrorType::ERR_DUPLICATE, "Program has multiple `%s' instances: %1%, %2%",
                IR::P4Program::main, main->getNode(), mainDecls->at(1)->getNode());
        return nullptr;
    }
    if (!main->is<IR::Declaration_Instance>()) {
        ::error(ErrorType::ERR_INVALID, "must be a package declaration", main->getNode());
        return nullptr;
    }
    auto block = getValue(main->getNode());
    if (block == nullptr)
        return nullptr;
    BUG_CHECK(block->is<IR::PackageBlock>(), "%1%: toplevel block is not a package", block);
    return block->to<IR::PackageBlock>();
}

}  // namespace IR
