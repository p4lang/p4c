#include "backends/p4tools/modules/smith/backends/psa.h"

#include <vector>

#include "backends/p4tools/modules/smith/backends/structFieldList.h"
#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/p4state.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "frontends/p4/toP4/toP4.h"

namespace P4Tools {

namespace P4Smith {

void PSA::generate_includes(std::ostream *ostream) {
    *ostream << "#include <core.p4>\n";
    *ostream << "#include <psa.p4>\n\n";
    *ostream << "bit<3> max(in bit<3> val, in bit<3> bound) {\n";
    *ostream << "    return val < bound ? val : bound;\n";
    *ostream << "}\n";
}

IR::P4Parser *PSA::gen_switch_ingress_parser() {
    IR::IndexedVector<IR::Declaration> parserLocals;
    P4Scope::start_local_scope();

    // generate type_parser !that this is labeled "p"
    IR::IndexedVector<IR::Parameter> params;

    params.push_back(Declarations().genParameter(IR::Direction::None, "pkt", "packet_in"));
    params.push_back(Declarations().genParameter(IR::Direction::Out, "hdr", SYS_HDR_NAME));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "user_meta", "metadata_t"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "istd",
                                                 "psa_ingress_parser_input_metadata_t"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "resubmit_meta", "empty_t"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "recirculate_meta", "empty_t"));
    auto par_list = new IR::ParameterList(params);

    IR::Type_Parser *tp_parser = new IR::Type_Parser("IngressParserImpl", par_list);

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

    // // generate decls
    // for (int i = 0; i < 5; i++) {
    //     auto var_decl = variableDeclaration::gen();
    //     parserLocals.push_back(var_decl);
    // }

    // generate states
    IR::IndexedVector<IR::ParserState> states;
    auto start_state = P4State::gen_start_state();
    states.push_back(start_state);
    states.push_back(P4State::gen_hdr_states());

    P4Scope::end_local_scope();

    // add to the whole scope
    auto p4parser = new IR::P4Parser("IngressParserImpl", tp_parser, parserLocals, states);
    P4Scope::add_to_scope(p4parser);
    return p4parser;
}

IR::P4Control *PSA::gen_switch_ingress() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "h", SYS_HDR_NAME));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "user_meta", "metadata_t"));
    params.push_back(
        Declarations().genParameter(IR::Direction::In, "istd", "psa_ingress_input_metadata_t"));
    params.push_back(
        Declarations().genParameter(IR::Direction::InOut, "ostd", "psa_ingress_output_metadata_t"));

    auto par_list = new IR::ParameterList(params);
    auto type_ctrl = new IR::Type_Control("ingress", par_list);

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
    // hardcode the output port to be zero
    auto output_port = new IR::PathExpression("ostd.egress_port");
    // PSA requires explicit casts of the output port variable
    // actually !sure why this is required...
    auto out_port_cast = new IR::Cast(new IR::Type_Name("PortId_t"), new IR::Constant(0));
    auto assign = new IR::AssignmentStatement(output_port, out_port_cast);
    // some hack to insert the expression at the beginning
    auto it = apply_block->components.begin();
    apply_block->components.insert(it, assign);
    // also make sure the packet isn't dropped...
    output_port = new IR::PathExpression("ostd.drop");
    // PSA requires explicit casts of the output port variable
    // actually !sure why this is required...
    assign = new IR::AssignmentStatement(output_port, new IR::BoolLiteral(false));
    // some hack to insert the expression at the beginning
    it = apply_block->components.begin();
    apply_block->components.insert(it, assign);
    // end of scope
    P4Scope::end_local_scope();

    // add to the whole scope
    IR::P4Control *p4ctrl = new IR::P4Control("ingress", type_ctrl, local_decls, apply_block);
    P4Scope::add_to_scope(p4ctrl);
    return p4ctrl;
}

IR::MethodCallStatement *PSA::gen_deparser_emit_call() {
    auto call = new IR::PathExpression("pkt.emit");
    IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();

    args->push_back(new IR::Argument(new IR::PathExpression("h")));

    auto mce = new IR::MethodCallExpression(call, args);
    auto mst = new IR::MethodCallStatement(mce);
    return mst;
}

