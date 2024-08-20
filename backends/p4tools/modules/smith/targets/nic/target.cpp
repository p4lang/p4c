#include "backends/p4tools/modules/smith/targets/nic/target.h"

#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <string>
#include <vector>

#include "backends/p4tools/common/lib/util.h"
#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/probabilities.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "backends/p4tools/modules/smith/core/target.h"
#include "backends/p4tools/modules/smith/util/util.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/vector.h"

namespace P4::P4Tools::P4Smith::Nic {

using namespace P4::literals;

/* =============================================================================================
 *  AbstractNicSmithTarget implementation
 * ============================================================================================= */

AbstractNicSmithTarget::AbstractNicSmithTarget(const std::string &deviceName,
                                               const std::string &archName)
    : SmithTarget(deviceName, archName) {}

/* =============================================================================================
 *  DpdkPnaSmithTarget implementation
 * ============================================================================================= */

DpdkPnaSmithTarget::DpdkPnaSmithTarget() : AbstractNicSmithTarget("dpdk", "pna") {}

void DpdkPnaSmithTarget::make() {
    static DpdkPnaSmithTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new DpdkPnaSmithTarget();
    }
}

/// This implementation is based on p4include/pna.p4.
IR::IndexedVector<IR::StructField> generatePnaMainParserInputMetadataFields() {
    IR::IndexedVector<IR::StructField> retFields;
    IR::IndexedVector<IR::Declaration_ID> declIds;
    retFields.push_back(new IR::StructField(
        "direction",
        new IR::Type_Enum("PNA_Direction_t", {new IR::Declaration_ID("NET_TO_HOST"),
                                              new IR::Declaration_ID("HOST_TO_NET")})));
    retFields.push_back(new IR::StructField("pass", new IR::Type_Name("PassNumber_t")));
    retFields.push_back(new IR::StructField("loopedback", IR::Type_Boolean::get()));
    retFields.push_back(new IR::StructField("input_port", new IR::Type_Name("PortId_t")));
    return retFields;
}

/// This implementation is based on p4include/pna.p4.
/// struct pna_pre_input_metadata_t {
///     PortId_t                 input_port;
///     ParserError_t            parser_error;
///     PNA_Direction_t          direction;
///     PassNumber_t             pass;
///     bool                     loopedback;
/// }
IR::IndexedVector<IR::StructField> generatePnaPreInputMetadataFields() {
    IR::IndexedVector<IR::StructField> retFields;
    IR::IndexedVector<IR::Declaration_ID> declIds;
    retFields.push_back(new IR::StructField("input_port", new IR::Type_Name("PortId_t")));
    retFields.push_back(new IR::StructField(
        "parser_error",
        new IR::Type_Enum("ParserError_t", {
                                               new IR::Declaration_ID("NoError"),
                                               new IR::Declaration_ID("PacketTooShort"),
                                               new IR::Declaration_ID("NoMatch"),
                                               new IR::Declaration_ID("StackOutOfBounds"),
                                               new IR::Declaration_ID("HeaderTooShort"),
                                               new IR::Declaration_ID("ParserTimeout"),
                                               new IR::Declaration_ID("ParserInvalidArgument"),
                                           })));
    retFields.push_back(new IR::StructField(
        "direction",
        new IR::Type_Enum("PNA_Direction_t", {new IR::Declaration_ID("NET_TO_HOST"),
                                              new IR::Declaration_ID("HOST_TO_NET")})));
    retFields.push_back(new IR::StructField("pass", new IR::Type_Name("PassNumber_t")));
    retFields.push_back(new IR::StructField("loopedback", IR::Type_Boolean::get()));
    return retFields;
}

/// This implementation is based on p4include/pna.p4.
/// struct pna_pre_output_metadata_t {
///     bool                     decrypt;  // TBD: or use said==0 to mean no decrypt?

///     // The following things are stored internally within the decrypt
///     // block, in a table indexed by said:

///     // + The decryption algorithm, e.g. AES256, etc.
///     // + The decryption key
///     // + Any read-modify-write state in the data plane used to
///     //   implement anti-replay attack detection.

