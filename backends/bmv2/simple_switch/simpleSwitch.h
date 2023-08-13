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

#ifndef BACKENDS_BMV2_SIMPLE_SWITCH_SIMPLESWITCH_H_
#define BACKENDS_BMV2_SIMPLE_SWITCH_SIMPLESWITCH_H_

#include <set>
#include <string>
#include <vector>

#include "backends/bmv2/common/backend.h"
#include "backends/bmv2/common/expression.h"
#include "backends/bmv2/common/extern.h"
#include "backends/bmv2/common/helpers.h"
#include "backends/bmv2/common/options.h"
#include "backends/bmv2/common/programStructure.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "frontends/p4/typeMap.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/visitor.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/json.h"
#include "midend/convertEnums.h"

namespace BMV2 {

class V1ProgramStructure : public ProgramStructure {
 public:
    std::set<cstring> pipeline_controls;
    std::set<cstring> non_pipeline_controls;

    const IR::P4Parser *parser = nullptr;
    const IR::P4Control *ingress = nullptr;
    const IR::P4Control *egress = nullptr;
    const IR::P4Control *compute_checksum = nullptr;
    const IR::P4Control *verify_checksum = nullptr;
    const IR::P4Control *deparser = nullptr;

    V1ProgramStructure() {}
    BlockConverted blockKind(const IR::Node *node) const {
        if (node == parser)
            return BlockConverted::Parser;
        else if (node == ingress)
            return BlockConverted::Ingress;
        else if (node == egress)
            return BlockConverted::Egress;
        else if (node == compute_checksum)
            return BlockConverted::ChecksumCompute;
        else if (node == verify_checksum)
            return BlockConverted::ChecksumVerify;
        else if (node == deparser)
            return BlockConverted::Deparser;
        return BlockConverted::None;
    }
};

class SimpleSwitchExpressionConverter : public ExpressionConverter {
    V1ProgramStructure *structure;

 public:
    SimpleSwitchExpressionConverter(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                    V1ProgramStructure *structure, cstring scalarsName)
        : ExpressionConverter(refMap, typeMap, structure, scalarsName), structure(structure) {}

    void modelError(const char *format, const IR::Node *node) {
        ::error(ErrorType::ERR_MODEL,
                (cstring(format) + "\nAre you using an up-to-date v1model.p4?").c_str(), node);
    }

    bool isStandardMetadataParameter(const IR::Parameter *param) {
        auto st = dynamic_cast<V1ProgramStructure *>(structure);
        auto params = st->parser->getApplyParameters();
        if (params->size() != 4) {
            modelError("%1%: Expected 4 parameter for parser", st->parser);
            return false;
        }
        if (params->parameters.at(3) == param) return true;

        params = st->ingress->getApplyParameters();
        if (params->size() != 3) {
            modelError("%1%: Expected 3 parameter for ingress", st->ingress);
            return false;
        }
        if (params->parameters.at(2) == param) return true;

        params = st->egress->getApplyParameters();
        if (params->size() != 3) {
            modelError("%1%: Expected 3 parameter for egress", st->egress);
            return false;
        }
        if (params->parameters.at(2) == param) return true;

        return false;
    }

    Util::IJson *convertParam(const IR::Parameter *param, cstring fieldName) override {
        if (isStandardMetadataParameter(param)) {
            auto result = new Util::JsonObject();
            if (fieldName != "") {
                result->emplace("type", "field");
                auto e = BMV2::mkArrayField(result, "value");
                e->append("standard_metadata");
                e->append(fieldName);
            } else {
                result->emplace("type", "header");
                result->emplace("value", "standard_metadata");
            }
            return result;
        }
        return nullptr;
    }
};

class ParseV1Architecture : public Inspector {
    V1ProgramStructure *structure;
    P4V1::V1Model &v1model;

 public:
    explicit ParseV1Architecture(V1ProgramStructure *structure)
        : structure(structure), v1model(P4V1::V1Model::instance) {}
    void modelError(const char *format, const IR::Node *node);
    bool preorder(const IR::PackageBlock *block) override;
};

class SimpleSwitchBackend : public Backend {
    BMV2Options &options;
    P4V1::V1Model &v1model;
    V1ProgramStructure *structure = nullptr;
    ExpressionConverter *conv = nullptr;

 protected:
    void createRecirculateFieldsList(ConversionContext *ctxt, const IR::ToplevelBlock *tlb,
                                     cstring scalarName);
    cstring createCalculation(cstring algo, const IR::Expression *fields,
                              Util::JsonArray *calculations, bool usePayload, const IR::Node *node);

 public:
    void modelError(const char *format, const IR::Node *place) const;
    void convertChecksum(const IR::BlockStatement *body, Util::JsonArray *checksums,
                         Util::JsonArray *calculations, bool verify);
    void createActions(ConversionContext *ctxt, V1ProgramStructure *structure);

    void convert(const IR::ToplevelBlock *tlb) override;
    SimpleSwitchBackend(BMV2Options &options, P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                        P4::ConvertEnums::EnumMapping *enumMap)
        : Backend(options, refMap, typeMap, enumMap),
          options(options),
          v1model(P4V1::V1Model::instance) {}
};

EXTERN_CONVERTER_W_FUNCTION(clone)
EXTERN_CONVERTER_W_FUNCTION(clone_preserving_field_list)
EXTERN_CONVERTER_W_FUNCTION_AND_MODEL(hash, P4V1::V1Model, v1model)
EXTERN_CONVERTER_W_FUNCTION(digest)
EXTERN_CONVERTER_W_FUNCTION(resubmit_preserving_field_list)
EXTERN_CONVERTER_W_FUNCTION(recirculate_preserving_field_list)
EXTERN_CONVERTER_W_FUNCTION(mark_to_drop)
EXTERN_CONVERTER_W_FUNCTION(log_msg)
EXTERN_CONVERTER_W_FUNCTION_AND_MODEL(random, P4V1::V1Model, v1model)
EXTERN_CONVERTER_W_FUNCTION_AND_MODEL(truncate, P4V1::V1Model, v1model)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE_AND_MODEL(register, P4V1::V1Model, v1model)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE_AND_MODEL(counter, P4V1::V1Model, v1model)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE_AND_MODEL(meter, P4V1::V1Model, v1model)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE(direct_counter)
EXTERN_CONVERTER_W_OBJECT_AND_INSTANCE_AND_MODEL(direct_meter, P4V1::V1Model, v1model)
EXTERN_CONVERTER_W_INSTANCE_AND_MODEL(action_profile, P4V1::V1Model, v1model)
EXTERN_CONVERTER_W_INSTANCE_AND_MODEL(action_selector, P4V1::V1Model, v1model)

}  // namespace BMV2

#endif /* BACKENDS_BMV2_SIMPLE_SWITCH_SIMPLESWITCH_H_ */
