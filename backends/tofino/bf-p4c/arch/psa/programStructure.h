/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BACKENDS_TOFINO_BF_P4C_ARCH_PSA_PROGRAMSTRUCTURE_H_
#define BACKENDS_TOFINO_BF_P4C_ARCH_PSA_PROGRAMSTRUCTURE_H_

#include "bf-p4c/arch/program_structure.h"
#include "bf-p4c/arch/psa/psa_model.h"
#include "bf-p4c/ir/gress.h"
#include "bf-p4c/midend/path_linearizer.h"
#include "bf-p4c/midend/type_categories.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "ir/ir.h"
#include "ir/namemap.h"
#include "lib/ordered_set.h"
#include "midend/eliminateSerEnums.h"

namespace BFN {

namespace PSA {

// structure to describe the info related to resubmit/clone/recirc/bridge in a PSA program
struct PacketPathInfo {
    /// The name of the metadata in ingress parser param.
    cstring paramNameInParser;

    /// The name of the metadata in egress deparser param.
    cstring paramNameInDeparser;

    /// name in compiler_generated_meta
    cstring generatedName;

    /// If packet path exists
    bool exists = false;

    /// Map user-defined name to arch-defined param name in source block.
    ordered_map<cstring, cstring> srcParams;

    /// Map arch-defined param name to user-defined name in dest block.
    ordered_map<cstring, cstring> dstParams;

    /// A P4 type for the data, based on the type of the parameter to
    /// the parser.
    const IR::Type *p4Type = nullptr;
    const IR::Type *structType = nullptr;
    // the statements in deparser to emit resubmit/clone/recir metadata, in PSA, the emit
    // is represented with assignments to out parameter in the deparser block.
    // the source typically has the following code pattern.
    // if (psa_resubmit(istd)) {
    //    resub_meta = user_meta.resub_meta;
    // }
    const IR::IfStatement *ifStatement = nullptr;

    ordered_map<const IR::StatOrDecl *, std::vector<const IR::Node *>> fieldLists;
};

using ParamInfo = ordered_map<cstring, cstring>;

struct PsaBlockInfo {
    ParamInfo psaParams;
};

enum PSA_TYPES {
    TYPE_IH = 0,
    TYPE_IM,
    TYPE_EH,
    TYPE_EM,
    TYPE_NM,
    TYPE_CI2EM,
    TYPE_CE2EM,
    TYPE_RESUBM,
    TYPE_RECIRCM,
    PSA_TOTAL_TYPES
};

static const cstring INP_INTR_MD = "__psa_inp_intrinsic_md__"_cs;
static const cstring OUT_INTR_MD = "__psa_out_intrinsic_md__"_cs;
static const cstring IG_INP_INTR_MD_TYPE = "psa_ingress_input_metadata_t"_cs;
static const cstring EG_INP_INTR_MD_TYPE = "psa_egress_input_metadata_t"_cs;
static const cstring IG_OUT_INTR_MD_TYPE = "psa_ingress_output_metadata_t"_cs;
static const cstring EG_OUT_INTR_MD_TYPE = "psa_egress_output_metadata_t"_cs;

struct ProgramStructure : BFN::ProgramStructure {
    cstring type_params[PSA_TOTAL_TYPES];

    PsaModel &psa_model;

    ordered_map<cstring, TranslationMap> methodcalls;

    const IR::Type *metadataType = nullptr;

    PacketPathInfo resubmit;
    PacketPathInfo clone_i2e;
    PacketPathInfo clone_e2e;
    PacketPathInfo recirculate;
    PacketPathInfo bridge;

    PsaBlockInfo ingress_parser;
    PsaBlockInfo ingress;
    PsaBlockInfo ingress_deparser;
    PsaBlockInfo egress_parser;
    PsaBlockInfo egress;
    PsaBlockInfo egress_deparser;

    void createParsers() override;
    void createControls() override;
    void createMain() override;
    void createPipeline();
    std::map<cstring, int> error_to_constant;

    std::map<gress_t, std::map<cstring, const IR::MethodCallExpression *>> state_to_verify;

    // Vector of bridge field assignment in deparser in PSA. These will be moved to ingress
    // control later
    std::vector<IR::AssignmentStatement *> bridgeFieldAssignments;
    const IR::P4Program *create(const IR::P4Program *program) override;
    void loadModel();

    ProgramStructure() : BFN::ProgramStructure(), psa_model(PsaModel::instance) {}
};

}  // namespace PSA

}  // namespace BFN

#endif /* BACKENDS_TOFINO_BF_P4C_ARCH_PSA_PROGRAMSTRUCTURE_H_ */
