#include "backends/p4tools/modules/smith/targets/tofino/target.h"

#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <string>
#include <vector>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/parser.h"
#include "backends/p4tools/modules/smith/common/probabilities.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "backends/p4tools/modules/smith/core/target.h"
#include "backends/p4tools/modules/smith/util/util.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/cstring.h"

namespace P4C::P4Tools::P4Smith::Tofino {

using namespace ::P4C::P4::literals;

/* =============================================================================================
 *  AbstractTofinoSmithTarget implementation
 * ============================================================================================= */

AbstractTofinoSmithTarget::AbstractTofinoSmithTarget(const std::string &deviceName,
                                                     const std::string &archName)
    : SmithTarget(deviceName, archName) {}

/* =============================================================================================
 *  TofinoTnaSmithTarget implementation
 * ============================================================================================= */

TofinoTnaSmithTarget::TofinoTnaSmithTarget() : AbstractTofinoSmithTarget("tofino", "tna") {}

void TofinoTnaSmithTarget::make() {
    static TofinoTnaSmithTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new TofinoTnaSmithTarget();
    }
}

namespace {

IR::Declaration_Instance *generatePackageDeclaration() {
    auto *args = new IR::Vector<IR::Argument>();

    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("SwitchIngressParser"))));
    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("ingress"))));
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("SwitchIngressDeparser"))));
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("SwitchEgressParser"))));
    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("SwitchEgress"))));
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("SwitchEgressDeparser"))));
    auto *packageName = new IR::Type_Name("Pipeline");
    return new IR::Declaration_Instance("pipe", packageName, args);
}

IR::Declaration_Instance *generateMainPackage() {
    auto *args = new IR::Vector<IR::Argument>();
    args->push_back(new IR::Argument(new IR::TypeNameExpression("pipe")));
    auto *packageName = new IR::Type_Name("Switch");
    return new IR::Declaration_Instance("main", packageName, args);
}

IR::IndexedVector<IR::StructField> generateStructFields(std::vector<cstring> &fields,
                                                        std::vector<int> &bit_size,
                                                        size_t vec_size) {
    IR::IndexedVector<IR::StructField> retFields;

    for (size_t i = 0; i < vec_size; i++) {
        cstring name = fields.at(i);
        retFields.push_back(new IR::StructField(name, IR::Type_Bits::get(bit_size.at(i), false)));
    }

    return retFields;
}

IR::IndexedVector<IR::StructField> generateIngressIntrinsicMetadata() {
    std::vector<cstring> fields = {
        // "resubmit_flag",
    };

    std::vector<int> bitSize = {
        // 1,
    };

    return generateStructFields(fields, bitSize, fields.size());
}

IR::IndexedVector<IR::StructField> generateIngressIntrinsicMetadataForTrafficManager() {
    std::vector<cstring> fields = {
        // "ucast_egress_port",
        // "bypass_egress",
        // "deflect_on_drop",
        // "ingress_cos",
        // "qid",
        // "icos_for_copy_to_cpu",
        // "copy_to_cpu",
        // "packet_color",
        // "disable_ucast_cutthru",
        // "enable_mcast_cutthru",
        // "mcast_grp_a",
        // "mcast_grp_b",
        // "level1_mcast_hash",
        // "level2_mcast_hash",
        // "level1_exclusion_id",
        // "level2_exclusion_id",
        // "rid",
    };

    std::vector<int> bitSize = {
        // 9,
        // 1,
        // 1,
        // 3,
        // 5,
        // 3,
        // 1,
        // 2,
        // 1,
        // 1,
        // 16,
        // 16,
        // 13,
        // 13,
        // 16,
        // 9,
        // 16,
    };

    return generateStructFields(fields, bitSize, fields.size());
}

IR::IndexedVector<IR::StructField> generateIngressIntrinsicMetadataFromParser() {
    std::vector<cstring> fields = {
        // "parser_err",
    };

    std::vector<int> bitSize = {
        // 16,
    };

    return generateStructFields(fields, bitSize, fields.size());
}

IR::IndexedVector<IR::StructField> generateIngressIntrinsicMetadataForDeparser() {
    std::vector<cstring> fields = {
        // "drop_ctl",
        // "digest_type",
        // "resubmit_type",
        // "mirror_type",
    };

    std::vector<int> bitSize = {
        // 3,
        // 3,
        // 3,
        // 3,
    };

    return generateStructFields(fields, bitSize, fields.size());
}

