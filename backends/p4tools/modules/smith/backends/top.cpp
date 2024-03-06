#include "backends/p4tools/modules/smith/backends/top.h"

#include <vector>

#include "backends/p4tools/modules/smith/backends/structFieldList.h"
#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/p4state.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "frontends/p4/toP4/toP4.h"

namespace P4Tools {

namespace P4Smith {

void Top::generate_includes(std::ostream *ostream) {
    *ostream << "#include <core.p4>\n";
    *ostream << "bit<3> max(in bit<3> val, in bit<3> bound) {\n";
    *ostream << "    return val < bound ? val : bound;\n";
    *ostream << "}\n";
}

IR::P4Parser *Top::gen_p() {
    IR::IndexedVector<IR::Declaration> parserLocals;

    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::None, "pkt", "packet_in"));
    params.push_back(Declarations().genParameter(IR::Direction::Out, "hdr", SYS_HDR_NAME));
    auto par_list = new IR::ParameterList(params);
    IR::Type_Parser *type_parser = new IR::Type_Parser("p", par_list);

    // add to the scope
    for (auto param : par_list->parameters) {
        P4Scope::add_to_scope(param);
        // add to the name_2_type
        // only add values that are !read-only to the modifiable types
        if (param->direction == IR::Direction::In) {
            P4Scope::add_lval(param->type, param->name.name, true);
        } else {
            P4Scope::add_lval(param->type, param->name.name, false);
        }
    }

    // generate decls
    for (int i = 0; i < 5; i++) {
        auto var_decl = Declarations().genVariableDeclaration();
        parserLocals.push_back(var_decl);
    }

    // generate states
    P4State::build_parser_tree();
    IR::IndexedVector<IR::ParserState> states = P4State::get_states();

    P4Scope::end_local_scope();

    // add to the whole scope
    auto p4parser = new IR::P4Parser("p", type_parser, parserLocals, states);
    P4Scope::add_to_scope(p4parser);
    return p4parser;
}

IR::P4Control *Top::gen_ingress() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "h", SYS_HDR_NAME));
    auto par_list = new IR::ParameterList(params);
    IR::Type_Control *type_ctrl = new IR::Type_Control("ingress", par_list);

    // add to the scope
    for (auto param : par_list->parameters) {
        P4Scope::add_to_scope(param);
        // add to the name_2_type
        // only add values that are !read-only to the modifiable types
        if (param->direction == IR::Direction::In) {
            P4Scope::add_lval(param->type, param->name.name, true);
        } else {
            P4Scope::add_lval(param->type, param->name.name, false);
        }
    }

    IR::IndexedVector<IR::Declaration> local_decls = Declarations().genLocalControlDecls();
    // apply body
    auto apply_block = Statements().genBlockStatement();

    // end of scope
    P4Scope::end_local_scope();

    // add to the whole scope
    IR::P4Control *p4ctrl = new IR::P4Control("ingress", type_ctrl, local_decls, apply_block);
    P4Scope::add_to_scope(p4ctrl);
    return p4ctrl;
}

IR::Declaration_Instance *Top::gen_main() {
    IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();

    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("p"))));
    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("ingress"))));
    auto package_name = new IR::Type_Name("top");
    return new IR::Declaration_Instance("main", package_name, args);
}

IR::Type_Parser *Top::gen_parser_type() {
    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::None, "b", "packet_in"));
    params.push_back(Declarations().genParameter(IR::Direction::Out, "hdr", SYS_HDR_NAME));
    auto par_list = new IR::ParameterList(params);
    return new IR::Type_Parser("Parser", par_list);
}

IR::Type_Control *Top::gen_ingress_type() {
    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "hdr", SYS_HDR_NAME));
    auto par_list = new IR::ParameterList(params);
    return new IR::Type_Control("Ingress", par_list);
}

IR::Type_Package *Top::gen_package() {
    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::None, "p", "Parser"));
    params.push_back(Declarations().genParameter(IR::Direction::None, "ig", "Ingress"));
    auto par_list = new IR::ParameterList(params);
    return new IR::Type_Package("top", par_list);
}

IR::P4Program *Top::gen() {
    // start to assemble the model
    auto objects = new IR::Vector<IR::Node>();

    objects->push_back(Declarations().genEthernetHeaderType());

    // generate some declarations
    int type_decls = getRndInt(DECL.MIN_TYPE, DECL.MAX_TYPE);
    for (int i = 0; i < type_decls; ++i) {
        objects->push_back(Declarations().genTypeDeclaration());
    }

    // generate struct Headers
    objects->push_back(Declarations().genHeaderStruct());

    // generate some callables
    int callable_decls = getRndInt(DECL.MIN_CALLABLES, DECL.MAX_CALLABLES);
    for (int i = 0; i < callable_decls; ++i) {
        std::vector<int64_t> percent = {70, 15, 10, 5};
        switch (randInt(percent)) {
            case 0: {
                objects->push_back(Declarations().genFunctionDeclaration());
                break;
            }
            case 1: {
                objects->push_back(Declarations().genActionDeclaration());
                break;
            }
            case 2: {
                objects->push_back(Declarations().genExternDeclaration());
                break;
            }
            case 3: {
                objects->push_back(Declarations().genControlDeclaration());
                break;
            }
        }
    }

    // generate all the necessary pipelines for the package
    objects->push_back(gen_p());
    objects->push_back(gen_ingress());

    // finally assemble the package
    objects->push_back(gen_parser_type());
    objects->push_back(gen_ingress_type());
    objects->push_back(gen_package());
    objects->push_back(gen_main());

    return new IR::P4Program(*objects);
}
}  // namespace P4Smith

}  // namespace P4Tools
