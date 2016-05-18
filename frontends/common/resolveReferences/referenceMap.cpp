#include "referenceMap.h"
#include "frontends/p4/reservedWords.h"
#include <sstream>

namespace P4 {

ReferenceMap::ReferenceMap() : ProgramMap("ReferenceMap") { clear(); }

void ReferenceMap::clear() {
    pathToDeclaration.clear();
    usedNames.clear();
    used.clear();
    usedNames.insert(P4::reservedWords.begin(), P4::reservedWords.end());
}

void ReferenceMap::setDeclaration(const IR::Path* path, const IR::IDeclaration* decl) {
    CHECK_NULL(path);
    CHECK_NULL(decl);
    LOG1("Resolved " << path << " to " << decl);
    auto previous = get(pathToDeclaration, path);
    if (previous != nullptr && previous != decl)
        BUG("%1% already resolved to %2% instead of %3%",
                                path, previous, decl);
    pathToDeclaration.emplace(path, decl);
    usedName(path->name.name);
    used.insert(decl);
}

const IR::IDeclaration* ReferenceMap::getDeclaration(const IR::Path* path, bool notNull) const {
    CHECK_NULL(path);
    auto result = get(pathToDeclaration, path);
    LOG1("Looking up " << path << " found " << result->getNode());
    if (notNull)
        BUG_CHECK(result != nullptr, "Cannot find declaration for %1%", path);
    return result;
}

void ReferenceMap::dbprint(std::ostream &out) const {
    if (pathToDeclaration.empty())
        out << "Empty" << std::endl;
    for (auto e : pathToDeclaration)
        out << dbp(e.first) << "->" << dbp(e.second) << std::endl;
}

cstring ReferenceMap::newName(cstring base) {
    // Maybe in the future we'll maintain information with per-scope identifiers,
    // but today we are content to generate globally-unique identifiers.

    // If base has a suffix of the form _(\d+), then we discard the suffix.
    // under the assumption that it is probably a generated suffix.
    // This will not impact correctness.
    unsigned len = base.size();
    const char digits[] = "0123456789";
    const char* s = base.c_str();
    while (len > 0 && strchr(digits, s[len-1])) len--;
    if (len > 0 && base[len - 1] == '_')
        base = base.substr(0, len - 1);

    cstring name = cstring::make_unique(usedNames, base, '_');
    usedNames.insert(name);
    return name;
}

}  // namespace P4
