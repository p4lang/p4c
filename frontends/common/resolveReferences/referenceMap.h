#ifndef _COMMON_RESOLVEREFERENCES_REFERENCEMAP_H_
#define _COMMON_RESOLVEREFERENCES_REFERENCEMAP_H_

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/map.h"
#include "frontends/common/programMap.h"

namespace P4 {

class ReferenceMap final : public ProgramMap {
    // Maps each path in the program to the corresponding declaration
    std::map<const IR::Path*, const IR::IDeclaration*> pathToDeclaration;
    std::set<const IR::IDeclaration*> used;

    // All names used within the program
    std::set<cstring> usedNames;

 public:
    ReferenceMap();
    const IR::IDeclaration* getDeclaration(const IR::Path* path, bool notNull = false) const;
    void setDeclaration(const IR::Path* path, const IR::IDeclaration* decl);
    void print() const { dbprint(std::cout); }
    void dbprint(std::ostream& cout) const;
    cstring newName(cstring base);
    void clear();
    bool isUsed(const IR::IDeclaration* decl) const { return used.count(decl) > 0; }
    void usedName(cstring name) { usedNames.insert(name); }
};

}  // namespace P4

#endif /* _COMMON_RESOLVEREFERENCES_REFERENCEMAP_H_ */
