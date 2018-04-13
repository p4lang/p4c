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
#include <typeinfo>
#include <utility>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/variant.hpp>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>
#include "p4/config/p4info.pb.h"
#include "p4/config/v1model.pb.h"
#include "p4/p4runtime.pb.h"

#include "frontends/common/options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/uniqueNames.h"
#include "ir/ir.h"
#include "lib/log.h"
#include "lib/nullstream.h"
#include "lib/ordered_set.h"
#include "midend/eliminateTuples.h"
#include "midend/removeParameters.h"
#include "midend/synthesizeValidField.h"
#include "PI/pi_base.h"

#include "p4RuntimeSerializer.h"

namespace P4 {

/** \defgroup control_plane Control Plane API Generation */
/** \addtogroup control_plane
 *  @{
 */
/// XXX(seth) High level goals of the generator go here!!
namespace ControlPlaneAPI {

// XXX(seth): Here are the known issues:
// - We don't currently distinguish between the case where the const default
//   action has mutable params and the case where it doesn't.
// - Locals are intentionally not exposed, but this prevents table match keys
//   which involve complex expressions from working, because those expressions
//   are desugared into a match against a local. This could be fixed just by
//   exposing locals, but first we need to decide how it makes sense to expose
//   this information to the control plane.

/// @return true if @type has an @metadata annotation.
static bool isMetadataType(const IR::Type* type) {
    return type->getAnnotation("metadata") != nullptr;
}

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

/**
 * A path through a sequence of P4 data structures (headers, header stacks,
 * structs, etc.). This is used to represent fully-qualified external names for
 * header instances and header instance fields.
 */
struct HeaderFieldPath {
    const HeaderFieldPath* parent;  // If non-null, our parent component.
    const boost::variant<cstring, uint32_t> field;  // Our field name or stack index.
    const IR::Type* type;  // If non-null, the P4 type of this component of the path.

    /// Create a new path starting at @root, corresponding to an IR node of @type.
    static HeaderFieldPath* root(const cstring& root, const IR::Type* type) {
        return new HeaderFieldPath{nullptr, boost::variant<cstring, uint32_t>(root), type};
    }

    /**
     * Create a new path from @expression, which should be a tree of Member
     * expressions and PathExpressions, using @refMap and @typeMap.
     * @return a path which names a header field, or nullptr if @expression was
     *         invalid or too complex.
     */
    static HeaderFieldPath* from(const IR::Expression* expression,
                                 const ReferenceMap* refMap,
                                 const TypeMap* typeMap) {
        auto type = typeMap->getType(expression->getNode(), true);
        if (expression->is<IR::PathExpression>()) {
            // for consistency with old P4_14 tools (e.g. the bmv2 JSON to p4runtime conversion
            // tool), we generate names differently in P4_14 mode
            if (refMap->isV1()) {
                // Unlike other top-level structs, top-level metadata structs are fully exposed to
                // P4Runtime and show up in P4Runtime field names.
                if (isMetadataType(type) && type->is<IR::Type_Declaration>()) {
                    auto name = type->to<IR::Type_Declaration>()->controlPlaneName();
                    return HeaderFieldPath::root(name, type);
                }

                // Top-level structs and unions don't show up in P4Runtime field names, so we use a
                // blank path component name.
                if (type->is<IR::Type_Struct>() || type->is<IR::Type_HeaderUnion>()) {
                    return HeaderFieldPath::root("", type);
                }
            }

            // This is a top-level header or a non-structlike value.
            auto declaration =
                refMap->getDeclaration(expression->to<IR::PathExpression>()->path);
            return HeaderFieldPath::root(declaration->controlPlaneName(), type);
        } else if (expression->is<IR::Member>()) {
            auto memberExpression = expression->to<IR::Member>();
            auto parent = memberExpression->expr;
            auto parentPath = from(parent, refMap, typeMap);
            if (parentPath == nullptr) {
                return nullptr;  // Propagate the error.
            }

            auto parentType = typeMap->getType(parent->getNode(), true);
            BUG_CHECK(parentType->is<IR::Type_StructLike>(),
                      "Member is not contained in a structlike type?");

            boost::optional<cstring> name;
            for (auto field : parentType->to<IR::Type_StructLike>()->fields) {
                if (field->name == memberExpression->member) {
                    name = field->controlPlaneName();
                    break;
                }
            }

            BUG_CHECK(name != boost::none, "Member not found in containing type?");
            return parentPath->append(*name, type);
        } else if (expression->is<IR::ArrayIndex>()) {
            auto indexExpression = expression->to<IR::ArrayIndex>();
            auto parent = indexExpression->left;
            auto parentPath = from(parent, refMap, typeMap);
            if (parentPath == nullptr) {
                return nullptr;  // Propagate the error.
            }

            auto index = indexExpression->right;
            if (!index->is<IR::Constant>()) {
              ::error("Index expression '%1%' is too complicated to represent in P4Runtime",
                      expression);
              return nullptr;
            }

            auto constant = index->to<IR::Constant>();
            return parentPath->append(uint32_t(constant->asInt()), type);
        } else {
            ::error("Expression '%1%' is too complicated to resolve to a header field", expression);
            return nullptr;
        }
    }

    /**
     * @return a new path with a @field, of @type, appended.
     *
     * If @field is a @cstring, it's treated as a normal field; if field is an
     * integer, it's treated as an index into a header stack.
     */
    HeaderFieldPath* append(const boost::variant<cstring, uint32_t>& field,
                            const IR::Type* type = nullptr) const {
        return new HeaderFieldPath{this, field, type};
    }

    /// Serialize this path to a string.
    std::string serialize() const { return serialize(""); }

 private:
    std::string serialize(const std::string& rest) const {
        std::string serialized;
        if (field.type() == typeid(uint32_t)) {
            serialized = "[" + Util::toString(boost::get<uint32_t>(field)) + "]";
        } else {
            serialized = boost::get<cstring>(field);
        }

        // Combine this path element with the rest of the path, adding a separator
        // if (1) there are two things to separate, and (2) the next element of the
        // path is not a header stack index.
        if (!serialized.empty() && !rest.empty() && rest[0] != '[') {
            serialized += ".";
        }
        serialized += rest;

        return parent != nullptr ? parent->serialize(serialized)
                                 : serialized;
    }
};

/// The information about a digest call which is needed to serialize it.
struct Digest {
    struct Field {
        const cstring name;       // The name of a field included in the digest.
        const uint32_t bitwidth;  // How wide the field is.
    };

    const cstring name;       // The fully qualified external name of the digest
                              // *data* - in P4-14, the field list name, or in
                              // P4-16, the type of the 'data' parameter.
    const uint64_t receiver;  // The numeric id of the digest receiver.
    const std::vector<Field> fields;  // The fields that make up the digest data.
};

/// The information about a default action which is needed to serialize it.
struct DefaultAction {
    const cstring name;  // The fully qualified external name of this action.
    const bool isConst;  // Is this a const default action?
};

/// The information about a match field which is needed to serialize it.
struct MatchField {
    using MatchType = ::p4::config::MatchField::MatchType;
    using MatchTypes = ::p4::config::MatchField;  // Make short enum names visible.

    const cstring name;       // The fully qualified external name of this field.
    const MatchType type;     // The match algorithm - exact, ternary, range, etc.
    const uint32_t bitwidth;  // How wide this field is.
    const IR::IAnnotated* annotations;  // If non-null, any annotations applied
                                        // to this field.
};

struct ActionRef {
    const cstring name;  // The fully qualified external name of the action.
    const IR::IAnnotated* annotations;  // If non-null, any annotations applied to this action
                                        // reference in the table declaration.
};

/// The types of action profiles available in the V1 model.
enum class ActionProfileType {
    INDIRECT,
    INDIRECT_WITH_SELECTOR
};

/// The information about an action profile which is necessary to generate its
/// serialized representation.
struct ActionProfile {
    const cstring name;  // The fully qualified external name of this action profile.
    const ActionProfileType type;
    const int64_t size;
    const IR::IAnnotated* annotations;  // If non-null, any annotations applied to this action
                                        // profile declaration.

