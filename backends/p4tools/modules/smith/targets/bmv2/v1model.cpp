#include "backends/p4tools/modules/smith/targets/bmv2/v1model.h"

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

using namespace P4::literals;

/* =============================================================================================
 *  Bmv2V1modelSmithTarget implementation
 * ============================================================================================= */

Bmv2V1modelSmithTarget::Bmv2V1modelSmithTarget() : AbstractBMv2SmithTarget("bmv2", "v1model") {}

void Bmv2V1modelSmithTarget::make() {
    static Bmv2V1modelSmithTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new Bmv2V1modelSmithTarget();
    }
}

IR::P4Parser *Bmv2V1modelSmithTarget::generateParserBlock() const {
    IR::IndexedVector<IR::Declaration> parserLocals;

    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "pkt"_cs, "packet_in"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::Out, "hdr"_cs, SYS_HDR_NAME));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "m"_cs, "Meta"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "sm"_cs,
                                                         "standard_metadata_t"_cs));
    auto *parList = new IR::ParameterList(params);
    auto *typeParser = new IR::Type_Parser("p", parList);

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

    // generate decls
    /*        for (int i = 0; i < 5; i++) {
                auto var_decl = variableDeclaration::gen();
                parserLocals.push_back(var_decl);
            }*/

    // generate states
    IR::IndexedVector<IR::ParserState> states;
    states.push_back(parserGenerator().genStartState());
    states.push_back(parserGenerator().genHdrStates());

    P4Scope::endLocalScope();

    // add to the whole scope
    auto *p4parser = new IR::P4Parser("p", typeParser, parserLocals, states);
    P4Scope::addToScope(p4parser);
    return p4parser;
}

IR::P4Control *Bmv2V1modelSmithTarget::generateIngressBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "h"_cs, SYS_HDR_NAME));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "m"_cs, "Meta"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "sm"_cs,
                                                         "standard_metadata_t"_cs));
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

    // end of scope
    P4Scope::endLocalScope();

    // add to the whole scope
    auto *p4ctrl = new IR::P4Control("ingress", typeCtrl, localDecls, applyBlock);
    P4Scope::addToScope(p4ctrl);
    return p4ctrl;
}

IR::P4Control *Bmv2V1modelSmithTarget::generateVerifyBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "h"_cs, SYS_HDR_NAME));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "m"_cs, "Meta"_cs));
    auto *parList = new IR::ParameterList(params);
    auto *typeCtrl = new IR::Type_Control("vrfy", parList);
    IR::IndexedVector<IR::Declaration> localDecls;
    auto *blkStat = new IR::BlockStatement();

    return new IR::P4Control("vrfy", typeCtrl, localDecls, blkStat);
}

IR::P4Control *Bmv2V1modelSmithTarget::generateUpdateBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "h"_cs, SYS_HDR_NAME));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "m"_cs, "Meta"_cs));
    auto *parList = new IR::ParameterList(params);
    auto *typeCtrl = new IR::Type_Control("update", parList);
    IR::IndexedVector<IR::Declaration> localDecls;
    auto *blkStat = new IR::BlockStatement();

    return new IR::P4Control("update", typeCtrl, localDecls, blkStat);
}

IR::P4Control *Bmv2V1modelSmithTarget::generateEgressBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "h"_cs, SYS_HDR_NAME));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "m"_cs, "Meta"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::InOut, "sm"_cs,
                                                         "standard_metadata_t"_cs));
    auto *parList = new IR::ParameterList(params);
    auto *typeCtrl = new IR::Type_Control("egress", parList);
    IR::IndexedVector<IR::Declaration> localDecls;
    auto *blkStat = new IR::BlockStatement();

    return new IR::P4Control("egress", typeCtrl, localDecls, blkStat);
}

IR::MethodCallStatement *generateDeparserEmitCall() {
    auto *call = new IR::PathExpression("pkt.emit");
    auto *args = new IR::Vector<IR::Argument>();

    args->push_back(new IR::Argument(new IR::PathExpression("h")));

    auto *mce = new IR::MethodCallExpression(call, args);
    auto *mst = new IR::MethodCallStatement(mce);
    return mst;
}

IR::P4Control *Bmv2V1modelSmithTarget::generateDeparserBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "pkt"_cs, "packet_out"_cs));
    params.push_back(declarationGenerator().genParameter(IR::Direction::In, "h"_cs, SYS_HDR_NAME));
    auto *parList = new IR::ParameterList(params);
    auto *typeCtrl = new IR::Type_Control("deparser"_cs, parList);
    IR::IndexedVector<IR::Declaration> localDecls;
    auto *blkStat = new IR::BlockStatement();
    blkStat->push_back(generateDeparserEmitCall());

    return new IR::P4Control("deparser"_cs, typeCtrl, localDecls, blkStat);
}

