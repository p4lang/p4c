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

#ifndef _FRONTENDS_P4_UNIQUENAMES_H_
#define _FRONTENDS_P4_UNIQUENAMES_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/typeMap.h"

namespace P4 {

class RenameMap {
    /// Internal declaration name
    std::map<const IR::IDeclaration*, cstring> newName;
    /// all actions that appear in tables
    std::set<const IR::P4Action*> inTable;
    /// Map from method call to action that is being called
    std::map<const IR::MethodCallExpression*, const IR::P4Action*> actionCall;

 public:
    void setNewName(const IR::IDeclaration* decl, cstring name);
    cstring getName(const IR::IDeclaration* decl) const {
        CHECK_NULL(decl);
        BUG_CHECK(newName.find(decl) != newName.end(), "%1%: no new name", decl);
        auto result = ::get(newName, decl);
        return result;
    }
    bool toRename(const IR::IDeclaration* decl) const {
        CHECK_NULL(decl);
        return newName.find(decl) != newName.end();
    }
    void foundInTable(const IR::P4Action* action);
    void markActionCall(const IR::P4Action* action, const IR::MethodCallExpression* call);
    bool isInTable(const IR::P4Action* action)
    { return inTable.find(action) != inTable.end(); }
    const IR::P4Action* actionCalled(const IR::MethodCallExpression* expression) const;
};

/// Give unique names to various declarations to make it easier to
/// move declarations around.
class UniqueNames : public PassManager {
 private:
    RenameMap    *renameMap;
 public:
    explicit UniqueNames(ReferenceMap* refMap);
};

/// Finds and allocates new names for some symbols:
/// Declaration_Variable, Declaration_Constant, Declaration_Instance,
/// P4Table, P4Action.
class FindSymbols : public Inspector {
    ReferenceMap *refMap;  // used to generate new names
    RenameMap    *renameMap;

 public:
    bool isTopLevel() const {
        return findContext<IR::P4Parser>() == nullptr &&
                findContext<IR::P4Control>() == nullptr;
    }
    FindSymbols(ReferenceMap *refMap, RenameMap *renameMap) :
            refMap(refMap), renameMap(renameMap)
    { CHECK_NULL(refMap); CHECK_NULL(renameMap); setName("FindSymbols"); }
    void doDecl(const IR::Declaration* decl) {
        cstring newName = refMap->newName(decl->getName());
        renameMap->setNewName(decl, newName);
    }
    void postorder(const IR::Declaration_Variable* decl) override
    { doDecl(decl); }
    void postorder(const IR::Declaration_Constant* decl) override
    { doDecl(decl); }
    void postorder(const IR::Declaration_Instance* decl) override
    { if (!isTopLevel()) doDecl(decl); }
    void postorder(const IR::P4Table* decl) override
    { doDecl(decl); }
    void postorder(const IR::P4Action* decl) override
    { if (!isTopLevel()) doDecl(decl); }
    void postorder(const IR::P4ValueSet* decl) override
    { if (!isTopLevel()) doDecl(decl); }
};

class RenameSymbols : public Transform {
    ReferenceMap *refMap;
    RenameMap    *renameMap;

    IR::ID* getName() const;
 public:
    RenameSymbols(ReferenceMap *refMap, RenameMap *renameMap) :
            refMap(refMap), renameMap(renameMap) {
        CHECK_NULL(refMap); CHECK_NULL(renameMap);
        visitDagOnce = false;
        setName("RenameSymbols"); }
    const IR::Node* postorder(IR::Declaration_Variable* decl) override;
    const IR::Node* postorder(IR::Declaration_Constant* decl) override;
    const IR::Node* postorder(IR::PathExpression* expression) override;
    const IR::Node* postorder(IR::Declaration_Instance* decl) override;
    const IR::Node* postorder(IR::P4Table* decl) override;
    const IR::Node* postorder(IR::P4Action* decl) override;
    const IR::Node* postorder(IR::P4ValueSet* decl) override;
    const IR::Node* postorder(IR::Parameter* param) override;
    const IR::Node* postorder(IR::Argument* argument) override;
};

/// Finds parameters for actions that will be given unique names
class FindParameters : public Inspector {
    ReferenceMap* refMap;  // used to generate new names
    RenameMap*    renameMap;

    // If all is true then rename all parameters, else rename only
    // directional parameters
    void doParameters(const IR::ParameterList* pl, bool all) {
        for (auto p : pl->parameters) {
            if (!all && p->direction == IR::Direction::None)
                continue;
            cstring newName = refMap->newName(p->name);
            renameMap->setNewName(p, newName);
        }
    }
 public:
    FindParameters(ReferenceMap* refMap, RenameMap* renameMap) :
            refMap(refMap), renameMap(renameMap)
    { CHECK_NULL(refMap); CHECK_NULL(renameMap); setName("FindParameters"); }
    void postorder(const IR::P4Action* action) override {
        bool inTable = renameMap->isInTable(action);
        doParameters(action->parameters, !inTable);
    }
};

/// Give each parameter of an action a new unique name
/// This must also rename named arguments
class UniqueParameters : public PassManager {
 private:
    RenameMap    *renameMap;
 public:
    UniqueParameters(ReferenceMap* refMap, TypeMap* typeMap);
};

}  // namespace P4

#endif /* _FRONTENDS_P4_UNIQUENAMES_H_ */
