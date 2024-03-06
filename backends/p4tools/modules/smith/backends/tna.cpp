#include "backends/p4tools/modules/smith/backends/tna.h"

#include <vector>

#include "backends/p4tools/modules/smith/backends/structFieldList.h"
#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/p4state.h"
#include "backends/p4tools/modules/smith/common/scope.h"
#include "backends/p4tools/modules/smith/common/statements.h"
#include "frontends/p4/toP4/toP4.h"

namespace P4Tools::P4Smith::Tofino {

void TNA::generate_includes(std::ostream *ostream) {
    *ostream << "#include <core.p4>\n";
    *ostream << "#define __TARGET_TOFINO__ 1\n";
    *ostream << "#include <tna.p4>\n\n";
}

IR::P4Parser *TNA::gen_switch_ingress_parser() {
    IR::IndexedVector<IR::Declaration> parserLocals;
    P4Scope::start_local_scope();

    // Generate type_parser, note that this is labeled "p".
    IR::IndexedVector<IR::Parameter> params;

    params.push_back(Declarations().genParameter(IR::Direction::None, "pkt", "packet_in"));
    params.push_back(Declarations().genParameter(IR::Direction::Out, "hdr", SYS_HDR_NAME));
    params.push_back(
        Declarations().genParameter(IR::Direction::Out, "ig_md", "ingress_metadata_t"));
    params.push_back(Declarations().genParameter(IR::Direction::Out, "ig_intr_md",
                                                 "ingress_intrinsic_metadata_t"));
    auto par_list = new IR::ParameterList(params);

    IR::Type_Parser *tp_parser = new IR::Type_Parser("SwitchIngressParser", par_list);

    // Add params to the parser scope.
    for (auto param : par_list->parameters) {
        P4Scope::add_to_scope(param);
        // we only add values that are not read-only, to the modifiable types
        if (param->direction == IR::Direction::In) {
            P4Scope::add_lval(param->type, param->name.name, true);
        } else {
            P4Scope::add_lval(param->type, param->name.name, false);
        }
    }

    // Generate variable declarations.
    for (int i = 0; i < 5; i++) {
        auto *var_decl = Declarations().genVariableDeclaration();
        parserLocals.push_back(var_decl);
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
    auto start_state = P4State::gen_start_state();

    // Insert custom parsing statements into the start state.
    auto pkt_path = new IR::PathExpression("pkt");
    auto pkt_extract = new IR::Member(pkt_path, "extract");
    auto pkt_advance = new IR::Member(pkt_path, "advance");
    auto ig_intr_md = new IR::PathExpression("ig_intr_md");
    auto extract_tofino_md = P4State::gen_hdr_extract(pkt_extract, ig_intr_md);
    start_state->components.push_back(extract_tofino_md);
    auto port_md_size = new IR::PathExpression("PORT_METADATA_SIZE");
    auto advance_tofino_md = P4State::gen_hdr_extract(pkt_advance, port_md_size);
    start_state->components.push_back(advance_tofino_md);
    // Done with custom statements.
    states.push_back(start_state);
    states.push_back(P4State::gen_hdr_states());

    P4Scope::end_local_scope();

    // Add the parser to the whole scope.
    auto p4parser = new IR::P4Parser("SwitchIngressParser", tp_parser, parserLocals, states);
    P4Scope::add_to_scope(p4parser);
    return p4parser;
}

IR::P4Control *TNA::gen_switch_ingress() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "h", SYS_HDR_NAME));
    params.push_back(
        Declarations().genParameter(IR::Direction::InOut, "ig_md", "ingress_metadata_t"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "ig_intr_md",
                                                 "ingress_intrinsic_metadata_t"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "ig_prsr_md",
                                                 "ingress_intrinsic_metadata_from_parser_t"));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "ig_dprsr_md",
                                                 "ingress_intrinsic_metadata_for_deparser_t"));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "ig_tm_md",
                                                 "ingress_intrinsic_metadata_for_tm_t"));

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
    auto output_port = new IR::PathExpression("ig_tm_md.ucast_egress_port");
    auto output_port_val = new IR::Constant(new IR::Type_InfInt(), 0);
    auto assign = new IR::AssignmentStatement(output_port, output_port_val);
    // some hack to insert the expression at the beginning
    auto it = apply_block->components.begin();
    apply_block->components.insert(it, assign);
    // end of scope
    P4Scope::end_local_scope();

    // add to the whole scope
    IR::P4Control *p4ctrl = new IR::P4Control("ingress", type_ctrl, local_decls, apply_block);
    P4Scope::add_to_scope(p4ctrl);
    return p4ctrl;
}