///     SecurityAssocId_t        said;
///     bit<16>                  decrypt_start_offset;  // in bytes?

///     // TBD whether it is important to explicitly pass information to a
///     // decryption extern in a way visible to a P4 program about where
///     // headers were parsed and found.  An alternative is to assume
///     // that the architecture saves the pre parser results somewhere,
///     // in a way not visible to the P4 program.
/// }
IR::IndexedVector<IR::StructField> generatePnaPreOutputMetadataFields() {
    IR::IndexedVector<IR::StructField> retFields;
    retFields.push_back(new IR::StructField("decrypt", IR::Type_Boolean::get()));
    retFields.push_back(
        new IR::StructField(IR::ID("said"), new IR::Type_Name(IR::ID("SecurityAssocId_t"))));
    retFields.push_back(new IR::StructField("decrypt_start_offset", IR::Type_Bits::get(16, false)));
    return retFields;
}

/// This implementation is based on p4include/pna.p4.
/// struct pna_main_parser_input_metadata_t {
///    // common fields initialized for all packets that are input to main
///    // parser, regardless of direction.
///    PNA_Direction_t          direction;
///    PassNumber_t             pass;
///    bool                     loopedback;
///    // If this packet has direction NET_TO_HOST, input_port contains
///    // the id of the network port on which the packet arrived.
///    // If this packet has direction HOST_TO_NET, input_port contains
///    // the id of the vport from which the packet came
///    PortId_t                 input_port;   // network port id
/// }
IR::IndexedVector<IR::StructField> generatePnaMainInputMetadataFields() {
    IR::IndexedVector<IR::StructField> retFields;
    retFields.push_back(new IR::StructField(
        "direction",
        new IR::Type_Enum("PNA_Direction_t", {new IR::Declaration_ID("NET_TO_HOST"),
                                              new IR::Declaration_ID("HOST_TO_NET")})));
    retFields.push_back(new IR::StructField("pass", new IR::Type_Name("PassNumber_t")));
    retFields.push_back(new IR::StructField("loopedback", IR::Type_Boolean::get()));
    retFields.push_back(new IR::StructField("timestamp", new IR::Type_Name("Timestamp_t")));
    retFields.push_back(new IR::StructField(
        "parser_error",
        new IR::Type_Enum("ParserError_t", {
                                               new IR::Declaration_ID("NoError"),
                                               new IR::Declaration_ID("PacketTooShort"),
                                               new IR::Declaration_ID("NoMatch"),
                                               new IR::Declaration_ID("StackOutOfBounds"),
                                               new IR::Declaration_ID("HeaderTooShort"),
                                               new IR::Declaration_ID("ParserTimeout"),
                                               new IR::Declaration_ID("ParserInvalidArgument"),
                                           })));
    retFields.push_back(
        new IR::StructField("class_of_service", new IR::Type_Name("ClassOfService_t")));
    retFields.push_back(new IR::StructField("input_port", new IR::Type_Name("PortId_t")));
    return retFields;
}

/// This implementation is based on p4include/pna.p4.
/// struct pna_main_output_metadata_t {
///   // common fields used by the architecture to decide what to do with
///   // the packet next, after the main parser, control, and deparser
///   // have finished executing one pass, regardless of the direction.
///   ClassOfService_t         class_of_service; // 0
/// }
IR::IndexedVector<IR::StructField> generatePnaMainOutputMetadataFields() {
    IR::IndexedVector<IR::StructField> retFields;
    retFields.push_back(new IR::StructField(IR::ID("class_of_service"),
                                            new IR::Type_Name(IR::ID("ClassOfService_t"))));
    return retFields;
}