IR::P4Control *PSA::gen_switch_ingress_deparser() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::None, "pkt", "packet_out"));
    params.push_back(Declarations().genParameter(IR::Direction::Out, "clone_i2e_meta", "empty_t"));
    params.push_back(Declarations().genParameter(IR::Direction::Out, "resubmit_meta", "empty_t"));
    params.push_back(Declarations().genParameter(IR::Direction::Out, "normal_meta", "empty_t"));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "h", SYS_HDR_NAME));
    params.push_back(Declarations().genParameter(IR::Direction::In, "user_meta", "metadata_t"));
    params.push_back(
        Declarations().genParameter(IR::Direction::In, "istd", "psa_ingress_output_metadata_t"));
    auto par_list = new IR::ParameterList(params);
    auto type_ctrl = new IR::Type_Control("IngressDeparserImpl", par_list);
    IR::IndexedVector<IR::Declaration> local_decls;
    auto blk_stat = new IR::BlockStatement();
    blk_stat->push_back(gen_deparser_emit_call());

    return new IR::P4Control("IngressDeparserImpl", type_ctrl, local_decls, blk_stat);
}

IR::P4Parser *PSA::gen_switch_egress_parser() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::None, "pkt", "packet_in"));
    params.push_back(Declarations().genParameter(IR::Direction::Out, "hdr", SYS_HDR_NAME));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "user_meta", "metadata_t"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "istd",
                                                 "psa_egress_parser_input_metadata_t"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "normal_meta", "empty_t"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "clone_i2e_meta", "empty_t"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "clone_e2e_meta", "empty_t"));
    auto par_list = new IR::ParameterList(params);
    auto type_parser = new IR::Type_Parser("EgressParserImpl", par_list);
    IR::IndexedVector<IR::Declaration> local_decls;
    // TODO: this hacky. FIX
    // generate states
    IR::IndexedVector<IR::ParserState> states;
    IR::IndexedVector<IR::StatOrDecl> components;
    IR::Expression *transition = new IR::PathExpression("accept");
    auto start_state = new IR::ParserState("start", components, transition);
    states.push_back(start_state);
    return new IR::P4Parser("EgressParserImpl", type_parser, local_decls, states);
}

IR::P4Control *PSA::gen_switch_egress() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "h", SYS_HDR_NAME));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "user_meta", "metadata_t"));
    params.push_back(
        Declarations().genParameter(IR::Direction::In, "istd", "psa_egress_input_metadata_t"));
    params.push_back(
        Declarations().genParameter(IR::Direction::InOut, "ostd", "psa_egress_output_metadata_t"));
    auto par_list = new IR::ParameterList(params);
    auto type_ctrl = new IR::Type_Control("egress", par_list);
    IR::IndexedVector<IR::Declaration> local_decls;
    auto blk_stat = new IR::BlockStatement();

    return new IR::P4Control("egress", type_ctrl, local_decls, blk_stat);
}

IR::P4Control *PSA::gen_switch_egress_deparser() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::None, "pkt", "packet_out"));
    params.push_back(Declarations().genParameter(IR::Direction::Out, "clone_e2e_meta", "empty_t"));
    params.push_back(
        Declarations().genParameter(IR::Direction::Out, "recirculate_meta", "empty_t"));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "h", SYS_HDR_NAME));
    params.push_back(Declarations().genParameter(IR::Direction::In, "user_meta", "metadata_t"));
    params.push_back(
        Declarations().genParameter(IR::Direction::In, "istd", "psa_egress_output_metadata_t"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "edstd",
                                                 "psa_egress_deparser_input_metadata_t"));
    auto par_list = new IR::ParameterList(params);
    auto type_ctrl = new IR::Type_Control("EgressDeparserImpl", par_list);
    IR::IndexedVector<IR::Declaration> local_decls;
    auto blk_stat = new IR::BlockStatement();
    blk_stat->push_back(gen_deparser_emit_call());

    return new IR::P4Control("EgressDeparserImpl", type_ctrl, local_decls, blk_stat);
}

IR::Declaration_Instance *PSA::gen_ingress_pipe_decl() {
    IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();

    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("IngressParserImpl"))));
    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("ingress"))));
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("IngressDeparserImpl"))));
    auto package_name = new IR::Type_Name("IngressPipeline");
    return new IR::Declaration_Instance("ip", package_name, args);
}

