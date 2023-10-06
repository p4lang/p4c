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
#include "p4RuntimeSerializer.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/json_util.h>
#pragma GCC diagnostic pop

#include <algorithm>
#include <iostream>
#include <iterator>
#include <optional>
#include <set>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <vector>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wpedantic"
#include "p4/v1/p4runtime.pb.h"
#pragma GCC diagnostic pop

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/reversed.hpp>

// TODO(antonin): this include should go away when we cleanup getMatchFields
// and tableNeedsPriority implementations.
#include "control-plane/bytestrings.h"
#include "control-plane/flattenHeader.h"
#include "frontends/common/options.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/externInstance.h"
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
#include "p4RuntimeAnnotations.h"
#include "p4RuntimeArchHandler.h"
#include "p4RuntimeArchStandard.h"
#include "p4RuntimeSymbolTable.h"
#include "typeSpecConverter.h"

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

/// @return the value of @item's explicit name annotation, if it has one. We use
/// this rather than e.g. controlPlaneName() when we want to prevent any
/// fallback.
static std::optional<cstring> explicitNameAnnotation(const IR::IAnnotated *item) {
    auto *anno = item->getAnnotation(IR::Annotation::nameAnnotation);
    if (!anno) return std::nullopt;
    if (anno->expr.size() != 1) {
        ::error(ErrorType::ERR_INVALID, "A %1% annotation must have one argument", anno);
        return std::nullopt;
    }
    auto *str = anno->expr[0]->to<IR::StringLiteral>();
    if (!str) {
        ::error(ErrorType::ERR_INVALID, "An %1% annotation's argument must be a string", anno);
        return std::nullopt;
    }
    return str->value;
}

