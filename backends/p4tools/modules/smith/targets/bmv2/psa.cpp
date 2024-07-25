#include "backends/p4tools/modules/smith/targets/bmv2/psa.h"

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
#include "backends/p4tools/modules/smith/targets/bmv2/target.h"
#include "backends/p4tools/modules/smith/util/util.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/node.h"
#include "ir/vector.h"

namespace P4::P4Tools::P4Smith::BMv2 {

using namespace ::P4::literals;

/* =============================================================================================
 *  Bmv2PsaSmithTarget implementation
 * ============================================================================================= */

Bmv2PsaSmithTarget::Bmv2PsaSmithTarget() : AbstractBMv2SmithTarget("bmv2", "psa") {}

void Bmv2PsaSmithTarget::make() {
    static Bmv2PsaSmithTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new Bmv2PsaSmithTarget();
    }
}

IR::P4Parser *Bmv2PsaSmithTarget::generateIngressParserBlock() const {
    IR::IndexedVector<IR::Declaration> parserLocals;
    P4Scope::startLocalScope();

    // generate type_parser !that this is labeled "p"
    IR::IndexedVector<IR::Parameter> params;

    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "pkt"_cs, "packet_in"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::Out, "hdr"_cs, SYS_HDR_NAME));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "user_meta"_cs, "metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "istd"_cs,
                                                         "psa_ingress_parser_input_metadata_t"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::In, "resubmit_meta"_cs, "empty_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "recirculate_meta"_cs,
                                                         "empty_t"_cs));
    auto *parList = new IR::ParameterList(params);

    auto *tpParser = new IR::Type_Parser("IngressParserImpl", parList);

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
    auto *p4parser = new IR::P4Parser("IngressParserImpl", tpParser, parserLocals, states);
    P4Scope::addToScope(p4parser);
    return p4parser;
}

IR::P4Control *Bmv2PsaSmithTarget::generateIngressBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "h"_cs, SYS_HDR_NAME));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "user_meta"_cs, "metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "istd"_cs,
                                                         "psa_ingress_input_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "ostd"_cs,
                                                         "psa_ingress_output_metadata_t"_cs));

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
    auto *outputPort = new IR::PathExpression("ostd.egress_port");
    // PSA requires explicit casts of the output port variable
    // actually !sure why this is required...
    auto *outPortCast = new IR::Cast(new IR::Type_Name("PortId_t"), new IR::Constant(0));
    auto *assign = new IR::AssignmentStatement(outputPort, outPortCast);
    // some hack to insert the expression at the beginning
    auto it = applyBlock->components.begin();
    applyBlock->components.insert(it, assign);
    // also make sure the packet isn't dropped...
    outputPort = new IR::PathExpression("ostd.drop");
    // PSA requires explicit casts of the output port variable
    // actually !sure why this is required...
    assign = new IR::AssignmentStatement(outputPort, new IR::BoolLiteral(false));
    // some hack to insert the expression at the beginning
    it = applyBlock->components.begin();
    applyBlock->components.insert(it, assign);
    // end of scope
    P4Scope::endLocalScope();

    // add to the whole scope
    auto *p4ctrl = new IR::P4Control("ingress", typeCtrl, localDecls, applyBlock);
    P4Scope::addToScope(p4ctrl);
    return p4ctrl;
}

IR::MethodCallStatement *genDeparserEmitCall() {
    auto *call = new IR::PathExpression("pkt.emit");
    auto *args = new IR::Vector<IR::Argument>();

    args->push_back(new IR::Argument(new IR::PathExpression("h")));

    auto *mce = new IR::MethodCallExpression(call, args);
    auto *mst = new IR::MethodCallStatement(mce);
    return mst;
}