IR::MethodCallStatement *TNA::gen_deparser_emit_call() {
    auto call = new IR::PathExpression("pkt.emit");
    IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();

    args->push_back(new IR::Argument(new IR::PathExpression("h")));

    auto mce = new IR::MethodCallExpression(call, args);
    auto mst = new IR::MethodCallStatement(mce);
    return mst;
}

IR::P4Control *TNA::gen_switch_ingress_deparser() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::None, "pkt", "packet_out"));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "h", SYS_HDR_NAME));
    params.push_back(Declarations().genParameter(IR::Direction::In, "ig_md", "ingress_metadata_t"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "ig_dprsr_md",
                                                 "ingress_intrinsic_metadata_for_deparser_t"));
    auto par_list = new IR::ParameterList(params);
    auto type_ctrl = new IR::Type_Control("SwitchIngressDeparser", par_list);
    IR::IndexedVector<IR::Declaration> local_decls;
    auto blk_stat = new IR::BlockStatement();
    blk_stat->push_back(gen_deparser_emit_call());

    return new IR::P4Control("SwitchIngressDeparser", type_ctrl, local_decls, blk_stat);
}

IR::P4Parser *TNA::gen_switch_egress_parser() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::None, "pkt", "packet_in"));
    params.push_back(Declarations().genParameter(IR::Direction::Out, "h", SYS_HDR_NAME));
    params.push_back(Declarations().genParameter(IR::Direction::Out, "eg_md", "egress_metadata_t"));
    params.push_back(Declarations().genParameter(IR::Direction::Out, "eg_intr_md",
                                                 "egress_intrinsic_metadata_t"));
    auto par_list = new IR::ParameterList(params);
    auto type_parser = new IR::Type_Parser("SwitchEgressParser", par_list);
    IR::IndexedVector<IR::Declaration> local_decls;
    // TODO: this hacky. FIX
    // generate states
    IR::IndexedVector<IR::ParserState> states;
    IR::IndexedVector<IR::StatOrDecl> components;
    IR::Expression *transition = new IR::PathExpression("accept");
    auto start_state = new IR::ParserState("start", components, transition);
    // insert custom parsing statements into the start state
    auto pkt_path = new IR::PathExpression("pkt");
    auto pkt_extract = new IR::Member(pkt_path, "extract");
    auto eg_intr_md = new IR::PathExpression("eg_intr_md");
    auto extract_tofino_md = P4State::gen_hdr_extract(pkt_extract, eg_intr_md);
    start_state->components.push_back(extract_tofino_md);
    states.push_back(start_state);
    return new IR::P4Parser("SwitchEgressParser", type_parser, local_decls, states);
}

IR::P4Control *TNA::gen_switch_egress_deparser() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::None, "pkt", "packet_out"));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "h", SYS_HDR_NAME));
    params.push_back(Declarations().genParameter(IR::Direction::In, "eg_md", "egress_metadata_t"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "eg_intr_dprs_md",
                                                 "egress_intrinsic_metadata_for_deparser_t"));
    auto par_list = new IR::ParameterList(params);
    auto type_ctrl = new IR::Type_Control("SwitchEgressDeparser", par_list);
    IR::IndexedVector<IR::Declaration> local_decls;
    auto blk_stat = new IR::BlockStatement();
    blk_stat->push_back(gen_deparser_emit_call());

    return new IR::P4Control("SwitchEgressDeparser", type_ctrl, local_decls, blk_stat);
}

IR::P4Control *TNA::gen_switch_egress() {
    // start of new scope
    P4Scope::start_local_scope();

    IR::IndexedVector<IR::Parameter> params;
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "h", SYS_HDR_NAME));
    params.push_back(
        Declarations().genParameter(IR::Direction::InOut, "eg_md", "egress_metadata_t"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "eg_intr_md",
                                                 "egress_intrinsic_metadata_t"));
    params.push_back(Declarations().genParameter(IR::Direction::In, "eg_intr_md_from_prsr",
                                                 "egress_intrinsic_metadata_from_parser_t"));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "eg_intr_dprs_md",
                                                 "egress_intrinsic_metadata_for_deparser_t"));
    params.push_back(Declarations().genParameter(IR::Direction::InOut, "eg_intr_oport_md",
                                                 "egress_intrinsic_metadata_for_output_port_t"));
    auto par_list = new IR::ParameterList(params);
    auto type_ctrl = new IR::Type_Control("SwitchEgress", par_list);
    IR::IndexedVector<IR::Declaration> local_decls;
    auto blk_stat = new IR::BlockStatement();

    return new IR::P4Control("SwitchEgress", type_ctrl, local_decls, blk_stat);
}

IR::Declaration_Instance *TNA::gen_package_decl() {
    IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();

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
    auto package_name = new IR::Type_Name("Pipeline");
    return new IR::Declaration_Instance("pipe", package_name, args);
}

