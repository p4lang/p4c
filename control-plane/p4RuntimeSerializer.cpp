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

#include <algorithm>
#include <iostream>
#include <set>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <google/protobuf/text_format.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <google/protobuf/util/json_util.h>
#include "p4/config/v1/p4info.pb.h"
#include "p4/config/v1/p4types.pb.h"
#include "p4/v1/p4runtime.pb.h"
#pragma GCC diagnostic pop

#include "frontends/common/options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/enumInstance.h"
// TODO(antonin): this include should go away when we cleanup getMatchFields
// and tableNeedsPriority implementations.
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/parseAnnotations.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "lib/ordered_set.h"
#include "midend/removeParameters.h"

#include "p4RuntimeSerializer.h"
#include "p4RuntimeArchHandler.h"
#include "p4RuntimeArchStandard.h"
#include "flattenHeader.h"

namespace p4v1 = ::p4::v1;
namespace p4configv1 = ::p4::config::v1;

namespace P4 {

/** \defgroup control_plane Control Plane API Generation */
/** \addtogroup control_plane
 *  @{
 */
/// TODO(antonin): High level goals of the generator go here!!
namespace ControlPlaneAPI {

using Helpers::addAnnotations;
using Helpers::addDocumentation;
using Helpers::setPreamble;

static const p4rt_id_t INVALID_ID = p4configv1::P4Ids::UNSPECIFIED;

/// @return true if @node has an @hidden annotation.
static bool isHidden(const IR::Node* node) {
    return node->getAnnotation("hidden") != nullptr;
}

/// @return true if @type has an @controller_header annotation.
static bool isControllerHeader(const IR::Type_Header* type) {
    return type->getAnnotation("controller_header") != nullptr;
}

/// @return the value of @item's explicit name annotation, if it has one. We use
/// this rather than e.g. controlPlaneName() when we want to prevent any
/// fallback.
static boost::optional<cstring>
explicitNameAnnotation(const IR::IAnnotated* item) {
    auto* anno = item->getAnnotation(IR::Annotation::nameAnnotation);
    if (!anno) return boost::none;
    if (anno->expr.size() != 1) {
        ::error("A %1% annotation must have one argument", anno);
        return boost::none;
    }
    auto* str = anno->expr[0]->to<IR::StringLiteral>();
    if (!str) {
        ::error("An %1% annotation's argument must be a string", anno);
        return boost::none;
    }
    return str->value;
}

namespace writers {

using google::protobuf::Message;

/// Serialize the protobuf @message to @destination in the binary protocol
/// buffers format.
static bool writeTo(const Message& message, std::ostream* destination) {
    CHECK_NULL(destination);
    if (!message.SerializeToOstream(destination)) return false;
    destination->flush();
    return true;
}

/// Serialize the protobuf @message to @destination in the JSON protocol buffers
/// format. This is intended for debugging and testing.
static bool writeJsonTo(const Message& message, std::ostream* destination) {
    using namespace google::protobuf::util;
    CHECK_NULL(destination);

    // Serialize the JSON in a human-readable format.
    JsonPrintOptions options;
    options.add_whitespace = true;

    std::string output;
    if (MessageToJsonString(message, &output, options) != Status::OK) {
        ::error("Failed to serialize protobuf message to JSON");
        return false;
    }

    *destination << output;
    if (!destination->good()) {
        ::error("Failed to write JSON protobuf message to the output");
        return false;
    }

    destination->flush();
    return true;
}

/// Serialize the protobuf @message to @destination in the text protocol buffers
/// format. This is intended for debugging and testing.
static bool writeTextTo(const Message& message, std::ostream* destination) {
    CHECK_NULL(destination);

    // According to the protobuf documentation, it would be better to use Print
    // with a FileOutputStream object for performance reasons. However all we
    // have here is a std::ostream and performance is not a concern.
    std::string output;
    if (!google::protobuf::TextFormat::PrintToString(message, &output)) {
        ::error("Failed to serialize protobuf message to text");
        return false;
    }

    *destination << output;
    if (!destination->good()) {
        ::error("Failed to write text protobuf message to the output");
        return false;
    }

    destination->flush();
    return true;
}

}  // namespace writers

/// The information about a default action which is needed to serialize it.
struct DefaultAction {
    const cstring name;  // The fully qualified external name of this action.
    const bool isConst;  // Is this a const default action?
};

/// The information about a match field which is needed to serialize it.
struct MatchField {
    using MatchType = p4configv1::MatchField::MatchType;
    using MatchTypes = p4configv1::MatchField;  // Make short enum names visible.

    const cstring name;       // The fully qualified external name of this field.
    const MatchType type;     // The match algorithm - exact, ternary, range, etc.
    const cstring other_match_type;  // If the match type is an arch-specific one
                                     // in this case, type must be MatchTypes::UNSPECIFIED
    const uint32_t bitwidth;  // How wide this field is.
    const IR::IAnnotated* annotations;  // If non-null, any annotations applied
                                        // to this field.
    const cstring type_name;  // Optional field used when field is Type_Newtype.
};

struct ActionRef {
    const cstring name;  // The fully qualified external name of the action.
    const IR::IAnnotated* annotations;  // If non-null, any annotations applied to this action
                                        // reference in the table declaration.
};

/// @return the value of any P4 '@id' annotation @declaration may have. The name
/// 'externalId' is in analogy with externalName().
static boost::optional<p4rt_id_t>
externalId(const IR::IDeclaration* declaration) {
    CHECK_NULL(declaration);
    if (!declaration->is<IR::IAnnotated>()) {
        return boost::none;  // Assign an id later; see below.
    }

    // If the user specified an @id annotation, use that.
    if (auto idAnnotation = declaration->getAnnotation("id")) {
        auto idConstant = idAnnotation->expr[0]->to<IR::Constant>();
        CHECK_NULL(idConstant);
        if (!idConstant->fitsInt()) {
            ::error("@id should be an integer for declaration %1%", declaration);
            return boost::none;
        }

        const uint32_t id = idConstant->value.get_si();
        return id;
    }

    // The user didn't assign an id.
    return boost::none;
}

/**
 * Stores a set of P4 symbol suffixes. Symbols consist of path components
 * separated by '.'; the suffixes this set stores consist of these components
 * rather than individual characters. The information in this set can be used
 * determine the shortest unique suffix for a P4 symbol.
 */
struct P4SymbolSuffixSet {
    /// Adds @symbol's suffixes to the set if it's not already present.
    void addSymbol(const cstring& symbol) {
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
            if (!result.second) return;  // It was already present.
        }

        // Split the symbol name into dot-separated components.
        std::vector<cstring> components;
        const char* cSymbol = symbol.c_str();
        boost::split(components, cSymbol, [](char c) { return c == '.'; });

        // Insert the components into our tree of suffixes. We work
        // right-to-left through the symbol name, since we're concerned with
        // suffixes. The edges represent components, and the nodes track how
        // many suffixes pass through that node. For example, if we have symbols
        // "a.b.c", "b.c", and "a.d.c", the tree will look like this:
        //   (root) -> "c" -> (3) -> "b" -> (2) -> "a" -> (1)
        //                       \-> "d" -> (1) -> "a" -> (1)
        // (Nodes are in parentheses, and edge labels are in quotes.)
        auto* node = suffixesRoot;
        for (auto& component : boost::adaptors::reverse(components)) {
            if (node->edges.find(component) == node->edges.end()) {
                node->edges[component] = new SuffixNode;
            }
            node = node->edges[component];
            node->instances++;
        }
    }

    cstring shortestUniqueSuffix(const cstring& symbol) const {
        BUG_CHECK(!symbol.isNullOrEmpty(), "Null or empty symbol name?");
        std::vector<cstring> components;
        const char* cSymbol = symbol.c_str();
        boost::split(components, cSymbol, [](char c) { return c == '.'; });

        // Determine how many suffix components we need to uniquely identify
        // this symbol. For example, if we have the symbols "d.a.c" and "e.b.c",
        // the suffixes "a.c" and "b.c" are enough to identify the symbols
        // uniquely, so in both cases we only need two components.
        unsigned neededComponents = 0;
        auto* node = suffixesRoot;
        for (auto& component : boost::adaptors::reverse(components)) {
            if (node->edges.find(component) == node->edges.end()) {
                BUG("Symbol is not in suffix set: %1%", symbol);
            }

            node = node->edges[component];
            neededComponents++;

            // If there's only one suffix that passes through this node, we have
            // a unique suffix right now, and we don't need the remaining
            // components.
            if (node->instances < 2) break;
        }

        // Serialize the suffix components into the final unique suffix that
        // we'll return.
        BUG_CHECK(neededComponents <= components.size(), "Too many components?");
        std::string uniqueSuffix;
        std::for_each(components.end() - neededComponents, components.end(),
                      [&](const cstring& component) {
            if (!uniqueSuffix.empty()) uniqueSuffix.append(".");
            uniqueSuffix.append(component);
        });

        return uniqueSuffix;
    }

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
        std::map<cstring, SuffixNode*> edges;
    };

    // The root of our tree of suffixes. Note that this is *not* the data
    // structure known as a suffix tree.
    SuffixNode* suffixesRoot = new SuffixNode;
};

