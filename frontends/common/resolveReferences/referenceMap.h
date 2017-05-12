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

#ifndef _COMMON_RESOLVEREFERENCES_REFERENCEMAP_H_
#define _COMMON_RESOLVEREFERENCES_REFERENCEMAP_H_

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/map.h"
#include "frontends/common/programMap.h"

namespace P4 {

class NameGenerator {
 public:
    virtual cstring newName(cstring base) = 0;
};


/// Class used to encode maps from paths to declarations.
class ReferenceMap final : public ProgramMap, public NameGenerator {
    /// If `isv1` is true, then the map is for a P4_14 program
    /// (possibly translated into P4_16).
    bool isv1;

    /// Maps paths in the program to declarations.
    std::map<const IR::Path*, const IR::IDeclaration*> pathToDeclaration;

    /// Set containing all declarations in the program.
    std::set<const IR::IDeclaration*> used;

    /// Map from `This` to declarations (an experimental feature).
    std::map<const IR::This*, const IR::IDeclaration*> thisToDeclaration;

    /// Set containing all names used in the program.
    std::set<cstring> usedNames;

 public:
    ReferenceMap();
    /// Looks up declaration for @p path. If @p notNull is false, then
    /// failure to find a declaration is an error.
    const IR::IDeclaration* getDeclaration(const IR::Path* path, bool notNull = false) const;

    /// Sets declaration for @p path to @p decl.
    void setDeclaration(const IR::Path* path, const IR::IDeclaration* decl);

    /// Looks up declaration for @p pointer. If @p notNull is false,
    /// then failure to find a declaration is an error.
    const IR::IDeclaration* getDeclaration(const IR::This* pointer, bool notNull = false) const;

    /// Sets declaration for @p pointer to @p decl.
    void setDeclaration(const IR::This* pointer, const IR::IDeclaration* decl);

    void dbprint(std::ostream& cout) const;

    /// Set boolean indicating whether map is for a P4_14 program to @p isV1.
    void setIsV1(bool isv1) { this->isv1 = isv1; }

    /// Generate a name from @p base that fresh for the program.
    cstring newName(cstring base);

    /// Clear the reference map
    void clear();

    /// @returns @true if this map is for a P4_14 program
    bool isV1() const { return isv1; }

    /// @returns @true if @p decl is used in the program.
    bool isUsed(const IR::IDeclaration* decl) const { return used.count(decl) > 0; }

    /// Indicate that @p name is used in the program.
    void usedName(cstring name) { usedNames.insert(name); }
};

}  // namespace P4

#endif /* _COMMON_RESOLVEREFERENCES_REFERENCEMAP_H_ */
