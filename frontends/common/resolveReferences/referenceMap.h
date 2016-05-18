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

class ReferenceMap final : public ProgramMap {
    // Maps each path in the program to the corresponding declaration
    std::map<const IR::Path*, const IR::IDeclaration*> pathToDeclaration;
    std::set<const IR::IDeclaration*> used;

    // All names used within the program
    std::set<cstring> usedNames;

 public:
    ReferenceMap() : ProgramMap("ReferenceMap") {}
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