/// A table which tracks the symbols which are visible to P4Runtime and their ids.
class P4RuntimeSymbolTable : public P4RuntimeSymbolTableIface {
 public:
    /**
     * @return a fully constructed P4Runtime symbol table with a unique id
     * computed for each symbol. The table is populated by @function, which can
     * call the various add*() methods on this class.
     *
     * This approach of constructing the symbol table is intended to encourage
     * correct usage. The symbol table should be used in phases: first, we collect
     * symbols and populate the table. Then, ids are assigned. Finally, the
     * P4Runtime serialization code can read the ids from the table as needed. To
     * ensure that no code accidentally adds new symbols after ids are assigned,
     * create() enforces that only code that runs before id assignment has access
     * to a non-const reference to the symbol table.
     */
    template <typename Func>
    static const P4RuntimeSymbolTable create(Func function) {
        // Create and initialize the symbol table. At this stage, ids aren't
        // available, because computing ids requires global knowledge of all the
        // P4Runtime symbols in the program.
        P4RuntimeSymbolTable symbols;
        function(symbols);

        // Now that the symbol table is initialized, we can compute ids.
        for (auto& table : symbols.symbolTables)
            symbols.computeIdsForSymbols(table.first);

        return symbols;
    }

    /// Add a @type symbol, extracting the name and id from @declaration.
    void add(P4RuntimeSymbolType type, const IR::IDeclaration* declaration) override {
        CHECK_NULL(declaration);
        add(type, declaration->controlPlaneName(), externalId(declaration));
    }

    /// Add a @type symbol with @name and possibly an explicit P4 '@id'.
    void add(P4RuntimeSymbolType type, cstring name,
             boost::optional<p4rt_id_t> id = boost::none) override {
        auto& symbolTable = symbolTables[type];
        if (symbolTable.find(name) != symbolTable.end()) {
            return;  // This is a duplicate, but that's OK.
        }

        symbolTable[name] = tryToAssignId(id);
        suffixSet.addSymbol(name);
    }

    /// @return the P4Runtime id for the symbol of @type corresponding to @declaration.
    p4rt_id_t getId(P4RuntimeSymbolType type, const IR::IDeclaration* declaration) const override {
        CHECK_NULL(declaration);
        return getId(type, declaration->controlPlaneName());
    }