IR::Declaration_Instance *generateMainV1ModelPackage() {
    auto *args = new IR::Vector<IR::Argument>();

    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("p"))));
    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("vrfy"))));
    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("ingress"))));
    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("egress"))));
    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("update"))));
    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("deparser"))));
    auto *packageName = new IR::Type_Name("V1Switch");
    return new IR::Declaration_Instance("main", packageName, args);
}

IR::Type_Struct *generateMetadataStructure() {
    // Do !emit meta fields for now, no need
    // FIXME: Design a way to emit these that plays nicely with all targets
    // auto   sfl   = new structFieldList(STRUCT, name->name);
    // IR::IndexedVector< IR::StructField > fields = sfl->gen(Utils::getRandInt(1,
    // 5));
    IR::IndexedVector<IR::StructField> fields;

    auto *ret = new IR::Type_Struct("Meta", fields);

    P4Scope::addToScope(ret);

    return ret;
}

IR::IndexedVector<IR::StructField> generateStandardMetadataVector() {
    IR::IndexedVector<IR::StructField> fields;

    // IR::ID   *name;
    // IR::Type *tp;

    /*
       name = new IR::ID("ingress_port");
       tp = IR::Type_Bits::get(9, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("egress_spec");
       tp = IR::Type_Bits::get(9, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("egress_port");
       tp = IR::Type_Bits::get(9, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("instance_type");
       tp = IR::Type_Bits::get(32, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("packet_length");
       tp = IR::Type_Bits::get(32, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("enq_timestamp");
       tp = IR::Type_Bits::get(32, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("enq_qdepth");
       tp = IR::Type_Bits::get(19, false);
       fields.push_back(new IR::StructField(*name, tp));
       // name = new IR::ID("dep_timedelta");
       // tp = IR::Type_Bits::get(32, false);
       // fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("deq_qdepth");
       tp = IR::Type_Bits::get(19, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("ingress_global_timestamp");
       tp = IR::Type_Bits::get(48, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("egress_global_timestamp");
       tp = IR::Type_Bits::get(48, false);
       fields.push_back(new IR::StructField(*name, tp));
       name = new IR::ID("egress_rid");
       tp = IR::Type_Bits::get(16, false);
       fields.push_back(new IR::StructField(*name, tp));
       // Tao: error is omitted here
       name = new IR::ID("priority");
       tp = IR::Type_Bits::get(3, false);
       fields.push_back(new IR::StructField(*name, tp));
     */

    return fields;
}

IR::Type_Struct *generateStandardMetadataStructure() {
    auto fields = generateStandardMetadataVector();

    auto *ret = new IR::Type_Struct("standard_metadata_t", fields);

    P4Scope::addToScope(ret);

    return ret;
}

void setProbabilitiesforBmv2V1model() {
    PCT.PARAMETER_NONEDIR_DERIVED_STRUCT = 0;
    PCT.PARAMETER_NONEDIR_DERIVED_HEADER = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_BOOL = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_ERROR = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_STRING = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_VARBIT = 0;
    // V1Model does !support headers that are !multiples of 8
    PCT.STRUCTTYPEDECLARATION_BASETYPE_BOOL = 0;
    // V1Model requires headers to be byte-aligned
    P4Scope::req.byte_align_headers = true;
}

int Bmv2V1modelSmithTarget::writeTargetPreamble(std::ostream *ostream) const {
    *ostream << "#include <core.p4>\n";
    *ostream << "#include <v1model.p4>\n\n";
    *ostream << "bit<3> max(in bit<3> val, in bit<3> bound) {\n";
    *ostream << "    return val < bound ? val : bound;\n";
    *ostream << "}\n";
    return EXIT_SUCCESS;
}

const IR::P4Program *Bmv2V1modelSmithTarget::generateP4Program() const {
    P4Scope::startLocalScope();

    // insert banned structures
    P4Scope::notInitializedStructs.insert("standard_metadata_t"_cs);
    // Set bmv2-v1model-specific probabilities.
    setProbabilitiesforBmv2V1model();

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

    // generate struct Meta
    objects->push_back(generateMetadataStructure());
    // insert standard_metadata_t
    generateStandardMetadataStructure();

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
    objects->push_back(generateParserBlock());
    objects->push_back(generateIngressBlock());
    objects->push_back(generateVerifyBlock());
    objects->push_back(generateUpdateBlock());
    objects->push_back(generateEgressBlock());
    objects->push_back(generateDeparserBlock());

    // finally assemble the package
    objects->push_back(generateMainV1ModelPackage());

    return new IR::P4Program(*objects);
}

}  // namespace P4::P4Tools::P4Smith::BMv2
