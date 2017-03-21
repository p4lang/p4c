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

// This must be included before any of our internal headers because ir/std.h
// places a vector class in the global namespace, which breaks protobuf.
#include "p4/config/p4info.pb.h"

#include <boost/algorithm/string.hpp>
#include <boost/optional.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/variant.hpp>
#include <google/protobuf/util/json_util.h>
#include <iostream>
#include <typeinfo>
#include <utility>
#include <vector>

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/ordered_set.h"
#include "PI/pi_base.h"

#include "p4RuntimeSerializer.h"

namespace P4 {
namespace ControlPlaneAPI {

// XXX(seth): Here are the known issues:
// - The @id pragma is not supported for header instances and especially not for
//   header stack instances, which currently requires allowing the programmer to
//   provide a sequence of ids in the @id pragma. Note that @id *is* supported
//   for header instance *fields*; this is just about the instances themselves.
// - Locals are intentionally not exposed, but this prevents table match keys
//   which involve complex expressions from working, because those expressions
//   are desugared into a match against a local. This could be fixed just by
//   exposing locals, but first we need to decide how it makes sense to expose
//   this information to the control plane.
// - There is some hardcoded stuff which is tied to BMV2 and won't necessarily
//   suit other backends. These can all be dealt with on a case-by-case basis,
//   but they require work outside of this code.

using MatchField = ::p4::config::MatchField;
using MatchType = ::p4::config::MatchField::MatchType;

/// @return true if @type has an @metadata annotation.
static bool isMetadataType(const IR::Type* type) {
    if (!type->is<IR::IAnnotated>()) return false;
    auto annotations = type->to<IR::IAnnotated>()->getAnnotations();
    auto metadataAnnotation = annotations->getSingle("metadata");
    return metadataAnnotation != nullptr;
}

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
            if (isMetadataType(type) && type->is<IR::Type_Declaration>()) {
                auto name = type->to<IR::Type_Declaration>()->externalName();
                return HeaderFieldPath::root(name, type);
            }
            if (type->is<IR::Type_Struct>() || type->is<IR::Type_Union>()) {
                return HeaderFieldPath::root("", type);
            }
            auto declaration =
                refMap->getDeclaration(expression->to<IR::PathExpression>()->path);
            return HeaderFieldPath::root(declaration->externalName(), type);
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
            for (auto field : *parentType->to<IR::Type_StructLike>()->fields) {
                if (field->name == memberExpression->member) {
                    name = field->externalName();
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
              ::error("Index expression '%1%' is too complicated to represent in P4Runtime", expression);
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

/**
 * The information about a header instance field which is necessary to generate
 * its serialized representation. The field may be synthesized, so there may be
 * no corresponding field at the IR level.
 */
struct HeaderField {
    // The header instance owning this field. This is a fully qualified external
    // name of the sort stored in a P4RuntimeSymbolTable.
    const cstring instance;

    // The fully qualified external name of this field. This is *also* fully
    // qualified - that is, this *includes* @instance.
    const cstring name;

    const size_t offset;  // This field's offset (really index) within the header.
    const uint32_t bitwidth;
    const boost::optional<pi_p4_id_t> id;  // Any '@id' P4 annotation for this field.
    const IR::IAnnotated* annotations;  // If non-null, this field's annotations.

    /// @return a HeaderField constructed from an instance @path and an @fieldName.
    static HeaderField* from(const HeaderFieldPath* path, cstring fieldName,
                             size_t offset, uint32_t bitwidth,
                             boost::optional<uint32_t> id,
                             const IR::IAnnotated* annotations) {
        return new HeaderField{path->serialize(), path->append(fieldName)->serialize(),
                               offset, bitwidth, id, annotations};
    }

    /// @return a HeaderField which has no corresponding IR field. Prefer this
    /// method over from() so synthesized fields are documented at the callsite.
    static HeaderField* synthesize(const HeaderFieldPath* path, cstring fieldName,
                                   size_t offset, uint32_t bitwidth) {
        return from(path, fieldName, offset, bitwidth, boost::none, nullptr);
    }
};

/// The information about a default action which is needed to serialize it.
struct DefaultAction {
    const cstring name;  // The fully qualified external name of this action.
    const bool isConst;  // Is this a const default action?
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

    bool operator<(const ActionProfile& other) const {
        return name < other.name && type < other.type && size < other.size;
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
        return P4V1::V1Model::instance.tableAttributes.directCounter.name;
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
        return P4V1::V1Model::instance.tableAttributes.directMeter.name;
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

        return Counterlike<Kind>{declaration->externalName(),
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
                                 table->externalName()};
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
    auto annotations = declaration->to<IR::IAnnotated>()->getAnnotations();
    auto idAnnotation = annotations->getSingle("id");
    if (idAnnotation != nullptr) {
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
/// include action parameters and header instance fields.
enum class P4RuntimeSymbolType {
    ACTION,
    ACTION_PROFILE,
    COUNTER,
    HEADER_FIELD_LIST,
    HEADER_INSTANCE,
    METER,
    TABLE
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

        // Now that the symbol table is initialized, we can compute ids. We begin by
        // computing ids which can be determined in isolation.
        symbols.computeIdsForSymbols(P4RuntimeSymbolType::ACTION);
        symbols.computeIdsForSymbols(P4RuntimeSymbolType::TABLE);
        symbols.computeIdsForSymbols(P4RuntimeSymbolType::HEADER_FIELD_LIST);
        symbols.computeIdsForSymbols(P4RuntimeSymbolType::HEADER_INSTANCE);
        symbols.computeIdsForSymbols(P4RuntimeSymbolType::ACTION_PROFILE);
        symbols.computeIdsForSymbols(P4RuntimeSymbolType::COUNTER);
        symbols.computeIdsForSymbols(P4RuntimeSymbolType::METER);

        // We can compute action parameter ids now that we have action ids.
        symbols.computeIdsForActionParameters();

        // We can compute header field ids now that we have header instance ids.
        symbols.computeIdsForHeaderFields();

        return symbols;
    }

    /// Add a @type symbol, extracting the name and id from @declaration.
    void add(P4RuntimeSymbolType type, const IR::IDeclaration* declaration) {
        CHECK_NULL(declaration);
        add(type, declaration->externalName(), externalId(declaration));
    }

    /// Add a @type symbol with @name and possibly an explicit P4 '@id'.
    void add(P4RuntimeSymbolType type, cstring name,
                      boost::optional<pi_p4_id_t> id = boost::none) {
        auto& symbolTable = symbolTables[type];
        if (symbolTable.find(name) != symbolTable.end()) {
            return;  // This is a duplicate, but that's OK.
        }

        symbolTable[name] = tryToAssignId(id);

        // Add this symbol to the symbol suffix set, so we can compute a minimal
        // alias for it later. We exclude header instances because they aren't
        // exposed directly to P4Runtime; including them would lead to more
        // conflicts between symbols suffixes which would produce longer aliases.
        if (type != P4RuntimeSymbolType::HEADER_INSTANCE) {
            suffixSet.addSymbol(name);
        }
    }

    /// Add an action parameter to the table. Note that only the index is tracked,
    /// not the name of the parameter, because the name doesn't affect the
    /// P4Runtime id of the parameter at all and is effectively just data.
    void addActionParameter(const IR::IDeclaration* action, size_t index) {
        auto actionParam = std::make_pair(action->externalName(), index);
        actionParams[actionParam] = PI_INVALID_ID;
    }

    /// Add a header instance @field to the table.
    void addHeaderField(const HeaderField* field) {
        const auto headerField = std::make_pair(field->instance, field->offset);
        if (headerFields.find(headerField) != headerFields.end()) {
            return;  // This is a duplicate, but that's OK.
        }

        headerFields[headerField] = tryToAssignId(field->id);
        suffixSet.addSymbol(field->name);

        // Although header instances aren't directly exposed to P4Runtime, we need
        // to compute ids for them in order to compute ids for header fields.
        add(P4RuntimeSymbolType::HEADER_INSTANCE, field->instance);

        // We also index the header fields by name.
        headerFieldsByName[field->name] = headerField;
    }

    /// @return the P4Runtime id for the symbol of @type corresponding to @declaration.
    pi_p4_id_t getId(P4RuntimeSymbolType type, const IR::IDeclaration* declaration) const {
        CHECK_NULL(declaration);
        return getId(type, declaration->externalName());
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

    /// @return the P4Runtime id for an action parameter, specified as an @action
    /// and an @index within the action's parameter list.
    pi_p4_id_t getActionParameterId(cstring action, size_t index) const {
        const auto actionParam = std::make_pair(action, index);
        const auto id = actionParams.find(actionParam);
        if (id == actionParams.end()) {
            BUG("Looking up action '%1%' parameter %2% without adding it to "
                "the table", action, index);
        }
        return id->second;
    }

    /// @return the P4Runtime id for the given header instance @field.
    pi_p4_id_t getHeaderFieldId(const HeaderField* field) const {
        const auto headerField = std::make_pair(field->instance, field->offset);
        const auto id = headerFields.find(headerField);
        if (id == headerFields.end()) {
            BUG("Looking up header field '%1%' without adding it to the table", field->name);
        }
        return id->second;
    }

    /// @return the P4Runtime id for the header instance field with the given
    /// fully qualified external @name. "External" here means that all of the
    /// components in the field's path are external names.
    pi_p4_id_t getHeaderFieldIdByName(cstring name) const {
        const auto headerField = headerFieldsByName.find(name);
        if (headerField == headerFieldsByName.end()) {
            BUG("Looking up header field '%1%' without adding it to the table", name);
        }
        const auto id = headerFields.find(headerField->second);
        if (id == headerFields.end()) {
            BUG("The symbol table entry for '%1%' is in an inconsistent state", name);
        }
        return id->second;
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
     * Given a @collection of resources (such as actions or tables) of type
     * @resourceType (PI_ACTION_ID, PI_TABLE_ID, etc...), assign an id to each
     * resource which does not yet have an id, and update the resource in place.
     * Existing @assignedIds are avoided to ensure that each id is unique, and
     * @assignedIds is updated with the newly added ids.
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

    void computeIdsForActionParameters() {
        // The format for action parameter ids is:
        //
        //   [resource type] [action name hash value] [parameter id]
        //    \____8_b____/   \_________16_b_______/   \____8_b___/
        //
        // The action name hash value is just bytes 3 and 4 of the action id. The
        // parameter id is *normally* the index of the parameter in the action's
        // argument list, but in the case of conflicts due to e.g. hash collisions
        // between different actions, linear probing is used to select a unique id.
        for (auto& actionParam : actionParams) {
            auto& action = actionParam.first.first;
            auto& index = actionParam.first.second;
            const uint32_t actionIdBits =
                (getId(P4RuntimeSymbolType::ACTION, action) & 0xffff) << 8;

            // Search for an unused action parameter id.
            boost::optional<pi_p4_id_t> id = probeForId(index, [=](uint32_t index) {
                return (PI_ACTION_PARAM_ID << 24) | actionIdBits | (index & 0xff);
            });

            if (!id) {
                ::error("Action '%1' has too many parameters to represent in P4Runtime", action);
                break;
            }

            // Assign the id.
            actionParam.second = *id;
        }
    }

    void computeIdsForHeaderFields() {
        // The format for header field ids is:
        //
        //   [resource type] [header instance name hash value] [field id]
        //    \____8_b____/   \_____________16_b____________/   \__8_b_/
        //
        // As above, the header instance name hash value is just bytes 3 and 4 of
        // the header instance id. The field id is normally the offset of the header
        // field except in the case of conflicts.
        for (auto& headerField : headerFields) {
            auto& instance = headerField.first.first;
            auto& offset = headerField.first.second;
            const uint32_t instanceIdBits =
                (getId(P4RuntimeSymbolType::HEADER_INSTANCE, instance) & 0xffff) << 8;

            // Search for an unused header field id.
            boost::optional<pi_p4_id_t> id = probeForId(offset, [=](uint32_t offset) {
                return (PI_FIELD_ID << 24) | instanceIdBits | (offset & 0xff);
            });

            if (!id) {
                ::error("No available id to represent header instance %1%'s "
                        "field %2% in P4Runtime", instance, offset);
                break;
            }

            // Assign the id.
            headerField.second = *id;
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
            case P4RuntimeSymbolType::COUNTER: return PI_COUNTER_ID;
            case P4RuntimeSymbolType::HEADER_FIELD_LIST: return PI_FIELD_LIST_ID;

            // Header instances are not exposed to P4Runtime, but we still need to
            // compute their ids because they're used internally for computing the ids
            // of other resources.
            case P4RuntimeSymbolType::HEADER_INSTANCE: return PI_INVALID_ID;  

            case P4RuntimeSymbolType::METER: return PI_METER_ID;
            case P4RuntimeSymbolType::TABLE: return PI_TABLE_ID;
        }
    }


    // All the ids we've assigned so far. Used to avoid id collisions; this is
    // especially crucial since ids can be set manually via the '@id' annotation.
    std::set<pi_p4_id_t> assignedIds;

    // Symbol tables, mapping symbols to P4Runtime ids.
    using SymbolTable = std::map<cstring, pi_p4_id_t>;
    std::map<P4RuntimeSymbolType, SymbolTable> symbolTables = {
        { P4RuntimeSymbolType::ACTION, SymbolTable() },
        { P4RuntimeSymbolType::ACTION_PROFILE, SymbolTable() },
        { P4RuntimeSymbolType::COUNTER, SymbolTable() },
        { P4RuntimeSymbolType::HEADER_FIELD_LIST, SymbolTable() },
        { P4RuntimeSymbolType::HEADER_INSTANCE, SymbolTable() },
        { P4RuntimeSymbolType::METER, SymbolTable() },
        { P4RuntimeSymbolType::TABLE, SymbolTable() }
    };

    // Action parameters aren't global symbols, but the tuple of
    // (action, parameter list index) *is* globally unique, so we store that
    // instead.
    std::map<std::pair<cstring, size_t>, pi_p4_id_t> actionParams;

    // Header instance fields *are* global symbols - they have a qualified name
    // that's globally unique - but that name isn't actually used to compute or
    // look up their ids, so it's more useful to store a tuple of
    // (header instance, field offset).
    std::map<std::pair<cstring, size_t>, pi_p4_id_t> headerFields;

    // A mapping from fully qualified header field names to the keys for the
    // corresponding entries in @headerFields. This allows us to look up header
    // field ids by name.
    std::map<cstring, std::pair<cstring, size_t>> headerFieldsByName;

    // A set which contains all the symbols in the program. It's used to compute
    // the shortest unique suffix of each symbol, which is the default alias we
    // use for P4Runtime objects.
    P4SymbolSuffixSet suffixSet;
};

/// A serializer which translates the information available in the P4 IR into a
/// representation of the control plane API which is consumed by P4Runtime.
class P4RuntimeSerializer {
    using Counter = ::p4::config::Counter;
    using Meter = ::p4::config::Meter;
    using Preamble = ::p4::config::Preamble;
    using P4Info = ::p4::config::P4Info;

public:
    P4RuntimeSerializer(const P4RuntimeSymbolTable& symbols,
                        ReferenceMap* refMap,
                        TypeMap* typeMap)
        : p4Info(new P4Info)
        , symbols(symbols)
        , refMap(refMap)
        , typeMap(typeMap)
    {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
    }

    /// Serialize the control plane API to @destination as a message in the
    /// binary protocol buffers format.
    void writeTo(std::ostream* destination) {
        CHECK_NULL(destination);
        if (!p4Info->SerializeToOstream(destination)) {
            ::error("Failed to serialize the P4Runtime API to the output");
            return;
        }
        destination->flush();
    }

    /// Serialize the control plane API to @destination as a message in the JSON
    /// protocol buffers format. This is intended for debugging and testing.
    void writeJsonTo(std::ostream* destination) {
        using namespace google::protobuf::util;
        CHECK_NULL(destination);

        // Serialize the JSON in a human-readable format.
        JsonPrintOptions options;
        options.add_whitespace = true;

        std::string output;
        if (MessageToJsonString(*p4Info, &output, options) != Status::OK) {
            ::error("Failed to serialize the P4Runtime API to JSON");
            return;
        }

        *destination << output;
        if (!destination->good()) {
            ::error("Failed to write the P4Runtime JSON to the output");
            return;
        }

        destination->flush();
    }

    void addHeaderField(const HeaderField* field) {
        auto headerField = p4Info->add_header_fields();
        headerField->mutable_preamble()->set_id(symbols.getHeaderFieldId(field));
        headerField->mutable_preamble()->set_name(field->name);
        headerField->mutable_preamble()->set_alias(symbols.getAlias(field->name));
        addAnnotations(headerField->mutable_preamble(), field->annotations);
        headerField->set_bitwidth(field->bitwidth);
    }

    void addCounter(const Counterlike<IR::Counter>& counterInstance) {
        auto counter = p4Info->add_counters();
        auto id = symbols.getId(P4RuntimeSymbolType::COUNTER, counterInstance.name);
        counter->mutable_preamble()->set_id(id);
        counter->mutable_preamble()->set_name(counterInstance.name);
        counter->mutable_preamble()->set_alias(symbols.getAlias(counterInstance.name));
        addAnnotations(counter->mutable_preamble(), counterInstance.annotations);
        counter->set_size(counterInstance.size);

        if (counterInstance.unit == "packets") {
            counter->set_unit(Counter::PACKETS);
        } else if (counterInstance.unit == "bytes") {
            counter->set_unit(Counter::BYTES);
        } else if (counterInstance.unit == "packets_and_bytes") {
            counter->set_unit(Counter::BOTH);
        } else {
            counter->set_unit(Counter::UNSPECIFIED);
        }

        if (counterInstance.table) {
            auto id = symbols.getId(P4RuntimeSymbolType::TABLE,
                                    *counterInstance.table);
            counter->set_direct_table_id(id);
        }
    }

    void addMeter(const Counterlike<IR::Meter>& meterInstance) {
        auto meter = p4Info->add_meters();
        auto id = symbols.getId(P4RuntimeSymbolType::METER, meterInstance.name);
        meter->mutable_preamble()->set_id(id);
        meter->mutable_preamble()->set_name(meterInstance.name);
        meter->mutable_preamble()->set_alias(symbols.getAlias(meterInstance.name));
        addAnnotations(meter->mutable_preamble(), meterInstance.annotations);
        meter->set_size(meterInstance.size);
        meter->set_type(Meter::COLOR_UNAWARE);  // A default; this isn't exposed.

        if (meterInstance.unit == "packets") {
            meter->set_unit(Meter::PACKETS);
        } else if (meterInstance.unit == "bytes") {
            meter->set_unit(Meter::BYTES);
        } else {
            meter->set_unit(Meter::UNSPECIFIED);
        }

        if (meterInstance.table) {
            auto id = symbols.getId(P4RuntimeSymbolType::TABLE,
                                    *meterInstance.table);
            meter->set_direct_table_id(id);
        }
    }

    void addAction(const IR::P4Action* actionDeclaration) {
        auto name = actionDeclaration->externalName();
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

        // XXX(seth): We add all action parameters below. However, this is not what
        // the BMV2 JSON converter does; it strips out parameters which aren't used.
        // Unless there's a good reason to do otherwise, we should probably either
        // retain the parameters or strip them out a separate compiler pass.
        size_t index = 0;
        for (auto actionParam : *actionDeclaration->parameters->getEnumerator()) {
            auto param = action->add_params();
            auto paramName = actionParam->externalName();
            param->set_id(symbols.getActionParameterId(name, index++));
            param->set_name(paramName);

            auto paramType = typeMap->getType(actionParam, true);
            if (!paramType->is<IR::Type_Bits>()) {
                ::error("Action %1% has a type which is not bit<> or int<>", actionParam);
                continue;
            }
            param->set_bitwidth(paramType->width_bits());
        }
    }

    void addSynthesizedAction(const cstring& name) {
        auto action = p4Info->add_actions();
        action->mutable_preamble()->set_id(symbols.getId(P4RuntimeSymbolType::ACTION, name));
        action->mutable_preamble()->set_name(name);
        action->mutable_preamble()->set_alias(symbols.getAlias(name));
    }

    void addTable(const IR::P4Table* tableDeclaration,
                  uint64_t tableSize,
                  const boost::optional<cstring>& implementation,
                  const boost::optional<Counterlike<IR::Counter>>& directCounter,
                  const boost::optional<Counterlike<IR::Meter>>& directMeter,
                  const boost::optional<DefaultAction>& defaultAction,
                  const std::vector<cstring>& actions,
                  const std::vector<std::pair<cstring, MatchType>>& matchFields) {
        auto name = tableDeclaration->externalName();
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
        }

        for (const auto& action : actions) {
            auto id = symbols.getId(P4RuntimeSymbolType::ACTION, action);
            table->add_action_ids(id);
        }

        for (const auto& field : matchFields) {
            auto match_field = table->add_match_fields();
            auto id = symbols.getHeaderFieldIdByName(field.first);
            match_field->set_header_field_id(id);
            match_field->set_match_type(field.second);
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
    }

    void addHeaderFieldList(const cstring& name,
                            const std::vector<const HeaderField*>& fields) {
        auto fieldList = p4Info->add_header_field_lists();
        fieldList->mutable_preamble()->set_id(symbols.getId(P4RuntimeSymbolType::HEADER_FIELD_LIST, name));
        fieldList->mutable_preamble()->set_name(name);
        fieldList->mutable_preamble()->set_alias(symbols.getAlias(name));
        for (auto& field : fields) {
            fieldList->add_header_field_ids(symbols.getHeaderFieldId(field));
        }
    }

private:
    /// Serialize @annotated's P4 annotations and attach them to a P4Info message
    /// with a @preamble. '@name' and '@id' are ignored.
    static void addAnnotations(Preamble* preamble, const IR::IAnnotated* annotated)
    {
        CHECK_NULL(preamble);

        // Synthesized resources may have no annotations.
        if (annotated == nullptr) return;

        for (const IR::Annotation* annotation : annotated->getAnnotations()->annotations) {
            // Don't output the @name or @id annotations; they're already captured by
            // the corresponding fields of the P4Info preamble.
            if (annotation->name == IR::Annotation::nameAnnotation) continue;
            if (annotation->name == "id") continue;

            // Serialize the annotation.
            // XXX(seth): Might be nice to do something better than rely on toString().
            std::string serializedAnnotation = "@" + annotation->name + "(";
            auto expressions = annotation->expr;
            for (unsigned i = 0; i < expressions.size() ; ++i) {
                serializedAnnotation.append(expressions[i]->toString());
                if (i + 1 < expressions.size()) serializedAnnotation.append(", ");
            }
            serializedAnnotation.append(")");

            preamble->add_annotations(serializedAnnotation);
        }
    }

    P4Info* p4Info;  // P4Runtime's representation of a program's control plane API.
    const P4RuntimeSymbolTable& symbols;  // The symbols used in the API and their ids.
    std::set<pi_p4_id_t> serializedActions;  // The actions we've serialized so far.
    ReferenceMap* refMap;
    TypeMap* typeMap;
};

namespace detail {

// These functions are used in the implementation of forAllHeaderFields() and
// forAllTupleFields(); see their definitions below.

template <typename Func>
static void forAllFieldsInDeclaration(const HeaderFieldPath* path,
                                      size_t offset,
                                      const IR::IDeclaration* field,
                                      const TypeMap* typeMap,
                                      Func function);

template <typename Func>
static void forAllFieldsInStructlike(const HeaderFieldPath* path,
                                     const TypeMap* typeMap,
                                     Func function) {
    auto structType = path->type->to<IR::Type_StructLike>();

    // Iterate over the fields, recursively handling nested structures if necessary.
    size_t offset = 0;
    for (auto field : *structType->fields) {
        forAllFieldsInDeclaration(path, offset++, field, typeMap, function);
    }

    if (structType->is<IR::Type_Header>()) {
        // Synthesize a '_valid' field, which is used to implement `isValid()`.
        function(HeaderField::synthesize(path, "_valid", offset, /* bitwidth = */ 1));
    }
}

template <typename Func> 
static void forAllFieldsInStack(const HeaderFieldPath* path,
                                const TypeMap* typeMap,
                                Func function) {
    auto stackType = path->type->to<IR::Type_Stack>();
    auto elementType = typeMap->getTypeType(stackType->elementType, true);
    BUG_CHECK(elementType->is<IR::Type_Header>(),
              "Header stack %1% has non-header element type %2%",
              path->serialize(), elementType);

    for (unsigned stackIndex = 0; stackIndex < stackType->getSize(); stackIndex++) {
        forAllFieldsInStructlike(path->append(stackIndex, elementType),
                                 typeMap, function);
    }
}

template <typename Func>
static void forAllFieldsInDeclaration(const HeaderFieldPath* path,
                                      size_t offset,
                                      const IR::IDeclaration* field,
                                      const TypeMap* typeMap,
                                      Func function) {
    CHECK_NULL(field);

    auto id = externalId(field);
    auto name = field->externalName();
    auto annotations = field->to<IR::IAnnotated>();

    auto fieldType = typeMap->getType(field->getNode(), true);
    if (fieldType->is<IR::Type_Boolean>()) {
        const uint32_t bitwidth = 1;
        function(HeaderField::from(path, name, offset, bitwidth, id, annotations));
    } else if (fieldType->is<IR::Type_Bits>()) {
        const uint32_t bitwidth = fieldType->to<IR::Type_Bits>()->size;
        function(HeaderField::from(path, name, offset, bitwidth, id, annotations));
    } else if (fieldType->is<IR::Type_StructLike>()) {
        forAllFieldsInStructlike(path->append(name, fieldType), typeMap, function);
    } else if (fieldType->is<IR::Type_Stack>()) {
        forAllFieldsInStack(path->append(name, fieldType), typeMap, function);
    } else {
        BUG("Header field '%1%' has unhandled type %2%", name, fieldType);
    }
}

} // namespace detail

/// Invoke @function for every primitive header field reachable from
/// @declaration by recursively expanding all composite fields.
// XXX(seth): At the moment, think of this as a view; we're not iterating over
// the real P4 header fields, but rather the header fields as seen from
// P4Runtime. That means that they don't exactly match the P4-level header
// fields in terms of naming and that certain fields don't really exist at the
// P4 level. Long term, we want those inconsistencies to go away.
template <typename Func>
static void forAllHeaderFields(const IR::IDeclaration* declaration,
                               const TypeMap* typeMap,
                               Func function) {
    CHECK_NULL(declaration);
    CHECK_NULL(typeMap);

    auto type = typeMap->getType(declaration->getNode(), true);
    if (isMetadataType(type)) {
        // Unlike other top-level structs, top-level metadata structs are fully
        // exposed to P4Runtime and show up in P4Runtime field names.
        BUG_CHECK(type->is<IR::Type_Struct>(), "Metadata should be a struct");
        const cstring prefix = type->to<IR::Type_Declaration>()->externalName();
        const auto path = HeaderFieldPath::root(prefix, type);
        detail::forAllFieldsInStructlike(path, typeMap, function);
        return;
    }

    // This is either a header or a structlike type that contains headers.
    BUG_CHECK(type->is<IR::Type_StructLike>(),
              "A header collection should be structlike");
    const auto path = HeaderFieldPath::root("", type);

    // Top-level structs and unions don't show up in P4Runtime field names; we
    // just iterate over their fields directly.
    if (type->is<IR::Type_Struct>() || type->is<IR::Type_Union>()) {
        detail::forAllFieldsInStructlike(path, typeMap, function);
        return;
    }

    // This is a top-level header.
    const size_t offset = 0;  // A top-level entity has no field offset.
    detail::forAllFieldsInDeclaration(path, offset, declaration, typeMap, function);
}

/// Invoke @function for every primitive header field reachable from the
/// components of the provided tuple @type by recursively expanding all
/// composite fields.
// XXX(seth): See caveats on @forAllHeaderFields() for more discussion.
template <typename Func>
static void forAllTupleFields(const IR::Type_Tuple* type,
                              const TypeMap* typeMap,
                              Func function) {
    CHECK_NULL(type);
    CHECK_NULL(typeMap);

    // Iterate over the components of the tuple and look for header fields.
    for (auto componentType : *type->components) {
        if (!componentType->is<IR::Type_StructLike>()) continue;
        const cstring prefix = componentType->to<IR::Type_Declaration>()->externalName();
        const auto path = HeaderFieldPath::root(prefix, componentType);
        detail::forAllFieldsInStructlike(path, typeMap, function);
    }
}

/// Invoke @function for every node of type @Node in the subtree rooted at
/// @root. The nodes are visited in postorder.
template <typename Node, typename Func>
static void forAllMatching(const IR::Node* root, Func&& function) {
    struct NodeVisitor : public Inspector {
        explicit NodeVisitor(Func&& function) : function(function) { }
        Func function;
        void postorder(const Node* node) override { function(node); }
    };

    root->apply(NodeVisitor(std::forward<Func>(function)));
}

/// @return an external name generated from the first in a sequence of type
/// arguments @args. If there aren't enough type arguments, or no name could be
/// generated, @fallback is returned.
static cstring externalNameFromTypeArgs(const IR::Vector<IR::Type>* args,
                                        const ReferenceMap* refMap,
                                        const cstring& fallback) {
    // XXX(seth): This whole concept seems suspect. In practice this is being
    // called with unnamed tuples and you end up with names like 'tuple_0' which
    // aren't really useful.
    if (args->size() != 1) return fallback;
    auto firstArg = args->at(0);
    if (!firstArg->is<IR::Type_Name>()) return fallback;
    auto origType = refMap->getDeclaration(firstArg->to<IR::Type_Name>()->path, true);
    if (!origType->is<IR::Type_Struct>()) return fallback;
    return origType->to<IR::Type_Struct>()->externalName();
}

static void collectControlSymbols(P4RuntimeSymbolTable& symbols,
                                  const IR::ControlBlock* controlBlock,
                                  ReferenceMap* refMap,
                                  TypeMap* typeMap) {
    CHECK_NULL(controlBlock);

    auto control = controlBlock->container;
    CHECK_NULL(control);

    for (auto declaration : *control->controlLocals) {
        // Collect the symbols for actions and action parameters.
        if (!declaration->is<IR::P4Action>()) continue;
        const IR::P4Action* action = declaration->to<IR::P4Action>();
        symbols.add(P4RuntimeSymbolType::ACTION, action);
        for (size_t index = 0; index < action->parameters->size(); index++) {
            symbols.addActionParameter(action, index);
        }

        // Collect the symbols for any header field lists which are passed to
        // standard externs in the action body.
        // XXX(seth): This is more hardcoded stuff.
        using CallStatement = IR::MethodCallStatement;
        forAllMatching<CallStatement>(action, [&](const CallStatement* callStatement) {
            auto call = callStatement->methodCall;
            auto instance = P4::MethodInstance::resolve(call, refMap, typeMap);
            if (!instance->is<P4::ExternFunction>()) return;
            auto externFunction = instance->to<P4::ExternFunction>();
            if (externFunction->method->name == "clone" ||
                    externFunction->method->name == "clone3") {
                symbols.add(P4RuntimeSymbolType::HEADER_FIELD_LIST, "fl");
            } else if (externFunction->method->name == "resubmit") {
                auto name = externalNameFromTypeArgs(call->typeArguments, refMap, "resubmit");
                symbols.add(P4RuntimeSymbolType::HEADER_FIELD_LIST, name);
            } else if (externFunction->method->name == "recirculate") {
                auto name = externalNameFromTypeArgs(call->typeArguments, refMap, "recirculate");
                symbols.add(P4RuntimeSymbolType::HEADER_FIELD_LIST, name);
            }
        });
    }
}

static void serializeFieldList(P4RuntimeSerializer& serializer,
                               TypeMap* typeMap,
                               const cstring& fieldListName,
                               const IR::Expression* fieldList) {
    auto fieldListType = typeMap->getType(fieldList, true);
    if (!fieldListType->is<IR::Type_Tuple>()) {
        ::error("Expected header field list '%1%' to be of tuple type", fieldList);
        return;
    }

    auto tuple = fieldListType->to<IR::Type_Tuple>();
    std::vector<const HeaderField*> fields;
    forAllTupleFields(tuple, typeMap, [&](const HeaderField* field) {
        fields.push_back(field);
    });

    serializer.addHeaderFieldList(fieldListName, fields);
}

static void serializeControl(P4RuntimeSerializer& serializer,
                             const IR::ControlBlock* controlBlock,
                             ReferenceMap* refMap,
                             TypeMap* typeMap) {
    CHECK_NULL(controlBlock);
    CHECK_NULL(refMap);
    CHECK_NULL(typeMap);

    auto control = controlBlock->container;
    CHECK_NULL(control);

    for (auto declaration : *control->controlLocals) {
        // Serialize actions and, implicitly, their parameters.
        if (!declaration->is<IR::P4Action>()) continue;
        auto action = declaration->to<IR::P4Action>();
        serializer.addAction(action);

        // Serialize any header field lists which are passed to standard externs in
        // the action body.
        // XXX(seth): This is more hardcoded stuff.
        using CallStatement = IR::MethodCallStatement;
        forAllMatching<CallStatement>(action, [&](const CallStatement* callStatement) {
            auto call = callStatement->methodCall;
            auto instance = P4::MethodInstance::resolve(call, refMap, typeMap);
            if (!instance->is<P4::ExternFunction>()) return;
            auto externFunction = instance->to<P4::ExternFunction>();
            if (externFunction->method->name == "clone") {
                serializer.addHeaderFieldList("fl", {});
            } else if (externFunction->method->name == "clone3") {
                serializeFieldList(serializer, typeMap, "fl", call->arguments->at(2));
            } else if (externFunction->method->name == "resubmit") {
                auto name = externalNameFromTypeArgs(call->typeArguments, refMap, "resubmit");
                serializeFieldList(serializer, typeMap, name, call->arguments->at(0));
            } else if (externFunction->method->name == "recirculate") {
                auto name = externalNameFromTypeArgs(call->typeArguments, refMap, "recirculate");
                serializeFieldList(serializer, typeMap, name, call->arguments->at(0));
            }
        });
    };
}

static void collectExternSymbols(P4RuntimeSymbolTable& symbols,
                                 const IR::ExternBlock* externBlock) {
    CHECK_NULL(externBlock);

    if (externBlock->type->name == "counter") {
        symbols.add(P4RuntimeSymbolType::COUNTER, externBlock->node->to<IR::IDeclaration>());
    } else if (externBlock->type->name == "meter") {
        symbols.add(P4RuntimeSymbolType::METER, externBlock->node->to<IR::IDeclaration>());
    }
}

static void serializeExtern(P4RuntimeSerializer& serializer,
                            const IR::ExternBlock* externBlock,
                            const TypeMap* typeMap) {
    CHECK_NULL(externBlock);
    CHECK_NULL(typeMap);

    if (externBlock->type->name == CounterlikeTraits<IR::Counter>::typeName()) {
        auto counter = Counterlike<IR::Counter>::from(externBlock);
        if (counter) serializer.addCounter(*counter);
    } else if (externBlock->type->name == CounterlikeTraits<IR::Meter>::typeName()) {
        auto meter = Counterlike<IR::Meter>::from(externBlock);
        if (meter) serializer.addMeter(*meter);
    }
}

static void collectParserSymbols(P4RuntimeSymbolTable& symbols,
                                 const IR::ParserBlock* parserBlock,
                                 TypeMap* typeMap) {
    CHECK_NULL(parserBlock);
    CHECK_NULL(parserBlock->container);
    CHECK_NULL(typeMap);

    auto parserType = parserBlock->container->type;
    for (auto param : *parserType->applyParams->getEnumerator()) {
        if (param->direction != IR::Direction::Out &&
            param->direction != IR::Direction::InOut) {
            // This parameter isn't an output, so nothing downstream can match on it.
            // That means that we don't need to expose it to P4Runtime.
            continue;
        }

            // This parameter is an output; it may contain header fields.
        forAllHeaderFields(param, typeMap, [&](const HeaderField* field) {
            symbols.addHeaderField(field);
        });
    }
}

static void serializeParser(P4RuntimeSerializer& serializer,
                            const IR::ParserBlock* parserBlock,
                            const TypeMap* typeMap) {
    CHECK_NULL(parserBlock);
    CHECK_NULL(parserBlock->container);
    CHECK_NULL(typeMap);

    auto parserType = parserBlock->container->type;
    for (auto param : *parserType->applyParams->getEnumerator()) {
        if (param->direction != IR::Direction::Out &&
            param->direction != IR::Direction::InOut) {
            // This parameter isn't an output, so nothing downstream can match on it.
            // That means that we don't need to expose it in P4Runtime.
            continue;
        }

            // This parameter is an output; it may contain header fields.
        forAllHeaderFields(param, typeMap, [&](const HeaderField* field) {
            serializer.addHeaderField(field);
        });
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
                propertyName, table->externalName(), property);
        return boost::none;
    }

    auto expr = property->value->to<IR::ExpressionValue>()->expression;
    auto name = property->externalName();
    auto externInstance = ExternInstance::resolve(expr, refMap, typeMap, name);
    if (!externInstance) {
        ::error("Expected %1% property value for table %2% to resolve to an "
                "extern instance: %3%", propertyName, table->externalName(),
                property);
        return boost::none;
    }

    return externInstance;
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
    auto impl = table->properties->getProperty(P4V1::V1Model::instance
                                                .tableAttributes
                                                .tableImplementation.name);
    if (impl) {
        symbols.add(P4RuntimeSymbolType::ACTION_PROFILE, impl->externalName());
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

/// @return true iff @expression is a call to the 'isValid()' builtin.
static const P4::BuiltInMethod*
tryCastToIsValidBuiltin(const IR::Expression* expression,
                        ReferenceMap* refMap,
                        TypeMap* typeMap) {
    if (!expression->is<IR::MethodCallExpression>()) return nullptr;
    auto methodCall = expression->to<IR::MethodCallExpression>();
    auto instance = P4::MethodInstance::resolve(methodCall, refMap, typeMap);
    if (!instance->is<P4::BuiltInMethod>()) return nullptr;
    auto builtin = instance->to<P4::BuiltInMethod>();
    if (builtin->name != IR::Type_Header::isValid) return nullptr;
    return builtin;
}

/// @return the header instance fields matched against by @table's key. The
/// fields are represented as a (fully qualified field name, match type) tuple.
static std::vector<std::pair<cstring, MatchType>>
getMatchFields(const IR::P4Table* table, ReferenceMap* refMap, TypeMap* typeMap) {
    std::vector<std::pair<cstring, MatchType>> matchFields;

    auto key = table->getKey();
    if (!key) return matchFields;

    for (auto keyElement : *key->keyElements) {
        auto matchTypeDecl = refMap->getDeclaration(keyElement->matchType->path, true)
                                   ->to<IR::Declaration_ID>();
        BUG_CHECK(matchTypeDecl != nullptr, "No declaration for match type '%1%'",
                                            keyElement->matchType);
        const auto matchTypeName = matchTypeDecl->name.name;

        auto expr = keyElement->expression;
        if (expr->is<IR::Slice>()) {
            expr = expr->to<IR::Slice>()->e0;
        }

        bool synthesizeValidField = false;
        MatchType matchType;
        if (matchTypeName == P4CoreLibrary::instance.exactMatch.name) {
            matchType = MatchField::EXACT;

            // If this is an exact match on the result of an isValid() call, we
            // desugar that to a "valid" match.
            auto callToIsValid = tryCastToIsValidBuiltin(expr, refMap, typeMap);
            if (callToIsValid != nullptr) {
                expr = callToIsValid->appliedTo;
                synthesizeValidField = true;
                matchType = MatchField::VALID;
            }
        } else if (matchTypeName == P4CoreLibrary::instance.lpmMatch.name) {
            matchType = MatchField::LPM;
        } else if (matchTypeName == P4CoreLibrary::instance.ternaryMatch.name) {
            matchType = MatchField::TERNARY;

            // If this is a ternary match on the result of an isValid() call, desugar
            // that to a ternary match against the '_valid' field of the appropriate
            // header instance.
            // XXX(seth): This is baking in BMV2-specific stuff. We should remove this
            // by performing the desugaring in a separate compiler pass.
            auto callToIsValid = tryCastToIsValidBuiltin(expr, refMap, typeMap);
            if (callToIsValid != nullptr) {
                expr = callToIsValid->appliedTo;
                synthesizeValidField = true;
            }
        } else if (matchTypeName == P4V1::V1Model::instance.rangeMatchType.name) {
            matchType = MatchField::RANGE;
        } else if (matchTypeName == P4V1::V1Model::instance.selectorMatchType.name) {
            // This match type indicates that this table is using an action
            // profile. We serialize action profiles separately from the tables
            // themselves, so we don't need to record any information about this
            // key, but it's worth doing a sanity check that this table has an
            // implementation property - if it doesn't, serializeActionProfiles()
            // will ignore it.
            auto impl = table->properties->getProperty(P4V1::V1Model::instance
                                                        .tableAttributes
                                                        .tableImplementation.name);
            BUG_CHECK(impl != nullptr, "Table '%1%' has match type 'selector' "
                                       "but no implementation property",
                      table->externalName());
            continue;
        } else {
            ::error("Table '%1%': cannot represent match type '%2%' in P4Runtime",
                    table->externalName(), matchTypeName);
            break;
        }

        HeaderFieldPath* path = HeaderFieldPath::from(expr, refMap, typeMap);
        if (!path) {
            ::error("Table '%1%': Match field '%2%' is too complicated "
                    "to represent in P4Runtime", table->externalName(), expr);
            break;
        }

        // If we're performing some variety of "valid" match, we desugar it into
        // a match against a hidden '_valid' field. That field doesn't really
        // exist in the program, of course, so we have to append it manually.
        if (synthesizeValidField) {
            path = path->append("_valid", new IR::Type_Boolean);
        }

        // XXX(seth): If there's only one component in the path, then it represents
        // a local variable. We need to support locals to support complex
        // expressions in keys, because such expressions are desugared into a match
        // against an intermediate local, but before implementing anything we need
        // to decide how it makes sense to present something like that in P4Runtime.
        if (!path->parent) {
            ::error("Table '%1%': Match field '%2%' references a local",
                    table->externalName(), expr);
            break;
        }

        matchFields.push_back(make_pair(path->serialize(), matchType));
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
        actionName = decl->to<IR::P4Action>()->externalName();
    } else if (expr->is<IR::MethodCallExpression>()) {
        auto callExpr = expr->to<IR::MethodCallExpression>();
        auto instance = P4::MethodInstance::resolve(callExpr, refMap, typeMap);
        BUG_CHECK(instance->is<P4::ActionCall>(), "Expected an action: %1%", expr);
        actionName = instance->to<P4::ActionCall>()->action->externalName();
    } else {
        ::error("Unexpected expression in default action for table %1%: %2%",
                table->externalName(), expr);
        return boost::none;
    }

    return DefaultAction{actionName, defaultActionProperty->isConstant};
}

static void serializeTable(P4RuntimeSerializer& serializer,
                           const IR::TableBlock* tableBlock,
                           ReferenceMap* refMap,
                           TypeMap* typeMap) {
    CHECK_NULL(tableBlock);

    auto table = tableBlock->container;
    auto tableSize = getTableSize(table);
    auto defaultAction = getDefaultAction(table, refMap, typeMap);
    auto matchFields = getMatchFields(table, refMap, typeMap);

    std::vector<cstring> actions;
    for (auto action : *table->getActionList()->actionList) {
        auto decl = refMap->getDeclaration(action->getPath(), true);
        BUG_CHECK(decl->is<IR::P4Action>(), "Not an action: '%1%'", decl);
        actions.push_back(decl->to<IR::P4Action>()->externalName());
    }

    auto impl = table->properties->getProperty(P4V1::V1Model::instance
                                                .tableAttributes
                                                .tableImplementation.name);
    boost::optional<cstring> implementation;
    if (impl) implementation = impl->externalName();

    auto directCounter = getDirectCounterlike<IR::Counter>(table, refMap, typeMap);
    auto directMeter = getDirectCounterlike<IR::Meter>(table, refMap, typeMap);

    serializer.addTable(table, tableSize, implementation, directCounter,
                        directMeter, defaultAction, actions, matchFields);
}

/// Visit evaluated blocks under the provided top-level block. Guarantees that
/// each block is visited only once, even if multiple paths to reach it exist.
template <typename Func>
static void forAllEvaluatedBlocks(const IR::ToplevelBlock* aToplevelBlock,
                                  Func function) {
    set<const IR::Block*> visited;
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
                "action profile: %2%", table->externalName(), instance->expression);
        return boost::none;
    }

    if (!sizeExpression->is<IR::Constant>()) {
      ::error("Action profile '%1%' has non-constant size '%1%'",
              *instance->name, sizeExpression);
      return boost::none;
    }

    const int64_t size = sizeExpression->to<IR::Constant>()->asInt();
    return ActionProfile{*instance->name, actionProfileType, size};
}

static void serializeActionProfiles(P4RuntimeSerializer& serializer,
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
          actionProfiles[*actionProfile].insert(table->externalName());
        }
    });

    for (const auto& actionProfile : actionProfiles) {
        const auto& profile = actionProfile.first;
        const auto& tables = actionProfile.second;
        serializer.addActionProfile(profile, tables);
    }
}

} // namespace ControlPlaneAPI

void serializeP4Runtime(std::ostream* destination,
                        const IR::P4Program* program,
                        const IR::ToplevelBlock* evaluatedProgram,
                        ReferenceMap* refMap,
                        TypeMap* typeMap,
                        P4RuntimeFormat format /* = P4RuntimeFormat::BINARY */) {
    using namespace ControlPlaneAPI;

    // Perform a first pass to collect all of the control plane visible symbols in
    // the program.
    // XXX(seth): Because it will contain synthesized symbols that don't exist at
    // the P4 level, and because the symbol names are mangled according to a
    // fairly specific scheme derived from the BMV2 JSON format, right now this
    // table isn't just a subset of the P4 symbol table for the same program as
    // you'd expect. Eventually, we'd like to get to the point where it is.
    auto symbols = P4RuntimeSymbolTable::create([=](P4RuntimeSymbolTable& symbols) {
        forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
            if (block->is<IR::ControlBlock>()) {
                collectControlSymbols(symbols, block->to<IR::ControlBlock>(), refMap, typeMap);
            } else if (block->is<IR::ExternBlock>()) {
                collectExternSymbols(symbols, block->to<IR::ExternBlock>());
            } else if (block->is<IR::ParserBlock>()) {
                collectParserSymbols(symbols, block->to<IR::ParserBlock>(), typeMap);
            } else if (block->is<IR::TableBlock>()) {
                collectTableSymbols(symbols, block->to<IR::TableBlock>(), refMap, typeMap);
            }
        });
    });

    // Construct and serialize a P4Runtime control plane API from the program.
    P4RuntimeSerializer serializer(symbols, refMap, typeMap);
    forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block* block) {
        if (block->is<IR::ControlBlock>()) {
            serializeControl(serializer, block->to<IR::ControlBlock>(), refMap, typeMap);
        } else if (block->is<IR::ExternBlock>()) {
            serializeExtern(serializer, block->to<IR::ExternBlock>(), typeMap);
        } else if (block->is<IR::ParserBlock>()) {
            serializeParser(serializer, block->to<IR::ParserBlock>(), typeMap);
        } else if (block->is<IR::TableBlock>()) {
            serializeTable(serializer, block->to<IR::TableBlock>(), refMap, typeMap);
        }
    });
    serializeActionProfiles(serializer, evaluatedProgram, refMap, typeMap);

    // Write the serialization out in the requested format.
    switch (format) {
        case P4RuntimeFormat::BINARY: serializer.writeTo(destination); break;
        case P4RuntimeFormat::JSON: serializer.writeJsonTo(destination); break;
    }
}

}  // namespace P4