    bool operator<(const ActionProfile& other) const {
        if (name != other.name) return name < other.name;
        if (type != other.type) return type < other.type;
        return size < other.size;
    }
};

/// @return @table's size property if available, falling back to the default size.
static int64_t getTableSize(const IR::P4Table* table) {
    const int64_t defaultTableSize =
        P4V1::V1Model::instance.tableAttributes.defaultTableSize;

    auto sizeProperty = table->properties->getProperty("size");
    if (sizeProperty == nullptr) {
        return defaultTableSize;
    }

    if (!sizeProperty->value->is<IR::ExpressionValue>()) {
        ::error("Expected an expression for table size property: %1%", sizeProperty);
        return defaultTableSize;
    }

    auto expression = sizeProperty->value->to<IR::ExpressionValue>()->expression;
    if (!expression->is<IR::Constant>()) {
        ::error("Expected a constant for table size property: %1%", sizeProperty);
        return defaultTableSize;
    }

    const int64_t tableSize = expression->to<IR::Constant>()->asInt();
    return tableSize == 0 ? defaultTableSize : tableSize;
}

/// A traits class describing the properties of "counterlike" things.
/// XXX(seth): We could avoid needing this by refactoring the Model code a
/// little; it's currently not friendly to metaprogramming.
template <typename Kind> struct CounterlikeTraits;

template<> struct CounterlikeTraits<IR::Counter> {
    static const cstring name() { return "counter"; }
    static const cstring directPropertyName() {
        return P4V1::V1Model::instance.tableAttributes.counters.name;
    }
    static const cstring typeName() {
        return P4V1::V1Model::instance.counter.name;
    }
    static const cstring directTypeName() {
        return P4V1::V1Model::instance.directCounter.name;
    }
};

template<> struct CounterlikeTraits<IR::Meter> {
    static const cstring name() { return "meter"; }
    static const cstring directPropertyName() {
        return P4V1::V1Model::instance.tableAttributes.meters.name;
    }
    static const cstring typeName() {
        return P4V1::V1Model::instance.meter.name;
    }
    static const cstring directTypeName() {
        return P4V1::V1Model::instance.directMeter.name;
    }
};

/**
 * The information about a counter or meter instance which is necessary to
 * serialize it. @Kind must be a class with a CounterlikeTraits<>
 * specialization.
 */
template <typename Kind>
struct Counterlike {
    /// The name of the instance.
    const cstring name;
    /// If non-null, the instance's annotations.
    const IR::IAnnotated* annotations;
    /// The units parameter to the instance; valid values vary depending on @Kind.
    const cstring unit;
    /// The size parameter to the instance.
    const int64_t size;
    /// If not none, the instance is a direct resource associated with @table.
    const boost::optional<cstring> table;

    /// @return the information required to serialize an explicit @instance of
    /// @Kind, which is defined inside a control block.
    static boost::optional<Counterlike<Kind>>
    from(const IR::ExternBlock* instance) {
        CHECK_NULL(instance);
        auto declaration = instance->node->to<IR::IDeclaration>();

        // Counter and meter externs refer to their unit as a "type"; this is
        // (confusingly) unrelated to the "type" field of a counter or meter in
        // P4Info.
        auto unit = instance->getParameterValue("type");
        if (!unit->is<IR::Declaration_ID>()) {
            ::error("%1% '%2%' has a unit type which is not an enum constant: %3%",
                    CounterlikeTraits<Kind>::name(), declaration, unit);
            return boost::none;
        }

        auto size = instance->getParameterValue("size")->to<IR::Constant>();
        if (!size->is<IR::Constant>()) {
            ::error("%1% '%2%' has a non-constant size: %3%",
                    CounterlikeTraits<Kind>::name(), declaration, size);
            return boost::none;
        }

        return Counterlike<Kind>{declaration->controlPlaneName(),
                                 declaration->to<IR::IAnnotated>(),
                                 unit->to<IR::Declaration_ID>()->name,
                                 size->to<IR::Constant>()->value.get_si(),
                                 boost::none};
    }

    /// @return the information required to serialize an @instance of @Kind which
    /// is either defined in or referenced by a property value of @table. (This
    /// implies that @instance is a direct resource of @table.)
    static boost::optional<Counterlike<Kind>>
    fromDirect(const ExternInstance& instance, const IR::P4Table* table) {
        CHECK_NULL(table);
        BUG_CHECK(instance.name != boost::none,
                  "Caller should've ensured we have a name");

        if (instance.type->name != CounterlikeTraits<Kind>::directTypeName()) {
            ::error("Expected a direct %1%: %2%", CounterlikeTraits<Kind>::name(),
                    instance.expression);
            return boost::none;
        }

        auto unitArgument = instance.arguments->at(0);
        if (unitArgument == nullptr) {
            ::error("Direct %1% instance %2% should take a constructor argument",
                    CounterlikeTraits<Kind>::name(), instance.expression);
            return boost::none;
        }
        if (!unitArgument->is<IR::Member>()) {
            ::error("Direct %1% instance %2% has an unexpected constructor argument",
                    CounterlikeTraits<Kind>::name(), instance.expression);
            return boost::none;
        }

        auto unit = unitArgument->to<IR::Member>()->member.name;
        return Counterlike<Kind>{*instance.name, instance.annotations,
                                 unit, getTableSize(table),
                                 table->controlPlaneName()};
    }
};

/// @return the value of any P4 '@id' annotation @declaration may have. The name
/// 'externalId' is in analogy with externalName().
static boost::optional<pi_p4_id_t>
externalId(const IR::IDeclaration* declaration) {
    CHECK_NULL(declaration);
    if (!declaration->is<IR::IAnnotated>()) {
        return boost::none;  // Assign an id later; see below.
    }

    // If the user specified an @id annotation, use that.
    if (auto idAnnotation = declaration->getAnnotation("id")) {
        if (idAnnotation->expr.size() != 1) {
            ::error("@id should be an integer for declaration %1%", declaration);
            return boost::none;
        }

        auto idConstant = idAnnotation->expr[0]->to<IR::Constant>();
        if (idConstant == nullptr || !idConstant->fitsInt()) {
            ::error("@id should be an integer for declaration %1%", declaration);
            return boost::none;
        }

        const uint32_t id = idConstant->value.get_si();
        return id;
    }

    // The user didn't assign an id.
    return boost::none;
}

/// The global symbols which are exposed to P4Runtime. Non-global symbols
/// include action parameters and extern instances.
enum class P4RuntimeSymbolType {
    ACTION,
    ACTION_PROFILE,
    CONTROLLER_HEADER,
    COUNTER,
    EXTERN,
    EXTERN_INSTANCE,
    METER,
    TABLE,
    VALUE_SET
};

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
        // XXX(seth): In the future we may be able to eliminate this check,
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
class P4RuntimeSymbolTable {
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
    void add(P4RuntimeSymbolType type, const IR::IDeclaration* declaration) {
        CHECK_NULL(declaration);
        add(type, declaration->controlPlaneName(), externalId(declaration));
    }

    /// Add a @type symbol with @name and possibly an explicit P4 '@id'.
    void add(P4RuntimeSymbolType type, cstring name,
                      boost::optional<pi_p4_id_t> id = boost::none) {
        auto& symbolTable = symbolTables[type];
        if (symbolTable.find(name) != symbolTable.end()) {
            return;  // This is a duplicate, but that's OK.
        }

        symbolTable[name] = tryToAssignId(id);
        suffixSet.addSymbol(name);
    }

    /// @return the P4Runtime id for the symbol of @type corresponding to @declaration.
    pi_p4_id_t getId(P4RuntimeSymbolType type, const IR::IDeclaration* declaration) const {
        CHECK_NULL(declaration);
        return getId(type, declaration->controlPlaneName());
    }

    /// @return the P4Runtime id for the symbol of @type with name @name.
    pi_p4_id_t getId(P4RuntimeSymbolType type, cstring name) const {
        const auto& symbolTable = symbolTables.at(type);
        const auto symbolId = symbolTable.find(name);
        if (symbolId == symbolTable.end()) {
            BUG("Looking up symbol '%1%' without adding it to the table", name);
        }
        return symbolId->second;
    }

    /// @return the alias for the given fully qualified external name. P4Runtime
    /// defines an alias for each object to make referring to objects easier.
    /// By default, the alias is the shortest unique suffix of path components in
    /// the name.
    cstring getAlias(cstring name) const {
        return suffixSet.shortestUniqueSuffix(name);
    }

 private:
    // Rather than call this constructor, use P4RuntimeSymbolTable::create().
    P4RuntimeSymbolTable() { }

    /// @return an initial (possibly invalid) id for a resource, and if the id is
    /// not invalid, record the assignment. An initial @id typically comes from
    /// the P4 '@id' annotation.
    pi_p4_id_t tryToAssignId(boost::optional<pi_p4_id_t> id) {
        if (!id) {
            // The user didn't assign an id, so return the special value PI_INVALID_ID
            // to indicate that computeIds() should assign one later.
            return PI_INVALID_ID;
        }

        if (assignedIds.find(*id) != assignedIds.end()) {
            ::error("@id %1% is assigned to multiple declarations", *id);
            return PI_INVALID_ID;
        }

        assignedIds.insert(*id);
        return *id;
    }