IR::P4Parser *DpdkPnaSmithTarget::generateMainParserBlock() const {
    IR::IndexedVector<IR::Declaration> parserLocals;
    P4Scope::startLocalScope();

    // generate type_parser !that this is labeled "p"
    IR::IndexedVector<IR::Parameter> params;

    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "pkt"_cs, "packet_in"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::Out, "hdr"_cs, SYS_HDR_NAME));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "user_meta"_cs,
                                                         "main_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "istd"_cs,
                                                         "pna_main_parser_input_metadata_t"_cs));
    auto *parList = new IR::ParameterList(params);

    auto *tpParser = new IR::Type_Parser("MainParserImpl", parList);

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

    // // generate decls
    // for (int i = 0; i < 5; i++) {
    //     auto var_decl = variableDeclaration::gen();
    //     parserLocals.push_back(var_decl);
    // }

    // generate states
    IR::IndexedVector<IR::ParserState> states;
    auto *startState = parserGenerator().genStartState();
    states.push_back(startState);
    states.push_back(parserGenerator().genHdrStates());

    P4Scope::endLocalScope();

    // add to the whole scope
    auto *p4parser = new IR::P4Parser("MainParserImpl", tpParser, parserLocals, states);
    P4Scope::addToScope(p4parser);
    return p4parser;
}

IR::P4Control *DpdkPnaSmithTarget::generatePreControlBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::In, "hdr"_cs, SYS_HDR_NAME));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "user_meta"_cs,
                                                         "main_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "istd"_cs,
                                                         "pna_pre_input_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "ostd"_cs,
                                                         "pna_pre_output_metadata_t"_cs));
    auto *parList = new IR::ParameterList(params);
    auto *typeCtrl = new IR::Type_Control("PreControlImpl", parList);
    IR::IndexedVector<IR::Declaration> localDecls;
    auto *blkStat = new IR::BlockStatement();

    return new IR::P4Control("PreControlImpl", typeCtrl, localDecls, blkStat);
}

IR::P4Control *DpdkPnaSmithTarget::generateMainControlBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "hdr"_cs, SYS_HDR_NAME));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "user_meta"_cs,
                                                         "main_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "istd"_cs,
                                                         "pna_main_input_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "ostd"_cs,
                                                         "pna_main_output_metadata_t"_cs));

    auto *parList = new IR::ParameterList(params);
    auto *typeCtrl = new IR::Type_Control("MainControlImpl", parList);

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
    P4Scope::endLocalScope();

    // add to the whole scope
    auto *p4ctrl = new IR::P4Control("MainControlImpl", typeCtrl, localDecls, applyBlock);
    P4Scope::addToScope(p4ctrl);
    return p4ctrl;
}

IR::MethodCallStatement *generateDeparserEmitCall() {
    auto *call = new IR::PathExpression("pkt.emit");
    auto *args = new IR::Vector<IR::Argument>();

    args->push_back(new IR::Argument(new IR::PathExpression("hdr")));

    auto *mce = new IR::MethodCallExpression(call, args);
    auto *mst = new IR::MethodCallStatement(mce);
    return mst;
}

IR::P4Control *DpdkPnaSmithTarget::generateMainDeparserBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "pkt"_cs, "packet_out"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::In, "hdr"_cs, SYS_HDR_NAME));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "user_meta"_cs,
                                                         "main_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "ostd"_cs,
                                                         "pna_main_output_metadata_t"_cs));
    auto *parList = new IR::ParameterList(params);
    auto *typeCtrl = new IR::Type_Control("MainDeparserImpl", parList);
    IR::IndexedVector<IR::Declaration> localDecls;
    auto *blkStat = new IR::BlockStatement();
    blkStat->push_back(generateDeparserEmitCall());

    return new IR::P4Control("MainDeparserImpl", typeCtrl, localDecls, blkStat);
}

IR::Declaration_Instance *generateMainPackage() {
    auto *args = new IR::Vector<IR::Argument>();
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("MainParserImpl"))));
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("PreControlImpl"))));
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("MainControlImpl"))));
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("MainDeparserImpl"))));
    auto *packageName = new IR::Type_Name("PNA_NIC");
    return new IR::Declaration_Instance("main", packageName, args);
}

IR::Type_Struct *generateUserMetadata() {
    // Do not emit meta fields for now, no need
    IR::IndexedVector<IR::StructField> fields;
    auto *ret = new IR::Type_Struct("main_metadata_t", fields);
    P4Scope::addToScope(ret);
    return ret;
}