IR::IndexedVector<IR::StructField> generateEgressIntrisicMetadata() {
    std::vector<cstring> fields = {
        // "egress_port",
    };

    std::vector<int> bitSize = {
        // 9,
    };

    return generateStructFields(fields, bitSize, fields.size());
}

IR::IndexedVector<IR::StructField> generateEgressIntrinsicMetadataFromParser() {
    std::vector<cstring> fields = {
        // "parser_err",
    };

    std::vector<int> bitSize = {
        // 16,
    };

    return generateStructFields(fields, bitSize, fields.size());
}

IR::IndexedVector<IR::StructField> generateEgressIntrinsicMetadataForDeparser() {
    std::vector<cstring> fields = {
        // "drop_ctl",
        // "mirror_type",
        // "coalesce_flush",
        // "coalesce_length",
    };

    std::vector<int> bitSize = {
        // 3,
        // 3,
        // 1,
        // 7,
    };

    return generateStructFields(fields, bitSize, fields.size());
}

IR::IndexedVector<IR::StructField> generateEgressIntrinsicMetadataForOutputPort() {
    std::vector<cstring> fields = {
        // "force_tx_error",
    };

    std::vector<int> bitSize = {
        // 1,
    };

    return generateStructFields(fields, bitSize, fields.size());
}

void generateTnaMetadata() {
    IR::ID *name = nullptr;
    // IR::IndexedVector<IR::StructField> fields;
    IR::Type_Struct *ret = nullptr;

    name = new IR::ID("ingress_intrinsic_metadata_t");
    ret = new IR::Type_Struct(*name, generateIngressIntrinsicMetadata());
    P4Scope::addToScope(ret);
    name = new IR::ID("ingress_intrinsic_metadata_for_tm_t");
    ret = new IR::Type_Struct(*name, generateIngressIntrinsicMetadataForTrafficManager());
    P4Scope::addToScope(ret);
    name = new IR::ID("ingress_intrinsic_metadata_from_parser_t");
    ret = new IR::Type_Struct(*name, generateIngressIntrinsicMetadataFromParser());
    P4Scope::addToScope(ret);
    name = new IR::ID("ingress_intrinsic_metadata_for_deparser_t");
    ret = new IR::Type_Struct(*name, generateIngressIntrinsicMetadataForDeparser());
    P4Scope::addToScope(ret);
    name = new IR::ID("egress_intrinsic_metadata_t");
    ret = new IR::Type_Struct(*name, generateEgressIntrisicMetadata());
    P4Scope::addToScope(ret);
    name = new IR::ID("egress_intrinsic_metadata_from_parser_t");
    ret = new IR::Type_Struct(*name, generateEgressIntrinsicMetadataFromParser());
    P4Scope::addToScope(ret);
    name = new IR::ID("egress_intrinsic_metadata_for_deparser_t");
    ret = new IR::Type_Struct(*name, generateEgressIntrinsicMetadataForDeparser());
    P4Scope::addToScope(ret);
    name = new IR::ID("egress_intrinsic_metadata_for_output_port_t");
    ret = new IR::Type_Struct(*name, generateEgressIntrinsicMetadataForOutputPort());
    P4Scope::addToScope(ret);
}

IR::Type_Struct *generateIngressMetadataT() {
    // Do !emit meta fields for now, no need
    IR::IndexedVector<IR::StructField> fields;
    auto *ret = new IR::Type_Struct("ingress_metadata_t", fields);
    P4Scope::addToScope(ret);
    return ret;
}

IR::Type_Struct *generateEgressMetadataT() {
    // Do !emit meta fields for now, no need
    IR::IndexedVector<IR::StructField> fields;
    auto *ret = new IR::Type_Struct("egress_metadata_t", fields);
    P4Scope::addToScope(ret);
    return ret;
}

void setTnaProbabilities() {
    PCT.PARAMETER_NONEDIR_DERIVED_STRUCT = 0;
    PCT.PARAMETER_NONEDIR_DERIVED_HEADER = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_BOOL = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_ERROR = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_STRING = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_VARBIT = 0;
    // TNA does not support headers that are not multiples of 8.
    PCT.STRUCTTYPEDECLARATION_BASETYPE_BOOL = 0;
    // TNA requires headers to be byte-aligned.
    P4Scope::req.byte_align_headers = true;
    // TNA requires constant header stack indices.
    P4Scope::constraints.const_header_stack_index = true;
    // TNA requires that the shift count in IR::SHL must be a constant.
    P4Scope::constraints.const_lshift_count = true;
    // TNA *currently* only supports single stage actions.
    P4Scope::constraints.single_stage_actions = true;
    // Saturating arithmetic operators mau not exceed maximum PHV container width.
    P4Scope::constraints.max_phv_container_width = 32;
}