IR::Declaration_Instance *PSA::gen_egress_pipe_decl() {
    IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();

    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("EgressParserImpl"))));
    args->push_back(
        new IR::Argument(new IR::MethodCallExpression(new IR::TypeNameExpression("egress"))));
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("EgressDeparserImpl"))));
    auto package_name = new IR::Type_Name("EgressPipeline");
    return new IR::Declaration_Instance("ep", package_name, args);
}

IR::Declaration_Instance *PSA::gen_main() {
    IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();
    args->push_back(new IR::Argument(new IR::TypeNameExpression("ip")));
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("PacketReplicationEngine"))));
    args->push_back(new IR::Argument(new IR::TypeNameExpression("ep")));
    args->push_back(new IR::Argument(
        new IR::MethodCallExpression(new IR::TypeNameExpression("BufferingQueueingEngine"))));
    auto package_name = new IR::Type_Name("PSA_Switch");
    return new IR::Declaration_Instance("main", package_name, args);
}

IR::Type_Struct *PSA::gen_metadata_t() {
    // Do !emit meta fields for now, no need
    IR::IndexedVector<IR::StructField> fields;
    auto ret = new IR::Type_Struct("metadata_t", fields);
    P4Scope::add_to_scope(ret);
    return ret;
}

IR::Type_Struct *PSA::gen_empty_t() {
    // Do !emit meta fields for now, no need
    IR::IndexedVector<IR::StructField> fields;
    auto ret = new IR::Type_Struct("empty_t", fields);
    P4Scope::add_to_scope(ret);
    return ret;
}

void PSA::gen_psa_md_t() {
    IR::ID *name;
    IR::Type_Struct *ret;
    IR::IndexedVector<IR::StructField> fields;

    name = new IR::ID("psa_ingress_parser_input_metadata_t");
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::add_to_scope(ret);
    name = new IR::ID("psa_ingress_input_metadata_t");
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::add_to_scope(ret);
    name = new IR::ID("psa_ingress_output_metadata_t");
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::add_to_scope(ret);
    name = new IR::ID("psa_egress_input_metadata_t");
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::add_to_scope(ret);
    name = new IR::ID("psa_egress_output_metadata_t");
    ret = new IR::Type_Struct(*name, fields);
    P4Scope::add_to_scope(ret);
}

void PSA::set_probabilities() {
    PCT.PARAMETER_NONEDIR_DERIVED_STRUCT = 0;
    PCT.PARAMETER_NONEDIR_DERIVED_HEADER = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_BOOL = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_ERROR = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_STRING = 0;
    PCT.PARAMETER_NONEDIR_BASETYPE_VARBIT = 0;
}

IR::P4Program *PSA::gen() {
    // insert banned structures
    P4Scope::not_initialized_structs.insert("psa_ingress_parser_input_metadata_t");
    P4Scope::not_initialized_structs.insert("psa_ingress_input_metadata_t");
    P4Scope::not_initialized_structs.insert("psa_ingress_output_metadata_t");
    P4Scope::not_initialized_structs.insert("psa_egress_input_metadata_t");
    P4Scope::not_initialized_structs.insert("psa_egress_output_metadata_t");
    // set psa-specific probabilities
    set_probabilities();
    // insert some dummy metadata
    gen_psa_md_t();

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
    // generate struct metadata_t
    objects->push_back(gen_metadata_t());
    // generate struct empty_t
    objects->push_back(gen_empty_t());

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
    objects->push_back(gen_switch_ingress_parser());
    objects->push_back(gen_switch_ingress());
    objects->push_back(gen_switch_ingress_deparser());
    objects->push_back(gen_switch_egress_parser());
    objects->push_back(gen_switch_egress());
    objects->push_back(gen_switch_egress_deparser());

    // finally assemble the package
    objects->push_back(gen_ingress_pipe_decl());
    objects->push_back(gen_egress_pipe_decl());
    objects->push_back(gen_main());

    return new IR::P4Program(*objects);
}
}  // namespace P4Smith

}  // namespace P4Tools