void generateMainMetadata() {
    IR::ID *name = nullptr;
    IR::Type_Struct *ret = nullptr;
    IR::IndexedVector<IR::StructField> fields;

    name = new IR::ID("pna_main_parser_input_metadata_t");
    ret = new IR::Type_Struct(*name, generatePnaMainParserInputMetadataFields());
    P4Scope::addToScope(ret);
    name = new IR::ID("pna_pre_input_metadata_t");
    ret = new IR::Type_Struct(*name, generatePnaPreInputMetadataFields());
    P4Scope::addToScope(ret);
    name = new IR::ID("pna_pre_output_metadata_t");
    ret = new IR::Type_Struct(*name, generatePnaPreOutputMetadataFields());
    P4Scope::addToScope(ret);
    name = new IR::ID("pna_main_input_metadata_t");
    ret = new IR::Type_Struct(*name, generatePnaMainInputMetadataFields());
    P4Scope::addToScope(ret);
    name = new IR::ID("pna_main_output_metadata_t");
    ret = new IR::Type_Struct(*name, generatePnaMainOutputMetadataFields());
    P4Scope::addToScope(ret);
}

void setPnaDpdkProbabilities() {
    Probabilities::get().PARAMETER_NONEDIR_DERIVED_STRUCT = 0;
    Probabilities::get().PARAMETER_NONEDIR_DERIVED_HEADER = 0;
    Probabilities::get().PARAMETER_NONEDIR_BASETYPE_BOOL = 0;
    Probabilities::get().PARAMETER_NONEDIR_BASETYPE_ERROR = 0;
    Probabilities::get().PARAMETER_NONEDIR_BASETYPE_STRING = 0;
    Probabilities::get().PARAMETER_NONEDIR_BASETYPE_VARBIT = 0;
    // Complex types are not supported for function calls.
    // TODO: Distinguish between actions and functions?
    Probabilities::get().PARAMETER_DERIVED_STRUCT = 0;
    Probabilities::get().PARAMETER_DERIVED_HEADER = 0;
    Probabilities::get().PARAMETER_BASETYPE_BOOL = 0;
    Probabilities::get().PARAMETER_BASETYPE_ERROR = 0;
    Probabilities::get().PARAMETER_BASETYPE_STRING = 0;
    Probabilities::get().PARAMETER_BASETYPE_VARBIT = 0;
    P4Scope::req.byte_align_headers = true;
    P4Scope::constraints.max_bitwidth = 64;
}

int DpdkPnaSmithTarget::writeTargetPreamble(std::ostream *ostream) const {
    *ostream << "#include <core.p4>\n";
    *ostream << "#include <dpdk/pna.p4>\n\n";
    *ostream << "bit<3> max(in bit<3> val, in bit<3> bound) {\n";
    *ostream << "    return val < bound ? val : bound;\n";
    *ostream << "}\n";
    return EXIT_SUCCESS;
}