IR::MethodCallStatement *generateDeparserEmitCall() {
    auto *call = new IR::PathExpression("pkt.emit");
    auto *args = new IR::Vector<IR::Argument>();

    args->push_back(new IR::Argument(new IR::PathExpression("h")));

    auto *mce = new IR::MethodCallExpression(call, args);
    auto *mst = new IR::MethodCallStatement(mce);
    return mst;
}

}  // namespace

IR::P4Parser *TofinoTnaSmithTarget::generateIngressParserBlock() const {
    IR::IndexedVector<IR::Declaration> parserLocals;
    P4Scope::startLocalScope();

    // Generate type_parser, note that this is labeled "p".
    IR::IndexedVector<IR::Parameter> params;

    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "pkt"_cs, "packet_in"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::Out, "hdr"_cs, SYS_HDR_NAME));
    params.push_back(declarationGenerator().genParameter(IR::Direction::Out, "ig_md"_cs,
                                                         "ingress_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::Out, "ig_intr_md"_cs,
                                                         "ingress_intrinsic_metadata_t"_cs));
    auto *parList = new IR::ParameterList(params);

    auto *tpParser = new IR::Type_Parser("SwitchIngressParser", parList);

    // Add params to the parser scope.
    for (const auto *param : parList->parameters) {
        P4Scope::addToScope(param);
        // we only add values that are not read-only, to the modifiable types
        if (param->direction == IR::Direction::In) {
            P4Scope::addLval(param->type, param->name.name, true);
        } else {
            P4Scope::addLval(param->type, param->name.name, false);
        }
    }

    // Generate variable declarations.
    for (int i = 0; i < 5; i++) {
        auto *varDecl = declarationGenerator().genVariableDeclaration();
        parserLocals.push_back(varDecl);
    }

    // Generate reguster actions.
    // These are currently disabled because
    // they cause a compiler bug, which is currently
    // being investigates
    for (int i = 0; i < 0; i++) {
        // Need to pass in parserLocals because we generate register declarations
        // inside genDeclInstance.
        // auto *reg = RegisterActionDeclaration::genDeclInstance(&parserLocals);
        // parserLocals.push_back(reg);
    }

    // Generate Parser states.
    IR::IndexedVector<IR::ParserState> states;
    auto *startState = parserGenerator().genStartState();

    // Insert custom parsing statements into the start state.
    auto *pktPath = new IR::PathExpression("pkt");
    auto *pktExtract = new IR::Member(pktPath, "extract");
    auto *pktAdvance = new IR::Member(pktPath, "advance");
    auto *igIntrMd = new IR::PathExpression("ig_intr_md");
    auto *extractTofinoMd = parserGenerator().genHdrExtract(pktExtract, igIntrMd);
    startState->components.push_back(extractTofinoMd);
    auto *portMdSize = new IR::PathExpression("PORT_METADATA_SIZE");
    auto *advanceTofinoMd = parserGenerator().genHdrExtract(pktAdvance, portMdSize);
    startState->components.push_back(advanceTofinoMd);
    // Done with custom statements.
    states.push_back(startState);
    states.push_back(parserGenerator().genHdrStates());

    P4Scope::endLocalScope();

    // Add the parser to the whole scope.
    auto *p4parser = new IR::P4Parser("SwitchIngressParser", tpParser, parserLocals, states);
    P4Scope::addToScope(p4parser);
    return p4parser;
}

