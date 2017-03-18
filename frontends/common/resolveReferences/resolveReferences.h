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

#ifndef _COMMON_RESOLVEREFERENCES_RESOLVEREFERENCES_H_
#define _COMMON_RESOLVEREFERENCES_RESOLVEREFERENCES_H_

#include "ir/ir.h"
#include "referenceMap.h"
#include "lib/exceptions.h"
#include "lib/cstring.h"

namespace P4 {
enum class ResolutionType {
    Any,
    Type,
    TypeVariable
};

class ResolutionContext : public IHasDbPrint {
    std::vector<const IR::INamespace*> stack;
    const IR::INamespace* rootNamespace;
    std::vector<const IR::INamespace*> globals;  // match_kind

 public:
    explicit ResolutionContext(const IR::INamespace* rootNamespace) :
            rootNamespace(rootNamespace)
    { push(rootNamespace); }

    void dbprint(std::ostream& out) const;
    void addGlobal(const IR::INamespace* e) {
        globals.push_back(e);
    }
    void push(const IR::INamespace* element) {
        CHECK_NULL(element);
        stack.push_back(element);
    }
    void pop(const IR::INamespace* element) {
        if (stack.empty())
            BUG("Empty stack in ResolutionContext::pop");
        const IR::INamespace* node = stack.back();
        if (node != element)
            BUG("Expected %1% on stack, found %2%", element, node);
        stack.pop_back();
    }
    void done();

    // Resolve a reference for the specified name.
    // The reference is restricted to be to an object of the specified type
    // If previousOnly is true, the reference must precede the point of the 'name' in the program
    std::vector<const IR::IDeclaration*>*
    resolve(IR::ID name, ResolutionType type, bool previousOnly) const;

    // Resolve a reference for the specified name; expect a single result
    const IR::IDeclaration*
    resolveUnique(IR::ID name, ResolutionType type, bool previousOnly) const;

    // Resolve a Type_Name to a concrete type
    const IR::Type *resolveType(const IR::Type *type) const;
};

// No prerequisites, but it usually must be run over the whole program.
// Builds output in refMap.
// The ReferenceMap maps each Path to a declaration.
class ResolveReferences : public Inspector {
    // Output: reference map
    ReferenceMap* refMap;
    ResolutionContext* context;
    const IR::INamespace* rootNamespace;
    // used as a stack
    std::vector<bool> resolveForward;  // if true allow resolution with declarations that follow use
    bool anyOrder;
    bool checkShadow;

 private:
    void addToContext(const IR::INamespace* ns);
    void removeFromContext(const IR::INamespace* ns);
    void addToGlobals(const IR::INamespace* ns);
    void resolvePath(const IR::Path* path, bool isType) const;

 public:
    explicit ResolveReferences(/* out */ P4::ReferenceMap* refMap,
                               bool checkShadow = false);

    Visitor::profile_t init_apply(const IR::Node* node) override;
    void end_apply(const IR::Node* node) override;
    using Inspector::preorder;
    using Inspector::postorder;

    bool preorder(const IR::Type_Name* type) override;
    bool preorder(const IR::PathExpression* path) override;
    bool preorder(const IR::This* pointer) override;

#define DECLARE(TYPE)                           \
    bool preorder(const IR::TYPE* t) override;  \
    void postorder(const IR::TYPE* t) override; \

    DECLARE(P4Program)
    DECLARE(P4Control)
    DECLARE(P4Parser)
    DECLARE(P4Action)
    DECLARE(Function)
    DECLARE(TableProperties)
    DECLARE(Type_Method)
    DECLARE(ParserState)
    DECLARE(Type_Extern)
    DECLARE(Type_ArchBlock)
    DECLARE(Type_StructLike)
    DECLARE(BlockStatement)
    DECLARE(Declaration_Instance)
    DECLARE(Property)
#undef DECLARE

    bool preorder(const IR::P4Table* table) override;
    bool preorder(const IR::Declaration_MatchKind* d) override;
    bool preorder(const IR::Declaration* d) override
    { refMap->usedName(d->getName().name); return true; }
    bool preorder(const IR::Type_Declaration* d) override
    { refMap->usedName(d->getName().name); return true; }

    void checkShadowing(const IR::INamespace*ns) const;
};

}  // namespace P4

#endif /* _COMMON_RESOLVEREFERENCES_RESOLVEREFERENCES_H_ */