    /**
     * Assign an id to each resource of @type (PI_ACTION_ID, PI_TABLE_ID, etc..)
     * which does not yet have an id, and update the resource in place.
     * Existing ids are avoided to ensure that each id is unique.
     */
    void computeIdsForSymbols(P4RuntimeSymbolType type) {
        // The id for most resources follows a standard format:
        //
        //   [resource type] [zero byte] [name hash value]
        //    \____8_b____/   \__8_b__/   \_____16_b____/
        auto& symbolTable = symbolTables.at(type);
        auto resourceType = piResourceType(type);

        // Extract the names of every resource in the collection that does not already
        // have an id assigned and associate them with an iterator that we can use to
        // access them again later.  The names are stored in a std::map to ensure that
        // they're sorted. This is necessary to provide deterministic ids; see below
        // for details.
        std::map<cstring, SymbolTable::iterator> nameToIteratorMap;
        for (auto it = symbolTable.begin(); it != symbolTable.end(); it++) {
            if (it->second == PI_INVALID_ID) {
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
            boost::optional<pi_p4_id_t> id = probeForId(nameId, [=](uint32_t nameId) {
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
    boost::optional<pi_p4_id_t> probeForId(const uint32_t sourceValue,
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

    /// @return the P4Runtime resource type for the given type of symbol.
    static pi_p4_id_t piResourceType(P4RuntimeSymbolType symbolType) {
        // XXX(seth): It may be a bit confusing that the P4Runtime resource types
        // we're working with here have "PI" in the name. That's just the name
        // P4Runtime had while it was under development.  Hopefully things will be
        // made more consistent at some point.
        switch (symbolType) {
            case P4RuntimeSymbolType::ACTION: return PI_ACTION_ID;
            case P4RuntimeSymbolType::ACTION_PROFILE: return PI_ACT_PROF_ID;

            // XXX(antonin): no support from PI library for now
            // the choice for the resource type id has little importance here
            case P4RuntimeSymbolType::CONTROLLER_HEADER: return 0xab;

            case P4RuntimeSymbolType::COUNTER: return PI_COUNTER_ID;

            // XXX(seth): These are arbitrary values for now, but we'll create
            // official PI resource types for these soon.
            case P4RuntimeSymbolType::EXTERN: return 0x98;
            case P4RuntimeSymbolType::EXTERN_INSTANCE: return 0x99;

            case P4RuntimeSymbolType::METER: return PI_METER_ID;
            case P4RuntimeSymbolType::TABLE: return PI_TABLE_ID;
            // XXX(antonin): reserve 0x03 for value sets
            case P4RuntimeSymbolType::VALUE_SET: return 0x03;
        }
        BUG("Unexpected P4RuntimeSymbolType");  // Unreachable.
    }


    // All the ids we've assigned so far. Used to avoid id collisions; this is
    // especially crucial since ids can be set manually via the '@id' annotation.
    std::set<pi_p4_id_t> assignedIds;

    // Symbol tables, mapping symbols to P4Runtime ids.
    using SymbolTable = std::map<cstring, pi_p4_id_t>;
    std::map<P4RuntimeSymbolType, SymbolTable> symbolTables = {
        { P4RuntimeSymbolType::ACTION, SymbolTable() },
        { P4RuntimeSymbolType::ACTION_PROFILE, SymbolTable() },
        { P4RuntimeSymbolType::CONTROLLER_HEADER, SymbolTable() },
        { P4RuntimeSymbolType::COUNTER, SymbolTable() },
        { P4RuntimeSymbolType::EXTERN, SymbolTable() },
        { P4RuntimeSymbolType::EXTERN_INSTANCE, SymbolTable() },
        { P4RuntimeSymbolType::METER, SymbolTable() },
        { P4RuntimeSymbolType::TABLE, SymbolTable() },
        { P4RuntimeSymbolType::VALUE_SET, SymbolTable() }
    };

    // A set which contains all the symbols in the program. It's used to compute
    // the shortest unique suffix of each symbol, which is the default alias we
    // use for P4Runtime objects.
    P4SymbolSuffixSet suffixSet;
};

/// An analyzer which translates the information available in the P4 IR into a
/// representation of the control plane API which is consumed by P4Runtime.
class P4RuntimeAnalyzer {
    using Counter = ::p4::config::Counter;
    using Meter = ::p4::config::Meter;
    using CounterSpec = ::p4::config::CounterSpec;
    using MeterSpec = ::p4::config::MeterSpec;
    using Extern = ::p4::config::Extern;
    using Preamble = ::p4::config::Preamble;
    using P4Info = ::p4::config::P4Info;

    P4RuntimeAnalyzer(const P4RuntimeSymbolTable& symbols,
                      TypeMap* typeMap)
        : p4Info(new P4Info)
        , symbols(symbols)
        , typeMap(typeMap) {
        CHECK_NULL(typeMap);
    }

    /// @return the P4Info message generated by this analyzer. This captures
    /// P4Runtime representations of all the P4 constructs added to the control
    /// plane API with the add*() methods.
    const P4Info* getP4Info() const {
        BUG_CHECK(p4Info != nullptr, "Didn't produce a P4Info object?");
        return p4Info;
    }

 public:
    /**
     * Analyze a P4 program and generate a P4Runtime control plane API.
     *
     * @param program  The P4 program to analyze.
     * @param evaluatedProgram  An up-to-date evaluated version of the program.
     * @param refMap  An up-to-date reference map.
     * @param refMap  An up-to-date type map.
     * @return a P4Info message representing the program's control plane API.
     *         Never returns null.
     */
    static P4RuntimeAPI analyze(const IR::P4Program* program,
                                const IR::ToplevelBlock* evaluatedProgram,
                                ReferenceMap* refMap,
                                TypeMap* typeMap);

    /// Set common fields between p4::config::Counter and p4::config::DirectCounter.
    template <typename Kind>
    void setCounterCommon(Kind *counter, const Counterlike<IR::Counter>& counterInstance) {
        auto id = symbols.getId(P4RuntimeSymbolType::COUNTER, counterInstance.name);
        counter->mutable_preamble()->set_id(id);
        counter->mutable_preamble()->set_name(counterInstance.name);
        counter->mutable_preamble()->set_alias(symbols.getAlias(counterInstance.name));
        addAnnotations(counter->mutable_preamble(), counterInstance.annotations);
        auto counter_spec = counter->mutable_spec();

        if (counterInstance.unit == "packets") {
            counter_spec->set_unit(CounterSpec::PACKETS);
        } else if (counterInstance.unit == "bytes") {
            counter_spec->set_unit(CounterSpec::BYTES);
        } else if (counterInstance.unit == "packets_and_bytes") {
            counter_spec->set_unit(CounterSpec::BOTH);
        } else {
            counter_spec->set_unit(CounterSpec::UNSPECIFIED);
        }
    }

    void addCounter(const Counterlike<IR::Counter>& counterInstance) {
        if (counterInstance.table) {
            auto counter = p4Info->add_direct_counters();
            setCounterCommon(counter, counterInstance);
            auto id = symbols.getId(P4RuntimeSymbolType::TABLE, *counterInstance.table);
            counter->set_direct_table_id(id);
        } else {
            auto counter = p4Info->add_counters();
            setCounterCommon(counter, counterInstance);
            counter->set_size(counterInstance.size);
        }
    }

    /// Set common fields between p4::config::Meter and p4::config::DirectMeter.
    template <typename Kind>
    void setMeterCommon(Kind *meter, const Counterlike<IR::Meter>& meterInstance) {
        auto id = symbols.getId(P4RuntimeSymbolType::METER, meterInstance.name);
        meter->mutable_preamble()->set_id(id);
        meter->mutable_preamble()->set_name(meterInstance.name);
        meter->mutable_preamble()->set_alias(symbols.getAlias(meterInstance.name));
        addAnnotations(meter->mutable_preamble(), meterInstance.annotations);
        auto meter_spec = meter->mutable_spec();
        meter_spec->set_type(MeterSpec::COLOR_UNAWARE);  // A default; this isn't exposed.

        if (meterInstance.unit == "packets") {
            meter_spec->set_unit(MeterSpec::PACKETS);
        } else if (meterInstance.unit == "bytes") {
            meter_spec->set_unit(MeterSpec::BYTES);
        } else {
            meter_spec->set_unit(MeterSpec::UNSPECIFIED);
        }
    }

    void addMeter(const Counterlike<IR::Meter>& meterInstance) {
        if (meterInstance.table) {
            auto meter = p4Info->add_direct_meters();
            setMeterCommon(meter, meterInstance);
            auto id = symbols.getId(P4RuntimeSymbolType::TABLE, *meterInstance.table);
            meter->set_direct_table_id(id);
        } else {
            auto meter = p4Info->add_meters();
            setMeterCommon(meter, meterInstance);
            meter->set_size(meterInstance.size);
        }
    }

    void addDigest(const Digest& digest) {
        // XXX(seth): It's a bit unclear when two calls to digest() should be
        // treated as being the same from the control plane's perspective.
        // Right now we only take the type of data included in the digest
        // (encoded in its name) into account, but it may be that we should also
        // consider the receiver.
        auto id = symbols.getId(P4RuntimeSymbolType::EXTERN_INSTANCE, digest.name);
        if (serializedInstances.find(id) != serializedInstances.end()) return;
        serializedInstances.insert(id);

        auto digestExtern = findExtern(P4V1::V1Model::instance.digest_receiver.name);
        auto instance = digestExtern->add_instances();
        instance->mutable_preamble()->set_id(id);
        instance->mutable_preamble()->set_name(digest.name);
        instance->mutable_preamble()->set_alias(symbols.getAlias(digest.name));

        ::p4::config::v1model::Digest digestInstance;
        digestInstance.set_receiver(digest.receiver);

        size_t index = 1;
        for (auto& field : digest.fields) {
            auto instanceField = digestInstance.add_fields();
            instanceField->set_id(index++);
            instanceField->set_name(field.name);
            instanceField->set_bitwidth(field.bitwidth);
        }

        instance->mutable_info()->PackFrom(digestInstance);
    }

    void addAction(const IR::P4Action* actionDeclaration) {
        if (isHidden(actionDeclaration)) return;

        auto name = actionDeclaration->controlPlaneName();
        auto id = symbols.getId(P4RuntimeSymbolType::ACTION, name);
        auto annotations = actionDeclaration->to<IR::IAnnotated>();

        // XXX(seth): The compiler creates a new instance of an action for each
        // table that references it; this is done to make it possible to
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
        action->mutable_preamble()->set_id(id);
        action->mutable_preamble()->set_name(name);
        action->mutable_preamble()->set_alias(symbols.getAlias(name));
        addAnnotations(action->mutable_preamble(), annotations);

        size_t index = 1;
        for (auto actionParam : *actionDeclaration->parameters->getEnumerator()) {
            auto param = action->add_params();
            auto paramName = actionParam->controlPlaneName();
            param->set_id(index++);
            param->set_name(paramName);
            addAnnotations(param, actionParam->to<IR::IAnnotated>());

            auto paramType = typeMap->getType(actionParam, true);
            if (!paramType->is<IR::Type_Bits>() && !paramType->is<IR::Type_Boolean>()) {
                ::error("Action parameter %1% has a type which is not bit<> or int<> or bool",
                        actionParam);
                continue;
            }
            if (paramType->is<IR::Type_Boolean>()) {
                param->set_bitwidth(1);
            } else {
                param->set_bitwidth(paramType->width_bits());
            }
        }
    }

    void addControllerHeader(const IR::Type_Header* type) {
        if (isHidden(type)) return;

        auto name = type->controlPlaneName();
        auto id = symbols.getId(P4RuntimeSymbolType::CONTROLLER_HEADER, name);
        auto annotations = type->to<IR::IAnnotated>();

        auto controllerAnnotation = type->getAnnotation("controller_header");
        CHECK_NULL(controllerAnnotation);

        if (controllerAnnotation->expr.size() != 1) {
            ::error("@controller_header should be a string for declaration %1%", type);
            return;
        }
        auto nameConstant = controllerAnnotation->expr[0]->to<IR::StringLiteral>();
        if (nameConstant == nullptr) {
            ::error("@controller_header should be a string for declaration %1%", type);
            return;
        }
        auto controllerName = nameConstant->value;

        auto header = p4Info->add_controller_packet_metadata();
        header->mutable_preamble()->set_id(id);
        // According to the p4info specification, we use the name specified in the annotation for
        // the p4info preamble, not the P4 fully-qualified name.
        header->mutable_preamble()->set_name(controllerName);
        addAnnotations(header->mutable_preamble(), annotations);

        size_t index = 1;
        for (auto headerField : type->fields) {
            if (isHidden(headerField)) continue;
            auto metadata = header->add_metadata();
            auto fieldName = headerField->controlPlaneName();
            metadata->set_id(index++);
            metadata->set_name(fieldName);
            addAnnotations(metadata, headerField->to<IR::IAnnotated>());

            auto fieldType = typeMap->getType(headerField, true);
            BUG_CHECK(fieldType->is<IR::Type_Bits>(),
                      "Header field %1% has a type which is not bit<> or int<>",
                      headerField);
            metadata->set_bitwidth(fieldType->width_bits());
        }
    }

    void addTable(const IR::P4Table* tableDeclaration,
                  uint64_t tableSize,
                  const boost::optional<cstring>& implementation,
                  const boost::optional<Counterlike<IR::Counter>>& directCounter,
                  const boost::optional<Counterlike<IR::Meter>>& directMeter,
                  const boost::optional<DefaultAction>& defaultAction,
                  const std::vector<ActionRef>& actions,
                  const std::vector<MatchField>& matchFields,
                  bool supportsTimeout,
                  bool isConstTable) {
        if (isHidden(tableDeclaration)) return;

        auto name = tableDeclaration->controlPlaneName();
        auto annotations = tableDeclaration->to<IR::IAnnotated>();

        auto table = p4Info->add_tables();
        table->mutable_preamble()->set_id(symbols.getId(P4RuntimeSymbolType::TABLE, name));
        table->mutable_preamble()->set_name(name);
        table->mutable_preamble()->set_alias(symbols.getAlias(name));
        addAnnotations(table->mutable_preamble(), annotations);
        table->set_size(tableSize);

        if (implementation) {
            auto id =
              symbols.getId(P4RuntimeSymbolType::ACTION_PROFILE, *implementation);
            table->set_implementation_id(id);
        }

        if (directCounter) {
            auto id = symbols.getId(P4RuntimeSymbolType::COUNTER,
                                    directCounter->name);
            table->add_direct_resource_ids(id);
            addCounter(*directCounter);
        }

        if (directMeter) {
            auto id = symbols.getId(P4RuntimeSymbolType::METER,
                                    directMeter->name);
            table->add_direct_resource_ids(id);
            addMeter(*directMeter);
        }

        if (defaultAction && defaultAction->isConst) {
            auto id = symbols.getId(P4RuntimeSymbolType::ACTION, defaultAction->name);
            table->set_const_default_action_id(id);

            // XXX(seth): Generally the parameters of const default actions are
            // bound at compile time, so this is a good default, but we need to
            // add support for the cases where they aren't.
            table->set_const_default_action_has_mutable_params(false);
        }

        for (const auto& action : actions) {
            auto id = symbols.getId(P4RuntimeSymbolType::ACTION, action.name);
            auto action_ref = table->add_action_refs();
            action_ref->set_id(id);
            addAnnotations(action_ref, action.annotations);
        }

        size_t index = 1;
        for (const auto& field : matchFields) {
            auto match_field = table->add_match_fields();
            match_field->set_id(index++);
            match_field->set_name(field.name);
            addAnnotations(match_field, field.annotations);
            match_field->set_bitwidth(field.bitwidth);
            match_field->set_match_type(field.type);
        }

        if (supportsTimeout) {
            table->set_with_entry_timeout(true);
        }

        if (isConstTable) {
            table->set_is_const_table(true);
        }
    }

    void addActionProfile(const ActionProfile& actionProfile,
                          const std::set<cstring>& tables) {
        auto profile = p4Info->add_action_profiles();
        auto id = symbols.getId(P4RuntimeSymbolType::ACTION_PROFILE,
                                actionProfile.name);
        profile->mutable_preamble()->set_id(id);
        profile->mutable_preamble()->set_name(actionProfile.name);
        profile->mutable_preamble()->set_alias(symbols.getAlias(actionProfile.name));
        profile->set_with_selector(actionProfile.type
                                     == ActionProfileType::INDIRECT_WITH_SELECTOR);
        profile->set_size(actionProfile.size);

        for (const auto& table : tables) {
            profile->add_table_ids(symbols.getId(P4RuntimeSymbolType::TABLE, table));
        }

        addAnnotations(profile->mutable_preamble(), actionProfile.annotations);
    }

    void addValueSet(const IR::P4ValueSet* inst) {
        // guaranteed by caller
        CHECK_NULL(inst);

        auto bitwidth = static_cast<uint32_t>(inst->elementType->width_bits());
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

        auto vs = p4Info->add_value_sets();
        auto id = symbols.getId(P4RuntimeSymbolType::VALUE_SET, name);
        vs->mutable_preamble()->set_id(id);
        vs->mutable_preamble()->set_name(name);
        vs->mutable_preamble()->set_alias(symbols.getAlias(name));
        vs->set_bitwidth(bitwidth);
        vs->set_size(size);
        addAnnotations(vs->mutable_preamble(), inst, [](cstring name){ return name == "size"; });
    }

 private:
    /// Serialize @annotated's P4 annotations and attach them to a P4Info message
    /// with an 'annotations' field. '@name' and '@id' are ignored, as well as
    /// annotations whose name satisfies predicate @p.
    template <typename Message, typename UnaryPredicate>
    static void addAnnotations(Message* message, const IR::IAnnotated* annotated,
                               UnaryPredicate p) {
        CHECK_NULL(message);

        // Synthesized resources may have no annotations.
        if (annotated == nullptr) return;

        for (const IR::Annotation* annotation : annotated->getAnnotations()->annotations) {
            // Don't output the @name or @id annotations; they're represented
            // elsewhere in P4Info messages.
            if (annotation->name == IR::Annotation::nameAnnotation) continue;
            if (annotation->name == "id") continue;
            if (p(annotation->name)) continue;

            // Serialize the annotation.
            // XXX(seth): Might be nice to do something better than rely on toString().
            std::string serializedAnnotation = "@" + annotation->name + "(";
            auto expressions = annotation->expr;
            for (unsigned i = 0; i < expressions.size(); ++i) {
                serializedAnnotation.append(expressions[i]->toString());
                if (i + 1 < expressions.size()) serializedAnnotation.append(", ");
            }
            serializedAnnotation.append(")");

            message->add_annotations(serializedAnnotation);
        }
    }

    /// calls addAnnotations with a unconditionally false predicate.
    template <typename Message>
    static void addAnnotations(Message* message, const IR::IAnnotated* annotated) {
        addAnnotations(message, annotated, [](cstring){ return false; });
    }

    /// @return the P4Runtime Extern message for the extern @name, creating a
    /// new one if no existing one is found.
    Extern* findExtern(cstring name) {
        auto externs = p4Info->mutable_externs();
        auto existingExtern =
          std::find_if(externs->pointer_begin(), externs->pointer_end(),
              [&](const Extern* e) { return e->extern_type_name() == name; });
        if (existingExtern != externs->pointer_end()) return *existingExtern;

        auto newExtern = p4Info->add_externs();
        auto id = symbols.getId(P4RuntimeSymbolType::EXTERN, name);
        newExtern->set_extern_type_id(id);
        newExtern->set_extern_type_name(name);
        return newExtern;
    }

    /// P4Runtime's representation of a program's control plane API.
    P4Info* p4Info;
    /// The symbols used in the API and their ids.
    const P4RuntimeSymbolTable& symbols;
    /// The actions we've serialized so far. Used for deduplication.
    std::set<pi_p4_id_t> serializedActions;
    /// The extern instances we've serialized so far. Used for deduplication.
    std::set<pi_p4_id_t> serializedInstances;
    /// Type information for the P4 program we're serializing.
    TypeMap* typeMap;
};

/// @return serialization information for the digest() call represented by
/// @call, or boost::none if @call is not a digest() call or is invalid.
static boost::optional<Digest>
getDigestCall(const IR::MethodCallExpression* call,
              ReferenceMap* refMap,
              TypeMap* typeMap) {
    auto instance = P4::MethodInstance::resolve(call, refMap, typeMap);
    if (!instance->is<P4::ExternFunction>()) return boost::none;

    auto function = instance->to<P4::ExternFunction>();
    if (function->method->name != P4V1::V1Model::instance.digest_receiver.name)
        return boost::none;

    BUG_CHECK(call->typeArguments->size() == 1,
              "%1%: Expected one type argument", call);
    BUG_CHECK(call->arguments->size() == 2, "%1%: Expected 2 arguments", call);

    // An invocation of digest() looks like this:
    //   digest<T>(receiver, { fields });
    // The name that shows up in the control plane API is the type name T.
    const IR::Type_StructLike* structType = nullptr;
    auto* typeArg = call->typeArguments->at(0);
    if (typeArg->is<IR::Type_StructLike>()) {
        structType = typeArg->to<IR::Type_StructLike>();
    } else if (auto* typeName = typeArg->to<IR::Type_Name>()) {
        auto* referencedType = refMap->getDeclaration(typeName->path, true);
        if (!referencedType || !referencedType->is<IR::Type_StructLike>()) {
            ::error("digest() is instantiated with an unexpected "
                    "type name: %1%", typeArg);
            return boost::none;
        }
        structType = referencedType->to<IR::Type_StructLike>();
    } else {
        ::error("digest() is instantiated with an unexpected type: %1%", typeArg);
        return boost::none;
    }

    // The first argument is a numeric constant identifying the receiver.
    auto receiver = call->arguments->at(0);
    if (!receiver->is<IR::Constant>()) {
        ::error("%1%: Expected constant receiver", call);
        return boost::none;
    }

    // The second argument is either a value of struct type or a list of values.
    // In either case we try to boil things down to a list of fields. In P4-16
    // we could theoretically support passing more complicated expressions to
    // digest(), but it's not well supported right now.
    std::vector<Digest::Field> fields;
    auto data = call->arguments->at(1);
    auto dataType = typeMap->getType(data, true);
    if (data->is<IR::ListExpression>()) {
        for (auto expression : data->to<IR::ListExpression>()->components) {
            auto path = HeaderFieldPath::from(expression, refMap, typeMap);
            if (!path) {
                ::error("Digest data '%1%' is too complicated to represent in "
                        "P4Runtime", expression);
                return boost::none;
            }
            fields.push_back({path->serialize(),
                              uint32_t(path->type->width_bits())});
        }
    } else if (dataType->is<IR::Type_StructLike>()) {
        for (auto field : dataType->to<IR::Type_StructLike>()->fields) {
            auto member = new IR::Member(data, field->name);
            typeMap->setType(member, typeMap->getType(field, true));
            auto path = HeaderFieldPath::from(member, refMap, typeMap);
            if (!path) {
                ::error("Digest data '%1%' is too complicated to represent in "
                        "P4Runtime", data);
                return boost::none;
            }
            fields.push_back({path->serialize(),
                              uint32_t(path->type->width_bits())});
        }
    } else {
        ::error("Digest data '%1%' is too complicated to represent in "
                "P4Runtime", data);
        return boost::none;
    }

    return Digest{structType->controlPlaneName(),
                  receiver->to<IR::Constant>()->value.get_ui(),
                  std::move(fields)};
}

static void collectControlSymbols(P4RuntimeSymbolTable& symbols,
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
        symbols.add(P4RuntimeSymbolType::ACTION, action);

        // Collect any extern functions it may invoke.
        forAllMatching<IR::MethodCallExpression>(action->body,
                      [&](const IR::MethodCallExpression* call) {
            auto digest = getDigestCall(call, refMap, typeMap);
            if (digest) {
                symbols.add(P4RuntimeSymbolType::EXTERN,
                            P4V1::V1Model::instance.digest_receiver.name);
                symbols.add(P4RuntimeSymbolType::EXTERN_INSTANCE, digest->name);
            }
        });
    });
}

static void analyzeControl(P4RuntimeAnalyzer& analyzer,
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
        // Generate P4Info for the action and, implicitly, its parameters.
        analyzer.addAction(action);

        // Generate P4Info for any extern functions it may invoke.
        forAllMatching<IR::MethodCallExpression>(action->body,
                      [&](const IR::MethodCallExpression* call) {
            auto digest = getDigestCall(call, refMap, typeMap);
            if (digest) analyzer.addDigest(*digest);
        });
    });
}

static void collectExternSymbols(P4RuntimeSymbolTable& symbols,
                                 const IR::ExternBlock* externBlock) {
    CHECK_NULL(externBlock);

    auto decl = externBlock->node->to<IR::IDeclaration>();
    if (decl == nullptr) return;

    if (externBlock->type->name == "counter") {
        symbols.add(P4RuntimeSymbolType::COUNTER, decl);
    } else if (externBlock->type->name == "meter") {
        symbols.add(P4RuntimeSymbolType::METER, decl);
    } else if (externBlock->type->name == P4V1::V1Model::instance.action_profile.name ||
               externBlock->type->name == P4V1::V1Model::instance.action_selector.name) {
        symbols.add(P4RuntimeSymbolType::ACTION_PROFILE, decl);
    }
}

static void analyzeExtern(P4RuntimeAnalyzer& analyzer,
                          const IR::ExternBlock* externBlock,
                          const TypeMap* typeMap) {
    CHECK_NULL(externBlock);
    CHECK_NULL(typeMap);

    if (externBlock->type->name == CounterlikeTraits<IR::Counter>::typeName()) {
        auto counter = Counterlike<IR::Counter>::from(externBlock);
        if (counter) analyzer.addCounter(*counter);
    } else if (externBlock->type->name == CounterlikeTraits<IR::Meter>::typeName()) {
        auto meter = Counterlike<IR::Meter>::from(externBlock);
        if (meter) analyzer.addMeter(*meter);
    }
}

/// @return an extern instance defined or referenced by the value of @table's
/// @propertyName property, or boost::none if no extern was referenced.
static boost::optional<ExternInstance>
getExternInstanceFromProperty(const IR::P4Table* table,
                              const cstring& propertyName,
                              ReferenceMap* refMap,
                              TypeMap* typeMap) {
    auto property = table->properties->getProperty(propertyName);
    if (property == nullptr) return boost::none;
    if (!property->value->is<IR::ExpressionValue>()) {
        ::error("Expected %1% property value for table %2% to be an expression: %3%",
                propertyName, table->controlPlaneName(), property);
        return boost::none;
    }

    auto expr = property->value->to<IR::ExpressionValue>()->expression;
    if (expr->is<IR::ConstructorCallExpression>()
        && property->getAnnotation(IR::Annotation::nameAnnotation) == nullptr) {
        ::error("Table '%1%' has an anonymous table property '%2%' with no name annotation, "
                "which is not supported by P4Runtime", table->controlPlaneName(), propertyName);
        return boost::none;
    }
    auto name = property->controlPlaneName();
    auto externInstance = ExternInstance::resolve(expr, refMap, typeMap, name);
    if (!externInstance) {
        ::error("Expected %1% property value for table %2% to resolve to an "
                "extern instance: %3%", propertyName, table->controlPlaneName(),
                property);
        return boost::none;
    }

    return externInstance;
}

/// @return the table implementation property, or nullptr if the table has no
/// such property.
static const IR::Property *
getTableImplementationProperty(const IR::P4Table* table) {
    return table->properties->getProperty(
        P4V1::V1Model::instance.tableAttributes.tableImplementation.name);
}

/// @return the direct counter associated with @table, if it has one, or
/// boost::none otherwise.
template <typename Kind>
static boost::optional<Counterlike<Kind>>
getDirectCounterlike(const IR::P4Table* table, ReferenceMap* refMap, TypeMap* typeMap) {
    auto propertyName = CounterlikeTraits<Kind>::directPropertyName();
    auto instance =
      getExternInstanceFromProperty(table, propertyName, refMap, typeMap);
    if (!instance) return boost::none;
    return Counterlike<Kind>::fromDirect(*instance, table);
}

static void collectTableSymbols(P4RuntimeSymbolTable& symbols,
                                const IR::TableBlock* tableBlock,
                                ReferenceMap* refMap,
                                TypeMap* typeMap) {
    CHECK_NULL(tableBlock);
    auto table = tableBlock->container;
    symbols.add(P4RuntimeSymbolType::TABLE, table);

    // If this table has an implementation property, it has an action profile.
    // We collect that symbol here as well, because in P4-16 action profiles are
    // just extern instantiations and aren't defined separately from the tables
    // that use them.
    auto impl = getTableImplementationProperty(table);
    if (impl) {
        // we only collect the implementation symbol if the action profile is instantiated within
        // the table declaration, otherwise the symbol is collected by collectExternSymbols.
        if (!impl->value->is<IR::ExpressionValue>()) {
            ::error("Expected implementation property value for table %1% to be an expression: %2%",
                    table->controlPlaneName(), impl);
            return;
        }
        auto expr = impl->value->to<IR::ExpressionValue>()->expression;
        if (expr->is<IR::ConstructorCallExpression>()) {
            symbols.add(P4RuntimeSymbolType::ACTION_PROFILE, impl->controlPlaneName());
        }
    }

    auto directCounter = getDirectCounterlike<IR::Counter>(table, refMap, typeMap);
    if (directCounter) {
        symbols.add(P4RuntimeSymbolType::COUNTER, directCounter->name);
    }

    auto directMeter = getDirectCounterlike<IR::Meter>(table, refMap, typeMap);
    if (directMeter) {
        symbols.add(P4RuntimeSymbolType::METER, directMeter->name);
    }
}

static void collectParserSymbols(P4RuntimeSymbolTable& symbols,
                                 const IR::ParserBlock* parserBlock) {
    CHECK_NULL(parserBlock);

    auto parser = parserBlock->container;
    CHECK_NULL(parser);

    for (auto s : parser->parserLocals) {
        if (auto inst = s->to<IR::P4ValueSet>()) {
            symbols.add(P4RuntimeSymbolType::VALUE_SET, inst);
        }
    }
}

/// @return the header instance fields matched against by @table's key. The
/// fields are represented as a (fully qualified field name, match type) tuple.
static std::vector<MatchField>
getMatchFields(const IR::P4Table* table, ReferenceMap* refMap, TypeMap* typeMap) {
    std::vector<MatchField> matchFields;

    auto key = table->getKey();
    if (!key) return matchFields;

    for (auto keyElement : key->keyElements) {
        auto matchTypeDecl = refMap->getDeclaration(keyElement->matchType->path, true)
                                   ->to<IR::Declaration_ID>();
        BUG_CHECK(matchTypeDecl != nullptr, "No declaration for match type '%1%'",
                                            keyElement->matchType);
        const auto matchTypeName = matchTypeDecl->name.name;

        MatchField::MatchType matchType;
        if (matchTypeName == P4CoreLibrary::instance.exactMatch.name) {
            matchType = MatchField::MatchTypes::EXACT;
        } else if (matchTypeName == P4CoreLibrary::instance.lpmMatch.name) {
            matchType = MatchField::MatchTypes::LPM;
        } else if (matchTypeName == P4CoreLibrary::instance.ternaryMatch.name) {
            matchType = MatchField::MatchTypes::TERNARY;
        } else if (matchTypeName == P4V1::V1Model::instance.rangeMatchType.name) {
            matchType = MatchField::MatchTypes::RANGE;
        } else if (matchTypeName == P4V1::V1Model::instance.selectorMatchType.name) {
            // This match type indicates that this table is using an action
            // profile. We serialize action profiles separately from the tables
            // themselves, so we don't need to record any information about this
            // key, but it's worth doing a sanity check that this table has an
            // implementation property - if it doesn't, analyzeActionProfiles()
            // will ignore it.
            auto impl = getTableImplementationProperty(table);
            BUG_CHECK(impl != nullptr, "Table '%1%' has match type 'selector' "
                                       "but no implementation property",
                      table->controlPlaneName());
            continue;
        } else {
            ::error("Table '%1%': cannot represent match type '%2%' in P4Runtime",
                    table->controlPlaneName(), matchTypeName);
            break;
        }

        auto matchFieldName = explicitNameAnnotation(keyElement);
        BUG_CHECK(bool(matchFieldName), "Table '%1%': Match field '%2%' has no "
                  "@name annotation", table->controlPlaneName(),
                  keyElement->expression);
        auto* matchFieldType =
          typeMap->getType(keyElement->expression->getNode(), true);
        BUG_CHECK(matchFieldType != nullptr,
                  "Couldn't determine type for key element %1%", keyElement);

        matchFields.push_back(MatchField{*matchFieldName, matchType,
                                         uint32_t(matchFieldType->width_bits()),
                                         keyElement->to<IR::IAnnotated>()});
    }

    return matchFields;
}

/// @return @table's default action, if it has one, or boost::none otherwise.
static boost::optional<DefaultAction>
getDefaultAction(const IR::P4Table* table, ReferenceMap* refMap, TypeMap* typeMap) {
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

/// @return true if @table's 'support_timeout' property exists and is true. This
/// indicates that @table supports entry ageing.
static bool getSupportsTimeout(const IR::P4Table* table) {
    auto timeout = table->properties->getProperty(P4V1::V1Model::instance
                                                    .tableAttributes
                                                    .supportTimeout.name);
    if (timeout == nullptr) return false;
    if (!timeout->value->is<IR::ExpressionValue>()) {
        ::error("Unexpected value %1% for supports_timeout on table %2%",
                timeout, table);
        return false;
    }

    auto expr = timeout->value->to<IR::ExpressionValue>()->expression;
    if (!expr->is<IR::BoolLiteral>()) {
        ::error("Unexpected non-boolean value %1% for supports_timeout "
                "property on table %2%", timeout, table);
        return false;
    }

    return expr->to<IR::BoolLiteral>()->value;
}

/// @return true if @table has a 'entries' property. The property must be const
/// as per the current P4_16 specification. The frontend already enforces that
/// check but we perform the check again here in case the constraint is relaxed
/// in the specification in the future.
static bool getConstTable(const IR::P4Table* table) {
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

static boost::optional<cstring>
getTableImplementationName(const IR::P4Table* table, ReferenceMap* refMap) {
    auto impl = getTableImplementationProperty(table);
    if (impl == nullptr) return boost::none;
    if (!impl->value->is<IR::ExpressionValue>()) {
        ::error("Expected implementation property value for table %1% to be an expression: %2%",
                table->controlPlaneName(), impl);
        return boost::none;
    }
    auto expr = impl->value->to<IR::ExpressionValue>()->expression;
    if (expr->is<IR::ConstructorCallExpression>()) return impl->controlPlaneName();
    if (expr->is<IR::PathExpression>()) {
        auto decl = refMap->getDeclaration(expr->to<IR::PathExpression>()->path, true);
        return decl->controlPlaneName();
    }
    return boost::none;
}

static void analyzeTable(P4RuntimeAnalyzer& analyzer,
                         const IR::TableBlock* tableBlock,
                         ReferenceMap* refMap,
                         TypeMap* typeMap) {
    CHECK_NULL(tableBlock);

    auto table = tableBlock->container;
    auto tableSize = getTableSize(table);
    auto defaultAction = getDefaultAction(table, refMap, typeMap);
    auto matchFields = getMatchFields(table, refMap, typeMap);
    auto actions = getActionRefs(table, refMap);

    auto implementation = getTableImplementationName(table, refMap);

    auto directCounter = getDirectCounterlike<IR::Counter>(table, refMap, typeMap);
    auto directMeter = getDirectCounterlike<IR::Meter>(table, refMap, typeMap);

    bool supportsTimeout = getSupportsTimeout(table);

    bool isConstTable = getConstTable(table);

    analyzer.addTable(table, tableSize, implementation, directCounter,
                      directMeter, defaultAction, actions, matchFields,
                      supportsTimeout, isConstTable);
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

/// Visit evaluated blocks under the provided top-level block. Guarantees that
/// each block is visited only once, even if multiple paths to reach it exist.
template <typename Func>
static void forAllEvaluatedBlocks(const IR::ToplevelBlock* aToplevelBlock,
                                  Func function) {
    std::set<const IR::Block*> visited;
    ordered_set<const IR::Block*> frontier{aToplevelBlock};

    while (!frontier.empty()) {
        // Pop a block off the frontier of blocks we haven't yet visited.
        auto evaluatedBlock = *frontier.begin();
        frontier.erase(frontier.begin());
        visited.insert(evaluatedBlock);

        function(evaluatedBlock);

        // Add child blocks to the frontier if we haven't already visited them.
        for (auto evaluatedChild : evaluatedBlock->constantValue) {
            if (!evaluatedChild.second->is<IR::Block>()) continue;
            auto evaluatedChildBlock = evaluatedChild.second->to<IR::Block>();
            if (visited.find(evaluatedChildBlock) != visited.end()) continue;
            frontier.insert(evaluatedChildBlock);
        }
    }
}

static const IR::IAnnotated*
getActionProfileAnnotations(const IR::P4Table* table, ReferenceMap* refMap) {
    auto impl = getTableImplementationProperty(table);
    if (impl == nullptr) return nullptr;
    if (!impl->value->is<IR::ExpressionValue>()) return nullptr;
    auto expr = impl->value->to<IR::ExpressionValue>()->expression;
    if (expr->is<IR::ConstructorCallExpression>()) return impl->to<IR::IAnnotated>();
    if (expr->is<IR::PathExpression>()) {
        auto decl = refMap->getDeclaration(expr->to<IR::PathExpression>()->path, true);
        return decl->to<IR::IAnnotated>();
    }
    return nullptr;
}

/// @return the action profile referenced in @table's implementation property,
/// if it has one, or boost::none otherwise.
static boost::optional<ActionProfile>
getActionProfile(const IR::P4Table* table, ReferenceMap* refMap, TypeMap* typeMap) {
    auto propertyName = P4V1::V1Model::instance.tableAttributes
                                               .tableImplementation.name;
    auto instance =
      getExternInstanceFromProperty(table, propertyName, refMap, typeMap);
    if (!instance) return boost::none;

    ActionProfileType actionProfileType;
    const IR::Expression* sizeExpression;
    if (instance->type->name == P4V1::V1Model::instance.action_selector.name) {
        actionProfileType = ActionProfileType::INDIRECT_WITH_SELECTOR;
        sizeExpression = instance->arguments->at(1);
    } else if (instance->type->name == P4V1::V1Model::instance.action_profile.name) {
        actionProfileType = ActionProfileType::INDIRECT;
        sizeExpression = instance->arguments->at(0);
    } else {
        ::error("Table '%1%' has an implementation which doesn't resolve to an "
                "action profile: %2%", table->controlPlaneName(),
                instance->expression);
        return boost::none;
    }

    if (!sizeExpression->is<IR::Constant>()) {
      ::error("Action profile '%1%' has non-constant size '%1%'",
              *instance->name, sizeExpression);
      return boost::none;
    }

    const int64_t size = sizeExpression->to<IR::Constant>()->asInt();
    return ActionProfile{*instance->name, actionProfileType, size,
                         getActionProfileAnnotations(table, refMap)};
}

static void analyzeActionProfiles(P4RuntimeAnalyzer& analyzer,
                                  const IR::ToplevelBlock* aToplevelBlock,
                                  ReferenceMap* refMap,
                                  TypeMap* typeMap) {
    std::map<ActionProfile, std::set<cstring>> actionProfiles;

    // Collect all of the action profiles referenced in the program. The same
    // action profile may be referenced by multiple tables.
    forAllEvaluatedBlocks(aToplevelBlock, [&](const IR::Block* block) {
        if (!block->is<IR::TableBlock>()) return;
        auto table = block->to<IR::TableBlock>()->container;
        auto actionProfile = getActionProfile(table, refMap, typeMap);
        if (actionProfile) {
            actionProfiles[*actionProfile].insert(table->controlPlaneName());
        }
    });

    for (const auto& actionProfile : actionProfiles) {
        const auto& profile = actionProfile.first;
        const auto& tables = actionProfile.second;
        analyzer.addActionProfile(profile, tables);
    }
}

/// A converter which translates the 'const entries' for P4 tables (if any)
/// into a P4Runtime WriteRequest message which can be used by a target to
/// initialize its tables.
class P4RuntimeEntriesConverter {
 private:
    friend class P4RuntimeAnalyzer;

    P4RuntimeEntriesConverter(const P4RuntimeSymbolTable& symbols)
        : entries(new p4::WriteRequest), symbols(symbols) { }

    /// @return the P4Runtime WriteRequest message generated by this analyzer.
    const p4::WriteRequest* getEntries() const {
        BUG_CHECK(entries != nullptr, "Didn't produce a P4Runtime WriteRequest object?");
        return entries;
    }

    /// Appends the 'const entries' for the table to the WriteRequest message.
    void addTableEntries(const IR::TableBlock* tableBlock, ReferenceMap* refMap) {
        CHECK_NULL(tableBlock);
        auto table = tableBlock->container;

        auto entriesList = table->getEntries();
        if (entriesList == nullptr) return;

        auto tableName = table->controlPlaneName();
        auto tableId = symbols.getId(P4RuntimeSymbolType::TABLE, tableName);

        int entryPriority = 1;
        auto needsPriority = tableNeedsPriority(table, refMap);
        for (auto e : entriesList->entries) {
            auto protoUpdate = entries->add_updates();
            protoUpdate->set_type(p4::Update::INSERT);
            auto protoEntity = protoUpdate->mutable_entity();
            auto protoEntry = protoEntity->mutable_table_entry();
            protoEntry->set_table_id(tableId);
            addMatchKey(protoEntry, table, e->getKeys(), refMap);
            addAction(protoEntry, e->getAction(), refMap);
            // TODO(antonin): according to the P4 specification, "Entries in a
            // table are matched in the program order, stopping at the first
            // matching entry." Based on the definition of 'priority' in
            // P4Runtime, we may need a different scheme to allocate priority
            // values. For now this assumes that the entry with priority '1' has
            // the highest priority.
            if (needsPriority) protoEntry->set_priority(entryPriority++);
        }
    }

    /// Checks if the @table entries need to be assigned a priority, i.e. does
    /// the match key for the table includes a ternary or range match?
    bool tableNeedsPriority(const IR::P4Table* table, ReferenceMap* refMap) const {
      for (auto e : table->getKey()->keyElements) {
          auto matchType = getKeyMatchType(e, refMap);
          if (matchType == P4CoreLibrary::instance.ternaryMatch.name ||
              matchType == P4V1::V1Model::instance.rangeMatchType.name) {
              return true;
          }
      }
      return false;
    }

    void addAction(p4::TableEntry* protoEntry,
                   const IR::Expression* actionRef,
                   ReferenceMap* refMap) const {
        if (!actionRef->is<IR::MethodCallExpression>()) {
            ::error("%1%: invalid action in entries list", actionRef);
            return;
        }
        auto actionCall = actionRef->to<IR::MethodCallExpression>();
        auto method = actionCall->method->to<IR::PathExpression>()->path;
        auto decl = refMap->getDeclaration(method, true);
        auto actionDecl = decl->to<IR::P4Action>();
        auto actionName = actionDecl->controlPlaneName();
        auto actionId = symbols.getId(P4RuntimeSymbolType::ACTION, actionName);

        auto protoAction = protoEntry->mutable_action()->mutable_action();
        protoAction->set_action_id(actionId);
        int parameterIndex = 0;
        int parameterId = 1;
        for (auto arg : *actionCall->arguments) {
            auto protoParam = protoAction->add_params();
            protoParam->set_param_id(parameterId++);
            auto parameter = actionDecl->parameters->parameters.at(parameterIndex);
            auto width = parameter->type->width_bits();
            auto value = stringRepr(arg->to<IR::Constant>()->value, ROUNDUP(width, 8));
            if (value == boost::none) continue;
            protoParam->set_value(*value);
        }
    }

    void addMatchKey(p4::TableEntry* protoEntry,
                     const IR::P4Table* table,
                     const IR::ListExpression* keyset,
                     ReferenceMap* refMap) const {
        int keyIndex = 0;
        int fieldId = 1;
        for (auto k : keyset->components) {
            auto tableKey = table->getKey()->keyElements.at(keyIndex++);
            auto keyWidth = tableKey->expression->type->width_bits();
            auto matchType = getKeyMatchType(tableKey, refMap);

            auto protoMatch = protoEntry->add_match();
            protoMatch->set_field_id(fieldId++);
            if (matchType == P4CoreLibrary::instance.exactMatch.name) {
                addExact(protoMatch, k, keyWidth);
            } else if (matchType == P4CoreLibrary::instance.lpmMatch.name) {
                addLpm(protoMatch, k, keyWidth);
            } else if (matchType == P4CoreLibrary::instance.ternaryMatch.name) {
                addTernary(protoMatch, k, keyWidth);
            } else if (matchType == P4V1::V1Model::instance.rangeMatchType.name) {
                addRange(protoMatch, k, keyWidth);
            } else {
                ::error("%1%: match type not supported by P4Runtime serializer", matchType);
                continue;
            }
        }
    }

    void addExact(p4::FieldMatch* protoMatch, const IR::Expression* k, int keyWidth) const {
        auto bytes = ROUNDUP(keyWidth, 8);
        auto protoExact = protoMatch->mutable_exact();
        if (k->is<IR::Constant>()) {
            auto value = stringRepr(k->to<IR::Constant>()->value, bytes);
            if (value == boost::none) return;
            protoExact->set_value(*value);
        } else {
            ::error("%1% invalid exact key expression", k);
        }
    }

    void addLpm(p4::FieldMatch* protoMatch, const IR::Expression* k, int keyWidth) const {
        auto bytes = ROUNDUP(keyWidth, 8);
        auto protoLpm = protoMatch->mutable_lpm();
        if (k->is<IR::Mask>()) {
            auto km = k->to<IR::Mask>();
            auto value = stringRepr(km->left->to<IR::Constant>()->value, bytes);
            if (value == boost::none) return;
            protoLpm->set_value(*value);
            auto trailing_zeros = [](unsigned long n) { return n ? __builtin_ctzl(n) : 0; };
            auto count_ones = [](unsigned long n) { return n ? __builtin_popcountl(n) : 0;};
            unsigned long mask = km->right->to<IR::Constant>()->value.get_ui();
            auto len = trailing_zeros(mask);
            if (len + count_ones(mask) != keyWidth) {  // any remaining 0s in the prefix?
                ::error("%1% invalid mask for LPM key", k);
                return;
            }
            protoLpm->set_prefix_len(keyWidth - len);
        } else if (k->is<IR::Constant>()) {
            auto value = stringRepr(k->to<IR::Constant>()->value, bytes);
            if (value == boost::none) return;
            protoLpm->set_value(*value);
            protoLpm->set_prefix_len(keyWidth);
        } else if (k->is<IR::DefaultExpression>()) {
            protoLpm->set_value(*stringRepr(0, bytes));
            protoLpm->set_prefix_len(0);
        } else {
            ::error("%1% invalid LPM key expression", k);
        }
    }

    void addTernary(p4::FieldMatch* protoMatch, const IR::Expression* k, int keyWidth) const {
        auto bytes = ROUNDUP(keyWidth, 8);
        auto protoTernary = protoMatch->mutable_ternary();
        if (k->is<IR::Mask>()) {
            auto km = k->to<IR::Mask>();
            auto value = stringRepr(km->left->to<IR::Constant>()->value, bytes);
            auto mask = stringRepr(km->right->to<IR::Constant>()->value, bytes);
            if (value == boost::none || mask == boost::none) return;
            protoTernary->set_value(*value);
            protoTernary->set_mask(*mask);
        } else if (k->is<IR::Constant>()) {
            auto value = stringRepr(k->to<IR::Constant>()->value, bytes);
            if (value == boost::none) return;
            protoTernary->set_value(*value);
            protoTernary->set_mask(*stringRepr(Util::mask(keyWidth), bytes));
        } else if (k->is<IR::DefaultExpression>()) {
            protoTernary->set_value(*stringRepr(0, bytes));
            protoTernary->set_mask(*stringRepr(0, bytes));
        } else {
            ::error("%1% invalid ternary key expression", k);
        }
    }

    void addRange(p4::FieldMatch* protoMatch, const IR::Expression* k, int keyWidth) const {
        auto bytes = ROUNDUP(keyWidth, 8);
        auto protoRange = protoMatch->mutable_range();
        if (k->is<IR::Range>()) {
            auto kr = k->to<IR::Range>();
            auto start = stringRepr(kr->left->to<IR::Constant>()->value, bytes);
            auto end = stringRepr(kr->right->to<IR::Constant>()->value, bytes);
            if (start == boost::none || end == boost::none) return;
            protoRange->set_low(*start);
            protoRange->set_high(*end);
        } else if (k->is<IR::DefaultExpression>()) {
            protoRange->set_low(*stringRepr(0, bytes));
            protoRange->set_high(*stringRepr((1 << keyWidth)-1, bytes));
        } else {
            ::error("%1% invalid range key expression", k);
        }
    }

    cstring getKeyMatchType(const IR::KeyElement* ke, ReferenceMap* refMap) const {
        auto path = ke->matchType->path;
        auto mt = refMap->getDeclaration(path, true)->to<IR::Declaration_ID>();
        BUG_CHECK(mt != nullptr, "%1%: could not find declaration", ke->matchType);
        return mt->name.name;
    }

    boost::optional<std::string> stringRepr(mpz_class value, int bytes) const {
        if (value < 0) {
            ::error("%1%: P4Runtime does not support negative values in match key", value);
            return boost::none;
        }
        BUG_CHECK(bytes > 0, "Cannot have match fields with width 0");
        auto bytes_required = [](mpz_class v) {
            return ROUNDUP(mpz_sizeinbase(v.get_mpz_t(), 2), 8);
        };
        BUG_CHECK(static_cast<size_t>(bytes) >= bytes_required(value),
                  "Cannot represent %1% on %2% bytes", value, bytes);
        std::vector<char> data(bytes);
        mpz_export(data.data(), NULL, 1 /* big endian word */, bytes,
                   1 /* big endian bytes */, 0 /* full words */, value.get_mpz_t());
        return std::string(data.begin(), data.end());
    }

    /// We represent all static table entries as one P4Runtime WriteRequest object
    p4::WriteRequest *entries;
    /// The symbols used in the API and their ids.
    const P4RuntimeSymbolTable& symbols;
};

/* static */ P4RuntimeAPI
P4RuntimeAnalyzer::analyze(const IR::P4Program* program,
                           const IR::ToplevelBlock* evaluatedProgram,
                           ReferenceMap* refMap,
                           TypeMap* typeMap) {
    using namespace ControlPlaneAPI;

    // Perform a first pass to collect all of the control plane visible symbols in
    // the program.
    auto symbols = P4RuntimeSymbolTable::create([=](P4RuntimeSymbolTable& symbols) {
        forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
            if (block->is<IR::ControlBlock>()) {
                collectControlSymbols(symbols, block->to<IR::ControlBlock>(), refMap, typeMap);
            } else if (block->is<IR::ExternBlock>()) {
                collectExternSymbols(symbols, block->to<IR::ExternBlock>());
            } else if (block->is<IR::TableBlock>()) {
                collectTableSymbols(symbols, block->to<IR::TableBlock>(), refMap, typeMap);
            } else if (block->is<IR::ParserBlock>()) {
                collectParserSymbols(symbols, block->to<IR::ParserBlock>());
            }
        });
        forAllMatching<IR::Type_Header>(program, [&](const IR::Type_Header* type) {
            if (isControllerHeader(type)) {
                symbols.add(P4RuntimeSymbolType::CONTROLLER_HEADER, type);
            }
        });
    });

    // Construct a P4Runtime control plane API from the program.
    P4RuntimeAnalyzer analyzer(symbols, typeMap);
    forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
        if (block->is<IR::ControlBlock>()) {
            analyzeControl(analyzer, block->to<IR::ControlBlock>(), refMap, typeMap);
        } else if (block->is<IR::ExternBlock>()) {
            analyzeExtern(analyzer, block->to<IR::ExternBlock>(), typeMap);
        } else if (block->is<IR::TableBlock>()) {
            analyzeTable(analyzer, block->to<IR::TableBlock>(), refMap, typeMap);
        } else if (block->is<IR::ParserBlock>()) {
            analyzeParser(analyzer, block->to<IR::ParserBlock>());
        }
    });
    analyzeActionProfiles(analyzer, evaluatedProgram, refMap, typeMap);
    forAllMatching<IR::Type_Header>(program, [&](const IR::Type_Header* type) {
        if (isControllerHeader(type)) {
            analyzer.addControllerHeader(type);
        }
    });

    P4RuntimeEntriesConverter entriesConverter(symbols);
    forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
        if (block->is<IR::TableBlock>())
            entriesConverter.addTableEntries(block->to<IR::TableBlock>(), refMap);
    });