IR::P4Control *Bmv2PsaSmithTarget::generateIngressDeparserBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "pkt"_cs, "packet_out"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::Out, "clone_i2e_meta"_cs, "empty_t"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::Out, "resubmit_meta"_cs, "empty_t"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::Out, "normal_meta"_cs, "empty_t"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "h"_cs, SYS_HDR_NAME));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::In, "user_meta"_cs, "metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "istd"_cs,
                                                         "psa_ingress_output_metadata_t"_cs));
    auto *parList = new IR::ParameterList(params);
    auto *typeCtrl = new IR::Type_Control("IngressDeparserImpl", parList);
    IR::IndexedVector<IR::Declaration> localDecls;
    auto *blkStat = new IR::BlockStatement();
    blkStat->push_back(genDeparserEmitCall());

    return new IR::P4Control("IngressDeparserImpl", typeCtrl, localDecls, blkStat);
}

IR::P4Parser *Bmv2PsaSmithTarget::generateEgressParserBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "pkt"_cs, "packet_in"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::Out, "hdr"_cs, SYS_HDR_NAME));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "user_meta"_cs, "metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "istd"_cs,
                                                         "psa_egress_parser_input_metadata_t"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::In, "normal_meta"_cs, "empty_t"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::In, "clone_i2e_meta"_cs, "empty_t"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::In, "clone_e2e_meta"_cs, "empty_t"_cs));
    auto *parList = new IR::ParameterList(params);
    auto *typeParser = new IR::Type_Parser("EgressParserImpl", parList);
    IR::IndexedVector<IR::Declaration> localDecls;
    // TODO(fruffy): this hacky. FIX
    // generate states
    IR::IndexedVector<IR::ParserState> states;
    IR::IndexedVector<IR::StatOrDecl> components;
    IR::Expression *transition = new IR::PathExpression("accept");
    auto *startState = new IR::ParserState("start", components, transition);
    states.push_back(startState);
    return new IR::P4Parser("EgressParserImpl", typeParser, localDecls, states);
}

IR::P4Control *Bmv2PsaSmithTarget::generateEgressBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "h"_cs, SYS_HDR_NAME));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "user_meta"_cs, "metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "istd"_cs,
                                                         "psa_egress_input_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "ostd"_cs,
                                                         "psa_egress_output_metadata_t"_cs));
    auto *parList = new IR::ParameterList(params);
    auto *typeCtrl = new IR::Type_Control("egress", parList);
    IR::IndexedVector<IR::Declaration> localDecls;
    auto *blkStat = new IR::BlockStatement();

    return new IR::P4Control("egress", typeCtrl, localDecls, blkStat);
}

IR::P4Control *Bmv2PsaSmithTarget::generateEgressDeparserBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "pkt"_cs, "packet_out"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::Out, "clone_e2e_meta"_cs, "empty_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::Out, "recirculate_meta"_cs,
                                                         "empty_t"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "h"_cs, SYS_HDR_NAME));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::In, "user_meta"_cs, "metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "istd"_cs,
                                                         "psa_egress_output_metadata_t"_cs));
    params.push_back(declarationGenerator().genParameter(
        IR::Direction::In, "edstd"_cs, "psa_egress_deparser_input_metadata_t"_cs));
    auto *parList = new IR::ParameterList(params);
    auto *typeCtrl = new IR::Type_Control("EgressDeparserImpl", parList);
    IR::IndexedVector<IR::Declaration> localDecls;
    auto *blkStat = new IR::BlockStatement();
    blkStat->push_back(genDeparserEmitCall());

    return new IR::P4Control("EgressDeparserImpl", typeCtrl, localDecls, blkStat);
}

namespace {

IR::Declaration_Instance *generateIngressPipeDeclaration() {
    auto *args = new IR::Vector<IR::Argument>();

    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("IngressParserImpl"))));
    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("ingress"))));
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("IngressDeparserImpl"))));
    auto *packageName = new IR::Type_Name("IngressPipeline");
    return new IR::Declaration_Instance("ip", packageName, args);
}

IR::Declaration_Instance *generateEgressPipeDeclaration() {
    auto *args = new IR::Vector<IR::Argument>();

    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("EgressParserImpl"))));
    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("egress"))));
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("EgressDeparserImpl"))));
    auto *packageName = new IR::Type_Name("EgressPipeline");
    return new IR::Declaration_Instance("ep", packageName, args);
}