namespace writers {

using google::protobuf::Message;

/// Serialize the protobuf @message to @destination in the binary protocol
/// buffers format.
static bool writeTo(const Message &message, std::ostream *destination) {
    CHECK_NULL(destination);
    if (!message.SerializeToOstream(destination)) return false;
    destination->flush();
    return true;
}

/// Serialize the protobuf @message to @destination in the JSON protocol buffers
/// format. This is intended for debugging and testing.
static bool writeJsonTo(const Message &message, std::ostream *destination) {
    using namespace google::protobuf::util;
    CHECK_NULL(destination);

    // Serialize the JSON in a human-readable format.
    JsonPrintOptions options;
    options.add_whitespace = true;

    std::string output;
    if (!MessageToJsonString(message, &output, options).ok()) {
        ::error(ErrorType::ERR_IO, "Failed to serialize protobuf message to JSON");
        return false;
    }

    *destination << output;
    if (!destination->good()) {
        ::error(ErrorType::ERR_IO, "Failed to write JSON protobuf message to the output");
        return false;
    }

    destination->flush();
    return true;
}

/// Serialize the protobuf @message to @destination in the text protocol buffers
/// format. This is intended for debugging and testing.
static bool writeTextTo(const Message &message, std::ostream *destination) {
    CHECK_NULL(destination);

    // According to the protobuf documentation, it would be better to use Print
    // with a FileOutputStream object for performance reasons. However all we
    // have here is a std::ostream and performance is not a concern.
    std::string output;
    google::protobuf::TextFormat::Printer textPrinter;
    // set to expand google.protobuf.Any payloads
    textPrinter.SetExpandAny(true);
    if (textPrinter.PrintToString(message, &output) == false) {
        ::error(ErrorType::ERR_IO, "Failed to serialize protobuf message to text");
        return false;
    }

    *destination << output;
    if (!destination->good()) {
        ::error(ErrorType::ERR_IO, "Failed to write text protobuf message to the output");
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

    const cstring name;    // The fully qualified external name of this field.
    const p4rt_id_t id;    // The id for this field - either user-provided or auto-allocated.
    const MatchType type;  // The match algorithm - exact, ternary, range, etc.
    const cstring other_match_type;     // If the match type is an arch-specific one
                                        // in this case, type must be MatchTypes::UNSPECIFIED
    const uint32_t bitwidth;            // How wide this field is.
    const IR::IAnnotated *annotations;  // If non-null, any annotations applied
                                        // to this field.
    const cstring type_name;            // Optional field used when field is Type_Newtype.
};

struct ActionRef {
    const cstring name;                 // The fully qualified external name of the action.
    const IR::IAnnotated *annotations;  // If non-null, any annotations applied to this action
                                        // reference in the table declaration.
};

/// FieldIdAllocator is used to allocate ids for non top-level P4Info objects
/// that need them (match fields, action parameters, packet IO metadata
/// fields). Some of these ids can come from the P4 program (@id annotation),
/// the rest is auto-generated.
template <typename T>
class FieldIdAllocator {
 public:
    // Parameters must be iterators of Ts.
    // All the user allocated ids must be provided in one-shot, which is why we
    // require all objects to be provided in the constructor.
    template <typename It>
    FieldIdAllocator(
        It begin, It end,
        typename std::enable_if<
            std::is_same<typename std::iterator_traits<It>::value_type, T>::value>::type * = 0) {
        // first pass: user-assigned ids
        for (auto it = begin; it != end; ++it) {
            auto id = getIdAnnotation(*it);
            if (!id) continue;
            if (*id == 0) {
                ::error(ErrorType::ERR_INVALID, "%1%: 0 is not a valid @id value", *it);
            } else if (assignedIds.count(*id) > 0) {
                ::error(ErrorType::ERR_DUPLICATE, "%1%: @id %2% is used multiple times", *it, *id);
            }
            idMapping[*it] = *id;
            assignedIds.insert(*id);
        }

        // second pass: allocate missing ids
        // in the absence of any user-provided @id, ids will be allocated
        // sequentially, starting at 1.
        p4rt_id_t index = 1;
        for (auto it = begin; it != end; ++it) {
            if (idMapping.find(*it) != idMapping.end()) {
                index++;
                continue;
            }
            while (assignedIds.count(index) > 0) {
                index++;
                BUG_CHECK(index > 0, "Cannot allocate default id for field");
            }
            idMapping[*it] = index;
            assignedIds.insert(index);
        }
    }

    p4rt_id_t getId(T v) {
        auto it = idMapping.find(v);
        BUG_CHECK(it != idMapping.end(), "Missing id allocation");
        return it->second;
    }

 private:
    std::set<p4rt_id_t> assignedIds;
    std::map<T, p4rt_id_t> idMapping;
};

/// @return @table's default action, if it has one, or std::nullopt otherwise.
static std::optional<DefaultAction> getDefaultAction(const IR::P4Table *table, ReferenceMap *refMap,
                                                     TypeMap *typeMap) {
    // not using getDefaultAction() here as I actually need the property IR node
    // to check if the default action is constant.
    auto defaultActionProperty =
        table->properties->getProperty(IR::TableProperties::defaultActionPropertyName);
    if (defaultActionProperty == nullptr) return std::nullopt;
    if (!defaultActionProperty->value->is<IR::ExpressionValue>()) {
        ::error(ErrorType::ERR_EXPECTED, "Expected an action: %1%", defaultActionProperty);
        return std::nullopt;
    }

    auto expr = defaultActionProperty->value->to<IR::ExpressionValue>()->expression;
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
        ::error(ErrorType::ERR_UNEXPECTED,
                "Unexpected expression in default action for table %1%: %2%",
                table->controlPlaneName(), expr);
        return std::nullopt;
    }

    return DefaultAction{actionName, defaultActionProperty->isConstant};
}

/// @return true if @table has a 'const entries' property.
static bool getConstTable(const IR::P4Table *table) {
    BUG_CHECK(table != nullptr, "Failed precondition for getConstTable");
    auto ep = table->properties->getProperty(IR::TableProperties::entriesPropertyName);
    if (ep == nullptr) return false;
    BUG_CHECK(ep->value->is<IR::EntriesList>(), "Invalid 'entries' property");
    return ep->isConstant;
}

/// @return true if @table has an 'entries' or 'const entries'
/// property, and there is at least one entry.
static bool getHasInitialEntries(const IR::P4Table *table) {
    BUG_CHECK(table != nullptr, "Failed precondition for getHasInitialEntries");
    auto entriesList = table->getEntries();
    if (entriesList == nullptr) return false;
    return (entriesList->entries.size() >= 1);
}

static std::vector<ActionRef> getActionRefs(const IR::P4Table *table, ReferenceMap *refMap) {
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

static cstring getMatchTypeName(const IR::PathExpression *matchPathExpr,
                                const ReferenceMap *refMap) {
    CHECK_NULL(matchPathExpr);
    auto matchTypeDecl =
        refMap->getDeclaration(matchPathExpr->path, true)->to<IR::Declaration_ID>();
    BUG_CHECK(matchTypeDecl != nullptr, "No declaration for match type '%1%'", matchPathExpr);
    return matchTypeDecl->name.name;
}

/// maps the match type name to the corresponding P4Info MatchType enum
/// member. If the match type should not be exposed to the control plane and
/// should be ignored, std::nullopt is returned. If the match type does not
/// correspond to any standard match type known to P4Info, default enum member
/// UNSPECIFIED is returned.
static std::optional<MatchField::MatchType> getMatchType(cstring matchTypeName) {
    if (matchTypeName == P4CoreLibrary::instance().exactMatch.name) {
        return MatchField::MatchTypes::EXACT;
    } else if (matchTypeName == P4CoreLibrary::instance().lpmMatch.name) {
        return MatchField::MatchTypes::LPM;
    } else if (matchTypeName == P4CoreLibrary::instance().ternaryMatch.name) {
        return MatchField::MatchTypes::TERNARY;
    } else if (matchTypeName == P4V1::V1Model::instance.rangeMatchType.name) {
        return MatchField::MatchTypes::RANGE;
    } else if (matchTypeName == P4V1::V1Model::instance.optionalMatchType.name) {
        return MatchField::MatchTypes::OPTIONAL;
    } else if (matchTypeName == P4V1::V1Model::instance.selectorMatchType.name) {
        // Nothing to do here, we cannot even perform some sanity-checking.
        return std::nullopt;
    } else {
        return MatchField::MatchTypes::UNSPECIFIED;
    }
}

// getTypeWidth returns the width in bits for the @type, except if it is a
// user-defined type with a @p4runtime_translation annotation, in which case it
// returns W if the type is bit<W>, and 0 otherwise (i.e. if the type is
// string).
static int getTypeWidth(const IR::Type *type, TypeMap *typeMap) {
    TranslationAnnotation annotation;
    if (hasTranslationAnnotation(type, &annotation)) {
        // W if the type is bit<W>, and 0 if the type is string
        return annotation.controller_type.width;
    }
    /* Treat error type as string */
    if (type->is<IR::Type_Error>()) return 0;

    return typeMap->widthBits(type, type->getNode(), false);
}

/// @return the header instance fields matched against by @table's key. The
/// fields are represented as a (fully qualified field name, match type) tuple.
static std::vector<MatchField> getMatchFields(const IR::P4Table *table, ReferenceMap *refMap,
                                              TypeMap *typeMap,
                                              p4configv1::P4TypeInfo *p4RtTypeInfo) {
    std::vector<MatchField> matchFields;

    auto key = table->getKey();
    if (!key) return matchFields;

    FieldIdAllocator<decltype(key->keyElements)::value_type> idAllocator(key->keyElements.begin(),
                                                                         key->keyElements.end());

    for (auto keyElement : key->keyElements) {
        auto matchTypeName = getMatchTypeName(keyElement->matchType, refMap);
        auto matchType = getMatchType(matchTypeName);
        if (matchType == std::nullopt) continue;

        auto id = idAllocator.getId(keyElement);

        auto matchFieldName = explicitNameAnnotation(keyElement);
        BUG_CHECK(bool(matchFieldName),
                  "Table '%1%': Match field '%2%' has no "
                  "@name annotation",
                  table->controlPlaneName(), keyElement->expression);
        auto *matchFieldType = typeMap->getType(keyElement->expression->getNode(), true);
        BUG_CHECK(matchFieldType != nullptr, "Couldn't determine type for key element %1%",
                  keyElement);
        // We ignore the return type on purpose, but the call is required to update p4RtTypeInfo if
        // the match field has a user-defined type.
        TypeSpecConverter::convert(refMap, typeMap, matchFieldType, p4RtTypeInfo);
        auto type_name = getTypeName(matchFieldType, typeMap);
        int width = getTypeWidth(matchFieldType, typeMap);
        matchFields.push_back(MatchField{*matchFieldName, id, *matchType, matchTypeName,
                                         uint32_t(width), keyElement->to<IR::IAnnotated>(),
                                         type_name});
    }

    return matchFields;
}

namespace {

// It must be an iterator type pointing to a p4info.proto message with a
// 'preamble' field of type p4configv1::Preamble. Fn is an arbitrary function
// with a single parameter of type p4configv1::Preamble.
template <typename It, typename Fn>
void forEachPreamble(It first, It last, Fn fn) {
    for (It it = first; it != last; it++) fn(it->preamble());
}

}  // namespace

/// An analyzer which translates the information available in the P4 IR into a
/// representation of the control plane API which is consumed by P4Runtime.
class P4RuntimeAnalyzer {
    using Preamble = p4configv1::Preamble;
    using P4Info = p4configv1::P4Info;

    P4RuntimeAnalyzer(const P4RuntimeSymbolTable &symbols, TypeMap *typeMap, ReferenceMap *refMap,
                      P4RuntimeArchHandlerIface *archHandler)
        : p4Info(new P4Info),
          symbols(symbols),
          typeMap(typeMap),
          refMap(refMap),
          archHandler(archHandler) {
        CHECK_NULL(typeMap);
    }

    /// @return the P4Info message generated by this analyzer. This captures
    /// P4Runtime representations of all the P4 constructs added to the control
    /// plane API with the add*() methods.
    const P4Info *getP4Info() const {
        BUG_CHECK(p4Info != nullptr, "Didn't produce a P4Info object?");
        return p4Info;
    }

    /// Check for duplicate names among objects of the same type in the
    /// generated P4Info message and @return the number of duplicates.
    template <typename T>
    size_t checkForDuplicatesOfSameType(const T &objs, cstring typeName,
                                        std::unordered_set<p4rt_id_t> *ids) const {
        size_t dupCnt = 0;
        std::unordered_set<std::string> names;

        auto checkOne = [&dupCnt, &names, &ids, typeName](const p4configv1::Preamble &pre) {
            auto pName = names.insert(pre.name());
            auto pId = ids->insert(pre.id());
            if (!pName.second) {
                ::error(ErrorType::ERR_DUPLICATE,
                        "Name '%1%' is used for multiple %2% objects in the P4Info message",
                        pre.name(), typeName);
                dupCnt++;
                return;
            }
            BUG_CHECK(pId.second, "Id '%1%' is used for multiple objects in the P4Info message",
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
        dupCnt += checkForDuplicatesOfSameType(p4Info->controller_packet_metadata(),
                                               "controller packet metadata", &ids);
        dupCnt += checkForDuplicatesOfSameType(p4Info->value_sets(), "value set", &ids);
        dupCnt += checkForDuplicatesOfSameType(p4Info->registers(), "register", &ids);
        dupCnt += checkForDuplicatesOfSameType(p4Info->digests(), "digest", &ids);

        for (const auto &externType : p4Info->externs()) {
            dupCnt += checkForDuplicatesOfSameType(externType.instances(),
                                                   externType.extern_type_name(), &ids);
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
    static P4RuntimeAPI analyze(const IR::P4Program *program,
                                const IR::ToplevelBlock *evaluatedProgram, ReferenceMap *refMap,
                                TypeMap *typeMap, P4RuntimeArchHandlerIface *archHandler,
                                cstring arch);

    void addAction(const IR::P4Action *actionDeclaration) {
        if (isHidden(actionDeclaration)) return;

        auto name = actionDeclaration->controlPlaneName();
        auto id = symbols.getId(P4RuntimeSymbolType::P4RT_ACTION(), name);
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
        setPreamble(action->mutable_preamble(), id, name, symbols.getAlias(name), annotations,
                    [this](cstring anno) { return archHandler->filterAnnotations(anno); });

        // Allocate ids for all action parameters.
        std::vector<const IR::Parameter *> actionParams;
        for (auto actionParam : *actionDeclaration->parameters->getEnumerator()) {
            actionParams.push_back(actionParam);
        }
        FieldIdAllocator<decltype(actionParams)::value_type> idAllocator(actionParams.begin(),
                                                                         actionParams.end());

        for (auto actionParam : actionParams) {
            auto param = action->add_params();
            auto paramName = actionParam->controlPlaneName();
            auto id = idAllocator.getId(actionParam);
            param->set_id(id);
            param->set_name(paramName);
            addAnnotations(param, actionParam->to<IR::IAnnotated>());
            addDocumentation(param, actionParam->to<IR::IAnnotated>());

            auto paramType = typeMap->getType(actionParam, true);
            if (!paramType->is<IR::Type_Bits>() && !paramType->is<IR::Type_Boolean>() &&
                !paramType->is<IR::Type_Newtype>() && !paramType->is<IR::Type_SerEnum>() &&
                !paramType->is<IR::Type_Enum>()) {
                ::error(ErrorType::ERR_TYPE_ERROR,
                        "Action parameter %1% has a type which is not "
                        "bit<>, int<>, bool, type or serializable enum",
                        actionParam);
                continue;
            }
            if (!paramType->is<IR::Type_Enum>()) {
                int w = getTypeWidth(paramType, typeMap);
                param->set_bitwidth(w);
            }
            // We ignore the return type on purpose, but the call is required to update p4RtTypeInfo
            // if the action parameter has a user-defined type.
            TypeSpecConverter::convert(refMap, typeMap, paramType, p4Info->mutable_type_info());
            auto type_name = getTypeName(paramType, typeMap);
            if (type_name) {
                auto namedType = param->mutable_type_name();
                namedType->set_name(type_name);
            } else if (auto e = paramType->to<IR::Type_Enum>()) {
                param->mutable_type_name()->set_name(std::string(e->controlPlaneName()));
            }
        }
    }

    void addControllerHeader(const IR::Type_Header *type) {
        if (isHidden(type)) return;

        auto flattenedHeaderType = FlattenHeader::flatten(typeMap, type);

        auto name = type->controlPlaneName();
        auto id = symbols.getId(P4RuntimeSymbolType::P4RT_CONTROLLER_HEADER(), name);
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
        setPreamble(header->mutable_preamble(), id, controllerName /* name */,
                    controllerName /* alias */, annotations,
                    [this](cstring anno) { return archHandler->filterAnnotations(anno); });

        FieldIdAllocator<decltype(flattenedHeaderType->fields)::value_type> idAllocator(
            flattenedHeaderType->fields.begin(), flattenedHeaderType->fields.end());

        for (auto headerField : flattenedHeaderType->fields) {
            if (isHidden(headerField)) continue;
            auto metadata = header->add_metadata();
            auto fieldName = headerField->controlPlaneName();
            auto id = idAllocator.getId(headerField);
            metadata->set_id(id);
            metadata->set_name(fieldName);
            addAnnotations(metadata, headerField->to<IR::IAnnotated>());

            auto fieldType = typeMap->getType(headerField, true);
            BUG_CHECK((fieldType->is<IR::Type_Bits>() || fieldType->is<IR::Type_Newtype>() ||
                       fieldType->is<IR::Type_SerEnum>()),
                      "Header field %1% has a type which is not bit<>, "
                      "int<>, type, or serializable enum",
                      headerField);
            auto w = getTypeWidth(fieldType, typeMap);
            metadata->set_bitwidth(w);
            // We ignore the return type on purpose, but the call is required to update p4RtTypeInfo
            // if the header field has a user-defined type.
            TypeSpecConverter::convert(refMap, typeMap, fieldType, p4Info->mutable_type_info());
            auto type_name = getTypeName(fieldType, typeMap);
            if (type_name) {
                auto namedType = metadata->mutable_type_name();
                namedType->set_name(type_name);
            }
        }
    }

    void addTable(const IR::TableBlock *tableBlock) {
        CHECK_NULL(tableBlock);

        auto tableDeclaration = tableBlock->container;
        if (isHidden(tableDeclaration)) return;

        auto tableSize = Helpers::getTableSize(tableDeclaration);
        auto defaultAction = getDefaultAction(tableDeclaration, refMap, typeMap);
        auto matchFields =
            getMatchFields(tableDeclaration, refMap, typeMap, p4Info->mutable_type_info());
        auto actions = getActionRefs(tableDeclaration, refMap);

        bool isConstTable = getConstTable(tableDeclaration);
        bool hasInitialEntries = getHasInitialEntries(tableDeclaration);

        auto name = archHandler->getControlPlaneName(tableBlock);
        auto annotations = tableDeclaration->to<IR::IAnnotated>();

        auto table = p4Info->add_tables();
        setPreamble(table->mutable_preamble(),
                    symbols.getId(P4RuntimeSymbolType::P4RT_TABLE(), name), name,
                    symbols.getAlias(name), annotations,
                    [this](cstring anno) { return archHandler->filterAnnotations(anno); });
        table->set_size(tableSize);

        if (defaultAction && defaultAction->isConst) {
            auto id = symbols.getId(P4RuntimeSymbolType::P4RT_ACTION(), defaultAction->name);
            table->set_const_default_action_id(id);
        }

        for (const auto &action : actions) {
            auto id = symbols.getId(P4RuntimeSymbolType::P4RT_ACTION(), action.name);
            auto action_ref = table->add_action_refs();
            action_ref->set_id(id);
            addAnnotations(action_ref, action.annotations);
            // set action ref scope
            auto isTableOnly = (action.annotations->getAnnotation("tableonly") != nullptr);
            auto isDefaultOnly = (action.annotations->getAnnotation("defaultonly") != nullptr);
            if (isTableOnly && isDefaultOnly) {
                ::error(ErrorType::ERR_INVALID,
                        "Table '%1%' has an action reference ('%2%') which is annotated "
                        "with both '@tableonly' and '@defaultonly'",
                        name, action.name);
            }
            if (isTableOnly)
                action_ref->set_scope(p4configv1::ActionRef::TABLE_ONLY);
            else if (isDefaultOnly)
                action_ref->set_scope(p4configv1::ActionRef::DEFAULT_ONLY);
            else
                action_ref->set_scope(p4configv1::ActionRef::TABLE_AND_DEFAULT);
        }

        for (const auto &field : matchFields) {
            auto match_field = table->add_match_fields();
            match_field->set_id(field.id);
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
        if (hasInitialEntries) {
            table->set_has_initial_entries(true);
        }

        archHandler->addTableProperties(symbols, p4Info, table, tableBlock);
    }

    void addExtern(const IR::ExternBlock *externBlock) {
        CHECK_NULL(externBlock);
        archHandler->addExternInstance(symbols, p4Info, externBlock);
    }

    void analyzeControl(const IR::ControlBlock *controlBlock) {
        CHECK_NULL(controlBlock);

        auto control = controlBlock->container;
        CHECK_NULL(control);

        forAllMatching<IR::P4Action>(&control->controlLocals, [&](const IR::P4Action *action) {
            // Generate P4Info for the action and, implicitly, its parameters.
            addAction(action);

            // Generate P4Info for any extern functions it may invoke.
            forAllMatching<IR::MethodCallExpression>(
                action->body, [&](const IR::MethodCallExpression *call) {
                    auto instance = P4::MethodInstance::resolve(call, refMap, typeMap);
                    if (instance->is<P4::ExternFunction>()) {
                        archHandler->addExternFunction(symbols, p4Info,
                                                       instance->to<P4::ExternFunction>());
                    }
                });
        });

        // Generate P4Info for any extern function invoked directly from control.
        forAllMatching<IR::MethodCallExpression>(
            control->body, [&](const IR::MethodCallExpression *call) {
                auto instance = P4::MethodInstance::resolve(call, refMap, typeMap);
                if (instance->is<P4::ExternFunction>()) {
                    archHandler->addExternFunction(symbols, p4Info,
                                                   instance->to<P4::ExternFunction>());
                }
            });
    }

    void addValueSet(const IR::P4ValueSet *inst) {
        // guaranteed by caller
        CHECK_NULL(inst);

        auto vs = p4Info->add_value_sets();

        auto name = inst->controlPlaneName();

        unsigned int size = 0;
        auto sizeConstant = inst->size->to<IR::Constant>();
        if (sizeConstant == nullptr || !sizeConstant->fitsInt()) {
            ::error(ErrorType::ERR_INVALID, "@size should be an integer for declaration %1%", inst);
            return;
        }
        if (sizeConstant->value < 0) {
            ::error(ErrorType::ERR_INVALID,
                    "@size should be a positive integer for declaration %1%", inst);
            return;
        }
        size = static_cast<unsigned int>(sizeConstant->value);

        auto id = symbols.getId(P4RuntimeSymbolType::P4RT_VALUE_SET(), name);
        setPreamble(vs->mutable_preamble(), id, name, symbols.getAlias(name),
                    inst->to<IR::IAnnotated>(),
                    [this](cstring anno) { return archHandler->filterAnnotations(anno); });
        vs->set_size(size);

        /// Look for a @match annotation on the struct field and set the match
        /// type of the match field appropriately.
        auto setMatchType = [this](const IR::StructField *sf, p4configv1::MatchField *match) {
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
            if (matchType == std::nullopt) {
                ::error(ErrorType::ERR_UNSUPPORTED,
                        "unsupported match type %1% for Value Set '@match' annotation",
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
            auto *match = vs->add_match();
            match->set_id(1);
            match->set_bitwidth(et->width_bits());
            match->set_match_type(MatchField::MatchTypes::EXACT);
        } else if (et->is<IR::Type_Struct>()) {
            auto fields = et->to<IR::Type_Struct>()->fields;
            // Allocate ids for all match fields, taking into account
            // user-provided @id annotations if any.
            FieldIdAllocator<decltype(fields)::value_type> idAllocator(fields.begin(),
                                                                       fields.end());
            for (auto f : fields) {
                auto fType = f->type;
                if (!fType->is<IR::Type_Bits>()) {
                    ::error(ErrorType::ERR_UNSUPPORTED,
                            "Unsupported type argument for Value Set; "
                            "this version of P4Runtime requires that when the type parameter "
                            "of a Value Set is a struct, all the fields of the struct "
                            "must be of type bit<W>, but %1% is not",
                            f);
                    continue;
                }
                auto *match = vs->add_match();
                auto fieldId = idAllocator.getId(f);
                match->set_id(fieldId);
                match->set_name(f->controlPlaneName());
                match->set_bitwidth(fType->width_bits());
                setMatchType(f, match);
                // add annotations save for the @match one
                addAnnotations(
                    match, f, [](cstring name) { return name == IR::Annotation::matchAnnotation; });
                addDocumentation(match, f);
            }
        } else if (et->is<IR::Type_SerEnum>()) {
            auto serEnum = et->to<IR::Type_SerEnum>();
            auto fType = serEnum->type;
            if (!fType->is<IR::Type_Bits>()) {
                ::error(ErrorType::ERR_UNSUPPORTED,
                        "Unsupported type argument for Value Set; "
                        "this version of P4Runtime requires that when the type parameter "
                        "of a Value Set is a serializable enum, "
                        "it must be of type bit<W>, but %1% is not",
                        serEnum);
            }
            auto fields = serEnum->members;
            p4rt_id_t index = 1;
            for (auto f : fields) {
                auto *match = vs->add_match();
                match->set_id(index++);
                match->set_name(f->controlPlaneName());
                match->set_bitwidth(fType->width_bits());
                match->set_match_type(MatchField::MatchTypes::EXACT);
            }

        } else if (et->is<IR::Type_BaseList>()) {
            ::error(ErrorType::ERR_UNSUPPORTED,
                    "%1%: Unsupported type argument for Value Set; "
                    "this version of P4Runtime requires the type parameter of a Value Set "
                    "to be a bit<W>, a struct of bit<W> fields, or a serializable enum",
                    inst);
        } else {
            ::error(ErrorType::ERR_INVALID,
                    "%1%: invalid type parameter for Value Set; "
                    "it must be one of bit<W>, struct, tuple or serializable enum",
                    inst);
        }
    }

    /// To be called after all objects have been added to P4Info. Calls the
    /// architecture-specific postAdd method for post-processing.
    void postAdd() const { archHandler->postAdd(symbols, p4Info); }

    /// Sets the pkg_info field of the P4Info message, using the annotations on
    /// the P4 program package.
    void addPkgInfo(const IR::ToplevelBlock *evaluatedProgram, cstring arch) const {
        auto *main = evaluatedProgram->getMain();
        if (main == nullptr) {
            ::warning(ErrorType::WARN_MISSING,
                      "Program does not contain a main module, "
                      "so P4Info's 'pkg_info' field will not be set");
            return;
        }
        auto *decl = main->node->to<IR::Declaration_Instance>();
        CHECK_NULL(decl);
        auto *pkginfo = p4Info->mutable_pkg_info();

        pkginfo->set_arch(arch);

        std::set<cstring> keysVisited;

        // @pkginfo annotation
        for (auto *annotation : decl->getAnnotations()->annotations) {
            if (annotation->name != IR::Annotation::pkginfoAnnotation) continue;
            for (auto *kv : annotation->kv) {
                auto name = kv->name.name;
                auto setStringField = [kv, pkginfo, &keysVisited](cstring fName) {
                    auto *v = kv->expression->to<IR::StringLiteral>();
                    if (v == nullptr) {
                        ::error(ErrorType::ERR_UNSUPPORTED,
                                "Value for '%1%' key in @pkginfo annotation is not a string", kv);
                        return;
                    }
                    // kv annotations are represented with an IndexedVector in
                    // the IR, so a program with duplicate keys is actually
                    // rejected at parse time.
                    BUG_CHECK(!keysVisited.count(fName), "Duplicate keys in @pkginfo");
                    keysVisited.insert(fName);
                    // use Protobuf reflection library to minimize code
                    // duplication.
                    auto *descriptor = pkginfo->GetDescriptor();
                    auto *f = descriptor->FindFieldByName(static_cast<std::string>(fName));
                    pkginfo->GetReflection()->SetString(pkginfo, f,
                                                        static_cast<std::string>(v->value));
                };
                if (name == "name" || name == "version" || name == "organization" ||
                    name == "contact" || name == "url") {
                    setStringField(name);
                } else if (name == "arch") {
                    ::warning(ErrorType::WARN_INVALID,
                              "The '%1%' field in PkgInfo should be set by the compiler, "
                              "not by the user",
                              kv);
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
    P4Info *p4Info;
    /// The symbols used in the API and their ids.
    const P4RuntimeSymbolTable &symbols;
    /// The actions we've serialized so far. Used for deduplication.
    std::set<p4rt_id_t> serializedActions;
    /// Type information for the P4 program we're serializing.
    TypeMap *typeMap;
    ReferenceMap *refMap;
    P4RuntimeArchHandlerIface *archHandler;
};

static void analyzeParser(P4RuntimeAnalyzer &analyzer, const IR::ParserBlock *parserBlock) {
    CHECK_NULL(parserBlock);

    auto parser = parserBlock->container;
    CHECK_NULL(parser);

    for (auto s : parser->parserLocals) {
        if (auto inst = s->to<IR::P4ValueSet>()) {
            analyzer.addValueSet(inst);
        }
    }
}

/// A converter which translates the 'entries' or 'const entries' for
/// P4 tables (if any) into a P4Runtime WriteRequest message which can
/// be used by a target to initialize its tables.
class P4RuntimeEntriesConverter {
 private:
    friend class P4RuntimeAnalyzer;

    explicit P4RuntimeEntriesConverter(const P4RuntimeSymbolTable &symbols)
        : entries(new p4v1::WriteRequest), symbols(symbols) {}

    /// @return the P4Runtime WriteRequest message generated by this analyzer.
    const p4v1::WriteRequest *getEntries() const {
        BUG_CHECK(entries != nullptr, "Didn't produce a P4Runtime WriteRequest object?");
        return entries;
    }

    /// Appends the 'const entries' for the table to the WriteRequest message.
    void addTableEntries(const IR::TableBlock *tableBlock, ReferenceMap *refMap, TypeMap *typeMap,
                         P4RuntimeArchHandlerIface *archHandler) {
        CHECK_NULL(tableBlock);
        auto table = tableBlock->container;

        auto entriesList = table->getEntries();
        if (entriesList == nullptr) return;

        bool isConst = getConstTable(table);
        auto tableName = archHandler->getControlPlaneName(tableBlock);
        auto tableId = symbols.getId(P4RuntimeSymbolType::P4RT_TABLE(), tableName);

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
            protoEntry->set_is_const(isConst || e->isConst);
            if (needsPriority) {
                if (!isConst) {
                    // The entry has a priority, use it.
                    CHECK_NULL(e->priority);
                    if (auto c = e->priority->to<IR::Constant>()) {
                        protoEntry->set_priority(c->asInt());
                    } else {
                        ::error(ErrorType::ERR_EXPECTED, "%1%: entry should have priority", e);
                        return;
                    }
                } else {
                    // According to the P4 specification, "Entries in a table are
                    // matched in the program order, stopping at the first matching
                    // entry." In P4Runtime, the lowest valid priority value is 1 and
                    // entries with a higher numerical priority value have higher
                    // priority. So we assign the first entry a priority of #entries and
                    // we decrement the priority by 1 for each entry. The last entry in
                    // the table will have priority 1.
                    protoEntry->set_priority(entryPriority--);
                }
            }

            auto priorityAnnotation = e->getAnnotation("priority");
            if (priorityAnnotation != nullptr) {
                ::warning(ErrorType::WARN_DEPRECATED,
                          "The @priority annotation on %1% is not part of the P4 specification, "
                          "nor of the P4Runtime specification, and will be ignored",
                          e);
            }
        }
    }

    /// Checks if the @table entries need to be assigned a priority, i.e. does
    /// the match key for the table includes a ternary, range, or optional match?
    bool tableNeedsPriority(const IR::P4Table *table, ReferenceMap *refMap) const {
        for (auto e : table->getKey()->keyElements) {
            auto matchType = getKeyMatchType(e, refMap);
            // TODO(antonin): remove dependency on v1model.
            if (matchType == P4CoreLibrary::instance().ternaryMatch.name ||
                matchType == P4V1::V1Model::instance.rangeMatchType.name ||
                matchType == P4V1::V1Model::instance.optionalMatchType.name) {
                return true;
            }
        }
        return false;
    }

    void addAction(p4v1::TableEntry *protoEntry, const IR::Expression *actionRef,
                   ReferenceMap *refMap, TypeMap *typeMap) const {
        if (!actionRef->is<IR::MethodCallExpression>()) {
            ::error(ErrorType::ERR_INVALID, "%1%: invalid action in entries list", actionRef);
            return;
        }
        auto actionCall = actionRef->to<IR::MethodCallExpression>();
        auto method = actionCall->method->to<IR::PathExpression>()->path;
        auto decl = refMap->getDeclaration(method, true);
        auto actionDecl = decl->to<IR::P4Action>();
        auto actionName = actionDecl->controlPlaneName();
        auto actionId = symbols.getId(P4RuntimeSymbolType::P4RT_ACTION(), actionName);

        auto protoAction = protoEntry->mutable_action()->mutable_action();
        protoAction->set_action_id(actionId);
        int parameterIndex = 0;
        int parameterId = 1;
        for (auto arg : *actionCall->arguments) {
            auto protoParam = protoAction->add_params();
            protoParam->set_param_id(parameterId++);
            auto parameter = actionDecl->parameters->parameters.at(parameterIndex++);
            int width = getTypeWidth(parameter->type, typeMap);
            auto ei = EnumInstance::resolve(arg->expression, typeMap);
            if (arg->expression->is<IR::Constant>()) {
                auto value = stringRepr(arg->expression->to<IR::Constant>(), width);
                protoParam->set_value(*value);
            } else if (arg->expression->is<IR::BoolLiteral>()) {
                auto value = stringRepr(arg->expression->to<IR::BoolLiteral>(), width);
                protoParam->set_value(*value);
            } else if (ei != nullptr && ei->is<SerEnumInstance>()) {
                auto sei = ei->to<SerEnumInstance>();
                auto value = stringRepr(sei->value->to<IR::Constant>(), width);
                protoParam->set_value(*value);
            } else {
                ::error(ErrorType::ERR_UNSUPPORTED, "%1% unsupported argument expression",
                        arg->expression);
                continue;
            }
        }
    }

    void addMatchKey(p4v1::TableEntry *protoEntry, const IR::P4Table *table,
                     const IR::ListExpression *keyset, ReferenceMap *refMap,
                     TypeMap *typeMap) const {
        int keyIndex = 0;
        int fieldId = 1;
        for (auto k : keyset->components) {
            auto tableKey = table->getKey()->keyElements.at(keyIndex++);
            auto keyWidth = getTypeWidth(tableKey->expression->type, typeMap);
            auto matchType = getKeyMatchType(tableKey, refMap);

            if (matchType == P4CoreLibrary::instance().exactMatch.name) {
                addExact(protoEntry, fieldId++, k, keyWidth, typeMap);
            } else if (matchType == P4CoreLibrary::instance().lpmMatch.name) {
                addLpm(protoEntry, fieldId++, k, keyWidth, typeMap);
            } else if (matchType == P4CoreLibrary::instance().ternaryMatch.name) {
                addTernary(protoEntry, fieldId++, k, keyWidth, typeMap);
            } else if (matchType == P4V1::V1Model::instance.rangeMatchType.name) {
                addRange(protoEntry, fieldId++, k, keyWidth, typeMap);
            } else if (matchType == P4V1::V1Model::instance.optionalMatchType.name) {
                addOptional(protoEntry, fieldId++, k, keyWidth, typeMap);
            } else {
                if (!k->is<IR::DefaultExpression>())
                    ::error(ErrorType::ERR_UNSUPPORTED,
                            "%1%: match type not supported by P4Runtime serializer", matchType);
                continue;
            }
        }
    }

    /// Convert a key expression to the P4Runtime bytes representation if the
    /// expression is simple (integer literal or boolean literal) or returns
    /// std::nullopt otherwise.
    std::optional<std::string> convertSimpleKeyExpression(const IR::Expression *k, int keyWidth,
                                                          TypeMap *typeMap) const {
        if (k->is<IR::Constant>()) {
            return stringRepr(k->to<IR::Constant>(), keyWidth);
        } else if (k->is<IR::BoolLiteral>()) {
            return stringRepr(k->to<IR::BoolLiteral>(), keyWidth);
        } else if (k->is<IR::Member>()) {  // handle SerEnum members
            auto mem = k->to<IR::Member>();
            auto se = mem->type->to<IR::Type_SerEnum>();
            auto ei = EnumInstance::resolve(mem, typeMap);
            if (!ei) return std::nullopt;
            if (auto sei = ei->to<SerEnumInstance>()) {
                auto type = sei->value->to<IR::Constant>();
                auto w = se->type->width_bits();
                BUG_CHECK(w == keyWidth, "SerEnum bitwidth mismatch");
                return stringRepr(type, w);
            }
            ::error(ErrorType::ERR_INVALID, "%1% invalid Member key expression", k);
            return std::nullopt;
        } else if (k->is<IR::Cast>()) {
            return convertSimpleKeyExpression(k->to<IR::Cast>()->expr, keyWidth, typeMap);
        } else {
            ::error(ErrorType::ERR_INVALID, "%1% invalid key expression", k);
            return std::nullopt;
        }
    }

    /// Convert a key expression to the big_int integer value if the
    /// expression is simple (integer literal or boolean literal) or returns
    /// std::nullopt otherwise.
    std::optional<big_int> simpleKeyExpressionValue(const IR::Expression *k,
                                                    TypeMap *typeMap) const {
        if (k->is<IR::Constant>()) {
            return k->to<IR::Constant>()->value;
        } else if (k->is<IR::BoolLiteral>()) {
            return static_cast<big_int>(k->to<IR::BoolLiteral>()->value ? 1 : 0);
        } else if (k->is<IR::Member>()) {  // handle SerEnum members
            auto mem = k->to<IR::Member>();
            auto ei = EnumInstance::resolve(mem, typeMap);
            if (!ei) return std::nullopt;
            if (auto sei = ei->to<SerEnumInstance>()) {
                return simpleKeyExpressionValue(sei->value, typeMap);
            }
            ::error(ErrorType::ERR_INVALID, "%1% invalid Member key expression", k);
            return std::nullopt;
        } else if (k->is<IR::Cast>()) {
            return simpleKeyExpressionValue(k->to<IR::Cast>()->expr, typeMap);
        } else {
            ::error(ErrorType::ERR_INVALID, "%1% invalid key expression", k);
            return std::nullopt;
        }
    }

    void addExact(p4v1::TableEntry *protoEntry, int fieldId, const IR::Expression *k, int keyWidth,
                  TypeMap *typeMap) const {
        auto protoMatch = protoEntry->add_match();
        protoMatch->set_field_id(fieldId);
        auto protoExact = protoMatch->mutable_exact();
        auto value = convertSimpleKeyExpression(k, keyWidth, typeMap);
        if (value == std::nullopt) return;
        protoExact->set_value(*value);
    }

    void addLpm(p4v1::TableEntry *protoEntry, int fieldId, const IR::Expression *k, int keyWidth,
                TypeMap *typeMap) const {
        if (k->is<IR::DefaultExpression>())  // don't care, skip in P4Runtime message
            return;
        int prefixLen;
        std::optional<std::string> valueStr;
        if (k->is<IR::Mask>()) {
            auto km = k->to<IR::Mask>();
            auto value = simpleKeyExpressionValue(km->left, typeMap);
            if (value == std::nullopt) return;
            auto trailing_zeros = [keyWidth](const big_int &n) -> int {
                return (n == 0) ? keyWidth : boost::multiprecision::lsb(n);
            };
            auto count_ones = [](const big_int &n) -> int { return bitcount(n); };
            auto mask = km->right->to<IR::Constant>()->value;
            auto len = trailing_zeros(mask);
            if (len + count_ones(mask) != keyWidth) {  // any remaining 0s in the prefix?
                ::error(ErrorType::ERR_INVALID, "%1% invalid mask for LPM key", k);
                return;
            }
            if ((*value & mask) != *value) {
                ::warning(ErrorType::WARN_MISMATCH,
                          "P4Runtime requires that LPM matches have masked-off bits set to 0, "
                          "updating value %1% to conform to the P4Runtime specification",
                          km->left);
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
        if (valueStr == std::nullopt) return;
        auto protoMatch = protoEntry->add_match();
        protoMatch->set_field_id(fieldId);
        auto protoLpm = protoMatch->mutable_lpm();
        protoLpm->set_value(*valueStr);
        protoLpm->set_prefix_len(prefixLen);
    }

    void addTernary(p4v1::TableEntry *protoEntry, int fieldId, const IR::Expression *k,
                    int keyWidth, TypeMap *typeMap) const {
        if (k->is<IR::DefaultExpression>())  // don't care, skip in P4Runtime message
            return;
        std::optional<std::string> valueStr;
        std::optional<std::string> maskStr;
        if (k->is<IR::Mask>()) {
            auto km = k->to<IR::Mask>();
            auto value = simpleKeyExpressionValue(km->left, typeMap);
            auto mask = simpleKeyExpressionValue(km->right, typeMap);
            if (value == std::nullopt || mask == std::nullopt) return;
            if ((*value & *mask) != *value) {
                ::warning(ErrorType::WARN_MISMATCH,
                          "P4Runtime requires that Ternary matches have masked-off bits set to 0, "
                          "updating value %1% to conform to the P4Runtime specification",
                          km->left);
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
        if (valueStr == std::nullopt || maskStr == std::nullopt) return;
        auto protoMatch = protoEntry->add_match();
        protoMatch->set_field_id(fieldId);
        auto protoTernary = protoMatch->mutable_ternary();
        protoTernary->set_value(*valueStr);
        protoTernary->set_mask(*maskStr);
    }

    void addRange(p4v1::TableEntry *protoEntry, int fieldId, const IR::Expression *k, int keyWidth,
                  TypeMap *typeMap) const {
        if (k->is<IR::DefaultExpression>())  // don't care, skip in P4Runtime message
            return;
        std::optional<std::string> startStr;
        std::optional<std::string> endStr;
        if (k->is<IR::Range>()) {
            auto kr = k->to<IR::Range>();
            auto start = simpleKeyExpressionValue(kr->left, typeMap);
            auto end = simpleKeyExpressionValue(kr->right, typeMap);
            if (start == std::nullopt || end == std::nullopt) return;
            // Error on invalid range values
            big_int maxValue = (big_int(1) << keyWidth) - 1;
            // NOTE: If end value is > max allowed for keyWidth, value gets
            // wrapped around. A warning is issued in this case by the frontend
            // earlier.
            // For e.g. 16 bit key has a max value of 65535, Range of (1..65536)
            // will be converted to (1..0) and will fail below check.
            if (*start > *end)
                ::error(ErrorType::ERR_INVALID, "%s Invalid range for table entry", kr->srcInfo);
            if (*start == 0 && *end == maxValue)  // don't care
                return;
            startStr = stringReprConstant(*start, keyWidth);
            endStr = stringReprConstant(*end, keyWidth);
        } else {
            startStr = convertSimpleKeyExpression(k, keyWidth, typeMap);
            endStr = startStr;
        }
        if (startStr == std::nullopt || endStr == std::nullopt) return;
        auto protoMatch = protoEntry->add_match();
        protoMatch->set_field_id(fieldId);
        auto protoRange = protoMatch->mutable_range();
        protoRange->set_low(*startStr);
        protoRange->set_high(*endStr);
    }

    void addOptional(p4v1::TableEntry *protoEntry, int fieldId, const IR::Expression *k,
                     int keyWidth, TypeMap *typeMap) const {
        if (k->is<IR::DefaultExpression>())  // don't care, skip in P4Runtime message
            return;
        auto protoMatch = protoEntry->add_match();
        protoMatch->set_field_id(fieldId);
        auto protoOptional = protoMatch->mutable_optional();
        auto value = convertSimpleKeyExpression(k, keyWidth, typeMap);
        if (value == std::nullopt) return;
        protoOptional->set_value(*value);
    }

    cstring getKeyMatchType(const IR::KeyElement *ke, ReferenceMap *refMap) const {
        auto path = ke->matchType->path;
        auto mt = refMap->getDeclaration(path, true)->to<IR::Declaration_ID>();
        BUG_CHECK(mt != nullptr, "%1%: could not find declaration", ke->matchType);
        return mt->name.name;
    }

    /// We represent all static table entries as one P4Runtime WriteRequest object
    p4v1::WriteRequest *entries;
    /// The symbols used in the API and their ids.
    const P4RuntimeSymbolTable &symbols;
};

/* static */ P4RuntimeAPI P4RuntimeAnalyzer::analyze(const IR::P4Program *program,
                                                     const IR::ToplevelBlock *evaluatedProgram,
                                                     ReferenceMap *refMap, TypeMap *typeMap,
                                                     P4RuntimeArchHandlerIface *archHandler,
                                                     cstring arch) {
    using namespace ControlPlaneAPI;

    CHECK_NULL(archHandler);

    // Perform a first pass to collect all of the control plane visible symbols in
    // the program.
    const auto *symbols = P4RuntimeSymbolTable::generateSymbols(program, evaluatedProgram, refMap,
                                                                typeMap, archHandler);

    archHandler->postCollect(*symbols);

    // Construct a P4Runtime control plane API from the program.
    P4RuntimeAnalyzer analyzer(*symbols, typeMap, refMap, archHandler);
    Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block *block) {
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
    forAllMatching<IR::Type_Header>(program, [&](const IR::Type_Header *type) {
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

    P4RuntimeEntriesConverter entriesConverter(*symbols);
    Helpers::forAllEvaluatedBlocks(evaluatedProgram, [&](const IR::Block *block) {
        if (block->is<IR::TableBlock>())
            entriesConverter.addTableEntries(block->to<IR::TableBlock>(), refMap, typeMap,
                                             archHandler);
        else if (block->is<IR::ExternBlock>()) {
            // add entries for arch specific extern types
            archHandler->addExternEntries(entriesConverter.getEntries(), *symbols,
                                          block->to<IR::ExternBlock>());
        }
    });

    auto *p4Info = analyzer.getP4Info();
    auto *p4Entries = entriesConverter.getEntries();
    return P4RuntimeAPI{p4Info, p4Entries};
}

}  // namespace ControlPlaneAPI

P4RuntimeAPI P4RuntimeSerializer::generateP4Runtime(const IR::P4Program *program, cstring arch) {
    using namespace ControlPlaneAPI;

    auto archHandlerBuilderIt = archHandlerBuilders.find(arch);
    if (archHandlerBuilderIt == archHandlerBuilders.end()) {
        ::error(ErrorType::ERR_UNSUPPORTED, "Arch '%1%' not supported by P4Runtime serializer",
                arch);
        return P4RuntimeAPI{new p4configv1::P4Info(), new p4v1::WriteRequest()};
    }

    // Generate a new version of the program that satisfies the prerequisites of
    // the P4Runtime analysis code.
    P4::ReferenceMap refMap;
    refMap.setIsV1(true);
    P4::TypeMap typeMap;
    auto *evaluator = new P4::EvaluatorPass(&refMap, &typeMap);
    PassManager p4RuntimeFixups = {
        new ControlPlaneAPI::ParseP4RuntimeAnnotations(),
        // Update types and reevaluate the program.
        new P4::TypeChecking(&refMap, &typeMap, /* updateExpressions = */ true), evaluator};
    auto *p4RuntimeProgram = program->apply(p4RuntimeFixups);
    auto *evaluatedProgram = evaluator->getToplevelBlock();

    if (!p4RuntimeProgram || !evaluatedProgram) {
        ::error(ErrorType::ERR_UNSUPPORTED,
                "%1%: unsupported P4 program (cannot apply necessary program transformations)",
                "Cannot generate P4Info message");
        return P4RuntimeAPI{new p4configv1::P4Info(), new p4v1::WriteRequest()};
    }

    auto archHandler = (*archHandlerBuilderIt->second)(&refMap, &typeMap, evaluatedProgram);

    return P4RuntimeAnalyzer::analyze(p4RuntimeProgram, evaluatedProgram, &refMap, &typeMap,
                                      archHandler, arch);
}

void P4RuntimeAPI::serializeP4InfoTo(std::ostream *destination, P4RuntimeFormat format) const {
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
    if (!success) ::error(ErrorType::ERR_IO, "Failed to serialize the P4Runtime API to the output");
}

void P4RuntimeAPI::serializeEntriesTo(std::ostream *destination, P4RuntimeFormat format) const {
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
        ::error(ErrorType::ERR_IO,
                "Failed to serialize the P4Runtime static table entries to the output");
}

static bool parseFileNames(cstring fileNameVector, std::vector<cstring> &files,
                           std::vector<P4::P4RuntimeFormat> &formats) {
    for (auto current = fileNameVector; current;) {
        cstring name = current;
        const char *comma = current.find(',');
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
                ::error(ErrorType::ERR_UNKNOWN,
                        "%1%: Could not detect p4runtime info file format from file suffix %2%",
                        name, suffix);
                return false;
            }
        } else {
            ::error(ErrorType::ERR_UNKNOWN,
                    "%1%: unknown file kind; known suffixes are .bin, .txt, .json", name);
            return false;
        }
    }
    return true;
}

void P4RuntimeSerializer::serializeP4RuntimeIfRequired(const IR::P4Program *program,
                                                       const CompilerOptions &options) {
    std::vector<cstring> files;
    std::vector<P4::P4RuntimeFormat> formats;

    // only generate P4Info is required by use-provided options
    if (options.p4RuntimeFile.isNullOrEmpty() && options.p4RuntimeFiles.isNullOrEmpty() &&
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

void P4RuntimeSerializer::serializeP4RuntimeIfRequired(const P4RuntimeAPI &p4Runtime,
                                                       const CompilerOptions &options) {
    std::vector<cstring> files;
    std::vector<P4::P4RuntimeFormat> formats;

    if (!options.p4RuntimeFile.isNullOrEmpty()) {
        files.push_back(options.p4RuntimeFile);
        formats.push_back(options.p4RuntimeFormat);
    }
    if (!parseFileNames(options.p4RuntimeFiles, files, formats)) return;

    if (!files.empty()) {
        for (unsigned i = 0; i < files.size(); i++) {
            cstring file = files.at(i);
            P4::P4RuntimeFormat format = formats.at(i);
            std::ostream *out = openFile(file, false);
            if (!out) {
                ::error(ErrorType::ERR_IO, "Couldn't open P4Runtime API file: %1%", file);
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

    if (!parseFileNames(options.p4RuntimeEntriesFiles, files, formats)) return;
    if (!files.empty()) {
        for (unsigned i = 0; i < files.size(); i++) {
            cstring file = files.at(i);
            P4::P4RuntimeFormat format = formats.at(i);
            std::ostream *out = openFile(file, false);
            if (!out) {
                ::error(ErrorType::ERR_IO, "Couldn't open P4Runtime static entries file: %1%",
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
    registerArch("pna", new ControlPlaneAPI::Standard::PNAArchHandlerBuilder());
    registerArch("ubpf", new ControlPlaneAPI::Standard::UBPFArchHandlerBuilder());
}

P4RuntimeSerializer *P4RuntimeSerializer::get() {
    static P4RuntimeSerializer instance;
    return &instance;
}

cstring P4RuntimeSerializer::resolveArch(const CompilerOptions &options) {
    if (auto arch = getenv("P4C_DEFAULT_ARCH")) {
        return cstring(arch);
    } else if (options.arch != nullptr) {
        return options.arch;
    } else {
        return "v1model";
    }
}

void P4RuntimeSerializer::registerArch(
    const cstring archName, const ControlPlaneAPI::P4RuntimeArchHandlerBuilderIface *builder) {
    archHandlerBuilders[archName] = builder;
}

P4RuntimeAPI generateP4Runtime(const IR::P4Program *program, cstring arch) {
    return P4RuntimeSerializer::get()->generateP4Runtime(program, arch);
}

void serializeP4RuntimeIfRequired(const IR::P4Program *program, const CompilerOptions &options) {
    P4RuntimeSerializer::get()->serializeP4RuntimeIfRequired(program, options);
}

/** @} */ /* end group control_plane */
}  // namespace P4