    auto* p4Info = analyzer.getP4Info();
    auto* p4Entries = entriesConverter.getEntries();
    return P4RuntimeAPI{p4Info, p4Entries};
}

}  // namespace ControlPlaneAPI

P4RuntimeAPI generateP4Runtime(const IR::P4Program* program) {
    using namespace ControlPlaneAPI;

    // Generate a new version of the program that satisfies the prerequisites of
    // the P4Runtime analysis code.
    // XXX(seth): Long term, generateP4Runtime() should be able to operate on
    // the version of the program we have after the frontend, without any
    // dependencies on additional passes. Clearly we have a way to go.
    P4::ReferenceMap refMap;
    refMap.setIsV1(true);
    P4::TypeMap typeMap;
    auto* evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    PassManager p4RuntimeFixups = {
        // We can only handle a very restricted class of action parameters - the
        // types need to be bit<> or int<> - so we fail without this pass.
        new P4::RemoveActionParameters(&refMap, &typeMap),
        // We need a $valid$ field preinserted before we generate P4Runtime.
        new P4::SynthesizeValidField(&refMap, &typeMap),
        // We currently can't handle tuples.
        new P4::EliminateTuples(&refMap, &typeMap),
        // Update types and reevaluate the program.
        new P4::TypeChecking(&refMap, &typeMap, /* updateExpressions = */ true),
        evaluator
    };
    auto* p4RuntimeProgram = program->apply(p4RuntimeFixups);
    auto* evaluatedProgram = evaluator->getToplevelBlock();

    BUG_CHECK(p4RuntimeProgram && evaluatedProgram,
              "Failed to transform the program into a "
              "P4Runtime-compatible form");

    return P4RuntimeAnalyzer::analyze(p4RuntimeProgram, evaluatedProgram,
                                      &refMap, &typeMap);
}

