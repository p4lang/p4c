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
    ReferenceMap *refMap;
    RenameMap    *renameMap;
 public:
    explicit UniqueNames(bool anyOrder);
};

class FindSymbols : public Inspector {
    ReferenceMap *refMap;
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
};

}  // namespace P4

#endif /* _MIDEND_UNIQUENAMES_H_ */