IR::Declaration_Instance *TNA::gen_main() {
    IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();
    args->push_back(new IR::Argument(new IR::TypeNameExpression("pipe")));
    auto package_name = new IR::Type_Name("Switch");
    return new IR::Declaration_Instance("main", package_name, args);
}

void TNA::gen_tf_md_t() {
    IR::ID *name;
    // IR::IndexedVector<IR::StructField> fields;
    IR::Type_Struct *ret;

    name = new IR::ID("ingress_intrinsic_metadata_t");
    ret = new IR::Type_Struct(*name, structFieldList::gen_tf_ing_md_t());
    P4Scope::add_to_scope(ret);
    name = new IR::ID("ingress_intrinsic_metadata_for_tm_t");
    ret = new IR::Type_Struct(*name, structFieldList::gen_tf_ing_md_for_tm_t());
    P4Scope::add_to_scope(ret);
    name = new IR::ID("ingress_intrinsic_metadata_from_parser_t");
    ret = new IR::Type_Struct(*name, structFieldList::gen_tf_ing_intr_md_from_prsr());
    P4Scope::add_to_scope(ret);
    name = new IR::ID("ingress_intrinsic_metadata_for_deparser_t");
    ret = new IR::Type_Struct(*name, structFieldList::gen_tf_ing_intr_md_for_deprsr());
    P4Scope::add_to_scope(ret);
    name = new IR::ID("egress_intrinsic_metadata_t");
    ret = new IR::Type_Struct(*name, structFieldList::gen_tf_eg_intr_md_t());
    P4Scope::add_to_scope(ret);
    name = new IR::ID("egress_intrinsic_metadata_from_parser_t");
    ret = new IR::Type_Struct(*name, structFieldList::gen_tf_eg_intr_md_from_prsr());
    P4Scope::add_to_scope(ret);
    name = new IR::ID("egress_intrinsic_metadata_for_deparser_t");
    ret = new IR::Type_Struct(*name, structFieldList::gen_tf_eg_intr_md_for_deprsr());
    P4Scope::add_to_scope(ret);
    name = new IR::ID("egress_intrinsic_metadata_for_output_port_t");
    ret = new IR::Type_Struct(*name, structFieldList::gen_tf_eg_intr_md_for_output_port());
    P4Scope::add_to_scope(ret);
}

IR::Type_Struct *TNA::gen_ingress_metadata_t() {
    // Do !emit meta fields for now, no need
    IR::IndexedVector<IR::StructField> fields;
    auto ret = new IR::Type_Struct("ingress_metadata_t", fields);
    P4Scope::add_to_scope(ret);
    return ret;
}

IR::Type_Struct *TNA::gen_egress_metadata_t() {
    // Do !emit meta fields for now, no need
    IR::IndexedVector<IR::StructField> fields;
    auto ret = new IR::Type_Struct("egress_metadata_t", fields);
    P4Scope::add_to_scope(ret);
    return ret;
}

void TNA::set_probabilities() {
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

IR::P4Program *TNA::gen() {
    // insert banned structures
    P4Scope::not_initialized_structs.insert("ingress_intrinsic_metadata_t");
    P4Scope::not_initialized_structs.insert("ingress_intrinsic_metadata_for_tm_t");
    P4Scope::not_initialized_structs.insert("ingress_intrinsic_metadata_from_parser_t");
    P4Scope::not_initialized_structs.insert("ingress_intrinsic_metadata_for_deparser_t");
    P4Scope::not_initialized_structs.insert("egress_intrinsic_metadata_t");
    P4Scope::not_initialized_structs.insert("egress_intrinsic_metadata_from_parser_t");
    P4Scope::not_initialized_structs.insert("egress_intrinsic_metadata_for_deparser_t");
    P4Scope::not_initialized_structs.insert("egress_intrinsic_metadata_for_output_port_t");

    // set tna-specific probabilities
    set_probabilities();

    // start to assemble the model
    auto objects = new IR::Vector<IR::Node>();

    // insert tofino metadata
    gen_tf_md_t();

    objects->push_back(Declarations().genEthernetHeaderType());

    // generate some declarations
    int type_decls = getRndInt(DECL.MIN_TYPE, DECL.MAX_TYPE);
    for (int i = 0; i < type_decls; ++i) {
        objects->push_back(Declarations().genTypeDeclaration());
    }

    // generate struct Headers
    objects->push_back(Declarations().genHeaderStruct());
    // generate struct ingress_metadata_t
    objects->push_back(gen_ingress_metadata_t());
    // generate struct egress_metadata_t
    objects->push_back(gen_egress_metadata_t());

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
    objects->push_back(gen_package_decl());
    objects->push_back(gen_main());

    return new IR::P4Program(*objects);
}
}  // namespace P4Tools::P4Smith::Tofino
