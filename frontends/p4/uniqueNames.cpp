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

#include "uniqueNames.h"

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeChecking/typeChecker.h"

namespace P4 {

void RenameMap::setNewName(const IR::IDeclaration *decl, cstring name, bool allowOverride) {
    CHECK_NULL(decl);
    BUG_CHECK(!name.isNullOrEmpty(), "Empty name");
    LOG1("Will rename " << dbp(decl) << " to " << name);
    BUG_CHECK(allowOverride || newName.find(decl) == newName.end(), "%1%: already renamed", decl);
    newName.insert_or_assign(decl, name);
}

const IR::P4Action *RenameMap::actionCalled(const IR::MethodCallExpression *expression) const {
    CHECK_NULL(expression);
    return ::get(actionCall, expression);
}

void RenameMap::markActionCall(const IR::P4Action *action, const IR::MethodCallExpression *call) {
    LOG2("Action " << dbp(action) << " called from " << dbp(call));
    actionCall.emplace(call, action);
}

namespace {

class FindActionCalls : public Inspector, public ResolutionContext {
    TypeMap *typeMap;
    RenameMap *renameMap;

 public:
    explicit FindActionCalls(TypeMap *typeMap, RenameMap *renameMap)
        : typeMap(typeMap), renameMap(renameMap) {
        CHECK_NULL(typeMap);
        CHECK_NULL(renameMap);
    }

    void postorder(const IR::MethodCallExpression *expression) {
        auto mi = MethodInstance::resolve(expression, this, typeMap);
        if (!mi->is<P4::ActionCall>()) return;
        auto ac = mi->to<P4::ActionCall>();
        renameMap->markActionCall(ac->action, getOriginal<IR::MethodCallExpression>());
    }
};

}  // namespace

Visitor::profile_t FindParameters::init_apply(const IR::Node *node) {
    auto rv = Inspector::init_apply(node);
    node->apply(nameGen);

    return rv;
}

UniqueNames::UniqueNames() : renameMap(new RenameMap) {
    setName("UniqueNames");
    visitDagOnce = false;
    passes.emplace_back(new FindSymbols(renameMap));
    passes.emplace_back(new RenameSymbols(renameMap));
}

UniqueParameters::UniqueParameters(TypeMap *typeMap) : renameMap(new RenameMap) {
    setName("UniqueParameters");
    CHECK_NULL(typeMap);
    passes.emplace_back(new TypeChecking(nullptr, typeMap));
    passes.emplace_back(new FindActionCalls(typeMap, renameMap));
    passes.emplace_back(new FindParameters(renameMap));
    passes.emplace_back(new RenameSymbols(renameMap));
    passes.emplace_back(new ClearTypeMap(typeMap));
}

/**************************************************************************/

IR::ID *RenameSymbols::getName() const { return getName(getOriginal<IR::IDeclaration>()); }

IR::ID *RenameSymbols::getName(const IR::IDeclaration *decl) const {
    auto newName = renameMap->get(decl);
    if (!newName.has_value()) return nullptr;
    auto name = new IR::ID(decl->getName().srcInfo, *newName, decl->getName().originalName);
    return name;
}

const IR::Annotations *RenameSymbols::addNameAnnotation(cstring name,
                                                        const IR::Annotations *annos) {
    if (annos == nullptr) annos = IR::Annotations::empty;
    return annos->addAnnotationIfNew(IR::Annotation::nameAnnotation, new IR::StringLiteral(name),
                                     false);
}

const IR::Node *RenameSymbols::postorder(IR::Declaration_Variable *decl) {
    return renameDeclWithNameAnnotation(decl);
}

const IR::Node *RenameSymbols::postorder(IR::Declaration_Constant *decl) {
    auto name = getName();
    if (name != nullptr && *name != decl->name) decl->name = *name;
    return decl;
}

const IR::Node *RenameSymbols::postorder(IR::Parameter *param) {
    return renameDeclWithNameAnnotation(param);
}

const IR::Node *RenameSymbols::postorder(IR::PathExpression *expression) {
    auto decl = getDeclaration(expression->path, true);
    if (!renameMap->toRename(decl)) return expression;
    // This should be a local name.
    BUG_CHECK(!expression->path->absolute, "%1%: renaming absolute path", expression);
    auto newName = renameMap->getName(decl);
    auto name =
        IR::ID(expression->path->name.srcInfo, newName, expression->path->name.originalName);
    LOG2("Renaming " << dbp(expression->path) << " to " << name);
    expression->path = new IR::Path(name);
    return expression;
}

const IR::Node *RenameSymbols::postorder(IR::Declaration_Instance *decl) {
    return renameDeclWithNameAnnotation(decl);
}

const IR::Node *RenameSymbols::postorder(IR::P4Table *decl) {
    return renameDeclWithNameAnnotation(decl);
}

const IR::Node *RenameSymbols::postorder(IR::P4Action *decl) {
    return renameDeclWithNameAnnotation(decl);
}

const IR::Node *RenameSymbols::postorder(IR::P4ValueSet *decl) {
    return renameDeclWithNameAnnotation(decl);
}

const IR::Node *RenameSymbols::postorder(IR::Argument *arg) {
    if (!arg->name) return arg;
    auto mce = findOrigCtxt<IR::MethodCallExpression>();
    if (mce == nullptr) return arg;
    LOG2("Found named argument " << arg << " of " << dbp(mce));
    auto action = renameMap->actionCalled(mce);
    if (action == nullptr) return arg;
    auto origParam = action->parameters->getParameter(arg->name.name);
    CHECK_NULL(origParam);

    if (!renameMap->toRename(origParam)) return arg;
    auto newName = renameMap->getName(origParam);
    LOG2("Renamed argument " << arg << " to " << newName);
    arg->name = IR::ID(arg->name.srcInfo, newName, arg->name.originalName);
    return arg;
}

}  // namespace P4
