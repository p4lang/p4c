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

#ifndef CONTROL_PLANE_P4RUNTIMESYMBOLTABLE_H_
#define CONTROL_PLANE_P4RUNTIMESYMBOLTABLE_H_

#include "lib/cstring.h"
#include "p4RuntimeArchHandler.h"
#include "typeSpecConverter.h"

namespace p4 {
namespace config {
namespace v1 {
class P4Info;
}  // namespace v1
}  // namespace config
namespace v1 {
class WriteRequest;
}  // namespace v1
}  // namespace p4

namespace IR {
class P4Program;
}  // namespace IR

namespace P4 {

namespace ControlPlaneAPI {

const p4rt_id_t INVALID_ID = ::p4::config::v1::P4Ids::UNSPECIFIED;

/// @return true if @type has an @controller_header annotation.
bool isControllerHeader(const IR::Type_Header *type);

/// @return true if @node has an @hidden annotation.
bool isHidden(const IR::Node *node);

/// @return the id allocated to the object through the @id annotation if any, or
/// std::nullopt.
std::optional<p4rt_id_t> getIdAnnotation(const IR::IAnnotated *node);

/**
 * Stores a set of P4 symbol suffixes. Symbols consist of path components
 * separated by '.'; the suffixes this set stores consist of these components
 * rather than individual characters. The information in this set can be used
 * determine the shortest unique suffix for a P4 symbol.
 */
struct P4SymbolSuffixSet {
    /// Adds @symbol's suffixes to the set if it's not already present.
    void addSymbol(const cstring &symbol);

    cstring shortestUniqueSuffix(const cstring &symbol) const;

 private:
    // All symbols in the set. We store these separately to make sure that no
    // symbol is added to the tree of suffixes more than once.
    std::set<cstring> symbols;

    // A node in the tree of suffixes. The tree of suffixes is a directed graph
    // of path components, with the edges pointing from the each component to
    // its predecessor, so that every suffix of every symbol corresponds to a
    // path through the tree. For example, "foo.bar[1].baz" would be represented
    // as "baz" -> "bar[1]" -> "foo".
    struct SuffixNode {
        // How many suffixes pass through this node? This includes suffixes that
        // terminate at this node.
        unsigned instances = 0;

        // Outgoing edges from this node. The SuffixNode should never be null.
        std::map<cstring, SuffixNode *> edges;
    };

    // The root of our tree of suffixes. Note that this is *not* the data
    // structure known as a suffix tree.
    SuffixNode *suffixesRoot = new SuffixNode;
};

/// A table which tracks the symbols which are visible to P4Runtime and their
/// ids.
class P4RuntimeSymbolTable : public P4RuntimeSymbolTableIface {
 public:
    /**
     * @return a fully constructed P4Runtime symbol table with a unique id
     * computed for each symbol. The table is populated by @function, which can
     * call the various add*() methods on this class.
     *
     * This approach of constructing the symbol table is intended to encourage
     * correct usage. The symbol table should be used in phases: first, we
     * collect symbols and populate the table. Then, ids are assigned. Finally,
     * the P4Runtime serialization code can read the ids from the table as
     * needed. To ensure that no code accidentally adds new symbols after ids
     * are assigned, create() enforces that only code that runs before id
     * assignment has access to a non-const reference to the symbol table.
     */
    template <typename Func>
    static P4RuntimeSymbolTable *create(Func function) {
        // Create and initialize the symbol table. At this stage, ids aren't
        // available, because computing ids requires global knowledge of all the
        // P4Runtime symbols in the program.
        auto *symbols = new P4RuntimeSymbolTable();
        function(*symbols);

        // Now that the symbol table is initialized, we can compute ids.
        for (auto &table : symbols->symbolTables) {
            symbols->computeIdsForSymbols(table.first);
        }

        return symbols;
    }

    static P4RuntimeSymbolTable *generateSymbols(const IR::P4Program *program,
                                                 const IR::ToplevelBlock *evaluatedProgram,
                                                 ReferenceMap *refMap, TypeMap *typeMap,
                                                 P4RuntimeArchHandlerIface *archHandler);

    /// Add a @type symbol, extracting the name and id from @declaration.
    void add(P4RuntimeSymbolType type, const IR::IDeclaration *declaration) override;

