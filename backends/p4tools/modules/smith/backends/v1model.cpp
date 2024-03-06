#include "backends/p4tools/modules/smith/backends/v1model.h"

#include <vector>

#include "backends/p4tools/modules/smith/backends/structFieldList.h"
#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/p4state.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "frontends/p4/toP4/toP4.h"

namespace P4Tools {

namespace P4Smith {

void V1Model::generate_includes(std::ostream *ostream) {
    *ostream << "#include <core.p4>\n";
    *ostream << "#include <v1model.p4>\n\n";
    *ostream << "bit<3> max(in bit<3> val, in bit<3> bound) {\n";
    *ostream << "    return val < bound ? val : bound;\n";
    *ostream << "}\n";
}

IR::P4Parser *V1Model::gen_p() {
    IR::IndexedVector<IR::Declaration> parserLocals;

    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::None, "pkt", "packet_in"));
    params.push_back(Declarations().genParameter(IR::Direction::Out, "hdr", SYS_HDR_NAME));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "m", "Meta"));
    params.push_back(
        Declarations().genParameter(IR::Direction::InOut, "sm", "standard_metadata_t"));
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
    /*        for (int i = 0; i < 5; i++) {
                auto var_decl = variableDeclaration::gen();
                parserLocals.push_back(var_decl);
            }*/

    // generate states
    IR::IndexedVector<IR::ParserState> states;
    states.push_back(P4State::gen_start_state());
    states.push_back(P4State::gen_hdr_states());

    P4Scope::end_local_scope();

    // add to the whole scope
    auto p4parser = new IR::P4Parser("p", type_parser, parserLocals, states);
    P4Scope::add_to_scope(p4parser);
    return p4parser;
}

IR::P4Control *V1Model::gen_ingress() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "h", SYS_HDR_NAME));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "m", "Meta"));
    params.push_back(
        Declarations().genParameter(IR::Direction::InOut, "sm", "standard_metadata_t"));
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

IR::P4Control *V1Model::gen_vrfy() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "h", SYS_HDR_NAME));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "m", "Meta"));
    auto par_list = new IR::ParameterList(params);
    auto type_ctrl = new IR::Type_Control("vrfy", par_list);
    IR::IndexedVector<IR::Declaration> local_decls;
    auto blk_stat = new IR::BlockStatement();

    return new IR::P4Control("vrfy", type_ctrl, local_decls, blk_stat);
}

IR::P4Control *V1Model::gen_update() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "h", SYS_HDR_NAME));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "m", "Meta"));
    auto par_list = new IR::ParameterList(params);
    auto type_ctrl = new IR::Type_Control("update", par_list);
    IR::IndexedVector<IR::Declaration> local_decls;
    auto blk_stat = new IR::BlockStatement();

    return new IR::P4Control("update", type_ctrl, local_decls, blk_stat);
}

IR::P4Control *V1Model::gen_egress() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "h", SYS_HDR_NAME));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "m", "Meta"));
    params.push_back(
        Declarations().genParameter(IR::Direction::InOut, "sm", "standard_metadata_t"));
    auto par_list = new IR::ParameterList(params);
    auto type_ctrl = new IR::Type_Control("egress", par_list);
    IR::IndexedVector<IR::Declaration> local_decls;
    auto blk_stat = new IR::BlockStatement();

    return new IR::P4Control("egress", type_ctrl, local_decls, blk_stat);
}

IR::MethodCallStatement *V1Model::gen_deparser_emit_call() {
    auto call = new IR::PathExpression("pkt.emit");
    IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();

    args->push_back(new IR::Argument(new IR::PathExpression("h")));

    auto mce = new IR::MethodCallExpression(call, args);
    auto mst = new IR::MethodCallStatement(mce);
    return mst;
}

IR::P4Control *V1Model::gen_deparser() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::None, "pkt", "packet_out"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "h", SYS_HDR_NAME));
    auto par_list = new IR::ParameterList(params);
    auto type_ctrl = new IR::Type_Control("deparser", par_list);
    IR::IndexedVector<IR::Declaration> local_decls;
    auto blk_stat = new IR::BlockStatement();
    blk_stat->push_back(gen_deparser_emit_call());

    return new IR::P4Control("deparser", type_ctrl, local_decls, blk_stat);
}

IR::Declaration_Instance *V1Model::gen_main() {
    IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();

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
    auto package_name = new IR::Type_Name("V1Switch");
    return new IR::Declaration_Instance("main", package_name, args);
}

IR::Type_Struct *V1Model::gen_meta() {
    // Do !emit meta fields for now, no need
    // FIXME: Design a way to emit these that plays nicely with all targets
    // auto   sfl   = new structFieldList(STRUCT, name->name);
    // IR::IndexedVector< IR::StructField > fields = sfl->gen(getRndInt(1,
    // 5));
    IR::IndexedVector<IR::StructField> fields;

    auto ret = new IR::Type_Struct("Meta", fields);

    P4Scope::add_to_scope(ret);

    return ret;
}

IR::Type_Struct *V1Model::gen_standard_metadata_t() {
    auto fields = structFieldList::gen_sm();

    auto ret = new IR::Type_Struct("standard_metadata_t", fields);

    P4Scope::add_to_scope(ret);

    return ret;
}

void V1Model::set_probabilities() {
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

IR::P4Program *V1Model::gen() {
    // insert banned structures
    P4Scope::not_initialized_structs.insert("standard_metadata_t");
    // set v1model-specific probabilities
    set_probabilities();

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

    // generate struct Meta
    objects->push_back(gen_meta());
    // insert standard_metadata_t
    gen_standard_metadata_t();

    // generate some callables
    int callable_decls = getRndInt(DECL.MIN_CALLABLES, DECL.MAX_CALLABLES);
    for (int i = 0; i < callable_decls; ++i) {
        std::vector<int64_t> percent = {80, 15, 0, 5};
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
    objects->push_back(gen_vrfy());
    objects->push_back(gen_update());
    objects->push_back(gen_egress());
    objects->push_back(gen_deparser());

    // finally assemble the package
    objects->push_back(gen_main());

    return new IR::P4Program(*objects);
}
}  // namespace P4Smith

}  // namespace P4Tools