    /// @return the P4Runtime id for the symbol of @type with name @name.
    p4rt_id_t getId(P4RuntimeSymbolType type, cstring name) const override {
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

    /// @return the alias for the given fully qualified external name. P4Runtime
    /// defines an alias for each object to make referring to objects easier.
    /// By default, the alias is the shortest unique suffix of path components in
    /// the name.
    cstring getAlias(cstring name) const override {
        return suffixSet.shortestUniqueSuffix(name);
    }

 private:
    // Rather than call this constructor, use P4RuntimeSymbolTable::create().
    P4RuntimeSymbolTable() { }

    /// @return an initial (possibly invalid) id for a resource, and if the id is
    /// not invalid, record the assignment. An initial @id typically comes from
    /// the P4 '@id' annotation.
    p4rt_id_t tryToAssignId(boost::optional<p4rt_id_t> id) {
        if (!id) {
            // The user didn't assign an id, so return the special value INVALID_ID
            // to indicate that computeIds() should assign one later.
            return INVALID_ID;
        }

        if (assignedIds.find(*id) != assignedIds.end()) {
            ::error("@id %1% is assigned to multiple declarations", *id);
            return INVALID_ID;
        }

        assignedIds.insert(*id);
        return *id;
    }

    /**
     * Assign an id to each resource of @type (ACTION, TABLE, etc..)  which does
     * not yet have an id, and update the resource in place.  Existing ids are
     * avoided to ensure that each id is unique.
     */
    void computeIdsForSymbols(P4RuntimeSymbolType type) {
        // The id for most resources follows a standard format:
        //
        //   [resource type] [zero byte] [name hash value]
        //    \____8_b____/   \__8_b__/   \_____16_b____/
        auto& symbolTable = symbolTables.at(type);
        auto resourceType = static_cast<p4rt_id_t>(type);

        // Extract the names of every resource in the collection that does not already
        // have an id assigned and associate them with an iterator that we can use to
        // access them again later.  The names are stored in a std::map to ensure that
        // they're sorted. This is necessary to provide deterministic ids; see below
        // for details.
        std::map<cstring, SymbolTable::iterator> nameToIteratorMap;
        for (auto it = symbolTable.begin(); it != symbolTable.end(); it++) {
            if (it->second == INVALID_ID) {
                nameToIteratorMap.emplace(std::make_pair(it->first, it));
            }
        }

        for (const auto& mapping : nameToIteratorMap) {
            const cstring name = mapping.first;
            const SymbolTable::iterator iterator = mapping.second;
            const uint32_t nameId = jenkinsOneAtATimeHash(name.c_str(), name.size());

            // Hash the name and construct an id. Because linear probing is used to
            // resolve hash collisions, the id that we select depends on the order in
            // which the names are hashed. This is why we sort the names above.
            boost::optional<p4rt_id_t> id = probeForId(nameId, [=](uint32_t nameId) {
                return (resourceType << 24) | (nameId & 0xffff);
            });

            if (!id) {
                ::error("No available id to represent %1% in P4Runtime", name);
                return;
            }

            // Update the resource in place with the new id.
            assignedIds.insert(*id);
            iterator->second = *id;
        }
    }

    /**
     * Construct an id from the provided @sourceValue using the provided function
     * @constructId, which is expected to be pure. If there's a collision,
     * @sourceValue is incremented and the id is recomputed until an available id
     * is found. We perform linear probing of @sourceValue rather than the id
     * itself to ensure that we don't end up affecting bits of the id that should
     * not depend on @sourceValue.  For example, @constructId will normally set
     * some of the bits in the id it generates to a fixed value indicating a
     * resource type, and those bits need to remain correct.
     */
    template <typename ConstructIdFunc>
    boost::optional<p4rt_id_t> probeForId(const uint32_t sourceValue,
                                          ConstructIdFunc constructId) {
        uint32_t value = sourceValue;
        while (assignedIds.find(constructId(value)) != assignedIds.end()) {
            ++value;
            if (value == sourceValue) {
                return boost::none;  // We wrapped around; there's no unassigned id left.
            }
        }

        return constructId(value);
    }

    // The hash function used for resource names.
    // Taken from: https://en.wikipedia.org/wiki/Jenkins_hash_function
    static uint32_t jenkinsOneAtATimeHash(const char* key, size_t length) {
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

    // All the ids we've assigned so far. Used to avoid id collisions; this is
    // especially crucial since ids can be set manually via the '@id' annotation.
    std::set<p4rt_id_t> assignedIds;

    // Symbol tables, mapping symbols to P4Runtime ids.
    using SymbolTable = std::map<cstring, p4rt_id_t>;
    std::map<P4RuntimeSymbolType, SymbolTable> symbolTables{};

    // A set which contains all the symbols in the program. It's used to compute
    // the shortest unique suffix of each symbol, which is the default alias we
    // use for P4Runtime objects.
    P4SymbolSuffixSet suffixSet;
};

/// @return @table's default action, if it has one, or boost::none otherwise.
static boost::optional<DefaultAction>
getDefaultAction(const IR::P4Table* table, ReferenceMap* refMap, TypeMap* typeMap) {
    // not using getDefaultAction() here as I actually need the property IR node
    // to check if the default action is constant.
    auto defaultActionProperty =
        table->properties->getProperty(IR::TableProperties::defaultActionPropertyName);
    if (defaultActionProperty == nullptr) return boost::none;
    if (!defaultActionProperty->value->is<IR::ExpressionValue>()) {
        ::error("Expected an action: %1%", defaultActionProperty);
        return boost::none;
    }

    auto expr =
        defaultActionProperty->value->to<IR::ExpressionValue>()->expression;
    cstring actionName;
    if (expr->is<IR::PathExpression>()) {
        auto decl = refMap->getDeclaration(expr->to<IR::PathExpression>()->path, true);
        BUG_CHECK(decl->is<IR::P4Action>(), "Expected an action: %1%", expr);
        actionName = decl->to<IR::P4Action>()->controlPlaneName();
    } else if (expr->is<IR::MethodCallExpression>()) {
        auto callExpr = expr->to<IR::MethodCallExpression>();
        auto instance = P4::MethodInstance::resolve(callExpr, refMap, typeMap);
        BUG_CHECK(instance->is<P4::ActionCall>(), "Expected an action: %1%", expr);
        actionName = instance->to<P4::ActionCall>()->action->controlPlaneName();
    } else {
        ::error("Unexpected expression in default action for table %1%: %2%",
                table->controlPlaneName(), expr);
        return boost::none;
    }

    return DefaultAction{actionName, defaultActionProperty->isConstant};
}

/// @return true if @table has a 'entries' property. The property must be const
/// as per the current P4_16 specification. The frontend already enforces that
/// check but we perform the check again here in case the constraint is relaxed
/// in the specification in the future.
static bool getConstTable(const IR::P4Table* table) {
    // not using IR::P4Table::getEntries() here as I need to check if the
    // property is constant.
    BUG_CHECK(table != nullptr, "Failed precondition for getConstTable");
    auto ep = table->properties->getProperty(IR::TableProperties::entriesPropertyName);
    if (ep == nullptr) return false;
    BUG_CHECK(ep->value->is<IR::EntriesList>(), "Invalid 'entries' property");
    if (!ep->isConstant)
        ::error("%1%: P4Runtime only supports constant table initializers", ep);
    return true;
}

static std::vector<ActionRef>
getActionRefs(const IR::P4Table* table, ReferenceMap* refMap) {
    std::vector<ActionRef> actions;
    for (auto action : table->getActionList()->actionList) {
        auto decl = refMap->getDeclaration(action->getPath(), true);
        BUG_CHECK(decl->is<IR::P4Action>(), "Not an action: '%1%'", decl);
        auto name = decl->to<IR::P4Action>()->controlPlaneName();
        // get annotations on the reference in the action list, not on the action declaration
        auto annotations = action->to<IR::IAnnotated>();
        actions.push_back(ActionRef{name, annotations});
    }
    return actions;
}

static cstring
getMatchTypeName(const IR::PathExpression* matchPathExpr, const ReferenceMap* refMap) {
    CHECK_NULL(matchPathExpr);
    auto matchTypeDecl = refMap->getDeclaration(matchPathExpr->path, true)
        ->to<IR::Declaration_ID>();
    BUG_CHECK(matchTypeDecl != nullptr, "No declaration for match type '%1%'", matchPathExpr);
    return matchTypeDecl->name.name;
}

/// maps the match type name to the corresponding P4Info MatchType enum
/// member. If the match type should not be exposed to the control plane and
/// should be ignored, boost::none is returned. If the match type does not
/// correspond to any standard match type known to P4Info, default enum member
/// UNSPECIFIED is returned.
static boost::optional<MatchField::MatchType>
getMatchType(cstring matchTypeName) {
    if (matchTypeName == P4CoreLibrary::instance.exactMatch.name) {
        return MatchField::MatchTypes::EXACT;
    } else if (matchTypeName == P4CoreLibrary::instance.lpmMatch.name) {
        return MatchField::MatchTypes::LPM;
    } else if (matchTypeName == P4CoreLibrary::instance.ternaryMatch.name) {
        return MatchField::MatchTypes::TERNARY;
    } else if (matchTypeName == P4V1::V1Model::instance.rangeMatchType.name) {
        return MatchField::MatchTypes::RANGE;
    } else if (matchTypeName == P4V1::V1Model::instance.selectorMatchType.name) {
        // Nothing to do here, we cannot even perform some sanity-checking.
        return boost::none;
    } else {
        return MatchField::MatchTypes::UNSPECIFIED;
    }
}

// P4Runtime defines sdnB as a 32-bit integer.
// The APIs in this file for width use an int
// Thus function returns a signed int.
static int
getTypeWidth(const IR::Type* type, TypeMap* typeMap) {
    auto ann = type->getAnnotation("p4runtime_translation");
    if (ann != nullptr) {
        auto sdnB = ann->expr[1]->to<IR::Constant>();
        if (!sdnB) {
            ::error("P4runtime annotation in serializer does not have sdn: %1%",
                    type);
            return -1;
        }
        auto value = sdnB->value;
        auto bitsRequired = static_cast<size_t>(mpz_sizeinbase(value.get_mpz_t(), 2));
        if (bitsRequired > 31) {
            ::error("Cannot represent %1% on 31 bits, require %2%", value.get_ui(),
                    bitsRequired);
            return -2;
        }
        return static_cast<int>(value.get_ui());
    }
    return typeMap->minWidthBits(type, type->getNode());
}

/*
 * The function returns a cstring for use as type_name for a Type_Newtype.
*/
static cstring
getTypeName(const IR::Type* type, TypeMap* typeMap) {
    CHECK_NULL(type);

    auto t = typeMap->getTypeType(type, true);
    if (auto newt = t->to<IR::Type_Newtype>()) {
        return newt->name;
    }
    return nullptr;
}

/// @return the header instance fields matched against by @table's key. The
/// fields are represented as a (fully qualified field name, match type) tuple.
static std::vector<MatchField>
getMatchFields(const IR::P4Table* table, ReferenceMap* refMap, TypeMap* typeMap) {
    std::vector<MatchField> matchFields;

    auto key = table->getKey();
    if (!key) return matchFields;

    for (auto keyElement : key->keyElements) {
        cstring type_name = nullptr;
        auto matchTypeName = getMatchTypeName(keyElement->matchType, refMap);
        auto matchType = getMatchType(matchTypeName);
        if (matchType == boost::none) continue;

        auto matchFieldName = explicitNameAnnotation(keyElement);
        BUG_CHECK(bool(matchFieldName), "Table '%1%': Match field '%2%' has no "
                  "@name annotation", table->controlPlaneName(),
                  keyElement->expression);
        auto* matchFieldType =
          typeMap->getType(keyElement->expression->getNode(), true);
        BUG_CHECK(matchFieldType != nullptr,
                  "Couldn't determine type for key element %1%", keyElement);
        type_name = getTypeName(matchFieldType, typeMap);
        int width = getTypeWidth(matchFieldType, typeMap);
        if (width < 0)
            return matchFields;
        matchFields.push_back(MatchField{*matchFieldName, *matchType,
                              matchTypeName, uint32_t(width),
                              keyElement->to<IR::IAnnotated>(), type_name});
    }

    return matchFields;
}

/// Parses P4Runtime-specific annotations.
class ParseAnnotations : public P4::ParseAnnotations {
 public:
    ParseAnnotations() : P4::ParseAnnotations("P4Runtime", false, {
        PARSE("controller_header", StringLiteral),
        PARSE_EMPTY("hidden"),
        PARSE("id", Constant),
        PARSE("brief", StringLiteral),
        PARSE("description", StringLiteral),
        // This annotation is architecture-specific in theory, but given that it
        // is "reserved" by the P4Runtime specification, I don't really have any
        // qualms about adding it here. I don't think it is possible to just run
        // a different ParseAnnotations pass in the constructor of the
        // architecture-specific P4RuntimeArchHandlerIface implementation, since
        // ParseAnnotations modifies the program. I don't really like the
        // possible alternatives either: 1) modify the P4RuntimeArchHandlerIface
        // interface so that each implementation can provide a custom
        // ParseAnnotations instance, or 2) run a ParseAnnotations pass
        // "locally" (in this case on action profile instances since this
        // annotation is for them).
        PARSE("max_group_size", Constant),
        // @p4runtime_translation has two args
        PARSE_PAIR("p4runtime_translation",
                   Expression),
    }) { }
};

namespace {

// It must be an iterator type pointing to a p4info.proto message with a
// 'preamble' field of type p4configv1::Preamble. Fn is an arbitrary function
// with a single parameter of type p4configv1::Preamble.
template <typename It, typename Fn> void
forEachPreamble(It first, It last, Fn fn) {
    for (It it = first; it != last; it++) fn(it->preamble());
}

} // namespace

/// An analyzer which translates the information available in the P4 IR into a
/// representation of the control plane API which is consumed by P4Runtime.
class P4RuntimeAnalyzer {
    using Preamble = p4configv1::Preamble;
    using P4Info = p4configv1::P4Info;

    P4RuntimeAnalyzer(const P4RuntimeSymbolTable& symbols,
                      TypeMap* typeMap, ReferenceMap* refMap,
                      P4RuntimeArchHandlerIface* archHandler)
        : p4Info(new P4Info)
        , symbols(symbols)
        , typeMap(typeMap)
        , refMap(refMap)
        , archHandler(archHandler) {
        CHECK_NULL(typeMap);
    }

    /// @return the P4Info message generated by this analyzer. This captures
    /// P4Runtime representations of all the P4 constructs added to the control
    /// plane API with the add*() methods.
    const P4Info* getP4Info() const {
        BUG_CHECK(p4Info != nullptr, "Didn't produce a P4Info object?");
        return p4Info;
    }

    /// Check for duplicate names among objects of the same type in the
    /// generated P4Info message and @return the number of duplicates.
    template <typename T>
    size_t checkForDuplicatesOfSameType(const T& objs, cstring typeName,
                                        std::unordered_set<p4rt_id_t>* ids) const {
        size_t dupCnt = 0;
        std::unordered_set<std::string> names;

        auto checkOne = [&dupCnt, &names, &ids, typeName](const p4configv1::Preamble& pre) {
            auto pName = names.insert(pre.name());
            auto pId = ids->insert(pre.id());
            if (!pName.second) {
                ::error(ErrorType::ERR_DUPLICATE,
                        "Name '%1%' is used for multiple %2% objects in the P4Info message",
                        pre.name(), typeName);
                dupCnt++;
                return;
            }
            BUG_CHECK(pId.second,
                      "Id '%1%' is used for multiple objects in the P4Info message",
                      pre.id());
        };

        forEachPreamble(objs.cbegin(), objs.cend(), checkOne);

        return dupCnt;
    }

    /// Check for objects with duplicate names in the generated P4Info message
    /// and @return the number of duplicates.
    size_t checkForDuplicates() const {
        size_t dupCnt = 0;
        // There is no real need to check for duplicate ids since the
        // SymbolTable ensures that there are not duplicates. But it certainly
        // does not hurt to it. Architecture-specific implementations may be
        // misusing the SymbolTable, or bypassing it and allocating incorrect
        // ids.
        std::unordered_set<p4rt_id_t> ids;

        // I considered using Protobuf reflection, but it didn't really make the
        // code less verbose, and it certainly didn't make it easier to read.
        dupCnt += checkForDuplicatesOfSameType(p4Info->tables(), "table", &ids);
        dupCnt += checkForDuplicatesOfSameType(p4Info->actions(), "action", &ids);
        dupCnt += checkForDuplicatesOfSameType(p4Info->action_profiles(), "action profile", &ids);
        dupCnt += checkForDuplicatesOfSameType(p4Info->counters(), "counter", &ids);
        dupCnt += checkForDuplicatesOfSameType(p4Info->direct_counters(), "direct counter", &ids);
        dupCnt += checkForDuplicatesOfSameType(p4Info->meters(), "meter", &ids);
        dupCnt += checkForDuplicatesOfSameType(p4Info->direct_meters(), "direct meter", &ids);
        dupCnt += checkForDuplicatesOfSameType(
            p4Info->controller_packet_metadata(), "controller packet metadata", &ids);
        dupCnt += checkForDuplicatesOfSameType(p4Info->value_sets(), "value set", &ids);
        dupCnt += checkForDuplicatesOfSameType(p4Info->registers(), "register", &ids);
        dupCnt += checkForDuplicatesOfSameType(p4Info->digests(), "digest", &ids);

        for (const auto& externType : p4Info->externs()) {
            dupCnt += checkForDuplicatesOfSameType(
                externType.instances(), externType.extern_type_name(), &ids);
        }

        return dupCnt;
    }

 public:
    /**
     * Analyze a P4 program and generate a P4Runtime control plane API.
     *
     * @param program  The P4 program to analyze.
     * @param evaluatedProgram  An up-to-date evaluated version of the program.
     * @param refMap  An up-to-date reference map.
     * @param refMap  An up-to-date type map.
     * @param archHandler  An implementation of P4RuntimeArchHandlerIface which
     * handles architecture-specific constructs (e.g. externs).
     * @param arch  The name of the P4_16 architecture the program was written
     * against.
     * @return a P4Info message representing the program's control plane API.
     *         Never returns null.
     */
    static P4RuntimeAPI analyze(const IR::P4Program* program,
                                const IR::ToplevelBlock* evaluatedProgram,
                                ReferenceMap* refMap,
                                TypeMap* typeMap,
                                P4RuntimeArchHandlerIface* archHandler,
                                cstring arch);

    void addAction(const IR::P4Action* actionDeclaration) {
        if (isHidden(actionDeclaration)) return;

        auto name = actionDeclaration->controlPlaneName();
        auto id = symbols.getId(P4RuntimeSymbolType::ACTION(), name);
        auto annotations = actionDeclaration->to<IR::IAnnotated>();

        // TODO(antonin): The compiler creates a new instance of an action for
        // each table that references it; this is done to make it possible to
        // optimize the actions differently depending on their context. There
        // are several issues with this behavior as it stands right now:
        //   (1) We don't rename the actions, which means they all have the same
        //       id and the control plane can't distinguish between them.
        //   (2) No version of the P4 spec allows the user to explicitly name
        //       a particular table's "version" of an action, which makes it
        //       hard to generate useful names for the replicated variants.
        //   (3) We don't perform any optimizations that leverage this anyway,
        //       so the duplication is pointless.
        // Considering these three issues together, for now the best bet seems
        // to be to deduplicate the actions during serialization. If we resolve
        // some of these issues, we'll want to revisit this decision.
        if (serializedActions.find(id) != serializedActions.end()) return;
        serializedActions.insert(id);

        auto action = p4Info->add_actions();
        setPreamble(action->mutable_preamble(), id, name, symbols.getAlias(name), annotations);

        size_t index = 1;
        for (auto actionParam : *actionDeclaration->parameters->getEnumerator()) {
            auto param = action->add_params();
            auto paramName = actionParam->controlPlaneName();
            param->set_id(index++);
            param->set_name(paramName);
            addAnnotations(param, actionParam->to<IR::IAnnotated>());
            addDocumentation(param, actionParam->to<IR::IAnnotated>());

            auto paramType = typeMap->getType(actionParam, true);
            if (!paramType->is<IR::Type_Bits>() && !paramType->is<IR::Type_Boolean>()
                && !paramType->is<IR::Type_Newtype>() &&
                !paramType->is<IR::Type_SerEnum>()) {
                ::error("Action parameter %1% has a type which is not "
                        "bit<>, int<>, bool, type or serializable enum", actionParam);
                continue;
            }
            int w = getTypeWidth(paramType, typeMap);
            if (w < 0)
                return;
            param->set_bitwidth(w);
            cstring type_name = getTypeName(paramType, typeMap);
            if (type_name) {
                auto namedType = param->mutable_type_name();
                namedType->set_name(type_name);
            }
        }
    }

    void addControllerHeader(const IR::Type_Header* type) {
        if (isHidden(type)) return;

        auto flattenedHeaderType = FlattenHeader::flatten(typeMap, type);

        auto name = type->controlPlaneName();
        auto id = symbols.getId(P4RuntimeSymbolType::CONTROLLER_HEADER(), name);
        auto annotations = type->to<IR::IAnnotated>();

        auto controllerAnnotation = type->getAnnotation("controller_header");
        CHECK_NULL(controllerAnnotation);

        auto nameConstant = controllerAnnotation->expr[0]->to<IR::StringLiteral>();
        CHECK_NULL(nameConstant);
        auto controllerName = nameConstant->value;

        auto header = p4Info->add_controller_packet_metadata();
        // According to the P4Info specification, we use the name specified in
        // the annotation for the p4info preamble, not the P4 fully-qualified
        // name.
        setPreamble(header->mutable_preamble(), id,
                    controllerName /* name */, controllerName /* alias */, annotations);

        size_t index = 1;
        for (auto headerField : flattenedHeaderType->fields) {
            if (isHidden(headerField)) continue;
            auto metadata = header->add_metadata();
            auto fieldName = headerField->controlPlaneName();
            metadata->set_id(index++);
            metadata->set_name(fieldName);
            addAnnotations(metadata, headerField->to<IR::IAnnotated>());

            auto fieldType = typeMap->getType(headerField, true);
            BUG_CHECK((fieldType->is<IR::Type_Bits>() ||
                      fieldType->is<IR::Type_Newtype>() ||
                      fieldType->is<IR::Type_SerEnum>()),
                      "Header field %1% has a type which is not bit<>, "
                      "int<>, type, or serializable enum",
                      headerField);
            auto w = getTypeWidth(fieldType, typeMap);
            if (w < 0)
                return;
            metadata->set_bitwidth(w);
        }
    }

    void addTable(const IR::TableBlock* tableBlock) {
        CHECK_NULL(tableBlock);

        auto tableDeclaration = tableBlock->container;
        if (isHidden(tableDeclaration)) return;

        auto tableSize = Helpers::getTableSize(tableDeclaration);
        auto defaultAction = getDefaultAction(tableDeclaration, refMap, typeMap);
        auto matchFields = getMatchFields(tableDeclaration, refMap, typeMap);
        auto actions = getActionRefs(tableDeclaration, refMap);

        bool isConstTable = getConstTable(tableDeclaration);

        auto name = archHandler->getControlPlaneName(tableBlock);
        auto annotations = tableDeclaration->to<IR::IAnnotated>();

        auto table = p4Info->add_tables();
        setPreamble(table->mutable_preamble(),
                    symbols.getId(P4RuntimeSymbolType::TABLE(), name),
                    name,
                    symbols.getAlias(name),
                    annotations);
        table->set_size(tableSize);

        if (defaultAction && defaultAction->isConst) {
            auto id = symbols.getId(P4RuntimeSymbolType::ACTION(), defaultAction->name);
            table->set_const_default_action_id(id);
        }

        for (const auto& action : actions) {
            auto id = symbols.getId(P4RuntimeSymbolType::ACTION(), action.name);
            auto action_ref = table->add_action_refs();
            action_ref->set_id(id);
            addAnnotations(action_ref, action.annotations);
            // set action ref scope
            auto isTableOnly = (action.annotations->getAnnotation("tableonly") != nullptr);
            auto isDefaultOnly = (action.annotations->getAnnotation("defaultonly") != nullptr);
            if (isTableOnly && isDefaultOnly) {
                ::error("Table '%1%' has an action reference ('%2%') which is annotated "
                        "with both '@tableonly' and '@defaultonly'", name, action.name);
            }
            if (isTableOnly) action_ref->set_scope(p4configv1::ActionRef::TABLE_ONLY);
            else if (isDefaultOnly) action_ref->set_scope(p4configv1::ActionRef::DEFAULT_ONLY);
            else action_ref->set_scope(p4configv1::ActionRef::TABLE_AND_DEFAULT);
        }

        size_t index = 1;
        for (const auto& field : matchFields) {
            auto match_field = table->add_match_fields();
            match_field->set_id(index++);
            match_field->set_name(field.name);
            addAnnotations(match_field, field.annotations);
            addDocumentation(match_field, field.annotations);
            match_field->set_bitwidth(field.bitwidth);
            if (field.type != MatchField::MatchTypes::UNSPECIFIED)
                match_field->set_match_type(field.type);
            else
                match_field->set_other_match_type(field.other_match_type);
            if (field.type_name) {
                auto namedType = match_field->mutable_type_name();
                namedType->set_name(field.type_name);
            }
        }

        if (isConstTable) {
            table->set_is_const_table(true);
        }

        archHandler->addTableProperties(symbols, p4Info, table, tableBlock);
    }

    void addExtern(const IR::ExternBlock* externBlock) {
        CHECK_NULL(externBlock);
        archHandler->addExternInstance(symbols, p4Info, externBlock);
    }

    void analyzeControl(const IR::ControlBlock* controlBlock) {
        CHECK_NULL(controlBlock);

        auto control = controlBlock->container;
        CHECK_NULL(control);

        forAllMatching<IR::P4Action>(&control->controlLocals,
                                     [&](const IR::P4Action* action) {
            // Generate P4Info for the action and, implicitly, its parameters.
            addAction(action);

            // Generate P4Info for any extern functions it may invoke.
            forAllMatching<IR::MethodCallExpression>(action->body,
                                                     [&](const IR::MethodCallExpression* call) {
                auto instance = P4::MethodInstance::resolve(call, refMap, typeMap);
                if (instance->is<P4::ExternFunction>()) {
                    archHandler->addExternFunction(
                        symbols, p4Info, instance->to<P4::ExternFunction>());
                }
            });
        });

        // Generate P4Info for any extern function invoked directly from control.
        forAllMatching<IR::MethodCallExpression>(control->body,
                                                 [&](const IR::MethodCallExpression* call) {
            auto instance = P4::MethodInstance::resolve(call, refMap, typeMap);
            if (instance->is<P4::ExternFunction>()) {
                archHandler->addExternFunction(
                    symbols, p4Info, instance->to<P4::ExternFunction>());
            }
        });
    }

    void addValueSet(const IR::P4ValueSet* inst) {
        // guaranteed by caller
        CHECK_NULL(inst);

        auto vs = p4Info->add_value_sets();

        auto name = inst->controlPlaneName();

        unsigned int size = 0;
        auto sizeConstant = inst->size->to<IR::Constant>();
        if (sizeConstant == nullptr || !sizeConstant->fitsInt()) {
            ::error("@size should be an integer for declaration %1%", inst);
            return;
        }
        if (sizeConstant->value < 0) {
            ::error("@size should be a positive integer for declaration %1%", inst);
            return;
        }
        size = sizeConstant->value.get_ui();

        auto id = symbols.getId(P4RuntimeSymbolType::VALUE_SET(), name);
        setPreamble(vs->mutable_preamble(), id, name, symbols.getAlias(name),
                    inst->to<IR::IAnnotated>());
        vs->set_size(size);

        /// Look for a @match annotation on the struct field and set the match
        /// type of the match field appropriately.
        auto setMatchType = [this](const IR::StructField* sf, p4configv1::MatchField* match) {
            auto matchAnnotation = sf->getAnnotation(IR::Annotation::matchAnnotation);
            // default is EXACT
            if (!matchAnnotation) {
                match->set_match_type(MatchField::MatchTypes::EXACT);  // default match type
                return;
            }
            auto matchPathExpr = matchAnnotation->expr[0]->to<IR::PathExpression>();
            CHECK_NULL(matchPathExpr);
            auto matchTypeName = getMatchTypeName(matchPathExpr, refMap);
            auto matchType = getMatchType(matchTypeName);
            if (matchType == boost::none) {
                ::error(ErrorType::ERR_UNSUPPORTED,
                        "match type for Value Set '@match' annotation",
                        matchAnnotation);
                return;
            }
            if (matchType != MatchField::MatchTypes::UNSPECIFIED)
                match->set_match_type(*matchType);
            else
                match->set_other_match_type(matchTypeName);
        };

        // TODO(antonin): handle new types

        // as per the P4Runtime v1.0.0 specification
        auto et = typeMap->getTypeType(inst->elementType, true);
        if (et->is<IR::Type_Bits>()) {
            auto* match = vs->add_match();
            match->set_id(1);
            match->set_bitwidth(et->width_bits());
            match->set_match_type(MatchField::MatchTypes::EXACT);
        } else if (et->is<IR::Type_Struct>()) {
            int fieldId = 1;
            for (auto f : et->to<IR::Type_Struct>()->fields) {
                auto fType = f->type;
                if (!fType->is<IR::Type_Bits>()) {
                    ::error(ErrorType::ERR_UNSUPPORTED,
                            "type parameter for Value Set; "
                            "this version of P4Runtime requires that when the type parameter "
                            "of a Value Set is a struct, all the fields of the struct "
                            "must be of type bit<W>, but %1% is not", f);
                    continue;
                }
                auto* match = vs->add_match();
                match->set_id(fieldId++);
                match->set_name(f->controlPlaneName());
                match->set_bitwidth(fType->width_bits());
                setMatchType(f, match);
                // add annotations save for the @match one
                addAnnotations(
                    match, f,
                    [](cstring name) { return name == IR::Annotation::matchAnnotation; });
                addDocumentation(match, f);
            }
        } else if (et->is<IR::Type_Tuple>()) {
            ::error(ErrorType::ERR_UNSUPPORTED,
                    "type parameter for Value Set; "
                    "this version of P4Runtime requires the type parameter of a Value Set "
                    "to be a bit<W> or a struct of bit<W> fields",
                    inst);
        } else {
            ::error(ErrorType::ERR_INVALID,
                    "type parameter for Value Set; it must be one of bit<W>, struct or tuple",
                    inst);
        }
    }

    /// To be called after all objects have been added to P4Info. Calls the
    /// architecture-specific postAdd method for post-processing.
    void postAdd() const {
        archHandler->postAdd(symbols, p4Info);
    }

    /// Sets the pkg_info field of the P4Info message, using the annotations on
    /// the P4 program package.
    void addPkgInfo(const IR::ToplevelBlock* evaluatedProgram, cstring arch) const {
        auto* main = evaluatedProgram->getMain();
        if (main == nullptr) {
            ::warning("Program does not contain a main module, "
                      "so P4Info's 'pkg_info' field will not be set");
            return;
        }
        auto* decl = main->node->to<IR::Declaration_Instance>();
        CHECK_NULL(decl);
        auto* pkginfo = p4Info->mutable_pkg_info();

        pkginfo->set_arch(arch);

        std::set<cstring> keysVisited;

        // @pkginfo annotation
        for (auto* annotation : decl->getAnnotations()->annotations) {
            if (annotation->name != IR::Annotation::pkginfoAnnotation) continue;
            for (auto* kv : annotation->kv) {
                auto name = kv->name.name;
                auto setStringField = [kv, pkginfo, &keysVisited](cstring fName) {
                    auto* v = kv->expression->to<IR::StringLiteral>();
                    if (v == nullptr) {
                        ::error("Value for '%1%' key in @pkginfo annotation is not a string", kv);
                        return;
                    }
                    // kv annotations are represented with an IndexedVector in
                    // the IR, so a program with duplicate keys is actually
                    // rejected at parse time.
                    BUG_CHECK(!keysVisited.count(fName), "Duplicate keys in @pkginfo");
                    keysVisited.insert(fName);
                    // use Protobuf reflection library to minimize code
                    // duplication.
                    auto* descriptor = pkginfo->GetDescriptor();
                    auto* f = descriptor->FindFieldByName(static_cast<std::string>(fName));
                    pkginfo->GetReflection()->SetString(
                        pkginfo, f, static_cast<std::string>(v->value));
                };
                if (name == "name" || name == "version" || name == "organization" ||
                    name == "contact" || name == "url") {
                    setStringField(name);
                } else if (name == "arch") {
                    ::warning(ErrorType::WARN_INVALID,
                              "The '%1%' field in PkgInfo should be set by the compiler, "
                              "not by the user", kv);
                    // override the value set previously with the user-provided
                    // value.
                    setStringField(name);
                } else {
                    ::warning(ErrorType::WARN_UNKNOWN,
                              "Unknown key name '%1%' in @pkginfo annotation", name);
                }
            }
        }

        // add other annotations on the P4 package to the message. @pkginfo is
        // ignored using the unary predicate argument to addAnnotations.
        addAnnotations(pkginfo, decl, [](cstring name) { return name == "pkginfo"; });

        addDocumentation(pkginfo, decl);
    }

 private:
    /// P4Runtime's representation of a program's control plane API.
    P4Info* p4Info;
    /// The symbols used in the API and their ids.
    const P4RuntimeSymbolTable& symbols;
    /// The actions we've serialized so far. Used for deduplication.
    std::set<p4rt_id_t> serializedActions;
    /// Type information for the P4 program we're serializing.
    TypeMap* typeMap;
    ReferenceMap* refMap;
    P4RuntimeArchHandlerIface* archHandler;
};

static void collectControlSymbols(P4RuntimeSymbolTable& symbols,
                                  P4RuntimeArchHandlerIface* archHandler,
                                  const IR::ControlBlock* controlBlock,
                                  ReferenceMap* refMap,
                                  TypeMap* typeMap) {
    CHECK_NULL(controlBlock);
    CHECK_NULL(refMap);
    CHECK_NULL(typeMap);

    auto control = controlBlock->container;
    CHECK_NULL(control);

    forAllMatching<IR::P4Action>(&control->controlLocals,
                                 [&](const IR::P4Action* action) {
        // Collect the action itself.
        symbols.add(P4RuntimeSymbolType::ACTION(), action);

        // Collect any extern functions it may invoke.
        forAllMatching<IR::MethodCallExpression>(action->body,
                      [&](const IR::MethodCallExpression* call) {
            auto instance = P4::MethodInstance::resolve(call, refMap, typeMap);
            if (instance->is<P4::ExternFunction>())
                archHandler->collectExternFunction(&symbols, instance->to<P4::ExternFunction>());
        });
    });

    // Collect any extern function invoked directly from the control.
    forAllMatching<IR::MethodCallExpression>(control->body,
                                             [&](const IR::MethodCallExpression* call) {
        auto instance = P4::MethodInstance::resolve(call, refMap, typeMap);
        if (instance->is<P4::ExternFunction>())
                archHandler->collectExternFunction(&symbols, instance->to<P4::ExternFunction>());
    });
}

static void collectExternSymbols(P4RuntimeSymbolTable& symbols,
                                 P4RuntimeArchHandlerIface* archHandler,
                                 const IR::ExternBlock* externBlock) {
    CHECK_NULL(externBlock);
    archHandler->collectExternInstance(&symbols, externBlock);
}

static void collectTableSymbols(P4RuntimeSymbolTable& symbols,
                                P4RuntimeArchHandlerIface* archHandler,
                                const IR::TableBlock* tableBlock) {
    CHECK_NULL(tableBlock);
    auto name = archHandler->getControlPlaneName(tableBlock);
    auto id = externalId(tableBlock->container);
    symbols.add(P4RuntimeSymbolType::TABLE(), name, id);
    archHandler->collectTableProperties(&symbols, tableBlock);
}

static void collectParserSymbols(P4RuntimeSymbolTable& symbols,
                                 const IR::ParserBlock* parserBlock) {
    CHECK_NULL(parserBlock);

    auto parser = parserBlock->container;
    CHECK_NULL(parser);

    for (auto s : parser->parserLocals) {
        if (auto inst = s->to<IR::P4ValueSet>()) {
            symbols.add(P4RuntimeSymbolType::VALUE_SET(), inst);
        }
    }
}

static void analyzeParser(P4RuntimeAnalyzer& analyzer,
                          const IR::ParserBlock* parserBlock) {
    CHECK_NULL(parserBlock);

    auto parser = parserBlock->container;
    CHECK_NULL(parser);

    for (auto s : parser->parserLocals) {
        if (auto inst = s->to<IR::P4ValueSet>()) {
            analyzer.addValueSet(inst);
        }
    }
}

/// A converter which translates the 'const entries' for P4 tables (if any)
/// into a P4Runtime WriteRequest message which can be used by a target to
/// initialize its tables.
class P4RuntimeEntriesConverter {
 private:
    friend class P4RuntimeAnalyzer;

    P4RuntimeEntriesConverter(const P4RuntimeSymbolTable& symbols)
        : entries(new p4v1::WriteRequest), symbols(symbols) { }

    /// @return the P4Runtime WriteRequest message generated by this analyzer.
    const p4v1::WriteRequest* getEntries() const {
        BUG_CHECK(entries != nullptr, "Didn't produce a P4Runtime WriteRequest object?");
        return entries;
    }

    /// Appends the 'const entries' for the table to the WriteRequest message.
    void addTableEntries(const IR::TableBlock* tableBlock, ReferenceMap* refMap,
                         TypeMap* typeMap, P4RuntimeArchHandlerIface* archHandler) {
        CHECK_NULL(tableBlock);
        auto table = tableBlock->container;

        auto entriesList = table->getEntries();
        if (entriesList == nullptr) return;

        auto tableName = archHandler->getControlPlaneName(tableBlock);
        auto tableId = symbols.getId(P4RuntimeSymbolType::TABLE(), tableName);

        int entryPriority = entriesList->entries.size();
        auto needsPriority = tableNeedsPriority(table, refMap);
        for (auto e : entriesList->entries) {
            auto protoUpdate = entries->add_updates();
            protoUpdate->set_type(p4v1::Update::INSERT);
            auto protoEntity = protoUpdate->mutable_entity();
            auto protoEntry = protoEntity->mutable_table_entry();
            protoEntry->set_table_id(tableId);
            addMatchKey(protoEntry, table, e->getKeys(), refMap, typeMap);
            addAction(protoEntry, e->getAction(), refMap, typeMap);
            // According to the P4 specification, "Entries in a table are
            // matched in the program order, stopping at the first matching
            // entry." In P4Runtime, the lowest valid priority value is 1 and
            // entries with a higher numerical priority value have higher
            // priority. So we assign the first entry a priority of #entries and
            // we decrement the priority by 1 for each entry. The last entry in
            // the table will have priority 1.
            if (needsPriority) protoEntry->set_priority(entryPriority--);

            auto priorityAnnotation = e->getAnnotation("priority");
            if (priorityAnnotation != nullptr) {
                ::warning(ErrorType::WARN_DEPRECATED,
                          "The @priority annotation on %1% is not part of the P4 specification, "
                          "nor of the P4Runtime specification, and will be ignored", e);
            }
        }
    }

    /// Checks if the @table entries need to be assigned a priority, i.e. does
    /// the match key for the table includes a ternary or range match?
    bool tableNeedsPriority(const IR::P4Table* table, ReferenceMap* refMap) const {
      for (auto e : table->getKey()->keyElements) {
          auto matchType = getKeyMatchType(e, refMap);
          // TODO(antonin): remove dependency on v1model.
          if (matchType == P4CoreLibrary::instance.ternaryMatch.name ||
              matchType == P4V1::V1Model::instance.rangeMatchType.name) {
              return true;
          }
      }
      return false;
    }

    void addAction(p4v1::TableEntry* protoEntry,
                   const IR::Expression* actionRef,
                   ReferenceMap* refMap,
                   TypeMap* typeMap) const {
        if (!actionRef->is<IR::MethodCallExpression>()) {
            ::error("%1%: invalid action in entries list", actionRef);
            return;
        }
        auto actionCall = actionRef->to<IR::MethodCallExpression>();
        auto method = actionCall->method->to<IR::PathExpression>()->path;
        auto decl = refMap->getDeclaration(method, true);
        auto actionDecl = decl->to<IR::P4Action>();
        auto actionName = actionDecl->controlPlaneName();
        auto actionId = symbols.getId(P4RuntimeSymbolType::ACTION(), actionName);

        auto protoAction = protoEntry->mutable_action()->mutable_action();
        protoAction->set_action_id(actionId);
        int parameterIndex = 0;
        int parameterId = 1;
        for (auto arg : *actionCall->arguments) {
            auto protoParam = protoAction->add_params();
            protoParam->set_param_id(parameterId++);
            auto parameter = actionDecl->parameters->parameters.at(parameterIndex++);
            int width = getTypeWidth(parameter->type, typeMap);
            if (width < 0)
                return;
            if (arg->expression->is<IR::Constant>()) {
                auto value = stringRepr(arg->expression->to<IR::Constant>(), width);
                protoParam->set_value(*value);
            } else if (arg->expression->is<IR::BoolLiteral>()) {
                auto value = stringRepr(arg->expression->to<IR::BoolLiteral>(), width);
                protoParam->set_value(*value);
            } else {
                ::error("%1% unsupported argument expression", arg->expression);
                continue;
            }
        }
    }

    void addMatchKey(p4v1::TableEntry* protoEntry,
                     const IR::P4Table* table,
                     const IR::ListExpression* keyset,
                     ReferenceMap* refMap,
                     TypeMap* typeMap) const {
        int keyIndex = 0;
        int fieldId = 1;
        for (auto k : keyset->components) {
            auto tableKey = table->getKey()->keyElements.at(keyIndex++);
            auto keyWidth = tableKey->expression->type->width_bits();
            auto matchType = getKeyMatchType(tableKey, refMap);

            if (matchType == P4CoreLibrary::instance.exactMatch.name) {
              addExact(protoEntry, fieldId++, k, keyWidth, typeMap);
            } else if (matchType == P4CoreLibrary::instance.lpmMatch.name) {
              addLpm(protoEntry, fieldId++, k, keyWidth, typeMap);
            } else if (matchType == P4CoreLibrary::instance.ternaryMatch.name) {
              addTernary(protoEntry, fieldId++, k, keyWidth, typeMap);
            } else if (matchType == P4V1::V1Model::instance.rangeMatchType.name) {
              addRange(protoEntry, fieldId++, k, keyWidth, typeMap);
            } else {
                if (!k->is<IR::DefaultExpression>())
                    ::error("%1%: match type not supported by P4Runtime serializer", matchType);
                continue;
            }
        }
    }

    /// Convert a key expression to the P4Runtime bytes representation if the
    /// expression is simple (integer literal or boolean literal) or returns
    /// boost::none otherwise.
    boost::optional<std::string> convertSimpleKeyExpression(
        const IR::Expression* k, int keyWidth, TypeMap* typeMap) const {
        if (k->is<IR::Constant>()) {
            return stringRepr(k->to<IR::Constant>(), keyWidth);
        } else if (k->is<IR::BoolLiteral>()) {
            return stringRepr(k->to<IR::BoolLiteral>(), keyWidth);
        } else if (k->is<IR::Member>()) {
             // A SerEnum is a member const entries are processed here.
             auto mem = k->to<IR::Member>();
             auto se = mem->type->to<IR::Type_SerEnum>();
             auto ei = EnumInstance::resolve(mem, typeMap);
             if (!ei) return boost::none;
             if (auto sei = ei->to<SerEnumInstance>()) {
                 auto type = sei->value->to<IR::Constant>();
                 auto w = se->type->width_bits();
                 return stringRepr(type, w);
             }
             ::error("%1% invalid Member key expression", k);
             return boost::none;
        } else {
            ::error("%1% invalid key expression", k);
            return boost::none;
        }
    }

    /// Convert a key expression to the mpz_class integer value if the
    /// expression is simple (integer literal or boolean literal) or returns
    /// boost::none otherwise.
    boost::optional<mpz_class> simpleKeyExpressionValue(const IR::Expression* k) const {
        if (k->is<IR::Constant>()) {
            return k->to<IR::Constant>()->value;
        } else if (k->is<IR::BoolLiteral>()) {
            return static_cast<mpz_class>(k->to<IR::BoolLiteral>()->value ? 1 : 0);
        } else {
            ::error("%1% invalid key expression", k);
            return boost::none;
        }
    }

    void addExact(p4v1::TableEntry* protoEntry, int fieldId,
                  const IR::Expression* k,
                  int keyWidth, TypeMap* typeMap) const {
        auto protoMatch = protoEntry->add_match();
        protoMatch->set_field_id(fieldId);
        auto protoExact = protoMatch->mutable_exact();
        auto value = convertSimpleKeyExpression(k, keyWidth, typeMap);
        if (value == boost::none) return;
        protoExact->set_value(*value);
    }

    void addLpm(p4v1::TableEntry* protoEntry, int fieldId,
                const IR::Expression* k,
                int keyWidth, TypeMap *typeMap) const {
        if (k->is<IR::DefaultExpression>())  // don't care, skip in P4Runtime message
            return;
        int prefixLen;
        boost::optional<std::string> valueStr;
        if (k->is<IR::Mask>()) {
            auto km = k->to<IR::Mask>();
            auto value = simpleKeyExpressionValue(km->left);
            if (value == boost::none) return;
            auto trailing_zeros = [keyWidth](const mpz_class& n) -> int {
                return (n == 0) ? keyWidth : mpz_scan1(n.get_mpz_t(), 0); };
            auto count_ones = [](const mpz_class& n) -> int {
                return mpz_popcount(n.get_mpz_t()); };
            auto mask = km->right->to<IR::Constant>()->value;
            auto len = trailing_zeros(mask);
            if (len + count_ones(mask) != keyWidth) {  // any remaining 0s in the prefix?
                ::error("%1% invalid mask for LPM key", k);
                return;
            }
            if ((*value & mask) != *value) {
                ::warning(ErrorType::WARN_MISMATCH,
                          "P4Runtime requires that LPM matches have masked-off bits set to 0, "
                          "updating value %1% to conform to the P4Runtime specification", km->left);
                *value &= mask;
            }
            if (mask == 0)  // don't care
                return;
            prefixLen = keyWidth - len;
            valueStr = stringReprConstant(*value, keyWidth);
        } else {
            prefixLen = keyWidth;
            valueStr = convertSimpleKeyExpression(k, keyWidth, typeMap);
        }
        if (valueStr == boost::none) return;
        auto protoMatch = protoEntry->add_match();
        protoMatch->set_field_id(fieldId);
        auto protoLpm = protoMatch->mutable_lpm();
        protoLpm->set_value(*valueStr);
        protoLpm->set_prefix_len(prefixLen);
    }

    void addTernary(p4v1::TableEntry* protoEntry, int fieldId,
                    const IR::Expression* k, int keyWidth,
                    TypeMap* typeMap) const {
        if (k->is<IR::DefaultExpression>())  // don't care, skip in P4Runtime message
            return;
        boost::optional<std::string> valueStr;
        boost::optional<std::string> maskStr;
        if (k->is<IR::Mask>()) {
            auto km = k->to<IR::Mask>();
            auto value = simpleKeyExpressionValue(km->left);
            auto mask = simpleKeyExpressionValue(km->right);
            if (value == boost::none || mask == boost::none) return;
            if ((*value & *mask) != *value) {
                ::warning(ErrorType::WARN_MISMATCH,
                          "P4Runtime requires that Ternary matches have masked-off bits set to 0, "
                          "updating value %1% to conform to the P4Runtime specification", km->left);
                *value &= *mask;
            }
            if (*mask == 0)  // don't care
                return;
            valueStr = stringReprConstant(*value, keyWidth);
            maskStr = stringReprConstant(*mask, keyWidth);
        } else {
            valueStr = convertSimpleKeyExpression(k, keyWidth, typeMap);
            maskStr = stringReprConstant(Util::mask(keyWidth), keyWidth);
        }
        if (valueStr == boost::none || maskStr == boost::none) return;
        auto protoMatch = protoEntry->add_match();
        protoMatch->set_field_id(fieldId);
        auto protoTernary = protoMatch->mutable_ternary();
        protoTernary->set_value(*valueStr);
        protoTernary->set_mask(*maskStr);
    }

    void addRange(p4v1::TableEntry* protoEntry, int fieldId,
                  const IR::Expression* k, int keyWidth, TypeMap* typeMap) const {
        if (k->is<IR::DefaultExpression>())  // don't care, skip in P4Runtime message
            return;
        boost::optional<std::string> startStr;
        boost::optional<std::string> endStr;
        if (k->is<IR::Range>()) {
            auto kr = k->to<IR::Range>();
            auto start = simpleKeyExpressionValue(kr->left);
            auto end = simpleKeyExpressionValue(kr->right);
            if (start == boost::none || end == boost::none) return;
            mpz_class maxValue = (mpz_class(1) << keyWidth) - 1;
            // These should be guaranteed by the frontend
            BUG_CHECK(*start <= *end, "Invalid range with start greater than end");
            BUG_CHECK(*end <= maxValue, "End of range is too large");
            if (*start == 0 && *end == maxValue)  // don't care
                return;
            startStr = stringReprConstant(*start, keyWidth);
            endStr = stringReprConstant(*end, keyWidth);
        } else {
            startStr = convertSimpleKeyExpression(k, keyWidth, typeMap);
            endStr = startStr;
        }
        if (startStr == boost::none || endStr == boost::none) return;
        auto protoMatch = protoEntry->add_match();
        protoMatch->set_field_id(fieldId);
        auto protoRange = protoMatch->mutable_range();
        protoRange->set_low(*startStr);
        protoRange->set_high(*endStr);
    }

    cstring getKeyMatchType(const IR::KeyElement* ke, ReferenceMap* refMap) const {
        auto path = ke->matchType->path;
        auto mt = refMap->getDeclaration(path, true)->to<IR::Declaration_ID>();
        BUG_CHECK(mt != nullptr, "%1%: could not find declaration", ke->matchType);
        return mt->name.name;
    }

    boost::optional<std::string> stringReprConstant(mpz_class value, int width) const {
        if (value < 0) {
            ::error("%1%: P4Runtime does not support negative values in match key", value);
            return boost::none;
        }
        BUG_CHECK(width > 0, "Cannot have match fields with width 0");
        auto bitsRequired = static_cast<size_t>(mpz_sizeinbase(value.get_mpz_t(), 2));
        BUG_CHECK(static_cast<size_t>(width) >= bitsRequired,
                  "Cannot represent %1% on %2% bits", value, width);
        // TODO(antonin): P4Runtime defines the canonical representation for
        // bit<W> value as the smallest binary string required to represent the
        // value (no 0 padding). Unfortunately the reference P4Runtime
        // implementation (https://github.com/p4lang/PI) does not currently
        // support the canonical representation, so instead we use a padded
        // binary string, which according to the P4Runtime specification is also
        // valid (but not the canonical representation, which means no RW
        // symmetry).
        // auto bytes = ROUNDUP(mpz_sizeinbase(value.get_mpz_t(), 2), 8);
        auto bytes = ROUNDUP(width, 8);
        std::vector<char> data(bytes);
        mpz_export(data.data(), NULL, 1 /* big endian word */, bytes,
                   1 /* big endian bytes */, 0 /* full words */, value.get_mpz_t());
        return std::string(data.begin(), data.end());
    }

    boost::optional<std::string> stringRepr(const IR::Constant* constant, int width) const {
        return stringReprConstant(constant->value, width);
    }

    boost::optional<std::string> stringRepr(const IR::BoolLiteral* constant, int width) const {
        auto v = static_cast<mpz_class>(constant->value ? 1 : 0);
        return stringReprConstant(v, width);
    }

    /// We represent all static table entries as one P4Runtime WriteRequest object
    p4v1::WriteRequest *entries;
    /// The symbols used in the API and their ids.
    const P4RuntimeSymbolTable& symbols;
};

/* static */ P4RuntimeAPI
P4RuntimeAnalyzer::analyze(const IR::P4Program* program,
                           const IR::ToplevelBlock* evaluatedProgram,
                           ReferenceMap* refMap,
                           TypeMap* typeMap,
                           P4RuntimeArchHandlerIface* archHandler,
                           cstring arch) {
    using namespace ControlPlaneAPI;

    CHECK_NULL(archHandler);

    // Perform a first pass to collect all of the control plane visible symbols in
    // the program.
    auto symbols = P4RuntimeSymbolTable::create([=](P4RuntimeSymbolTable& symbols) {
        Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
            if (block->is<IR::ControlBlock>()) {
                collectControlSymbols(symbols, archHandler, block->to<IR::ControlBlock>(), refMap, typeMap);
            } else if (block->is<IR::ExternBlock>()) {
                collectExternSymbols(symbols, archHandler, block->to<IR::ExternBlock>());
            } else if (block->is<IR::TableBlock>()) {
                collectTableSymbols(symbols, archHandler, block->to<IR::TableBlock>());
            } else if (block->is<IR::ParserBlock>()) {
                collectParserSymbols(symbols, block->to<IR::ParserBlock>());
            }
        });
        forAllMatching<IR::Type_Header>(program, [&](const IR::Type_Header* type) {
            if (isControllerHeader(type)) {
                symbols.add(P4RuntimeSymbolType::CONTROLLER_HEADER(), type);
            }
        });
        archHandler->collectExtra(&symbols);
    });

