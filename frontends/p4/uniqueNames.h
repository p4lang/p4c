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

#ifndef FRONTENDS_P4_UNIQUENAMES_H_
#define FRONTENDS_P4_UNIQUENAMES_H_

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "ir/visitor.h"

namespace P4 {

class RenameMap {
    /// Internal declaration name
    std::map<const IR::IDeclaration *, cstring> newName;
    /// Map from method call to action that is being called
    std::map<const IR::MethodCallExpression *, const IR::P4Action *> actionCall;

 public:
    /// @brief Add rename entry for the declaration to be named with the given name.
    /// @param allowOverride If set to true, don't fail if a new name was already set but replace it
    /// instead.
    void setNewName(const IR::IDeclaration *decl, cstring name, bool allowOverride = false);

    /// Get new name for the declaration, fails if none exists.
    cstring getName(const IR::IDeclaration *decl) const {
        auto n = get(decl);
        BUG_CHECK(n.has_value(), "%1%: no new name", decl);
        return *n;
    }

    /// @returns true if there is a new name for the declaration, false otherwise.
    bool toRename(const IR::IDeclaration *decl) const {
        CHECK_NULL(decl);
        return newName.find(decl) != newName.end();
    }

    /// Get new name for the declaration (wrapped in optional), or std::nullopt if there is none.
    std::optional<cstring> get(const IR::IDeclaration *decl) const {
        CHECK_NULL(decl);
        if (auto it = newName.find(decl); it != newName.end()) {
            return it->second;
        }
        return {};
    }

    void foundInTable(const IR::P4Action *action);
    void markActionCall(const IR::P4Action *action, const IR::MethodCallExpression *call);
    const IR::P4Action *actionCalled(const IR::MethodCallExpression *expression) const;
};

/// Give unique names to various declarations to make it easier to
/// move declarations around.
class UniqueNames : public PassManager {
 private:
    RenameMap *renameMap;

 public:
    UniqueNames();
};

/// Finds and allocates new names for some symbols:
/// Declaration_Variable, Declaration_Constant, Declaration_Instance,
/// P4Table, P4Action.
class FindSymbols : public Inspector {
    MinimalNameGenerator nameGen;  // used to generate new names
    RenameMap *renameMap;

 public:
    bool isTopLevel() const {
        return findContext<IR::P4Parser>() == nullptr && findContext<IR::P4Control>() == nullptr;
    }
    explicit FindSymbols(RenameMap *renameMap) : renameMap(renameMap) {
        CHECK_NULL(renameMap);
        setName("FindSymbols");
    }
    profile_t init_apply(const IR::Node *node) override {
        auto rv = Inspector::init_apply(node);
        node->apply(nameGen);
        return rv;
    }

    void doDecl(const IR::Declaration *decl) {
        cstring newName = nameGen.newName(decl->getName().name.string_view());
        renameMap->setNewName(decl, newName);
    }
    void postorder(const IR::Declaration_Variable *decl) override { doDecl(decl); }
    void postorder(const IR::Declaration_Constant *decl) override {
        // Skip toplevel constants with names like __
        // We assume that these do not clash and no new symbols with
        // these names will be added.
        if (decl->getName().name.startsWith("__") && getParent<IR::P4Program>()) return;
        doDecl(decl);
    }
    void postorder(const IR::Declaration_Instance *decl) override {
        if (!isTopLevel()) doDecl(decl);
    }
    void postorder(const IR::P4Table *decl) override { doDecl(decl); }
    void postorder(const IR::P4Action *decl) override {
        if (!isTopLevel()) doDecl(decl);
    }
    void postorder(const IR::P4ValueSet *decl) override {
        if (!isTopLevel()) doDecl(decl);
    }
};

class RenameSymbols : public Transform, public ResolutionContext {
 protected:
    RenameMap *renameMap;

    /// Get new name of the current declaration or nullptr if the declaration is not to be renamed.
    IR::ID *getName() const;
    /// Get new name of the given declaration or nullptr if the declaration is not to be renamed.
    /// @param decl Declaration *in the original/non-transformed* P4 IR.
    IR::ID *getName(const IR::IDeclaration *decl) const;

    /// Add a @name annotation ONLY if it does not already exist.
    /// Otherwise do nothing.
    static const IR::Annotations *addNameAnnotation(cstring name, const IR::Annotations *annos);

    /// Rename any declaration where we want to add @name annotation with the original name.
    /// Has to be a template as there is no common base for declarations with annotations member.
    template <typename D>
    const IR::Node *renameDeclWithNameAnnotation(D *decl) {
        auto name = getName();
        if (name != nullptr && *name != decl->name) {
            decl->annotations = addNameAnnotation(decl->name, decl->annotations);
            decl->name = *name;
        }
        return decl;
    }

 public:
    explicit RenameSymbols(RenameMap *renameMap) : renameMap(renameMap) {
        CHECK_NULL(renameMap);
        visitDagOnce = false;
        setName("RenameSymbols");
    }
    const IR::Node *postorder(IR::Declaration_Variable *decl) override;
    const IR::Node *postorder(IR::Declaration_Constant *decl) override;
    const IR::Node *postorder(IR::PathExpression *expression) override;
    const IR::Node *postorder(IR::Declaration_Instance *decl) override;
    const IR::Node *postorder(IR::P4Table *decl) override;
    const IR::Node *postorder(IR::P4Action *decl) override;
    const IR::Node *postorder(IR::P4ValueSet *decl) override;
    const IR::Node *postorder(IR::Parameter *param) override;
    const IR::Node *postorder(IR::Argument *argument) override;
};

/// Finds parameters for actions that will be given unique names
class FindParameters : public Inspector {
    MinimalNameGenerator nameGen;
    RenameMap *renameMap;

    void doParameters(const IR::ParameterList *pl) {
        for (auto p : pl->parameters) {
            cstring newName = nameGen.newName(p->name.name.string_view());
            renameMap->setNewName(p, newName);
        }
    }

 public:
    explicit FindParameters(RenameMap *renameMap) : renameMap(renameMap) {
        CHECK_NULL(renameMap);
        setName("FindParameters");
    }
    void postorder(const IR::P4Action *action) override { doParameters(action->parameters); }
    profile_t init_apply(const IR::Node *node) override;
};

/// Give each parameter of an action a new unique name
/// This must also rename named arguments
class UniqueParameters : public PassManager {
 private:
    RenameMap *renameMap;

 public:
    explicit UniqueParameters(TypeMap *typeMap);
};

}  // namespace P4

#endif /* FRONTENDS_P4_UNIQUENAMES_H_ */