    /// Add a @type symbol with @name and possibly an explicit P4 '@id'.
    void add(P4RuntimeSymbolType type, cstring name,
             std::optional<p4rt_id_t> id = std::nullopt) override;

    /// @return the P4Runtime id for the symbol of @type corresponding to
    /// @declaration.
    p4rt_id_t getId(P4RuntimeSymbolType type, const IR::IDeclaration *declaration) const override;

    /// @return the P4Runtime id for the symbol of @type with name @name.
    p4rt_id_t getId(P4RuntimeSymbolType type, cstring name) const override;

    /// @return the alias for the given fully qualified external name. P4Runtime
    /// defines an alias for each object to make referring to objects easier.
    /// By default, the alias is the shortest unique suffix of path components
    /// in the name.
    cstring getAlias(cstring name) const override;

 private:
    // Rather than call this constructor, use P4RuntimeSymbolTable::create().
    P4RuntimeSymbolTable() = default;

    /// @return an initial (possibly invalid) id for a resource, and if the id
    /// is not invalid, record the assignment. An initial @id typically comes
    /// from the P4 '@id' annotation.
    p4rt_id_t tryToAssignId(std::optional<p4rt_id_t> id);

    /**
     * Assign an id to each resource of @type (ACTION, TABLE, etc..)  which does
     * not yet have an id, and update the resource in place.  Existing ids are
     * avoided to ensure that each id is unique.
     */
    void computeIdsForSymbols(P4RuntimeSymbolType type);

    /**
     * Construct an id from the provided @sourceValue using the provided
     * function
     * @constructId, which is expected to be pure. If there's a collision,
     * @sourceValue is incremented and the id is recomputed until an available
     * id is found. We perform linear probing of @sourceValue rather than the id
     * itself to ensure that we don't end up affecting bits of the id that
     * should not depend on @sourceValue.  For example, @constructId will
     * normally set some of the bits in the id it generates to a fixed value
     * indicating a resource type, and those bits need to remain correct.
     */
    template <typename ConstructIdFunc>
    std::optional<p4rt_id_t> probeForId(const uint32_t sourceValue, ConstructIdFunc constructId) {
        uint32_t value = sourceValue;
        while (assignedIds.find(constructId(value)) != assignedIds.end()) {
            ++value;
            if (value == sourceValue) {
                return std::nullopt;  // We wrapped around; there's no unassigned
                                      // id left.
            }
        }

        return constructId(value);
    }

    // The hash function used for resource names.
    // Taken from: https://en.wikipedia.org/wiki/Jenkins_hash_function
    static uint32_t jenkinsOneAtATimeHash(const char *key, size_t length);

    // All the ids we've assigned so far. Used to avoid id collisions; this is
    // especially crucial since ids can be set manually via the '@id'
    // annotation.
    std::set<p4rt_id_t> assignedIds;

    // Symbol tables, mapping symbols to P4Runtime ids.
    using SymbolTable = std::map<cstring, p4rt_id_t>;
    std::map<P4RuntimeSymbolType, SymbolTable> symbolTables{};

    // A set which contains all the symbols in the program. It's used to compute
    // the shortest unique suffix of each symbol, which is the default alias we
    // use for P4Runtime objects.
    P4SymbolSuffixSet suffixSet;
};

void collectControlSymbols(P4RuntimeSymbolTable &symbols, P4RuntimeArchHandlerIface *archHandler,
                           const IR::ControlBlock *controlBlock, ReferenceMap *refMap,
                           TypeMap *typeMap);

void collectExternSymbols(P4RuntimeSymbolTable &symbols, P4RuntimeArchHandlerIface *archHandler,
                          const IR::ExternBlock *externBlock);

void collectTableSymbols(P4RuntimeSymbolTable &symbols, P4RuntimeArchHandlerIface *archHandler,
                         const IR::TableBlock *tableBlock);

void collectParserSymbols(P4RuntimeSymbolTable &symbols, const IR::ParserBlock *parserBlock);

}  // namespace ControlPlaneAPI

}  // namespace P4

#endif /* CONTROL_PLANE_P4RUNTIMESYMBOLTABLE_H_ */
