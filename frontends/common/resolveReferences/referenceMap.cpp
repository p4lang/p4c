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

#include "referenceMap.h"

#include <string.h>

#include <list>
#include <sstream>
#include <utility>

#include "frontends/p4/reservedWords.h"
#include "lib/exceptions.h"
#include "lib/log.h"
#include "lib/map.h"
#include "lib/null.h"

namespace P4 {

MinimalNameGenerator::MinimalNameGenerator() {
    for (auto &reserved : P4::reservedWords) usedNames.insert({reserved, 0});
}

ReferenceMap::ReferenceMap() : ProgramMap("ReferenceMap"), isv1(false) { clear(); }

void ReferenceMap::clear() {
    LOG2("Clearing reference map");
    pathToDeclaration.clear();
    usedNames.clear();
    used.clear();
    thisToDeclaration.clear();
    for (auto &reserved : P4::reservedWords) usedNames.insert({reserved, 0});
    ProgramMap::clear();
}

void ReferenceMap::setDeclaration(const IR::Path *path, const IR::IDeclaration *decl) {
    CHECK_NULL(path);
    CHECK_NULL(decl);
    LOG3("Resolved " << dbp(path) << " to " << dbp(decl));
    auto previous = get(pathToDeclaration, path);
    if (previous != nullptr && previous != decl)
        BUG("%1% already resolved to %2% instead of %3%", dbp(path), dbp(previous),
            dbp(decl->getNode()));
    pathToDeclaration.emplace(path, decl);
    usedName(path->name.name);
    used.insert(decl);
}

void ReferenceMap::setDeclaration(const IR::This *pointer, const IR::IDeclaration *decl) {
    CHECK_NULL(pointer);
    CHECK_NULL(decl);
    LOG3("Resolved " << dbp(pointer) << " to " << dbp(decl));
    auto previous = get(thisToDeclaration, pointer);
    if (previous != nullptr && previous != decl)
        BUG("%1% already resolved to %2% instead of %3%", dbp(pointer), dbp(previous), dbp(decl));
    thisToDeclaration.emplace(pointer, decl);
}

const IR::IDeclaration *ReferenceMap::getDeclaration(const IR::This *pointer, bool notNull) const {
    CHECK_NULL(pointer);
    auto result = get(thisToDeclaration, pointer);

    if (result)
        LOG3("Looking up " << dbp(pointer) << " found " << dbp(result));
    else
        LOG3("Looking up " << dbp(pointer) << " found nothing");

    if (notNull) BUG_CHECK(result != nullptr, "Cannot find declaration for %1%", pointer);
    return result;
}

const IR::IDeclaration *ReferenceMap::getDeclaration(const IR::Path *path, bool notNull) const {
    CHECK_NULL(path);
    auto result = get(pathToDeclaration, path);

    if (result)
        LOG3("Looking up " << dbp(path) << " found " << dbp(result));
    else
        LOG3("Looking up " << dbp(path) << " found nothing");

    if (notNull) BUG_CHECK(result != nullptr, "Cannot find declaration for %1%", path);
    return result;
}

void ReferenceMap::dbprint(std::ostream &out) const {
    if (pathToDeclaration.empty()) out << "Empty" << std::endl;
    for (auto e : pathToDeclaration) out << dbp(e.first) << "->" << dbp(e.second) << std::endl;
}

cstring ReferenceMap::newName(cstring base) {
    // Maybe in the future we'll maintain information with per-scope identifiers,
    // but today we are content to generate globally-unique identifiers.

    // If base has a suffix of the form _(\d+), then we discard the suffix.
    // under the assumption that it is probably a generated suffix.
    // This will not impact correctness.
    unsigned len = base.size();
    const char digits[] = "0123456789";
    const char *s = base.c_str();
    while (len > 0 && strchr(digits, s[len - 1])) len--;
    if (len > 0 && base[len - 1] == '_') base = base.substr(0, len - 1);

    cstring name = base;
    if (usedNames.count(name)) name = cstring::make_unique(usedNames, name, usedNames[base], '_');
    usedNames.insert({name, 0});
    return name;
}

cstring MinimalNameGenerator::newName(cstring base) {
    // Maybe in the future we'll maintain information with per-scope identifiers,
    // but today we are content to generate globally-unique identifiers.

    // If base has a suffix of the form _(\d+), then we discard the suffix.
    // under the assumption that it is probably a generated suffix.
    // This will not impact correctness.
    unsigned len = base.size();
    const char digits[] = "0123456789";
    const char *s = base.c_str();
    while (len > 0 && strchr(digits, s[len - 1])) len--;
    if (len > 0 && base[len - 1] == '_') base = base.substr(0, len - 1);

    cstring name = base;
    if (usedNames.count(name)) name = cstring::make_unique(usedNames, name, usedNames[base], '_');
    usedNames.insert({name, 0});
    return name;
}

}  // namespace P4