void P4RuntimeAPI::serializeP4InfoTo(std::ostream* destination, P4RuntimeFormat format) {
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

void P4RuntimeAPI::serializeEntriesTo(std::ostream* destination, P4RuntimeFormat format) {
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

void serializeP4RuntimeIfRequired(const IR::P4Program* program,
                                  const CompilerOptions& options) {
    // The options parser in the frontend already prints a warning if
    // '--p4runtime-entries-file' is used without '--p4runtime-file'.
    if (options.p4RuntimeFile.isNullOrEmpty()) return;

    if (Log::verbose())
        std::cout << "Generating P4Runtime output" << std::endl;

    auto p4Runtime = P4::generateP4Runtime(program);

    std::ostream* out = openFile(options.p4RuntimeFile, false);
    if (!out) {
        ::error("Couldn't open P4Runtime API file: %1%", options.p4RuntimeFile);
        return;
    }
    p4Runtime.serializeP4InfoTo(out, options.p4RuntimeFormat);

    if (!options.p4RuntimeEntriesFile.isNullOrEmpty()) {
        std::ostream* out = openFile(options.p4RuntimeEntriesFile, false);
        if (!out) {
            ::error("Couldn't open P4Runtime static entries file: %1%",
                    options.p4RuntimeEntriesFile);
            return;
        }
        p4Runtime.serializeEntriesTo(out, options.p4RuntimeFormat);
    }
}

/** @} */  /* end group control_plane */
}  // namespace P4
