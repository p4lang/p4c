/*
Copyright 2018-present Barefoot Networks, Inc.

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

#ifndef CONTROL_PLANE_P4RUNTIMEARCHHANDLER_H_
#define CONTROL_PLANE_P4RUNTIMEARCHHANDLER_H_

#include <boost/optional.hpp>

#include <set>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "p4/config/v1/p4info.pb.h"
#pragma GCC diagnostic pop

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/externInstance.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "lib/ordered_set.h"

namespace P4 {

/** \addtogroup control_plane
 *  @{
 */
namespace ControlPlaneAPI {

using p4rt_id_t = uint32_t;

/// Base class for the different types introduced by the P4 architecture. The
/// class includes static factory methods for all the built-in P4 types common
/// to all architectures. When defining the P4Info serialization logic for a new
/// architecture, the first step is to inherit from this class and add factory
/// methods for architecture-specific types. The class is used to separate all
/// symbols by type in the symbol table which is responsible for control-plane
/// id assignment.
class P4RuntimeSymbolType {
 public:
    virtual ~P4RuntimeSymbolType() { }

    /// An explicit conversion operator that is useful when building the P4Info
    /// message. It returns the id prefix for that type, as defined in the
    /// p4info.proto file.
    explicit operator p4rt_id_t() const { return id; }

    static P4RuntimeSymbolType ACTION() {
        return P4RuntimeSymbolType(::p4::config::v1::P4Ids::ACTION);
    }
    static P4RuntimeSymbolType TABLE() {
        return P4RuntimeSymbolType(::p4::config::v1::P4Ids::TABLE);
    }
    static P4RuntimeSymbolType VALUE_SET() {
        return P4RuntimeSymbolType(::p4::config::v1::P4Ids::VALUE_SET);
    }
    static P4RuntimeSymbolType CONTROLLER_HEADER() {
        return P4RuntimeSymbolType(::p4::config::v1::P4Ids::CONTROLLER_HEADER);
    }

    bool operator==(const P4RuntimeSymbolType& other) const {
        return id == other.id;
    }

    bool operator!=(const P4RuntimeSymbolType& other) const {
        return !(*this == other);
    }

    bool operator<(const P4RuntimeSymbolType& other) const {
        return id < other.id;
    }

 protected:
    static P4RuntimeSymbolType make(p4rt_id_t id) {
        return P4RuntimeSymbolType(id);
    }

 private:
    // even if the constructor is protected, the static functions in the derived
    // classes cannot access it, which is why we use the make factory function
    constexpr P4RuntimeSymbolType(p4rt_id_t id) noexcept
        : id(id) { }

