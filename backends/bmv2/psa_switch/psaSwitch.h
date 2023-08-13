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

#ifndef BACKENDS_BMV2_PSA_SWITCH_PSASWITCH_H_
#define BACKENDS_BMV2_PSA_SWITCH_PSASWITCH_H_

#include <boost/multiprecision/cpp_int.hpp>

#include "backends/bmv2/common/JsonObjects.h"
#include "backends/bmv2/common/backend.h"
#include "backends/bmv2/common/expression.h"
#include "backends/bmv2/common/extern.h"
#include "backends/bmv2/common/helpers.h"
#include "backends/bmv2/common/options.h"
#include "backends/bmv2/common/programStructure.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/algorithm.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/json.h"
#include "lib/null.h"
#include "midend/convertEnums.h"
#include "psaProgramStructure.h"

namespace BMV2 {

class PsaSwitchExpressionConverter : public ExpressionConverter {
 public:
    PsaSwitchExpressionConverter(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                 ProgramStructure *structure, cstring scalarsName)
        : BMV2::ExpressionConverter(refMap, typeMap, structure, scalarsName) {}

    void modelError(const char *format, const cstring field) {
        ::error(ErrorType::ERR_MODEL,
                (cstring(format) + "\nInvalid metadata parameter value for PSA").c_str(), field);
    }

    Util::IJson *convertParam(UNUSED const IR::Parameter *param, cstring fieldName) override {
        cstring ptName = param->type->toString();
        if (PsaProgramStructure::isCounterMetadata(ptName)) {  // check if its counter metadata
            auto jsn = new Util::JsonObject();
            jsn->emplace("name", param->toString());
            jsn->emplace("type", "hexstr");
            auto bitwidth = param->type->width_bits();

            // encode the counter type from enum -> int
            if (fieldName == "BYTES") {
                cstring repr = BMV2::stringRepr(0, ROUNDUP(bitwidth, 32));
                jsn->emplace("value", repr);
            } else if (fieldName == "PACKETS") {
                cstring repr = BMV2::stringRepr(1, ROUNDUP(bitwidth, 32));
                jsn->emplace("value", repr);
            } else if (fieldName == "PACKETS_AND_BYTES") {
                cstring repr = BMV2::stringRepr(2, ROUNDUP(bitwidth, 32));
                jsn->emplace("value", repr);
            } else {
                modelError("%1%: Exptected a PSA_CounterType_t", fieldName);
                return nullptr;
            }
            return jsn;
        } else if (PsaProgramStructure::isStandardMetadata(ptName)) {  // check if its psa metadata
            auto jsn = new Util::JsonObject();

            // encode the metadata type and field in json
            jsn->emplace("type", "field");
            auto a = mkArrayField(jsn, "value");
            a->append(ptName.exceptLast(2));
            a->append(fieldName);
            return jsn;
        } else {
            // not a special type
            return nullptr;
        }
        return nullptr;
    }
};

class PsaCodeGenerator : public PsaProgramStructure {
 public:
    PsaCodeGenerator(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : PsaProgramStructure(refMap, typeMap) {}

    void create(ConversionContext *ctxt);
    void createStructLike(ConversionContext *ctxt, const IR::Type_StructLike *st);
    void createTypes(ConversionContext *ctxt);
    void createHeaders(ConversionContext *ctxt);
    void createScalars(ConversionContext *ctxt);
    void createParsers(ConversionContext *ctxt);
    void createExterns();
    void createActions(ConversionContext *ctxt);
    void createControls(ConversionContext *ctxt);
    void createDeparsers(ConversionContext *ctxt);
    void createGlobals();
    cstring convertHashAlgorithm(cstring algo);
};

class ConvertPsaToJson : public Inspector {
 public:
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    const IR::ToplevelBlock *toplevel;
    JsonObjects *json;
    PsaCodeGenerator *structure;

    ConvertPsaToJson(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                     const IR::ToplevelBlock *toplevel, JsonObjects *json,
                     PsaCodeGenerator *structure)
        : refMap(refMap), typeMap(typeMap), toplevel(toplevel), json(json), structure(structure) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        CHECK_NULL(toplevel);
        CHECK_NULL(json);
        CHECK_NULL(structure);
    }

    void postorder(UNUSED const IR::P4Program *program) override {
        cstring scalarsName = "scalars";
        // This visitor is used in multiple passes to convert expression to json
        auto conv = new PsaSwitchExpressionConverter(refMap, typeMap, structure, scalarsName);
        auto ctxt = new ConversionContext(refMap, typeMap, toplevel, structure, conv, json);
        structure->create(ctxt);
    }
};

class PsaSwitchBackend : public Backend {
    BMV2Options &options;

 public:
    void convert(const IR::ToplevelBlock *tlb) override;
    PsaSwitchBackend(BMV2Options &options, P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                     P4::ConvertEnums::EnumMapping *enumMap)
        : Backend(options, refMap, typeMap, enumMap), options(options) {}
};

EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(Hash)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(Checksum)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(InternetChecksum)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(Counter)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(DirectCounter)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(Meter)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(DirectMeter)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(Register)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(Random)
EXTERN_CONVERTER_W_INSTANCE(ActionProfile)
EXTERN_CONVERTER_W_INSTANCE(ActionSelector)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(Digest)

}  // namespace BMV2

#endif /* BACKENDS_BMV2_PSA_SWITCH_PSASWITCH_H_ */
