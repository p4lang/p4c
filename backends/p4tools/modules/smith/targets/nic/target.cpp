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

namespace p4c::P4Tools::P4Smith::Nic {

using namespace ::p4c::P4::literals;

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
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::addToScope(ret);
    name = new IR::ID("pna_pre_input_metadata_t");
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::addToScope(ret);
    name = new IR::ID("pna_pre_output_metadata_t");
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::addToScope(ret);
    name = new IR::ID("pna_main_input_metadata_t");
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::addToScope(ret);
    name = new IR::ID("pna_main_output_metadata_t");
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::addToScope(ret);
}

void setPnaDpdkProbabilities() {
    PCT.PARAMETER_NONEDIR_DERIVED_STRUCT = 0;
    PCT.PARAMETER_NONEDIR_DERIVED_HEADER = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_BOOL = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_ERROR = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_STRING = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_VARBIT = 0;
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
    objects->push_back(generateUserMetadata());

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
    objects->push_back(generateMainParserBlock());
    objects->push_back(generatePreControlBlock());
    objects->push_back(generateMainControlBlock());
    objects->push_back(generateMainDeparserBlock());

    // finally assemble the package
    objects->push_back(generateMainPackage());

    return new IR::P4Program(*objects);
}

}  // namespace p4c::P4Tools::P4Smith::Nic