    archHandler->postCollect(symbols);

    // Construct a P4Runtime control plane API from the program.
    P4RuntimeAnalyzer analyzer(symbols, typeMap, refMap, archHandler);
    Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
        if (block->is<IR::ControlBlock>()) {
            analyzer.analyzeControl(block->to<IR::ControlBlock>());
        } else if (block->is<IR::ExternBlock>()) {
            analyzer.addExtern(block->to<IR::ExternBlock>());
        } else if (block->is<IR::TableBlock>()) {
            analyzer.addTable(block->to<IR::TableBlock>());
        } else if (block->is<IR::ParserBlock>()) {
            analyzeParser(analyzer, block->to<IR::ParserBlock>());
        }
    });
    forAllMatching<IR::Type_Header>(program, [&](const IR::Type_Header* type) {
        if (isControllerHeader(type)) {
            analyzer.addControllerHeader(type);
        }
    });

    analyzer.postAdd();

    // Unfortunately we cannot just rely on the SymbolTable to detect
    // duplicates as it would break existing code. For example, top-level
    // actions are inlined in every control which uses them and the actions end
    // up being "added" to the SymbolTable mutiple times (which is harmless for
    // P4Info generation).
    auto dupCnt = analyzer.checkForDuplicates();
    if (dupCnt > 0) {
        ::error(ErrorType::ERR_DUPLICATE, "Found %1% duplicate name(s) in the P4Info", dupCnt);
    }

    analyzer.addPkgInfo(evaluatedProgram, arch);

    P4RuntimeEntriesConverter entriesConverter(symbols);
    Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
        if (block->is<IR::TableBlock>())
            entriesConverter.addTableEntries(block->to<IR::TableBlock>(), refMap,
                                             typeMap, archHandler);
    });

    auto* p4Info = analyzer.getP4Info();
    auto* p4Entries = entriesConverter.getEntries();
    return P4RuntimeAPI{p4Info, p4Entries};
}

}  // namespace ControlPlaneAPI

