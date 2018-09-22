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

#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/json.h"
#include "lib/ordered_map.h"
#include "JsonObjects.h"
#include "controlFlowGraph.h"
#include "expression.h"
#include "frontends/common/model.h"
#include "programStructure.h"

namespace BMV2 {

// forward declaration to avoid circular-dependency
class SharedActionSelectorCheck;

#ifndef UNUSED
#   define UNUSED __attribute__((__unused__))
#endif

/// constant used in bmv2 backend code generation
class TableImplementation {
 public:
    static const cstring actionProfileName;
    static const cstring actionSelectorName;
    static const cstring directMeterName;
};

class MatchImplementation {
 public:
    static const cstring selectorMatchTypeName;
    static const cstring rangeMatchTypeName;
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

// XXX(hanw): This convenience class stores pointers to the data structures
// that are commonly used during the program translation. Due to the limitation
// of current IR structure, these data structure are only refreshed by the
// evaluator pass. In the long term, integrating these data structures as part
// of the IR tree would simplify this kind of bookkeeping effort.
struct ConversionContext {
    // context
    P4::ReferenceMap*                refMap;
    P4::TypeMap*                     typeMap;
    const IR::ToplevelBlock*         toplevel;
    //
    ProgramStructure*                structure;
    // expression converter is used in many places.
    ExpressionConverter*             conv;
    // final json output.
    BMV2::JsonObjects*               json;

    // for action profile conversion
    Util::JsonArray*                 action_profiles;
    SharedActionSelectorCheck*       selector_check;

    ConversionContext(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                      const IR::ToplevelBlock* toplevel, ProgramStructure* structure,
                      ExpressionConverter* conv, JsonObjects* json) :
        refMap(refMap), typeMap(typeMap), toplevel(toplevel), structure(structure),
        conv(conv), json(json) { }
};

using BlockTypeMap = std::map<const IR::Block*, const IR::Type*>;

Util::IJson* nodeName(const CFG::Node* node);
Util::JsonArray* mkArrayField(Util::JsonObject* parent, cstring name);
Util::JsonArray* mkParameters(Util::JsonObject* object);
Util::JsonObject* mkPrimitive(cstring name, Util::JsonArray* appendTo);
Util::JsonObject* mkPrimitive(cstring name);
cstring stringRepr(mpz_class value, unsigned bytes = 0);
unsigned nextId(cstring group);

}  // namespace BMV2

#endif /* BACKENDS_BMV2_COMMON_HELPERS_H_ */
