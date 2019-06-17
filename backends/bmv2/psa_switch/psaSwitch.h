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

#include "ir/ir.h"
#include "lib/gmputil.h"
#include "lib/json.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/unusedDeclarations.h"
#include "backends/bmv2/common/action.h"
#include "backends/bmv2/common/control.h"
#include "backends/bmv2/common/deparser.h"
#include "backends/bmv2/common/extern.h"
#include "backends/bmv2/common/header.h"
#include "backends/bmv2/common/helpers.h"
#include "backends/bmv2/common/lower.h"
#include "backends/bmv2/common/parser.h"
#include "backends/bmv2/common/programStructure.h"

namespace BMV2 {

class PsaSwitchExpressionConverter : public ExpressionConverter {
 public:
    PsaSwitchExpressionConverter(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                                 ProgramStructure* structure, cstring scalarsName) :
    BMV2::ExpressionConverter(refMap, typeMap, structure, scalarsName) { }

    void modelError(const char* format, const cstring field) {
      ::error(format, field);
      ::error("Invalid metadata parameter value");
    }

    /**
     * Checks if a string is of type PSA_CounterType_t returns true
     * if it is, false otherwise.
     */
    static bool isCounterMetadata(cstring ptName) {
      return !strcmp(ptName, "PSA_CounterType_t");
    }

    /**
     * Checks if a string is a psa metadata returns true
     * if it is, false otherwise.
     */
    static bool isStandardMetadata(cstring ptName) {
      return (!strcmp(ptName, "psa_ingress_parser_input_metadata_t") ||
        !strcmp(ptName, "psa_egress_parser_input_metadata_t") ||
        !strcmp(ptName, "psa_ingress_input_metadata_t") ||
        !strcmp(ptName, "psa_ingress_output_metadata_t") ||
        !strcmp(ptName, "psa_egress_input_metadata_t") ||
        !strcmp(ptName, "psa_egress_deparser_input_metadata_t") ||
        !strcmp(ptName, "psa_egress_output_metadata_t"));
    }


