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

#ifndef COMMON_RESOLVEREFERENCES_RESOLVEREFERENCES_H_
#define COMMON_RESOLVEREFERENCES_RESOLVEREFERENCES_H_

#include "absl/container/flat_hash_map.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/iterator_range.h"
#include "referenceMap.h"

namespace P4 {

/// Helper class to indicate types of nodes that may be returned during resolution.
enum class ResolutionType { Any, Type, TypeVariable };

/// Visitor mixin for looking up names in enclosing scopes from the Visitor::Context
class ResolutionContext : virtual public Visitor, public DeclarationLookup {
 private:
    // Returns a vector of the decls that exist in the given namespace, and caches the result
    // for future lookups.
    const std::vector<const IR::IDeclaration *> &memoizeDeclarations(
        const IR::INamespace *ns) const;

    // Returns a mapping from name -> decl for the given namespace, and caches the result for
    // future lookups.
    std::unordered_multimap<cstring, const IR::IDeclaration *> &memoizeDeclsByName(
        const IR::INamespace *ns) const;

    mutable absl::flat_hash_map<const IR::INamespace *, std::vector<const IR::IDeclaration *>,
                                Util::Hash>
        namespaceDecls;
    mutable absl::flat_hash_map<const IR::INamespace *,
                                std::unordered_multimap<cstring, const IR::IDeclaration *>,
                                Util::Hash>
        namespaceDeclNames;

 protected:
    // Note that all errors have been merged by the parser into
    // a single error { } namespace.
    std::vector<const IR::IDeclaration *> lookup(const IR::INamespace *ns, const IR::ID &name,
                                                 ResolutionType type) const;

    // match kinds exist in their own special namespace, made from all the match_kind
    // declarations in the global scope.  Unlike errors, we don't merge those scopes in
    // the parser, so we have to find them and scan them here.
    std::vector<const IR::IDeclaration *> lookupMatchKind(const IR::ID &name) const;

    // P4_14 allows things to be used before their declaration while P4_16 (generally)
    // does not, so we will resolve names to things declared later only when translating
    // from P4_14 or Type_Vars or ParserStates, or after code transforms that may reorder
    // the code.
    bool anyOrder;

    ResolutionContext();
    explicit ResolutionContext(bool ao) : anyOrder(ao) {}

    /// We are resolving a method call.  Find the arguments from the context.
    const IR::Vector<IR::Argument> *methodArguments(cstring name) const;

 public:
    /// Resolve references for @p name, restricted to @p type declarations.
    std::vector<const IR::IDeclaration *> resolve(const IR::ID &name, ResolutionType type) const;

    /// Resolve reference for @p name, restricted to @p type declarations, and expect one result.
    const IR::IDeclaration *resolveUnique(const IR::ID &name, ResolutionType type,
                                          const IR::INamespace * = nullptr) const;

    /// Resolve @p path; if @p isType is `true` then resolution will
    /// only return type nodes.
    virtual const IR::IDeclaration *resolvePath(const IR::Path *path, bool isType) const;

    /// Resolve a refrence to a type @p type.
    const IR::Type *resolveType(const IR::Type *type) const;

    const IR::IDeclaration *getDeclaration(const IR::Path *path, bool notNull = false) const;
    const IR::IDeclaration *getDeclaration(const IR::This *, bool notNull = false) const;

    /// Returns the set of decls that exist in the given namespace.
    auto getDeclarations(const IR::INamespace *ns) const {
        auto nsIt = namespaceDecls.find(ns);
        const auto &decls = nsIt != namespaceDecls.end() ? nsIt->second : memoizeDeclarations(ns);
        return Util::iterator_range(decls);
    }

    /// Returns the set of decls with the given name that exist in the given namespace.
    auto getDeclsByName(const IR::INamespace *ns, cstring name) const {
        auto nsIt = namespaceDeclNames.find(ns);
        auto &namesToDecls =
            nsIt != namespaceDeclNames.end() ? nsIt->second : memoizeDeclsByName(ns);

        auto decls = Values(namesToDecls.equal_range(name));
        return Util::iterator_range(decls.begin(), decls.end());
    }
};

/** Inspector that computes `refMap`: a map from paths to declarations.
 *
 * @pre: None
 *
 * @post: produces an up-to-date `refMap`
 *
 */
class ResolveReferences : public Inspector, private ResolutionContext {
    /// Reference map -- essentially from paths to declarations.
    ReferenceMap *refMap;

    /// If @true, then warn if one declaration shadows another.
    bool checkShadow;

 private:
    /// Resolve @p path; if @p isType is `true` then resolution will
    /// only return type nodes.
    const IR::IDeclaration *resolvePath(const IR::Path *path, bool isType) const override;

 public:
    explicit ResolveReferences(/* out */ P4::ReferenceMap *refMap, bool checkShadow = false);

    Visitor::profile_t init_apply(const IR::Node *node) override;
    void end_apply(const IR::Node *node) override;

    bool preorder(const IR::Type_Name *type) override;
    bool preorder(const IR::PathExpression *path) override;
    bool preorder(const IR::KeyElement *path) override;
    bool preorder(const IR::This *pointer) override;
    bool preorder(const IR::Declaration_Instance *decl) override;

    bool preorder(const IR::P4Program *t) override;
    void postorder(const IR::P4Program *t) override;
    bool preorder(const IR::P4Control *t) override;
    bool preorder(const IR::P4Parser *t) override;
    bool preorder(const IR::P4Action *t) override;
    bool preorder(const IR::Function *t) override;
    bool preorder(const IR::TableProperties *t) override;
    bool preorder(const IR::Type_Method *t) override;
    bool preorder(const IR::ParserState *t) override;
    bool preorder(const IR::Type_Extern *t) override;
    bool preorder(const IR::Type_ArchBlock *t) override;
    void postorder(const IR::Type_ArchBlock *t) override;
    bool preorder(const IR::Type_StructLike *t) override;
    bool preorder(const IR::BlockStatement *t) override;

    bool preorder(const IR::P4Table *table) override;
    bool preorder(const IR::Declaration *d) override {
        refMap->usedName(d->getName().name);
        return true;
    }
    bool preorder(const IR::Type_Declaration *d) override {
        refMap->usedName(d->getName().name);
        return true;
    }

    void checkShadowing(const IR::INamespace *ns) const;
};

}  // namespace P4

#endif /* COMMON_RESOLVEREFERENCES_RESOLVEREFERENCES_H_ */
