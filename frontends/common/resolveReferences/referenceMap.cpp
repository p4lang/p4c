#include "referenceMap.h"
#include <sstream>

namespace P4 {
void ReferenceMap::setDeclaration(const IR::Path* path, const IR::IDeclaration* decl) {
    CHECK_NULL(path);
    CHECK_NULL(decl);
    LOG1("Resolved " << path << " to " << decl);
    auto previous = get(pathToDeclaration, path);
    if (previous != nullptr && previous != decl)
        BUG("%1% already resolved to %2% instead of %3%",
                                path, previous, decl);
    pathToDeclaration.emplace(path, decl);
    usedNames.insert(path->name.name);
    used.insert(decl);
}

const IR::IDeclaration* ReferenceMap::getDeclaration(const IR::Path* path, bool notNull) const {
    CHECK_NULL(path);
    auto result = get(pathToDeclaration, path);
    LOG1("Looking up " << path << " found " << result->getNode());
    if (notNull)
        CHECK_NULL(result);
    return result;
}

void ReferenceMap::dbprint(std::ostream &out) const {
    if (pathToDeclaration.empty())
        out << "Empty" << std::endl;
    for (auto e : pathToDeclaration)
        out << e.first << "->" << e.second << std::endl;
}

cstring ReferenceMap::newName(cstring base) {
    // Maybe in the future we'll maintain information with per-scope identifiers,
    // but today we are content to generate globally-unique identifiers.
    cstring name = cstring::make_unique(usedNames, base, '_');
    usedNames.insert(name);
    return name;
}

}  // namespace P4
