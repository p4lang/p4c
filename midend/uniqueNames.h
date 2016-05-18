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

#ifndef _MIDEND_UNIQUENAMES_H_
#define _MIDEND_UNIQUENAMES_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

class RenameMap {
    // Internal declaration name
    std::map<const IR::IDeclaration*, cstring> newName;
 public:
    void setNewName(const IR::IDeclaration* decl, cstring name) {
        CHECK_NULL(decl);
        BUG_CHECK(!name.isNullOrEmpty(), "Empty name");
        LOG1("Renaming " << decl << " to " << name);
        if (newName.find(decl) != newName.end())
            BUG("%1%: already renamed", decl);
        newName.emplace(decl, name);
    }
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
};

// give unique names to various declarations to make it easier to inline
class UniqueNames : public PassManager {
 private:
    RenameMap    *renameMap;
 public:
    UniqueNames(ReferenceMap* refMap, bool anyOrder);
};

// Finds and allocates new names for some symbols:
// Declaration_Variable, Declaration_Constant, Declaration_Instance,
// P4Table, P4Action.
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
};

class RenameSymbols : public Transform {
    ReferenceMap *refMap;
    RenameMap    *renameMap;

    IR::ID* getName() const;
 public:
    RenameSymbols(ReferenceMap *refMap, RenameMap *renameMap) :
            refMap(refMap), renameMap(renameMap)
    { CHECK_NULL(refMap); CHECK_NULL(renameMap); setName("RenameSymbols"); }
    const IR::Node* postorder(IR::Declaration_Variable* decl) override;
    const IR::Node* postorder(IR::Declaration_Constant* decl) override;
    const IR::Node* postorder(IR::PathExpression* expression) override;
    const IR::Node* postorder(IR::Declaration_Instance* decl) override;
    const IR::Node* postorder(IR::P4Table* decl) override;
    const IR::Node* postorder(IR::P4Action* decl) override;
    const IR::Node* postorder(IR::Parameter* param) override;
};

// Finds parameters for tables and actions that will be given unique names
class FindParameters : public Inspector {
    ReferenceMap* refMap;  // used to generate new names
    RenameMap*    renameMap;

    void doParameters(const IR::ParameterList* pl) {
        for (auto p : *pl->parameters) {
            if (p->direction == IR::Direction::None)
                continue;
            cstring newName = refMap->newName(p->name);
            renameMap->setNewName(p, newName);
        }
    }
 public:
    FindParameters(ReferenceMap* refMap, RenameMap* renameMap) :
            refMap(refMap), renameMap(renameMap)
    { CHECK_NULL(refMap); CHECK_NULL(renameMap); setName("FindParameters"); }
    void postorder(const IR::P4Table* table) override
    { doParameters(table->parameters); }
    void postorder(const IR::P4Action* action) override
    { doParameters(action->parameters); }
};

class UniqueParameters : public PassManager {
 private:
    RenameMap    *renameMap;
 public:
    UniqueParameters(ReferenceMap* refMap, bool anyOrder);
};

}  // namespace P4

#endif /* _MIDEND_UNIQUENAMES_H_ */
