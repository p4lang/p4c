#include "backends/p4tools/common/lib/namespace_context.h"

#include <set>
#include <vector>

#include "ir/id.h"
#include "lib/enumerator.h"
#include "lib/exceptions.h"

namespace P4Tools {

const NamespaceContext *NamespaceContext::Empty = new NamespaceContext(nullptr, nullptr);

const NamespaceContext *NamespaceContext::push(const IR::INamespace *ns) const {
    return new NamespaceContext(ns, this);
}

const NamespaceContext *NamespaceContext::pop() const {
    BUG_CHECK(this != Empty, "Popped an empty namespace context");
    return outer;
}

const IR::IDeclaration *NamespaceContext::findNestedDecl(
    const IR::INestedNamespace *nestedNameSpace, const IR::Path *path) const {
    auto name = path->name.name;
    const auto namespaces = nestedNameSpace->getNestedNamespaces();
    for (const auto *subNamespace : namespaces) {
        // Handle case where current namespace is an ISimpleNamespace.
        // If there is no match, we fall through. P4 Nodes may have multiple namespace types with
        // different levels of declaration visibility.
        if (const auto *ns = subNamespace->to<IR::ISimpleNamespace>()) {
            if (const auto *decl = ns->getDeclByName(name)) {
                return decl;
            }
        }

        // Handle case where current namespace is an IGeneralNamespace.
        // If there is no match, we fall through.
        if (const auto *ns = subNamespace->to<IR::IGeneralNamespace>()) {
            auto *decl = ns->getDeclsByName(name)->singleOrDefault();
            if (decl != nullptr) return decl;
        }

        // As last resort, fall through to a NestedNamespace check.
        if (const auto *ns = subNamespace->to<IR::INestedNamespace>()) {
            const auto *decl = findNestedDecl(ns, path);
            if (decl != nullptr) {
                return decl;
            }
            // Nothing found in this namespace, move on to the next namespace.
            continue;
        }
    }
    // Nothing found in this nested namespace. Return a nullptr.
    return nullptr;
}

const IR::IDeclaration *NamespaceContext::findDecl(const IR::Path *path) const {
    BUG_CHECK(this != Empty, "Variable %1% not found in the available namespaces.", path);

    // Handle absolute paths by ensuring they are looked up in the outermost non-empty namespace
    // context.
    if (path->absolute && outer != Empty) {
        return outer->findDecl(path);
    }
    auto name = path->name.name;

    // Handle case where current namespace is an ISimpleNamespace.
    // If there is no match, we fall through. P4 Nodes may have multiple namespace types with
    // different levels of declaration visibility.
    if (const auto *ns = curNamespace->to<IR::ISimpleNamespace>()) {
        if (const auto *decl = ns->getDeclByName(name)) {
            return decl;
        }
    }

    // Handle case where current namespace is an IGeneralNamespace.
    // If there is no match, we fall through.
    if (const auto *ns = curNamespace->to<IR::IGeneralNamespace>()) {
        auto *decl = ns->getDeclsByName(name)->singleOrDefault();
        if (decl != nullptr) return decl;
    }
    // As last resort, check if the NestedNamespace contains the declaration.
    if (const auto *ns = curNamespace->to<IR::INestedNamespace>()) {
        const auto *decl = findNestedDecl(ns, path);
        if (decl != nullptr) {
            return decl;
        }
    }
    // Not found at current level. Move on the next level.
    return outer->findDecl(path);
}

const std::set<cstring> &NamespaceContext::getUsedNames() const {
    if (!usedNames) {
        if (this == Empty) {
            usedNames = *new std::set<cstring>();
        } else {
            usedNames = outer->getUsedNames();

            // Add names in curNamespace.
            for (const auto *decl : *curNamespace->getDeclarations()) {
                usedNames->insert(decl->getName());
            }
        }
    }

    return *usedNames;
}

cstring NamespaceContext::genName(cstring name, char sep) const {
    return cstring::make_unique(getUsedNames(), name, sep);
}

}  // namespace P4Tools