P4RuntimeAPI
P4RuntimeSerializer::generateP4Runtime(const IR::P4Program* program, cstring arch) {
    using namespace ControlPlaneAPI;

    auto archHandlerBuilderIt = archHandlerBuilders.find(arch);
    if (archHandlerBuilderIt == archHandlerBuilders.end()) {
        ::error("Arch '%1%' not supported by P4Runtime serializer", arch);
        return P4RuntimeAPI{new p4configv1::P4Info(), new p4v1::WriteRequest()};
    }

    // Generate a new version of the program that satisfies the prerequisites of
    // the P4Runtime analysis code.
    P4::ReferenceMap refMap;
    refMap.setIsV1(true);
    P4::TypeMap typeMap;
    auto* evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    PassManager p4RuntimeFixups = {
        new ControlPlaneAPI::ParseAnnotations(),
        // We can only handle a very restricted class of action parameters - the
        // types need to be bit<> or int<> - so we fail without this pass.
        new P4::RemoveActionParameters(&refMap, &typeMap),
        // Update types and reevaluate the program.
        new P4::TypeChecking(&refMap, &typeMap, /* updateExpressions = */ true),
        evaluator
    };
    auto* p4RuntimeProgram = program->apply(p4RuntimeFixups);
    auto* evaluatedProgram = evaluator->getToplevelBlock();

    if (!p4RuntimeProgram || !evaluatedProgram) {
        ::error(ErrorType::ERR_UNSUPPORTED,
                "P4 program (cannot apply necessary program transformations)",
                "Cannot generate P4Info message");
        return P4RuntimeAPI{new p4configv1::P4Info(), new p4v1::WriteRequest()};
    }

    auto archHandler = (*archHandlerBuilderIt->second)(&refMap, &typeMap, evaluatedProgram);

    return P4RuntimeAnalyzer::analyze(p4RuntimeProgram, evaluatedProgram,
                                      &refMap, &typeMap, archHandler, arch);
}

