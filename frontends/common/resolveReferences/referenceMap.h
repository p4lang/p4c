#ifndef _COMMON_RESOLVEREFERENCES_REFERENCEMAP_H_
#define _COMMON_RESOLVEREFERENCES_REFERENCEMAP_H_

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/map.h"

namespace P4 {

class ReferenceMap final {
    // Maps each path in the program to the corresponding declaration
    std::map<const IR::Path*, const IR::IDeclaration*> pathToDeclaration;
    std::set<const IR::IDeclaration*> used;

    // All names used within the program
    std::set<cstring> usedNames;

 public:
    const IR::P4Program* program = nullptr;  // program whose map is represented

    ReferenceMap() = default;
    const IR::IDeclaration* getDeclaration(const IR::Path* path, bool notNull = false) const;
    void setDeclaration(const IR::Path* path, const IR::IDeclaration* decl);
    void print() const { dbprint(std::cout); }
    void dbprint(std::ostream& cout) const;
    cstring newName(cstring base);
    void clear() { program = nullptr; pathToDeclaration.clear(); usedNames.clear(); used.clear(); }
    bool isUsed(const IR::IDeclaration* decl) const { return used.count(decl) > 0; }
    void usedName(cstring name) { usedNames.insert(name); }
};

}  // namespace P4

#endif /* _COMMON_RESOLVEREFERENCES_REFERENCEMAP_H_ */