IR::Declaration_Instance *generateMainPsaPackage() {
    auto *args = new IR::Vector<IR::Argument>();
    args->push_back(new IR::Argument(new IR::TypeNameExpression("ip")));
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("PacketReplicationEngine"))));
    args->push_back(new IR::Argument(new IR::TypeNameExpression("ep")));
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("BufferingQueueingEngine"))));
    auto *packageName = new IR::Type_Name("PSA_Switch");
    return new IR::Declaration_Instance("main", packageName, args);
}

IR::Type_Struct *generateUserMetadataType() {
    // Do !emit meta fields for now, no need
    IR::IndexedVector<IR::StructField> fields;
    auto *ret = new IR::Type_Struct("metadata_t", fields);
    P4Scope::addToScope(ret);
    return ret;
}

IR::Type_Struct *generateEmptyMetadataType() {
    // Do !emit meta fields for now, no need
    IR::IndexedVector<IR::StructField> fields;
    auto *ret = new IR::Type_Struct("empty_t", fields);
    P4Scope::addToScope(ret);
    return ret;
}

void generatePsaMetadata() {
    IR::ID *name = nullptr;
    IR::Type_Struct *ret = nullptr;
    IR::IndexedVector<IR::StructField> fields;

    name = new IR::ID("psa_ingress_parser_input_metadata_t");
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::addToScope(ret);
    name = new IR::ID("psa_ingress_input_metadata_t");
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::addToScope(ret);
    name = new IR::ID("psa_ingress_output_metadata_t");
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::addToScope(ret);
    name = new IR::ID("psa_egress_input_metadata_t");
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::addToScope(ret);
    name = new IR::ID("psa_egress_output_metadata_t");
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::addToScope(ret);
}

void setBmv2PsaProbabilities() {
    PCT.PARAMETER_NONEDIR_DERIVED_STRUCT = 0;
    PCT.PARAMETER_NONEDIR_DERIVED_HEADER = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_BOOL = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_ERROR = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_STRING = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_VARBIT = 0;
}

}  // namespace

int Bmv2PsaSmithTarget::writeTargetPreamble(std::ostream *ostream) const {
    *ostream << "#include <core.p4>\n";
    *ostream << "#include <psa.p4>\n\n";
    *ostream << "bit<3> max(in bit<3> val, in bit<3> bound) {\n";
    *ostream << "    return val < bound ? val : bound;\n";
    *ostream << "}\n";
    return EXIT_SUCCESS;
}

const IR::P4Program *Bmv2PsaSmithTarget::generateP4Program() const {
    P4Scope::startLocalScope();

    // insert banned structures
    P4Scope::notInitializedStructs.insert("psa_ingress_parser_input_metadata_t"_cs);
    P4Scope::notInitializedStructs.insert("psa_ingress_input_metadata_t"_cs);
    P4Scope::notInitializedStructs.insert("psa_ingress_output_metadata_t"_cs);
    P4Scope::notInitializedStructs.insert("psa_egress_input_metadata_t"_cs);
    P4Scope::notInitializedStructs.insert("psa_egress_output_metadata_t"_cs);
    // set psa-specific probabilities
    setBmv2PsaProbabilities();
    // insert some dummy metadata
    generatePsaMetadata();

    // start to assemble the model
    auto *objects = new IR::Vector<IR::Node>();

    objects->push_back(declarationGenerator().genEthernetHeaderType());

    // generate some declarations
    int typeDecls = Utils::getRandInt(DECL.MIN_TYPE, DECL.MAX_TYPE);
    for (int i = 0; i < typeDecls; ++i) {
        objects->push_back(declarationGenerator().genTypeDeclaration());
    }

    // generate struct Headers
    objects->push_back(declarationGenerator().genHeaderStruct());
    // generate struct metadata_t
    objects->push_back(generateUserMetadataType());
    // generate struct empty_t
    objects->push_back(generateEmptyMetadataType());

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
    objects->push_back(generateIngressPipeDeclaration());
    objects->push_back(generateEgressPipeDeclaration());
    objects->push_back(generateMainPsaPackage());

    return new IR::P4Program(*objects);
}

}  // namespace P4::P4Tools::P4Smith::BMv2