    /// The 8-bit id prefix for that type, as per the p4info.proto file.
    p4rt_id_t id;
};

/// A table which tracks the symbols which are visible to P4Runtime and their
/// ids.
class P4RuntimeSymbolTableIface {
 public:
    virtual ~P4RuntimeSymbolTableIface() { }
    /// Add a @type symbol, extracting the name and id from @declaration.
    virtual void add(P4RuntimeSymbolType type, const IR::IDeclaration* declaration) = 0;
    /// Add a @type symbol with @name and possibly an explicit P4 '@id'.
    virtual void add(P4RuntimeSymbolType type, cstring name,
                     boost::optional<p4rt_id_t> id = boost::none) = 0;
    /// @return the P4Runtime id for the symbol of @type corresponding to
    /// @declaration.
    virtual p4rt_id_t getId(P4RuntimeSymbolType type,
                            const IR::IDeclaration* declaration) const = 0;
    /// @return the P4Runtime id for the symbol of @type with name @name.
    virtual p4rt_id_t getId(P4RuntimeSymbolType type, cstring name) const = 0;
    /// @return the alias for the given fully qualified external name. P4Runtime
    /// defines an alias for each object to make referring to objects easier.
    /// By default, the alias is the shortest unique suffix of path components
    /// in the name.
    virtual cstring getAlias(cstring name) const = 0;
};

/// The interface for defining the P4Info serialization logic for a specific P4
/// architecture. The goal is to reduce code duplication between
/// architectures. The @ref P4RuntimeSerializer will call these methods when
/// generating the P4Info message to handle architecture-specific parts. @ref
/// P4RuntimeSerializer generates the P4Info in two passes: first it collects
/// all the control-plane visible symbols from the program into the symbol
/// table, then it builds the P4Info message by adding each collected entity to
/// the Protobuf message. The collect* methods are called in the first pass, the
/// add* methods are called in the second pass.
class P4RuntimeArchHandlerIface {
 public:
    virtual ~P4RuntimeArchHandlerIface() { }
    /// Collects architecture-specific properties for @tableBlock in @symbols
    /// table.
    virtual void collectTableProperties(P4RuntimeSymbolTableIface* symbols,
                                        const IR::TableBlock* tableBlock) = 0;
    /// Collects architecture-specific @externBlock instance in @symbols table.
    virtual void collectExternInstance(P4RuntimeSymbolTableIface* symbols,
                                       const IR::ExternBlock* externBlock) = 0;
    /// Collects extern method call @externFunction in @symbols table in case it
    /// needs to be exposed to the control-plane (e.g. digest call for v1model).
    virtual void collectExternFunction(P4RuntimeSymbolTableIface* symbols,
                                       const P4::ExternFunction* externFunction) = 0;
    /// Collects any extra symbols you may want to include in the symbol table
    /// and that are not covered by the above collection methods.
    virtual void collectExtra(P4RuntimeSymbolTableIface* symbols) = 0;
    /// This method is called between the two passes (collect and add) in case
    /// the architecture requires some logic to be performed then.
    virtual void postCollect(const P4RuntimeSymbolTableIface& symbols) = 0;
    /// Adds architecture-specific properties for @tableBlock to the @table
    /// Protobuf message.
    virtual void addTableProperties(const P4RuntimeSymbolTableIface& symbols,
                                    ::p4::config::v1::P4Info* p4info,
                                    ::p4::config::v1::Table* table,
                                    const IR::TableBlock* tableBlock) = 0;
    /// Adds relevant information about @externBlock instance to the @p4info
    /// message.
    virtual void addExternInstance(const P4RuntimeSymbolTableIface& symbols,
                                   ::p4::config::v1::P4Info* p4info,
                                   const IR::ExternBlock* externBlock) = 0;
    /// Adds relevant information about @externFunction method call - if it
    /// needs to be exposed to the control-plane - to @p4info message.
    virtual void addExternFunction(const P4RuntimeSymbolTableIface& symbols,
                                   ::p4::config::v1::P4Info* p4info,
                                   const P4::ExternFunction* externFunction) = 0;
    /// This method is called after the add pass in case the architecture
    /// requires some logic to be performed then.
    virtual void postAdd(const P4RuntimeSymbolTableIface& symbols,
                         ::p4::config::v1::P4Info* p4info) = 0;
};

/// A functor interface that needs to be implemented for each
/// architecture-specific handler in charge of generating the appropriate P4Info
/// message.
struct P4RuntimeArchHandlerBuilderIface {
    virtual ~P4RuntimeArchHandlerBuilderIface() { }

    /// Called by the @ref P4RuntimeSerializer to build an instance of the
    /// appropriate @ref P4RuntimeArchHandlerIface implementation for the
    /// architecture, with the appropriate @refMap, @typeMap and
    /// @evaluatedProgram.
    virtual P4RuntimeArchHandlerIface* operator()(
        ReferenceMap* refMap,
        TypeMap* typeMap,
        const IR::ToplevelBlock* evaluatedProgram) const = 0;
};

/// A collection of helper functions which can be used to implement @ref
/// P4RuntimeArchHandlerIface for different architectures.
namespace Helpers {

/// @return an extern instance defined or referenced by the value of @table's
/// @propertyName property, or boost::none if no extern was referenced.
boost::optional<ExternInstance>
getExternInstanceFromProperty(const IR::P4Table* table,
                              const cstring& propertyName,
                              ReferenceMap* refMap,
                              TypeMap* typeMap,
                              bool *isConstructedInPlace = nullptr);

/// @return true if the extern instance assigned to property @propertyName for
/// the @table was constructed in-place or outside of teh @table declaration.
bool isExternPropertyConstructedInPlace(const IR::P4Table* table,
                                        const cstring& propertyName);

/// Visit evaluated blocks under the provided top-level block. Guarantees that
/// each block is visited only once, even if multiple paths to reach it exist.
template <typename Func>
void forAllEvaluatedBlocks(const IR::ToplevelBlock* aToplevelBlock, Func function) {
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
            // child block may be nullptr due to optional argument.
            if (!evaluatedChild.second) continue;
            if (!evaluatedChild.second->is<IR::Block>()) continue;
            auto evaluatedChildBlock = evaluatedChild.second->to<IR::Block>();
            if (visited.find(evaluatedChildBlock) != visited.end()) continue;
            frontier.insert(evaluatedChildBlock);
        }
    }
}

std::string serializeOneAnnotation(const IR::Annotation* annotation);