    Util::IJson* convertParam(UNUSED const IR::Parameter* param, cstring fieldName) override {
      cstring ptName = param->type->toString();
      if (isCounterMetadata(ptName)) {  // check if its counter metadata
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
      } else if (isStandardMetadata(ptName)) {  // check if its psa metadata
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

class PsaProgramStructure : public ProgramStructure {
    P4::ReferenceMap*    refMap;
    P4::TypeMap*         typeMap;

 public:
    // We place scalar user metadata fields (i.e., bit<>, bool)
    // in the scalarsName metadata object, so we may need to rename
    // these fields.  This map holds the new names.
    std::vector<const IR::StructField*> scalars;
    unsigned                            scalars_width = 0;
    unsigned                            error_width = 32;
    unsigned                            bool_width = 1;

    // architecture related information
    ordered_map<const IR::Node*, std::pair<gress_t, block_t>> block_type;

    ordered_map<cstring, const IR::Type_Header*> header_types;
    ordered_map<cstring, const IR::Type_Struct*> metadata_types;
    ordered_map<cstring, const IR::Type_HeaderUnion*> header_union_types;
    ordered_map<cstring, const IR::Declaration_Variable*> headers;
    ordered_map<cstring, const IR::Declaration_Variable*> metadata;
    ordered_map<cstring, const IR::Declaration_Variable*> header_stacks;
    ordered_map<cstring, const IR::Declaration_Variable*> header_unions;
    ordered_map<cstring, const IR::Type_Error*> errors;
    ordered_map<cstring, const IR::Type_Enum*> enums;
    ordered_map<cstring, const IR::P4Parser*> parsers;
    ordered_map<cstring, const IR::P4ValueSet*> parse_vsets;
    ordered_map<cstring, const IR::P4Control*> deparsers;
    ordered_map<cstring, const IR::P4Control*> pipelines;
    ordered_map<cstring, const IR::Declaration_Instance*> extern_instances;
    ordered_map<cstring, cstring> field_aliases;

    std::vector<const IR::ExternBlock*> globals;

 public:
    PsaProgramStructure(P4::ReferenceMap* refMap, P4::TypeMap* typeMap)
        : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
    }

    void create(ConversionContext* ctxt);
    void createStructLike(ConversionContext* ctxt, const IR::Type_StructLike* st);
    void createTypes(ConversionContext* ctxt);
    void createHeaders(ConversionContext* ctxt);
    void createParsers(ConversionContext* ctxt);
    void createExterns();
    void createActions(ConversionContext* ctxt);
    void createControls(ConversionContext* ctxt);
    void createDeparsers(ConversionContext* ctxt);
    void createGlobals();

    std::set<cstring> non_pipeline_controls;
    std::set<cstring> pipeline_controls;

    bool hasVisited(const IR::Type_StructLike* st) {
        if (auto h = st->to<IR::Type_Header>())
            return header_types.count(h->getName());
        else if (auto s = st->to<IR::Type_Struct>())
            return metadata_types.count(s->getName());
        else if (auto u = st->to<IR::Type_HeaderUnion>())
            return header_union_types.count(u->getName());
        return false;
    }
};

class ParsePsaArchitecture : public Inspector {
    PsaProgramStructure* structure;
 public:
    explicit ParsePsaArchitecture(PsaProgramStructure* structure) :
        structure(structure) { CHECK_NULL(structure); }

    bool preorder(const IR::ToplevelBlock* block) override;
    bool preorder(const IR::PackageBlock* block) override;
    bool preorder(const IR::ExternBlock* block) override;

    profile_t init_apply(const IR::Node *root) override {
        structure->block_type.clear();
        structure->globals.clear();
        return Inspector::init_apply(root);
    }
};

class InspectPsaProgram : public Inspector {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
    PsaProgramStructure *pinfo;

 public:
    InspectPsaProgram(P4::ReferenceMap* refMap, P4::TypeMap* typeMap, PsaProgramStructure *pinfo)
        : refMap(refMap), typeMap(typeMap), pinfo(pinfo) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        CHECK_NULL(pinfo);
        setName("InspectPsaProgram");
    }

    void postorder(const IR::P4Parser *p) override;
    void postorder(const IR::P4Control* c) override;
    void postorder(const IR::Declaration_Instance* di) override;

    bool isHeaders(const IR::Type_StructLike* st);
    void addTypesAndInstances(const IR::Type_StructLike* type, bool meta);
    void addHeaderType(const IR::Type_StructLike *st);
    void addHeaderInstance(const IR::Type_StructLike *st, cstring name);
    bool preorder(const IR::Parameter* parameter) override;
};

class ConvertPsaToJson : public Inspector {
 public:
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    const IR::ToplevelBlock *toplevel;
    JsonObjects *json;
    PsaProgramStructure *structure;

    ConvertPsaToJson(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                     const IR::ToplevelBlock *toplevel,
                     JsonObjects *json, PsaProgramStructure *structure)
        : refMap(refMap), typeMap(typeMap), toplevel(toplevel), json(json),
          structure(structure) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
        CHECK_NULL(toplevel);
        CHECK_NULL(json);
        CHECK_NULL(structure); }

    void postorder(UNUSED const IR::P4Program* program) override {
        cstring scalarsName = refMap->newName("scalars");
        // This visitor is used in multiple passes to convert expression to json
        auto conv = new PsaSwitchExpressionConverter(refMap, typeMap, structure, scalarsName);
        auto ctxt = new ConversionContext(refMap, typeMap, toplevel, structure, conv, json);
        structure->create(ctxt);
    }
};

class PsaSwitchBackend : public Backend {
    BMV2Options &options;

 public:
    void convert(const IR::ToplevelBlock* tlb) override;
    PsaSwitchBackend(BMV2Options& options, P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
                          P4::ConvertEnums::EnumMapping* enumMap) :
        Backend(options, refMap, typeMap, enumMap), options(options) { }
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

#endif  /* BACKENDS_BMV2_PSA_SWITCH_PSASWITCH_H_ */
