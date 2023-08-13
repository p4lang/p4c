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

#ifndef BACKENDS_BMV2_COMMON_HELPERS_H_
#define BACKENDS_BMV2_COMMON_HELPERS_H_

#include <stddef.h>

#include <algorithm>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "JsonObjects.h"
#include "controlFlowGraph.h"
#include "expression.h"
#include "frontends/common/model.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "lib/big_int_util.h"
#include "lib/cstring.h"
#include "lib/json.h"
#include "programStructure.h"

namespace BMV2 {

#ifndef UNUSED
#define UNUSED __attribute__((__unused__))
#endif

class MatchImplementation {
 public:
    static const cstring selectorMatchTypeName;
    static const cstring rangeMatchTypeName;
    static const cstring optionalMatchTypeName;
};

class TableAttributes {
 public:
    static const unsigned defaultTableSize;
};

class V1ModelProperties {
 public:
    static const cstring jsonMetadataParameterName;

    /// The name of BMV2's valid field. This is a hidden bit<1> field
    /// automatically added by BMV2 to all header types; reading from it tells
    /// you whether the header is valid, just as if you had called isValid().
    static const cstring validField;
};

namespace Standard {

/// We re-use as much code as possible between PSA and v1model. The two
/// architectures have some differences though, in particular regarding naming
/// (of table properties, extern types, parameter names). We define some
/// "traits" for each extern type, templatized by the architecture name (using
/// the Arch enum class defined below), as a convenient way to access
/// architecture-specific names in the unified code.
/// The V1MODEL2020 is the modified v1model.p4 file with a version
/// >= 20200408
enum class Arch { V1MODEL, PSA, V1MODEL2020 };

/// Traits for the action profile extern, must be specialized for v1model and
/// PSA.
template <Arch arch>
struct ActionProfileTraits;

template <>
struct ActionProfileTraits<Arch::V1MODEL> {
    static const cstring name() { return "action profile"; }
    static const cstring propertyName() {
        return P4V1::V1Model::instance.tableAttributes.tableImplementation.name;
    }
    static const cstring typeName() { return P4V1::V1Model::instance.action_profile.name; }
    static const cstring sizeParamName() { return "size"; }
};

template <>
struct ActionProfileTraits<Arch::V1MODEL2020> : public ActionProfileTraits<Arch::V1MODEL> {};

template <>
struct ActionProfileTraits<Arch::PSA> {
    static const cstring name() { return "action profile"; }
    static const cstring propertyName() { return "implementation"; }
    static const cstring typeName() { return "ActionProfile"; }
    static const cstring sizeParamName() { return "size"; }
};

/// Traits for the action selector extern, must be specialized for v1model and
/// PSA. Inherits from ActionProfileTraits because of their similarities.
template <Arch arch>
struct ActionSelectorTraits;

template <>
struct ActionSelectorTraits<Arch::V1MODEL> : public ActionProfileTraits<Arch::V1MODEL> {
    static const cstring name() { return "action selector"; }
    static const cstring typeName() { return P4V1::V1Model::instance.action_selector.name; }
};

template <>
struct ActionSelectorTraits<Arch::V1MODEL2020> : public ActionProfileTraits<Arch::V1MODEL2020> {};

template <>
struct ActionSelectorTraits<Arch::PSA> : public ActionProfileTraits<Arch::PSA> {
    static const cstring name() { return "action selector"; }
    static const cstring typeName() { return "ActionSelector"; }
};

/// Traits for the register extern, must be specialized for v1model and PSA.
template <Arch arch>
struct RegisterTraits;

template <>
struct RegisterTraits<Arch::V1MODEL> {
    static const cstring name() { return "register"; }
    static const cstring typeName() { return P4V1::V1Model::instance.registers.name; }
    static const cstring sizeParamName() { return "size"; }
    // the index of the type parameter for the data stored in the register, in
    // the type parameter list of the extern type declaration
    static size_t dataTypeParamIdx() { return 0; }
    static std::optional<size_t> indexTypeParamIdx() { return std::nullopt; }
};

template <>
struct RegisterTraits<Arch::V1MODEL2020> : public RegisterTraits<Arch::V1MODEL> {
    static std::optional<size_t> indexTypeParamIdx() { return 1; }
};

template <>
struct RegisterTraits<Arch::PSA> {
    static const cstring name() { return "register"; }
    static const cstring typeName() { return "Register"; }
    static const cstring sizeParamName() { return "size"; }
    static size_t dataTypeParamIdx() { return 0; }
    // the index of the type parameter for the register index, in the type
    // parameter list of the extern type declaration.
    static std::optional<size_t> indexTypeParamIdx() { return 1; }
};

template <Arch arch>
struct CounterExtern {};
template <Arch arch>
struct MeterExtern {};

}  // namespace Standard

namespace Helpers {

template <typename Kind>
struct CounterlikeTraits;

// According to the C++11 standard: An explicit specialization shall be declared
// in a namespace enclosing the specialized template. An explicit specialization
// whose declarator-id is not qualified shall be declared in the nearest
// enclosing namespace of the template, or, if the namespace is inline (7.3.1),
// any namespace from its enclosing namespace set. Such a declaration may also
// be a definition. If the declaration is not a definition, the specialization
// may be defined later (7.3.1.2).
//
// gcc reports an error when trying so specialize CounterlikeTraits<> for
// Standard::CounterExtern & Standard::MeterExtern outside of the Helpers
// namespace, even when qualifying CounterlikeTraits<> with Helpers::. It seems
// to be related to this bug:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56480.

/// @ref CounterlikeTraits<> specialization for @ref CounterExtern for v1model
template <>
struct CounterlikeTraits<Standard::CounterExtern<Standard::Arch::V1MODEL>> {
    static const cstring name() { return "counter"; }
    static const cstring directPropertyName() {
        return P4V1::V1Model::instance.tableAttributes.counters.name;
    }
    static const cstring typeName() { return P4V1::V1Model::instance.counter.name; }
    static const cstring directTypeName() { return P4V1::V1Model::instance.directCounter.name; }
    static const cstring sizeParamName() { return "size"; }
    static std::optional<size_t> indexTypeParamIdx() { return std::nullopt; }
};

template <>
struct CounterlikeTraits<Standard::CounterExtern<Standard::Arch::V1MODEL2020>> {
    static const cstring name() { return "counter"; }
    static const cstring directPropertyName() {
        return P4V1::V1Model::instance.tableAttributes.counters.name;
    }
    static const cstring typeName() { return P4V1::V1Model::instance.counter.name; }
    static const cstring directTypeName() { return P4V1::V1Model::instance.directCounter.name; }
    static const cstring sizeParamName() { return "size"; }
    static std::optional<size_t> indexTypeParamIdx() { return 0; }
};

/// @ref CounterlikeTraits<> specialization for @ref CounterExtern for PSA
template <>
struct CounterlikeTraits<Standard::CounterExtern<Standard::Arch::PSA>> {
    static const cstring name() { return "counter"; }
    static const cstring directPropertyName() { return "psa_direct_counter"; }
    static const cstring typeName() { return "Counter"; }
    static const cstring directTypeName() { return "DirectCounter"; }
    static const cstring sizeParamName() { return "n_counters"; }
    // the index of the type parameter for the counter index, in the type
    // parameter list of the extern type declaration.
    static std::optional<size_t> indexTypeParamIdx() { return 1; }
};

/// @ref CounterlikeTraits<> specialization for @ref MeterExtern for v1model
template <>
struct CounterlikeTraits<Standard::MeterExtern<Standard::Arch::V1MODEL>> {
    static const cstring name() { return "meter"; }
    static const cstring directPropertyName() {
        return P4V1::V1Model::instance.tableAttributes.meters.name;
    }
    static const cstring typeName() { return P4V1::V1Model::instance.meter.name; }
    static const cstring directTypeName() { return P4V1::V1Model::instance.directMeter.name; }
    static const cstring sizeParamName() { return "size"; }
    static std::optional<size_t> indexTypeParamIdx() { return std::nullopt; }
};

template <>
struct CounterlikeTraits<Standard::MeterExtern<Standard::Arch::V1MODEL2020>> {
    static const cstring name() { return "meter"; }
    static const cstring directPropertyName() {
        return P4V1::V1Model::instance.tableAttributes.meters.name;
    }
    static const cstring typeName() { return P4V1::V1Model::instance.meter.name; }
    static const cstring directTypeName() { return P4V1::V1Model::instance.directMeter.name; }
    static const cstring sizeParamName() { return "size"; }
    static std::optional<size_t> indexTypeParamIdx() { return 0; }
};

/// @ref CounterlikeTraits<> specialization for @ref MeterExtern for PSA
template <>
struct CounterlikeTraits<Standard::MeterExtern<Standard::Arch::PSA>> {
    static const cstring name() { return "meter"; }
    static const cstring directPropertyName() { return "psa_direct_meter"; }
    static const cstring typeName() { return "Meter"; }
    static const cstring directTypeName() { return "DirectMeter"; }
    static const cstring sizeParamName() { return "n_meters"; }
    // the index of the type parameter for the meter index, in the type
    // parameter list of the extern type declaration.
    static std::optional<size_t> indexTypeParamIdx() { return 0; }
};

}  // namespace Helpers

using BlockTypeMap = std::map<const IR::Block *, const IR::Type *>;

// XXX(hanw): This convenience class stores pointers to the data structures
// that are commonly used during the program translation. Due to the limitation
// of current IR structure, these data structure are only refreshed by the
// evaluator pass. In the long term, integrating these data structures as part
// of the IR tree would simplify this kind of bookkeeping effort.
using SelectorInput = std::vector<const IR::Expression *>;

struct ConversionContext {
    // context
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    const IR::ToplevelBlock *toplevel;
    // Block currently being converted
    BlockConverted blockConverted;
    //
    ProgramStructure *structure;
    // expression converter is used in many places.
    ExpressionConverter *conv;
    // final json output.
    BMV2::JsonObjects *json;