void P4RuntimeAPI::serializeP4InfoTo(std::ostream* destination, P4RuntimeFormat format) const {
    using namespace ControlPlaneAPI;

    bool success = true;
    // Write the serialization out in the requested format.
    switch (format) {
        case P4RuntimeFormat::BINARY:
            success = writers::writeTo(*p4Info, destination);
            break;
        case P4RuntimeFormat::JSON:
            success = writers::writeJsonTo(*p4Info, destination);
            break;
        case P4RuntimeFormat::TEXT:
            success = writers::writeTextTo(*p4Info, destination);
            break;
    }
    if (!success)
        ::error("Failed to serialize the P4Runtime API to the output");
}

void P4RuntimeAPI::serializeEntriesTo(std::ostream* destination, P4RuntimeFormat format) const {
    using namespace ControlPlaneAPI;

    bool success = true;
    // Write the serialization out in the requested format.
    switch (format) {
        case P4RuntimeFormat::BINARY:
            success = writers::writeTo(*entries, destination);
            break;
        case P4RuntimeFormat::JSON:
            success = writers::writeJsonTo(*entries, destination);
            break;
        case P4RuntimeFormat::TEXT:
            success = writers::writeTextTo(*entries, destination);
            break;
    }
    if (!success)
        ::error("Failed to serialize the P4Runtime static table entries to the output");
}