const IR::P4Program *DpdkPnaSmithTarget::generateP4Program() const {
    P4Scope::startLocalScope();
    // insert banned structures
    P4Scope::notInitializedStructs.insert("psa_ingress_parser_input_metadata_t"_cs);
    P4Scope::notInitializedStructs.insert("psa_ingress_input_metadata_t"_cs);
    P4Scope::notInitializedStructs.insert("psa_ingress_output_metadata_t"_cs);
    P4Scope::notInitializedStructs.insert("psa_egress_input_metadata_t"_cs);
    P4Scope::notInitializedStructs.insert("psa_egress_output_metadata_t"_cs);
    // set psa-specific probabilities
    setPnaDpdkProbabilities();
    // insert some dummy metadata
    generateMainMetadata();

    // typedef bit<32> PortIdUint_t;
    // typedef bit<32> InterfaceIdUint_t;
    // typedef bit<32> MulticastGroupUint_t;
    // typedef bit<16> MirrorSessionIdUint_t;
    // typedef bit<8>  MirrorSlotIdUint_t;
    // typedef bit<8>  ClassOfServiceUint_t;
    // typedef bit<16> PacketLengthUint_t;
    // typedef bit<16> MulticastInstanceUint_t;
    // typedef bit<64> TimestampUint_t;
    // typedef bit<32> FlowIdUint_t;
    // typedef bit<8>  ExpireTimeProfileIdUint_t;
    // typedef bit<3>  PassNumberUint_t;
    // typedef bit<32> SecurityAssocIdUint_t;

    auto *portIdTypedef = new IR::Type_Typedef("PortId_t", IR::Type_Bits::get(32, false));
    auto *interfaceIdTypedef = new IR::Type_Typedef("InterfaceId_t", IR::Type_Bits::get(32, false));
    auto *multicastGroupTypedef =
        new IR::Type_Typedef("MulticastGroup_t", IR::Type_Bits::get(32, false));
    auto *mirrorSessionIdTypedef =
        new IR::Type_Typedef("MirrorSessionId_t", IR::Type_Bits::get(16, false));
    auto *mirrorSlotIdTypedef =
        new IR::Type_Typedef("MirrorSlotId_t", IR::Type_Bits::get(8, false));
    auto *classOfServiceTypedef =
        new IR::Type_Typedef("ClassOfService_t", IR::Type_Bits::get(8, false));
    auto *packetLengthTypedef =
        new IR::Type_Typedef("PacketLength_t", IR::Type_Bits::get(16, false));
    auto *multicastInstanceTypedef =
        new IR::Type_Typedef("MulticastInstance_t", IR::Type_Bits::get(8, false));
    auto *timestampTypedef = new IR::Type_Typedef("Timestamp_t", IR::Type_Bits::get(64, false));
    auto *flowIdTypedef = new IR::Type_Typedef("FlowId_t", IR::Type_Bits::get(32, false));
    auto *expireTimeProfileIdTypedef =
        new IR::Type_Typedef("ExpireTimeProfileId_t", IR::Type_Bits::get(8, false));
    auto *passNumberTypedef = new IR::Type_Typedef("PassNumber_t", IR::Type_Bits::get(3, false));
    auto *securityAssocIdTypedef =
        new IR::Type_Typedef("SecurityAssocId_t", IR::Type_Bits::get(32, false));

    P4Scope::addToScope(portIdTypedef);
    P4Scope::addToScope(interfaceIdTypedef);
    P4Scope::addToScope(multicastGroupTypedef);
    P4Scope::addToScope(mirrorSessionIdTypedef);
    P4Scope::addToScope(mirrorSlotIdTypedef);
    P4Scope::addToScope(classOfServiceTypedef);
    P4Scope::addToScope(packetLengthTypedef);
    P4Scope::addToScope(multicastInstanceTypedef);
    P4Scope::addToScope(timestampTypedef);
    P4Scope::addToScope(flowIdTypedef);
    P4Scope::addToScope(expireTimeProfileIdTypedef);
    P4Scope::addToScope(passNumberTypedef);
    P4Scope::addToScope(securityAssocIdTypedef);

    // start to assemble the model
    auto *objects = new IR::Vector<IR::Node>();

    objects->push_back(declarationGenerator().genEthernetHeaderType());

    // generate some declarations
    int typeDecls = Utils::getRandInt(Declarations::get().MIN_TYPE, Declarations::get().MAX_TYPE);
    for (int i = 0; i < typeDecls; ++i) {
        objects->push_back(declarationGenerator().genTypeDeclaration());
    }

    // generate struct Headers
    objects->push_back(declarationGenerator().genHeaderStruct());
    // generate struct metadata_t
    objects->push_back(generateUserMetadata());

    // generate some callables
    int callableDecls =
        Utils::getRandInt(Declarations::get().MIN_CALLABLES, Declarations::get().MAX_CALLABLES);
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
    objects->push_back(generateMainParserBlock());
    objects->push_back(generatePreControlBlock());
    objects->push_back(generateMainControlBlock());
    objects->push_back(generateMainDeparserBlock());

    // finally assemble the package
    objects->push_back(generateMainPackage());

    return new IR::P4Program(*objects);
}

}  // namespace P4::P4Tools::P4Smith::Nic
