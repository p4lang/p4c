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
#include "p4RuntimeSymbolTable.h"

#include <iosfwd>
#include <unordered_map>

#include <boost/algorithm/string/split.hpp>
#include <boost/range/adaptor/reversed.hpp>

#include "lib/cstring.h"
#include "p4RuntimeArchHandler.h"
#include "typeSpecConverter.h"

namespace P4 {

namespace ControlPlaneAPI {

bool isControllerHeader(const IR::Type_Header *type) {
    return type->getAnnotation("controller_header") != nullptr;
}

bool isHidden(const IR::Node *node) { return node->getAnnotation("hidden") != nullptr; }

std::optional<p4rt_id_t> getIdAnnotation(const IR::IAnnotated *node) {
    const auto *idAnnotation = node->getAnnotation("id");
    if (idAnnotation == nullptr) {
        return std::nullopt;
    }
    const auto *idConstant = idAnnotation->expr[0]->to<IR::Constant>();
    CHECK_NULL(idConstant);
    if (!idConstant->fitsUint()) {
        ::error(ErrorType::ERR_INVALID, "%1%: @id should be an unsigned integer", node);
        return std::nullopt;
    }
    return static_cast<p4rt_id_t>(idConstant->value);
}

/// @return the value of any P4 '@id' annotation @declaration may have, and
/// ensure that the value is correct with respect to the P4Runtime
/// specification. The name 'externalId' is in analogy with externalName().
static std::optional<p4rt_id_t> externalId(const P4RuntimeSymbolType &type,
                                           const IR::IDeclaration *declaration) {
    CHECK_NULL(declaration);
    if (!declaration->is<IR::IAnnotated>()) {
        return std::nullopt;  // Assign an id later; see below.
    }

    // If the user specified an @id annotation, use that.
    auto idOrNone = getIdAnnotation(declaration->to<IR::IAnnotated>());
    if (!idOrNone) {
        return std::nullopt;  // the user didn't assign an id
    }
    auto id = *idOrNone;

    // If the id already has an 8-bit type prefix, make sure it is correct for
    // the resource type; otherwise assign the correct prefix.
    const auto typePrefix = static_cast<p4rt_id_t>(type) << 24;
    const auto prefixMask = static_cast<p4rt_id_t>(0xff) << 24;
    if ((id & prefixMask) != 0 && (id & prefixMask) != typePrefix) {
        ::error(ErrorType::ERR_INVALID, "%1%: @id has the wrong 8-bit prefix", declaration);
        return std::nullopt;
    }
    id |= typePrefix;

    return id;
}

void collectControlSymbols(P4RuntimeSymbolTable &symbols, P4RuntimeArchHandlerIface *archHandler,
                           const IR::ControlBlock *controlBlock, ReferenceMap *refMap,
                           TypeMap *typeMap) {
    CHECK_NULL(controlBlock);
    CHECK_NULL(refMap);
    CHECK_NULL(typeMap);

    const auto *control = controlBlock->container;
    CHECK_NULL(control);

    forAllMatching<IR::P4Action>(&control->controlLocals, [&](const IR::P4Action *action) {
        // Collect the action itself.
        symbols.add(P4RuntimeSymbolType::P4RT_ACTION(), action);

        // Collect any extern functions it may invoke.
        forAllMatching<IR::MethodCallExpression>(
            action->body, [&](const IR::MethodCallExpression *call) {
                auto *instance = P4::MethodInstance::resolve(call, refMap, typeMap);
                if (instance->is<P4::ExternFunction>()) {
                    archHandler->collectExternFunction(&symbols,
                                                       instance->to<P4::ExternFunction>());
                }
            });
    });

    // Collect any use of something in an assignment statement
    forAllMatching<IR::AssignmentStatement>(
        control->body, [&](const IR::AssignmentStatement *assign) {
            archHandler->collectAssignmentStatement(&symbols, assign);
        });

    // Collect any extern function invoked directly from the control.
    forAllMatching<IR::MethodCallExpression>(
        control->body, [&](const IR::MethodCallExpression *call) {
            auto *instance = P4::MethodInstance::resolve(call, refMap, typeMap);
            if (instance->is<P4::ExternFunction>()) {
                archHandler->collectExternFunction(&symbols, instance->to<P4::ExternFunction>());
            } else if (instance->is<P4::ExternMethod>()) {
                archHandler->collectExternMethod(&symbols, instance->to<P4::ExternMethod>());
            }
        });
}

void collectExternSymbols(P4RuntimeSymbolTable &symbols, P4RuntimeArchHandlerIface *archHandler,
                          const IR::ExternBlock *externBlock) {
    CHECK_NULL(externBlock);
    archHandler->collectExternInstance(&symbols, externBlock);
}

void collectTableSymbols(P4RuntimeSymbolTable &symbols, P4RuntimeArchHandlerIface *archHandler,
                         const IR::TableBlock *tableBlock) {
    CHECK_NULL(tableBlock);
    auto name = archHandler->getControlPlaneName(tableBlock);
    auto id = externalId(P4RuntimeSymbolType::P4RT_TABLE(), tableBlock->container);
    symbols.add(P4RuntimeSymbolType::P4RT_TABLE(), name, id);
    archHandler->collectTableProperties(&symbols, tableBlock);
}

void collectParserSymbols(P4RuntimeSymbolTable &symbols, const IR::ParserBlock *parserBlock) {
    CHECK_NULL(parserBlock);

    const auto *parser = parserBlock->container;
    CHECK_NULL(parser);

    for (const auto *s : parser->parserLocals) {
        if (const auto *inst = s->to<IR::P4ValueSet>()) {
            symbols.add(P4RuntimeSymbolType::P4RT_VALUE_SET(), inst);
        }
    }
}

P4::ControlPlaneAPI::P4RuntimeSymbolTable *
P4::ControlPlaneAPI::P4RuntimeSymbolTable::generateSymbols(
    const IR::P4Program *program, const IR::ToplevelBlock *evaluatedProgram, ReferenceMap *refMap,
    TypeMap *typeMap, P4RuntimeArchHandlerIface *archHandler) {
    return P4RuntimeSymbolTable::create([=](P4RuntimeSymbolTable &symbols) {
        Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block *block) {
            if (block->is<IR::ControlBlock>()) {
                collectControlSymbols(symbols, archHandler, block->to<IR::ControlBlock>(), refMap,
                                      typeMap);
            } else if (block->is<IR::ExternBlock>()) {
                collectExternSymbols(symbols, archHandler, block->to<IR::ExternBlock>());
            } else if (block->is<IR::TableBlock>()) {
                collectTableSymbols(symbols, archHandler, block->to<IR::TableBlock>());
            } else if (block->is<IR::ParserBlock>()) {
                collectParserSymbols(symbols, block->to<IR::ParserBlock>());
            }
        });
        forAllMatching<IR::Type_Header>(program, [&](const IR::Type_Header *type) {
            if (isControllerHeader(type)) {
                symbols.add(P4RuntimeSymbolType::P4RT_CONTROLLER_HEADER(), type);
            }
        });
        archHandler->collectExtra(&symbols);
    });
}