static bool parseFileNames(cstring fileNameVector,
                           std::vector<cstring> &files,
                           std::vector<P4::P4RuntimeFormat> &formats) {
    for (auto current = fileNameVector; current; ) {
        cstring name = current;
        const char* comma = current.find(',');
        if (comma != nullptr) {
            name = current.before(comma);
            current = comma + 1;
        } else {
            current = cstring();
        }
        files.push_back(name);

        if (cstring suffix = name.findlast('.')) {
            if (suffix == ".json") {
                formats.push_back(P4::P4RuntimeFormat::JSON);
            } else if (suffix == ".bin") {
                formats.push_back(P4::P4RuntimeFormat::BINARY);
            } else if (suffix == ".txt") {
                formats.push_back(P4::P4RuntimeFormat::TEXT);
            } else {
                ::error("%1%: Could not detect p4runtime info file format from file suffix %2%",
                        name, suffix);
                return false;
            }
        } else {
            ::error("%1%: unknown file kind; known suffixes are .bin, .txt, .json", name);
            return false;
        }
    }
    return true;
}

void
P4RuntimeSerializer::serializeP4RuntimeIfRequired(const IR::P4Program* program,
                                                  const CompilerOptions& options) {
    std::vector<cstring> files;
    std::vector<P4::P4RuntimeFormat> formats;

    // only generate P4Info is required by use-provided options
    if (options.p4RuntimeFile.isNullOrEmpty() &&
        options.p4RuntimeFiles.isNullOrEmpty() &&
        options.p4RuntimeEntriesFile.isNullOrEmpty() &&
        options.p4RuntimeEntriesFiles.isNullOrEmpty()) {
        return;
    }
    auto arch = P4RuntimeSerializer::resolveArch(options);
    if (Log::verbose())
        std::cout << "Generating P4Runtime output for architecture " << arch << std::endl;
    auto p4Runtime = get()->generateP4Runtime(program, arch);
    serializeP4RuntimeIfRequired(p4Runtime, options);
}

