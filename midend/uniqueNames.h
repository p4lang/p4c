#ifndef _MIDEND_UNIQUENAMES_H_
#define _MIDEND_UNIQUENAMES_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace P4 {

class RenameMap {
    std::map<const IR::IDeclaration*, cstring> newName;
 public:
    void setNewName(const IR::IDeclaration* decl, cstring name) {
        CHECK_NULL(decl);
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
    FindSymbols(ReferenceMap *refMap, RenameMap *renameMap) :
            refMap(refMap), renameMap(renameMap)
    { CHECK_NULL(refMap); CHECK_NULL(renameMap); }
    void postorder(const IR::Declaration_Variable* decl) override;
    void postorder(const IR::Declaration_Constant* decl) override;
};

class RenameSymbols : public Transform {
    ReferenceMap *refMap;
    RenameMap    *renameMap;
 public:
    RenameSymbols(ReferenceMap *refMap, RenameMap *renameMap) :
            refMap(refMap), renameMap(renameMap)
    { CHECK_NULL(refMap); CHECK_NULL(renameMap); }
    const IR::Node* postorder(IR::Declaration_Variable* decl) override;
    const IR::Node* postorder(IR::Declaration_Constant* decl) override;
    const IR::Node* postorder(IR::PathExpression* expression) override;
};

}  // namespace P4

#endif /* _MIDEND_UNIQUENAMES_H_ */