void P4::ControlPlaneAPI::P4RuntimeSymbolTable::add(P4RuntimeSymbolType type,
                                                    const IR::IDeclaration *declaration) {
    CHECK_NULL(declaration);
    add(type, declaration->controlPlaneName(), externalId(type, declaration));
}

void P4::ControlPlaneAPI::P4RuntimeSymbolTable::add(P4RuntimeSymbolType type, cstring name,
                                                    std::optional<p4rt_id_t> id) {
    auto &symbolTable = symbolTables[type];
    if (symbolTable.find(name) != symbolTable.end()) {
        return;  // This is a duplicate, but that's OK.
    }

    symbolTable[name] = tryToAssignId(id);
    suffixSet.addSymbol(name);
}

P4::ControlPlaneAPI::p4rt_id_t P4::ControlPlaneAPI::P4RuntimeSymbolTable::getId(
    P4RuntimeSymbolType type, const IR::IDeclaration *declaration) const {
    CHECK_NULL(declaration);
    return getId(type, declaration->controlPlaneName());
}

P4::ControlPlaneAPI::p4rt_id_t P4::ControlPlaneAPI::P4RuntimeSymbolTable::getId(
    P4RuntimeSymbolType type, cstring name) const {
    const auto symbolTable = symbolTables.find(type);

    if (symbolTable == symbolTables.end()) {
        BUG("Invalid symbol type");
    }
    const auto symbolId = symbolTable->second.find(name);
    if (symbolId == symbolTable->second.end()) {
        BUG("Looking up symbol '%1%' without adding it to the table", name);
    }
    return symbolId->second;
}

cstring P4::ControlPlaneAPI::P4RuntimeSymbolTable::getAlias(cstring name) const {
    return suffixSet.shortestUniqueSuffix(name);
}

P4::ControlPlaneAPI::p4rt_id_t P4::ControlPlaneAPI::P4RuntimeSymbolTable::tryToAssignId(
    std::optional<p4rt_id_t> id) {
    if (!id) {
        // The user didn't assign an id, so return the special value
        // INVALID_ID to indicate that computeIds() should assign one later.
        return INVALID_ID;
    }

    if (assignedIds.find(*id) != assignedIds.end()) {
        ::error(ErrorType::ERR_INVALID, "@id %1% is assigned to multiple declarations", *id);
        return INVALID_ID;
    }

    assignedIds.insert(*id);
    return *id;
}