void
P4RuntimeSerializer::serializeP4RuntimeIfRequired(const P4RuntimeAPI& p4Runtime,
                                                  const CompilerOptions& options) {
    std::vector<cstring> files;
    std::vector<P4::P4RuntimeFormat> formats;

    if (!options.p4RuntimeFile.isNullOrEmpty()) {
        files.push_back(options.p4RuntimeFile);
        formats.push_back(options.p4RuntimeFormat);
    }
    if (!parseFileNames(options.p4RuntimeFiles, files, formats))
        return;

    if (!files.empty()) {
        for (unsigned i = 0; i < files.size(); i++) {
            cstring file = files.at(i);
            P4::P4RuntimeFormat format = formats.at(i);
            std::ostream* out = openFile(file, false);
            if (!out) {
                ::error("Couldn't open P4Runtime API file: %1%", file);
                continue;
            }
            p4Runtime.serializeP4InfoTo(out, format);
        }
    }

    // Do the same for the entries files
    files.clear();
    formats.clear();
    if (!options.p4RuntimeEntriesFile.isNullOrEmpty()) {
        files.push_back(options.p4RuntimeEntriesFile);
        formats.push_back(options.p4RuntimeFormat);
    }

    if (!parseFileNames(options.p4RuntimeEntriesFiles, files, formats))
        return;
    if (!files.empty()) {
        for (unsigned i = 0; i < files.size(); i++) {
            cstring file = files.at(i);
            P4::P4RuntimeFormat format = formats.at(i);
            std::ostream* out = openFile(file, false);
            if (!out) {
                ::error("Couldn't open P4Runtime static entries file: %1%",
                        options.p4RuntimeEntriesFile);
                continue;
            }
            p4Runtime.serializeEntriesTo(out, format);
        }
    }
}

P4RuntimeSerializer::P4RuntimeSerializer() {
    registerArch("v1model", new ControlPlaneAPI::Standard::V1ModelArchHandlerBuilder());
    registerArch("psa", new ControlPlaneAPI::Standard::PSAArchHandlerBuilder());
}

P4RuntimeSerializer*
P4RuntimeSerializer::get() {
    static P4RuntimeSerializer instance;
    return &instance;
}

cstring
P4RuntimeSerializer::resolveArch(const CompilerOptions& options) {
    if (auto arch = getenv("P4C_DEFAULT_ARCH")) {
        return cstring(arch);
    } else if (options.arch != nullptr) {
        return options.arch;
    } else {
        return "v1model";
    }
}

void
P4RuntimeSerializer::registerArch(
    const cstring archName,
    const ControlPlaneAPI::P4RuntimeArchHandlerBuilderIface* builder) {
    archHandlerBuilders[archName] = builder;
}

P4RuntimeAPI generateP4Runtime(const IR::P4Program* program, cstring arch) {
    return P4RuntimeSerializer::get()->generateP4Runtime(program, arch);
}

void serializeP4RuntimeIfRequired(const IR::P4Program* program,
                                  const CompilerOptions& options) {
    P4RuntimeSerializer::get()->serializeP4RuntimeIfRequired(program, options);
}

/** @} */  /* end group control_plane */
}  // namespace P4