IR::P4Control *TofinoTnaSmithTarget::generateIngressBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "h"_cs, SYS_HDR_NAME));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "ig_md"_cs,
                                                         "ingress_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "ig_intr_md"_cs,
                                                         "ingress_intrinsic_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(
        IR::Direction::In, "ig_prsr_md"_cs, "ingress_intrinsic_metadata_from_parser_t"_cs));
    params.push_back(declarationGenerator().genParameter(
        IR::Direction::InOut, "ig_dprsr_md"_cs, "ingress_intrinsic_metadata_for_deparser_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "ig_tm_md"_cs,
                                                         "ingress_intrinsic_metadata_for_tm_t"_cs));

    auto *parList = new IR::ParameterList(params);
    auto *typeCtrl = new IR::Type_Control("ingress", parList);

    // add to the scope
    for (const auto *param : parList->parameters) {
        P4Scope::addToScope(param);
        // add to the name_2_type
        // only add values that are !read-only to the modifiable types
        if (param->direction == IR::Direction::In) {
            P4Scope::addLval(param->type, param->name.name, true);
        } else {
            P4Scope::addLval(param->type, param->name.name, false);
        }
    }

    IR::IndexedVector<IR::Declaration> localDecls = declarationGenerator().genLocalControlDecls();
    // apply body
    auto *applyBlock = statementGenerator().genBlockStatement(false);
    // hardcode the output port to be zero
    auto *outputPort = new IR::PathExpression("ig_tm_md.ucast_egress_port");
    auto *outputPortVal = new IR::Constant(IR::Type_InfInt::get(), 0);
    auto *assign = new IR::AssignmentStatement(outputPort, outputPortVal);
    // some hack to insert the expression at the beginning
    auto it = applyBlock->components.begin();
    applyBlock->components.insert(it, assign);
    // end of scope
    P4Scope::endLocalScope();

    // add to the whole scope
    auto *p4ctrl = new IR::P4Control("ingress", typeCtrl, localDecls, applyBlock);
    P4Scope::addToScope(p4ctrl);
    return p4ctrl;
}

IR::P4Control *TofinoTnaSmithTarget::generateIngressDeparserBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "pkt"_cs, "packet_out"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "h"_cs, SYS_HDR_NAME));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "ig_md"_cs,
                                                         "ingress_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(
        IR::Direction::In, "ig_dprsr_md"_cs, "ingress_intrinsic_metadata_for_deparser_t"_cs));
    auto *parList = new IR::ParameterList(params);
    auto *typeCtrl = new IR::Type_Control("SwitchIngressDeparser", parList);
    IR::IndexedVector<IR::Declaration> localDecls;
    auto *blkStat = new IR::BlockStatement();
    blkStat->push_back(generateDeparserEmitCall());

    return new IR::P4Control("SwitchIngressDeparser", typeCtrl, localDecls, blkStat);
}

IR::P4Parser *TofinoTnaSmithTarget::generateEgressParserBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "pkt"_cs, "packet_in"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::Out, "h"_cs, SYS_HDR_NAME));
    params.push_back(declarationGenerator().genParameter(IR::Direction::Out, "eg_md"_cs,
                                                         "egress_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::Out, "eg_intr_md"_cs,
                                                         "egress_intrinsic_metadata_t"_cs));
    auto *parList = new IR::ParameterList(params);
    auto *typeParser = new IR::Type_Parser("SwitchEgressParser", parList);
    IR::IndexedVector<IR::Declaration> localDecls;
    // TODO(fruffy): this hacky. FIX
    // generate states
    IR::IndexedVector<IR::ParserState> states;
    IR::IndexedVector<IR::StatOrDecl> components;
    IR::Expression *transition = new IR::PathExpression("accept");
    auto *startState = new IR::ParserState("start", components, transition);
    // insert custom parsing statements into the start state
    auto *pktPath = new IR::PathExpression("pkt");
    auto *pktExtract = new IR::Member(pktPath, "extract");
    auto *egIntrMd = new IR::PathExpression("eg_intr_md");
    auto *extractTofinoMd = parserGenerator().genHdrExtract(pktExtract, egIntrMd);
    startState->components.push_back(extractTofinoMd);
    states.push_back(startState);
    return new IR::P4Parser("SwitchEgressParser", typeParser, localDecls, states);
}

IR::P4Control *TofinoTnaSmithTarget::generateEgressDeparserBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "pkt"_cs, "packet_out"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "h"_cs, SYS_HDR_NAME));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::In, "eg_md"_cs, "egress_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(
        IR::Direction::In, "eg_intr_dprs_md"_cs, "egress_intrinsic_metadata_for_deparser_t"_cs));
    auto *parList = new IR::ParameterList(params);
    auto *typeCtrl = new IR::Type_Control("SwitchEgressDeparser"_cs, parList);
    IR::IndexedVector<IR::Declaration> localDecls;
    auto *blkStat = new IR::BlockStatement();
    blkStat->push_back(generateDeparserEmitCall());

    return new IR::P4Control("SwitchEgressDeparser", typeCtrl, localDecls, blkStat);
}