void P4::ControlPlaneAPI::P4RuntimeSymbolTable::computeIdsForSymbols(P4RuntimeSymbolType type) {
    // The id for most resources follows a standard format:
    //
    //   [resource type] [name hash value]
    //    \____8_b____/   \_____24_b____/
    auto &symbolTable = symbolTables.at(type);
    auto resourceType = static_cast<p4rt_id_t>(type);

    // Extract the names of every resource in the collection that does not
    // already have an id assigned and associate them with an iterator that
    // we can use to access them again later.  The names are stored in a
    // std::map to ensure that they're sorted. This is necessary to provide
    // deterministic ids; see below for details.
    std::map<cstring, SymbolTable::iterator> nameToIteratorMap;
    for (auto it = symbolTable.begin(); it != symbolTable.end(); it++) {
        if (it->second == INVALID_ID) {
            nameToIteratorMap.emplace(std::make_pair(it->first, it));
        }
    }

    for (const auto &mapping : nameToIteratorMap) {
        const cstring name = mapping.first;
        const auto iterator = mapping.second;
        const uint32_t nameId = jenkinsOneAtATimeHash(name.c_str(), name.size());

        // Hash the name and construct an id. Because linear probing is used
        // to resolve hash collisions, the id that we select depends on the
        // order in which the names are hashed. This is why we sort the
        // names above.
        std::optional<p4rt_id_t> id = probeForId(
            nameId, [=](uint32_t nameId) { return (resourceType << 24) | (nameId & 0xffffff); });

        if (!id) {
            ::error(ErrorType::ERR_OVERLIMIT, "No available id to represent %1% in P4Runtime",
                    name);
            return;
        }

        // Update the resource in place with the new id.
        assignedIds.insert(*id);
        iterator->second = *id;
    }
}

uint32_t P4::ControlPlaneAPI::P4RuntimeSymbolTable::jenkinsOneAtATimeHash(const char *key,
                                                                          size_t length) {
    size_t i = 0;
    uint32_t hash = 0;
    while (i != length) {
        hash += uint8_t(key[i++]);
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

cstring P4::ControlPlaneAPI::P4SymbolSuffixSet::shortestUniqueSuffix(const cstring &symbol) const {
    BUG_CHECK(!symbol.isNullOrEmpty(), "Null or empty symbol name?");
    std::vector<cstring> components;
    const char *cSymbol = symbol.c_str();
    boost::split(components, cSymbol, [](char c) { return c == '.'; });

    // Determine how many suffix components we need to uniquely identify
    // this symbol. For example, if we have the symbols "d.a.c" and "e.b.c",
    // the suffixes "a.c" and "b.c" are enough to identify the symbols
    // uniquely, so in both cases we only need two components.
    unsigned neededComponents = 0;
    auto *node = suffixesRoot;
    for (auto &component : boost::adaptors::reverse(components)) {
        if (node->edges.find(component) == node->edges.end()) {
            BUG("Symbol is not in suffix set: %1%", symbol);
        }

        node = node->edges[component];
        neededComponents++;

        // If there's only one suffix that passes through this node, we have
        // a unique suffix right now, and we don't need the remaining
        // components.
        if (node->instances < 2) {
            break;
        }
    }

    // Serialize the suffix components into the final unique suffix that
    // we'll return.
    BUG_CHECK(neededComponents <= components.size(), "Too many components?");
    std::string uniqueSuffix;
    std::for_each(components.end() - neededComponents, components.end(),
                  [&](const cstring &component) {
                      if (!uniqueSuffix.empty()) {
                          uniqueSuffix.append(".");
                      }
                      uniqueSuffix.append(component);
                  });

    return uniqueSuffix;
}

void P4::ControlPlaneAPI::P4SymbolSuffixSet::addSymbol(const cstring &symbol) {
    BUG_CHECK(!symbol.isNullOrEmpty(), "Null or empty symbol name?");

    // Check if the symbol is already in the set. This is necessary because
    // adding the same symbol more than once will break the algorithm below.
    // TODO(antonin): In the future we may be able to eliminate this check,
    // since we already check for duplicate symbols in P4RuntimeSymbolTable.
    // There are some edge cases, though - for example, symbols of different
    // types can have the same name, which can happen in P4-14 natively and
    // in P4-16 due to annotations. Until we handle those cases more
    // strictly and have tests for them, it's safest to ensure this
    // precondition here.
    {
        auto result = symbols.insert(symbol);
        if (!result.second) {
            return;  // It was already present.
        }
    }

    // Split the symbol name into dot-separated components.
    std::vector<cstring> components;
    const char *cSymbol = symbol.c_str();
    boost::split(components, cSymbol, [](char c) { return c == '.'; });

    // Insert the components into our tree of suffixes. We work
    // right-to-left through the symbol name, since we're concerned with
    // suffixes. The edges represent components, and the nodes track how
    // many suffixes pass through that node. For example, if we have symbols
    // "a.b.c", "b.c", and "a.d.c", the tree will look like this:
    //   (root) -> "c" -> (3) -> "b" -> (2) -> "a" -> (1)
    //                       \-> "d" -> (1) -> "a" -> (1)
    // (Nodes are in parentheses, and edge labels are in quotes.)
    auto *node = suffixesRoot;
    for (auto &component : boost::adaptors::reverse(components)) {
        if (node->edges.find(component) == node->edges.end()) {
            node->edges[component] = new SuffixNode;
        }
        node = node->edges[component];
        node->instances++;
    }
}

}  // namespace ControlPlaneAPI

}  // namespace P4
