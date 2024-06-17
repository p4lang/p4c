/*
Copyright 2024 Marvell Technology, Inc.

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

#ifndef BACKENDS_BMV2_PNA_NIC_PNANIC_H_
#define BACKENDS_BMV2_PNA_NIC_PNANIC_H_

#include "backends/bmv2/common/action.h"
#include "backends/bmv2/common/control.h"
#include "backends/bmv2/common/deparser.h"
#include "backends/bmv2/common/extern.h"
#include "backends/bmv2/common/header.h"
#include "backends/bmv2/common/helpers.h"
#include "backends/bmv2/common/lower.h"
#include "backends/bmv2/common/parser.h"
#include "backends/bmv2/common/programStructure.h"
#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/strengthReduction.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/unusedDeclarations.h"
#include "ir/ir.h"
#include "lib/big_int_util.h"
#include "lib/json.h"
#include "pnaProgramStructure.h"

namespace BMV2 {

class PnaNicExpressionConverter : public ExpressionConverter {
 public:
    PnaNicExpressionConverter(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                              ProgramStructure *structure, cstring scalarsName)
        : BMV2::ExpressionConverter(refMap, typeMap, structure, scalarsName) {}

    void modelError(const char *format, const cstring field) {
        ::error(ErrorType::ERR_MODEL,
                (cstring(format) + "\nInvalid metadata parameter value for PNA").c_str(), field);
    }

    Util::IJson *convertParam(UNUSED const IR::Parameter *param, cstring fieldName) override {
        cstring ptName = param->type->toString();
        if (PnaProgramStructure::isCounterMetadata(ptName)) {  // check if its counter metadata
            auto jsn = new Util::JsonObject();
            jsn->emplace("name"_cs, param->toString());
            jsn->emplace("type"_cs, "hexstr");
            auto bitwidth = param->type->width_bits();

            // encode the counter type from enum -> int
            if (fieldName == "BYTES") {
                cstring repr = BMV2::stringRepr(0, ROUNDUP(bitwidth, 32));
                jsn->emplace("value"_cs, repr);
            } else if (fieldName == "PACKETS") {
                cstring repr = BMV2::stringRepr(1, ROUNDUP(bitwidth, 32));
                jsn->emplace("value"_cs, repr);
            } else if (fieldName == "PACKETS_AND_BYTES") {
                cstring repr = BMV2::stringRepr(2, ROUNDUP(bitwidth, 32));
                jsn->emplace("value"_cs, repr);
            } else {
                modelError("%1%: Exptected a PNA_CounterType_t", fieldName);
                return nullptr;
            }
            return jsn;
        } else if (PnaProgramStructure::isStandardMetadata(ptName)) {  // check if its pna metadata
            auto jsn = new Util::JsonObject();

            // encode the metadata type and field in json
            jsn->emplace("type"_cs, "field");
            auto a = mkArrayField(jsn, "value"_cs);
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

class PnaCodeGenerator : public PnaProgramStructure {
 public:
    PnaCodeGenerator(P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
        : PnaProgramStructure(refMap, typeMap) {}

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

class ConvertPnaToJson : public Inspector {
 public:
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    const IR::ToplevelBlock *toplevel;
    JsonObjects *json;
    PnaCodeGenerator *structure;

    ConvertPnaToJson(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                     const IR::ToplevelBlock *toplevel, JsonObjects *json,
                     PnaCodeGenerator *structure)
        : refMap(refMap), typeMap(typeMap), toplevel(toplevel), json(json), structure(structure) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        CHECK_NULL(toplevel);
        CHECK_NULL(json);
        CHECK_NULL(structure);
    }

    void postorder(UNUSED const IR::P4Program *program) override {
        cstring scalarsName = "scalars"_cs;
        // This visitor is used in multiple passes to convert expression to json
        auto conv = new PnaNicExpressionConverter(refMap, typeMap, structure, scalarsName);
        auto ctxt = new ConversionContext(refMap, typeMap, toplevel, structure, conv, json);
        structure->create(ctxt);
    }
};

class PnaNicBackend : public Backend {
    BMV2Options &options;

 public:
    void convert(const IR::ToplevelBlock *tlb) override;
    PnaNicBackend(BMV2Options &options, P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
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

#endif /* BACKENDS_BMV2_PNA_NIC_PNANIC_H_ */