/// Serialize @annotated's P4 annotations and attach them to a P4Info message
/// with an 'annotations' field. '@name', '@id' and documentation annotations
/// are ignored, as well as annotations whose name satisfies predicate @p.
template <typename Message, typename UnaryPredicate>
void addAnnotations(Message* message, const IR::IAnnotated* annotated, UnaryPredicate p) {
    CHECK_NULL(message);

    // Synthesized resources may have no annotations.
    if (annotated == nullptr) return;

    for (const IR::Annotation* annotation : annotated->getAnnotations()->annotations) {
        // Don't output the @name or @id annotations; they're represented
        // elsewhere in P4Info messages.
        if (annotation->name == IR::Annotation::nameAnnotation) continue;
        if (annotation->name == "id") continue;

        // Don't output the @brief or @description annotations; they're
        // represented using the documentation fields.
        if (annotation->name == "brief" || annotation->name == "description")
          continue;

        if (p(annotation->name)) continue;

        message->add_annotations(serializeOneAnnotation(annotation));
    }
}

/// calls addAnnotations with a unconditionally false predicate.
template <typename Message>
void addAnnotations(Message* message, const IR::IAnnotated* annotated) {
    addAnnotations(message, annotated, [](cstring){ return false; });
}

/// Set the 'doc' field for a P4Info message, using the '@brief' and
/// '@description' annotations if present.
template <typename Message>
void addDocumentation(Message* message, const IR::IAnnotated* annotated) {
    CHECK_NULL(message);

    // Synthesized resources may have no annotations.
    if (annotated == nullptr) return;

    ::p4::config::v1::Documentation doc;
    bool hasDoc = false;

    // we iterate over all annotations looking for '@brief' and / or
    // '@description'. As per the P4Runtime spec, we only set the 'doc' field in
    // the message if at least one of them is present.
    for (const IR::Annotation* annotation : annotated->getAnnotations()->annotations) {
        if (annotation->name == "brief") {
            auto brief = annotation->expr[0]->to<IR::StringLiteral>();
            // guaranteed by ParseAnnotations pass
            CHECK_NULL(brief);
            doc.set_brief(brief->value);
            hasDoc = true;
            continue;
        }
        if (annotation->name == "description") {
            auto description = annotation->expr[0]->to<IR::StringLiteral>();
            // guaranteed by ParseAnnotations pass
            CHECK_NULL(description);
            doc.set_description(description->value);
            hasDoc = true;
            continue;
        }
    }

    if (hasDoc) message->mutable_doc()->CopyFrom(doc);
}

/// Set all the fields in the @preamble, including the 'annotations' and 'doc'
/// fields.
void setPreamble(::p4::config::v1::Preamble* preamble,
                 p4rt_id_t id, cstring name, cstring alias, const IR::IAnnotated* annotated);

/// @return @table's size property if available, falling back to the
/// architecture's default size.
int64_t getTableSize(const IR::P4Table* table);

/// A traits class describing the properties of "counterlike" things.
template <typename Kind> struct CounterlikeTraits;

/// The information about a counter or meter instance which is necessary to
/// serialize it. @Kind must be a class with a CounterlikeTraits<>
/// specialization. This can be useful for different architectures as many
/// define extern types for counter / meter. The architecture-specific code must
/// specialize CounterlikeTraits<> appropriately.
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

        auto size = instance->getParameterValue(CounterlikeTraits<Kind>::sizeParamName());
        if (!size->template is<IR::Constant>()) {
            ::error("%1% '%2%' has a non-constant size: %3%",
                    CounterlikeTraits<Kind>::name(), declaration, size);
            return boost::none;
        }

        return Counterlike<Kind>{declaration->controlPlaneName(),
                                 declaration->to<IR::IAnnotated>(),
                                 unit->to<IR::Declaration_ID>()->name,
                                 size->template to<IR::Constant>()->value.get_si(),
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

        auto unitArgument = instance.substitution.lookupByName("type")->expression;
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
                                 unit, Helpers::getTableSize(table),
                                 table->controlPlaneName()};
    }
};

/// @return the direct counter associated with @table, if it has one, or
/// boost::none otherwise.
template <typename Kind>
boost::optional<Counterlike<Kind>>
getDirectCounterlike(const IR::P4Table* table, ReferenceMap* refMap, TypeMap* typeMap) {
    auto propertyName = CounterlikeTraits<Kind>::directPropertyName();
    auto instance =
      getExternInstanceFromProperty(table, propertyName, refMap, typeMap);
    if (!instance) return boost::none;
    return Counterlike<Kind>::fromDirect(*instance, table);
}

}  // namespace Helpers

}  // namespace ControlPlaneAPI

/** @} */  /* end group control_plane */
}  // namespace P4

#endif  /* CONTROL_PLANE_P4RUNTIMEARCHHANDLER_H_ */