IR::P4Control *TofinoTnaSmithTarget::generateEgressBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "h"_cs, SYS_HDR_NAME));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "eg_md"_cs,
                                                         "egress_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "eg_intr_md"_cs,
                                                         "egress_intrinsic_metadata_t"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::In, "eg_intr_md_from_prsr"_cs,
                                            "egress_intrinsic_metadata_from_parser_t"_cs));
    params.push_back(declarationGenerator().genParameter(
        IR::Direction::InOut, "eg_intr_dprs_md"_cs, "egress_intrinsic_metadata_for_deparser_t"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "eg_intr_oport_md"_cs,
                                            "egress_intrinsic_metadata_for_output_port_t"_cs));
    auto *parList = new IR::ParameterList(params);
    auto *typeCtrl = new IR::Type_Control("SwitchEgress", parList);
    IR::IndexedVector<IR::Declaration> localDecls;
    auto *blkStat = new IR::BlockStatement();

    return new IR::P4Control("SwitchEgress", typeCtrl, localDecls, blkStat);
}

int TofinoTnaSmithTarget::writeTargetPreamble(std::ostream *ostream) const {
    *ostream << "#include <core.p4>\n";
    *ostream << "#define __TARGET_TOFINO__ 1\n";
    *ostream << "#include <tna.p4>\n\n";
    return EXIT_SUCCESS;
}

const IR::P4Program *TofinoTnaSmithTarget::generateP4Program() const {
    P4Scope::startLocalScope();

    // insert banned structures
    P4Scope::notInitializedStructs.insert("ingress_intrinsic_metadata_t"_cs);
    P4Scope::notInitializedStructs.insert("ingress_intrinsic_metadata_for_tm_t"_cs);
    P4Scope::notInitializedStructs.insert("ingress_intrinsic_metadata_from_parser_t"_cs);
    P4Scope::notInitializedStructs.insert("ingress_intrinsic_metadata_for_deparser_t"_cs);
    P4Scope::notInitializedStructs.insert("egress_intrinsic_metadata_t"_cs);
    P4Scope::notInitializedStructs.insert("egress_intrinsic_metadata_from_parser_t"_cs);
    P4Scope::notInitializedStructs.insert("egress_intrinsic_metadata_for_deparser_t"_cs);
    P4Scope::notInitializedStructs.insert("egress_intrinsic_metadata_for_output_port_t"_cs);

    // set tna-specific probabilities
    setTnaProbabilities();

    // start to assemble the model
    auto *objects = new IR::Vector<IR::Node>();

    // insert tofino metadata
    generateTnaMetadata();

    objects->push_back(declarationGenerator().genEthernetHeaderType());

    // generate some declarations
    int typeDecls = Utils::getRandInt(DECL.MIN_TYPE, DECL.MAX_TYPE);
    for (int i = 0; i < typeDecls; ++i) {
        objects->push_back(declarationGenerator().genTypeDeclaration());
    }

    // generate struct Headers
    objects->push_back(declarationGenerator().genHeaderStruct());
    // generate struct ingress_metadata_t
    objects->push_back(generateIngressMetadataT());
    // generate struct egress_metadata_t
    objects->push_back(generateEgressMetadataT());

    // generate some callables
    int callableDecls = Utils::getRandInt(DECL.MIN_CALLABLES, DECL.MAX_CALLABLES);
    for (int i = 0; i < callableDecls; ++i) {
        std::vector<int64_t> percent = {80, 15, 0, 5};
        switch (Utils::getRandInt(percent)) {
            case 0: {
                objects->push_back(declarationGenerator().genFunctionDeclaration());
                break;
            }
            case 1: {
                objects->push_back(declarationGenerator().genActionDeclaration());
                break;
            }
            case 2: {
                objects->push_back(declarationGenerator().genExternDeclaration());
                break;
            }
            case 3: {
                objects->push_back(declarationGenerator().genControlDeclaration());
                break;
            }
        }
    }

    // generate all the necessary pipelines for the package
    objects->push_back(generateIngressParserBlock());
    objects->push_back(generateIngressBlock());
    objects->push_back(generateIngressDeparserBlock());
    objects->push_back(generateEgressParserBlock());
    objects->push_back(generateEgressBlock());
    objects->push_back(generateEgressDeparserBlock());

    // finally assemble the package
    objects->push_back(generatePackageDeclaration());
    objects->push_back(generateMainPackage());

    return new IR::P4Program(*objects);
}
}  // namespace P4C::P4Tools::P4Smith::Tofino
