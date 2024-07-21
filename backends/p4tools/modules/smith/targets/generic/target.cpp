#include "backends/p4tools/modules/smith/targets/generic/target.h"

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

namespace p4c::P4Tools::P4Smith::Generic {

using namespace ::p4c::P4::literals;

/* =============================================================================================
 *  AbstractGenericSmithTarget implementation
 * ============================================================================================= */

AbstractGenericSmithTarget::AbstractGenericSmithTarget(const std::string &deviceName,
                                                       const std::string &archName)
    : SmithTarget(deviceName, archName) {}

/* =============================================================================================
 *  GenericCoreSmithTarget implementation
 * ============================================================================================= */

GenericCoreSmithTarget::GenericCoreSmithTarget() : AbstractGenericSmithTarget("generic", "core") {}

void GenericCoreSmithTarget::make() {
    static GenericCoreSmithTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new GenericCoreSmithTarget();
    }
}

GenericCoreSmithTarget *GenericCoreSmithTarget::getInstance() {
    static GenericCoreSmithTarget *INSTANCE = nullptr;
    if (INSTANCE == nullptr) {
        INSTANCE = new GenericCoreSmithTarget();
    }
    return INSTANCE;
}

IR::P4Parser *GenericCoreSmithTarget::generateParserBlock() const {
    IR::IndexedVector<IR::Declaration> parserLocals;

    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "pkt"_cs, "packet_in"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::Out, "hdr"_cs, SYS_HDR_NAME));
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
    for (int i = 0; i < 5; i++) {
        auto *varDecl = declarationGenerator().genVariableDeclaration();
        parserLocals.push_back(varDecl);
    }

    // generate states
    parserGenerator().buildParserTree();
    IR::IndexedVector<IR::ParserState> states = parserGenerator().getStates();

    P4Scope::endLocalScope();

    // add to the whole scope
    auto *p4parser = new IR::P4Parser("p", typeParser, parserLocals, states);
    P4Scope::addToScope(p4parser);
    return p4parser;
}

IR::P4Control *GenericCoreSmithTarget::generateIngressBlock() const {
    // start of new scope
    P4Scope::startLocalScope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "h"_cs, SYS_HDR_NAME));
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

IR::Declaration_Instance *GenericCoreSmithTarget::generateMainPackage() {
    auto *args = new IR::Vector<IR::Argument>();

    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("p"))));
    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("ingress"))));
    auto *packageName = new IR::Type_Name("top");
    return new IR::Declaration_Instance("main", packageName, args);
}

IR::Type_Parser *GenericCoreSmithTarget::generateParserBlockType() const {
    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "b"_cs, "packet_in"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::Out, "hdr"_cs, SYS_HDR_NAME));
    auto *parList = new IR::ParameterList(params);
    return new IR::Type_Parser("Parser", parList);
}

IR::Type_Control *GenericCoreSmithTarget::generateIngressBlockType() const {
    IR::IndexedVector<IR::Parameter> params;
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::InOut, "hdr"_cs, SYS_HDR_NAME));
    auto *parList = new IR::ParameterList(params);
    return new IR::Type_Control("Ingress", parList);
}

IR::Type_Package *GenericCoreSmithTarget::generatePackageType() const {
    IR::IndexedVector<IR::Parameter> params;
    params.push_back(declarationGenerator().genParameter(IR::Direction::None, "p"_cs, "Parser"_cs));
    params.push_back(
        declarationGenerator().genParameter(IR::Direction::None, "ig"_cs, "Ingress"_cs));
    auto *parList = new IR::ParameterList(params);
    return new IR::Type_Package("top", parList);
}

int GenericCoreSmithTarget::writeTargetPreamble(std::ostream *ostream) const {
    *ostream << "#include <core.p4>\n";
    *ostream << "bit<3> max(in bit<3> val, in bit<3> bound) {\n";
    *ostream << "    return val < bound ? val : bound;\n";
    *ostream << "}\n";
    return EXIT_SUCCESS;
}

const IR::P4Program *GenericCoreSmithTarget::generateP4Program() const {
    P4Scope::startLocalScope();

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

    // generate some callables
    int callableDecls = Utils::getRandInt(DECL.MIN_CALLABLES, DECL.MAX_CALLABLES);
    for (int i = 0; i < callableDecls; ++i) {
        std::vector<int64_t> percent = {70, 15, 10, 5};
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

    // finally assemble the package
    objects->push_back(generateParserBlockType());
    objects->push_back(generateIngressBlockType());
    objects->push_back(generatePackageType());
    objects->push_back(generateMainPackage());

    return new IR::P4Program(*objects);
}

}  // namespace p4c::P4Tools::P4Smith::Generic