    // for action profile conversion
    Util::JsonArray *action_profiles = nullptr;

    std::map<const IR::Declaration_Instance *, SelectorInput> selector_input_map;

    const SelectorInput *get_selector_input(const IR::Declaration_Instance *selector) {
        auto it = selector_input_map.find(selector);
        if (it == selector_input_map.end()) return nullptr;  // selector never used
        return &it->second;
    }

    ConversionContext(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                      const IR::ToplevelBlock *toplevel, ProgramStructure *structure,
                      ExpressionConverter *conv, JsonObjects *json)
        : refMap(refMap),
          typeMap(typeMap),
          toplevel(toplevel),
          blockConverted(BlockConverted::None),
          structure(structure),
          conv(conv),
          json(json) {}

    void addToFieldList(const IR::Expression *expr, Util::JsonArray *fl);
    int createFieldList(const IR::Expression *expr, cstring listName, bool learn = false);
    cstring createCalculation(cstring algo, const IR::Expression *fields,
                              Util::JsonArray *calculations, bool usePayload, const IR::Node *node);
    static void modelError(const char *format, const IR::Node *place);
};

Util::IJson *nodeName(const CFG::Node *node);
Util::JsonArray *mkArrayField(Util::JsonObject *parent, cstring name);
Util::JsonArray *mkParameters(Util::JsonObject *object);
Util::JsonObject *mkPrimitive(cstring name, Util::JsonArray *appendTo);
Util::JsonObject *mkPrimitive(cstring name);
cstring stringRepr(big_int value, unsigned bytes = 0);
unsigned nextId(cstring group);
/// Converts expr into a ListExpression or returns nullptr if not possible
const IR::ListExpression *convertToList(const IR::Expression *expr, P4::TypeMap *typeMap);

}  // namespace BMV2

#endif /* BACKENDS_BMV2_COMMON_HELPERS_H_ */
