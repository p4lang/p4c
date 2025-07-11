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

#include <strings.h>

#include <functional>
#include <list>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "ir/declaration.h"
#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/enumerator.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/null.h"
#include "lib/ordered_map.h"

namespace P4::IR {

const cstring ParserState::accept = "accept"_cs;
const cstring ParserState::reject = "reject"_cs;
const cstring ParserState::start = "start"_cs;
const cstring ParserState::verify = "verify"_cs;

const cstring TableProperties::actionsPropertyName = "actions"_cs;
const cstring TableProperties::keyPropertyName = "key"_cs;
const cstring TableProperties::defaultActionPropertyName = "default_action"_cs;
const cstring TableProperties::entriesPropertyName = "entries"_cs;
const cstring TableProperties::sizePropertyName = "size"_cs;
const cstring IApply::applyMethodName = "apply"_cs;
const cstring P4Program::main = "main"_cs;
const cstring Type_Error::error = "error"_cs;

long IR::Declaration::nextId = 0;
long IR::This::nextId = 0;

const Type_Method *P4Control::getConstructorMethodType() const {
    return new Type_Method(getTypeParameters(), type, constructorParams, getName());
}

const Type_Method *P4Parser::getConstructorMethodType() const {
    return new Type_Method(getTypeParameters(), type, constructorParams, getName());
}

const Type_Method *Type_Package::getConstructorMethodType() const {
    return new Type_Method(getTypeParameters(), this, constructorParams, getName());
}

Util::Enumerator<const IR::IDeclaration *> *IGeneralNamespace::getDeclsByName(cstring name) const {
    return getDeclarations()->where([name](const IDeclaration *d) {
        CHECK_NULL(d);
        return name == d->getName().name;
    });
}

Util::Enumerator<const IDeclaration *> *INestedNamespace::getDeclarations() const {
    Util::Enumerator<const IDeclaration *> *rv = nullptr;
    for (const auto *nested : getNestedNamespaces()) {
        if (nested == nullptr) continue;

        rv = rv ? rv->concat(nested->getDeclarations()) : nested->getDeclarations();
    }
    return rv ? rv : new Util::EmptyEnumerator<const IDeclaration *>;
}

bool IFunctional::callMatches(const Vector<Argument> *arguments) const {
    auto paramList = getParameters()->parameters;
    absl::flat_hash_map<cstring, const IR::Parameter *, Util::Hash> paramNames;
    for (auto param : paramList) paramNames.emplace(param->name.name, param);

    size_t index = 0;
    for (auto arg : *arguments) {
        if (paramNames.size() == 0)
            // Too many arguments
            return false;
        cstring argName = arg->name;
        if (argName.isNullOrEmpty()) argName = paramList.at(index)->name.name;

        if (!paramNames.erase(argName)) {
            // Argument name does not match a parameter
            return false;
        }
        index++;
    }
    // Check if all remaining parameters have default values
    // or are optional.
    for (const auto &[_, param] : paramNames) {
        if (!param->isOptional() && !param->defaultValue) return false;
    }
    return true;
}

void IGeneralNamespace::checkDuplicateDeclarations() const {
    absl::flat_hash_set<ID, Util::Hash> seen;
    for (const auto *decl : *getDeclarations()) {
        IR::ID name = decl->getName();
        auto [it, inserted] = seen.emplace(name);
        if (!inserted) {
            ::P4::error(ErrorType::ERR_DUPLICATE,
                        "Duplicate declaration of %1%: previous declaration at %2%", name,
                        it->srcInfo);
        }
    }
}

void P4Parser::checkDuplicates() const {
    for (auto decl : states) {
        auto prev = parserLocals.getDeclaration(decl->getName().name);
        if (prev != nullptr)
            ::P4::error(ErrorType::ERR_DUPLICATE, "State %1% has same name as %2%", decl, prev);
    }
}

bool Type_Array::sizeKnown() const { return size->is<Constant>(); }

size_t Type_Array::getSize() const {
    if (!sizeKnown()) BUG("%1%: Size not yet known", size);
    auto cst = size->to<IR::Constant>();
    if (!cst->fitsInt()) {
        ::P4::error(ErrorType::ERR_OVERLIMIT, "Index too large: %1%", cst);
        return 0;
    }
    auto size = cst->asInt();
    if (size < 0) {
        ::P4::error(ErrorType::ERR_OVERLIMIT, "Illegal array size: %1%", cst);
        return 0;
    }
    return static_cast<size_t>(size);
}

const Method *Type_Extern::lookupMethod(IR::ID name, const Vector<Argument> *arguments) const {
    const Method *result = nullptr;
    bool reported = false;
    for (auto m : methods) {
        if (m->name != name) continue;
        if (m->callMatches(arguments)) {
            if (result == nullptr) {
                result = m;
            } else {
                ::P4::error(ErrorType::ERR_DUPLICATE, "Ambiguous method %1%", name);
                if (!reported) {
                    ::P4::error(ErrorType::ERR_DUPLICATE, "Candidate is %1%", result);
                    reported = true;
                }
                ::P4::error(ErrorType::ERR_DUPLICATE, "Candidate is %1%", m);
                return nullptr;
            }
        }
    }
    return result;
}

const Type_Method *Type_Parser::getApplyMethodType() const {
    return new Type_Method(applyParams, getName());
}

const Type_Method *Type_Control::getApplyMethodType() const {
    return new Type_Method(applyParams, getName());
}

const IR::Path *ActionListElement::getPath() const {
    auto expr = expression;
    if (expr->is<IR::MethodCallExpression>()) expr = expr->to<IR::MethodCallExpression>()->method;
    if (expr->is<IR::PathExpression>()) return expr->to<IR::PathExpression>()->path;
    BUG("%1%: unexpected expression", expression);
}

const Type_Method *P4Table::getApplyMethodType() const {
    // Synthesize a new type for the return
    auto actions = properties->getProperty(IR::TableProperties::actionsPropertyName);
    if (actions == nullptr) {
        ::P4::error(ErrorType::ERR_INVALID, "%1%: table does not contain a list of actions", this);
        return nullptr;
    }
    if (!actions->value->is<IR::ActionList>())
        BUG("Action property is not an IR::ActionList, but %1%", actions);
    auto alv = actions->value->to<IR::ActionList>();
    auto hit = new IR::StructField(IR::Type_Table::hit, IR::Type_Boolean::get());
    auto miss = new IR::StructField(IR::Type_Table::miss, IR::Type_Boolean::get());
    auto label = new IR::StructField(IR::Type_Table::action_run, new IR::Type_ActionEnum(alv));
    auto rettype = new IR::Type_Struct(ID(name), {hit, miss, label});
    auto applyMethod = new IR::Type_Method(rettype, new IR::ParameterList(), getName());
    return applyMethod;
}

const Type_Method *Type_Table::getApplyMethodType() const { return table->getApplyMethodType(); }

void BlockStatement::append(const StatOrDecl *stmt) {
    srcInfo += stmt->srcInfo;
    if (auto bs = stmt->to<BlockStatement>()) {
        bool merge = true;
        for (const auto *annot : bs->getAnnotations()) {
            auto a = getAnnotation(annot->name);
            if (!a || !a->equiv(*annot)) {
                merge = false;
                break;
            }
        }
        if (merge) {
            components.append(bs->components);
            return;
        }
    }
    components.push_back(stmt);
}

void Block::setValue(const Node *node, const CompileTimeValue *value) {
    CHECK_NULL(node);
    auto it = constantValue.find(node);
    if (it != constantValue.end())
        BUG_CHECK(value->equiv(*constantValue[node]), "%1% already set in %2% to %3%, not %4%",
                  node, this, value, constantValue[node]);
    else
        constantValue[node] = value;
}

void InstantiatedBlock::instantiate(std::vector<const CompileTimeValue *> *args) {
    CHECK_NULL(args);
    auto it = args->begin();
    for (auto p : *getConstructorParameters()->getEnumerator()) {
        if (it == args->end()) {
            BUG_CHECK(p->isOptional(), "Missing nonoptional arg %s", p);
            continue;
        }
        LOG1("Set " << p << " to " << *it << " in " << id);
        setValue(p, *it);
        ++it;
    }
}

const IR::CompileTimeValue *InstantiatedBlock::getParameterValue(cstring paramName) const {
    auto param = getConstructorParameters()->getDeclByName(paramName);
    BUG_CHECK(param != nullptr, "No parameter named %1%", paramName);
    BUG_CHECK(param->is<IR::Parameter>(), "No parameter named %1%", paramName);
    return getValue(param->getNode());
}

const IR::CompileTimeValue *InstantiatedBlock::findParameterValue(cstring paramName) const {
    auto *param = getConstructorParameters()->getDeclByName(paramName);
    if (!param) return nullptr;
    if (!param->is<IR::Parameter>()) return nullptr;
    return getValue(param->getNode());
}

Util::Enumerator<const IDeclaration *> *P4Program::getDeclarations() const {
    return objects.getEnumerator()->as<const IDeclaration *>()->where(
        [](const IDeclaration *d) { return d != nullptr; });
}

const IR::PackageBlock *ToplevelBlock::getMain() const {
    auto program = getProgram();
    auto mainDecls = program->getDeclsByName(IR::P4Program::main)->toVector();
    if (mainDecls.empty()) {
        ::P4::warning(ErrorType::WARN_MISSING, "Program does not contain a `%s' module",
                      IR::P4Program::main);
        return nullptr;
    }
    auto main = mainDecls[0];
    if (mainDecls.size() > 1) {
        ::P4::error(ErrorType::ERR_DUPLICATE, "Program has multiple `%s' instances: %1%, %2%",
                    IR::P4Program::main, main->getNode(), mainDecls[1]->getNode());
        return nullptr;
    }
    if (!main->is<IR::Declaration_Instance>()) {
        ::P4::error(ErrorType::ERR_INVALID, "%1%: must be a package declaration", main->getNode());
        return nullptr;
    }
    auto block = getValue(main->getNode());
    if (block == nullptr) return nullptr;
    if (!block->is<IR::PackageBlock>()) {
        ::P4::error(ErrorType::ERR_EXPECTED, "%1%: expected package declaration", block);
        return nullptr;
    }
    return block->to<IR::PackageBlock>();
}

}  // namespace P4::IR
