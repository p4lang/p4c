#include "backends/p4tools/modules/smith/common/p4state.h"

#include <algorithm>
#include <map>
#include <vector>

#include "backends/p4tools/modules/smith/common/declarations.h"
#include "backends/p4tools/modules/smith/common/statements.h"

namespace P4Tools {

namespace P4Smith {

IR::IndexedVector<IR::ParserState> P4State::state_list;

IR::MethodCallStatement *P4State::gen_hdr_extract(IR::Member *pkt_call, IR::Expression *mem) {
    IR::Vector<IR::Argument> *args = new IR::Vector<IR::Argument>();
    IR::Argument *arg = new IR::Argument(mem);

    args->push_back(arg);
    auto mce = new IR::MethodCallExpression(pkt_call, args);
    return new IR::MethodCallStatement(mce);
}

void P4State::gen_hdr_union_extract(IR::IndexedVector<IR::StatOrDecl> &components,
                                    const IR::Type_HeaderUnion *hdru, IR::ArrayIndex *arr_ind,
                                    IR::Member *pkt_call) {
    auto sf = hdru->fields.at(0);
    // for (auto sf : hdru->fields) {
    // auto mem = new IR::Member(arr_ind,
    // sf->type->to<IR::Type_Name>()->path->name);
    auto mem = new IR::Member(arr_ind, sf->name);

    components.push_back(gen_hdr_extract(pkt_call, mem));
    // }
}

IR::ParserState *P4State::gen_start_state() {
    IR::IndexedVector<IR::StatOrDecl> components;
    IR::Expression *transition = new IR::PathExpression("parse_hdrs");
    auto ret = new IR::ParserState("start", components, transition);

    P4Scope::add_to_scope(ret);
    return ret;
}

IR::ParserState *P4State::gen_hdr_states() {
    IR::Expression *transition;
    IR::IndexedVector<IR::StatOrDecl> components;
    std::vector<cstring> hdr_fields_names;
    std::map<const cstring, const IR::Type *> hdr_fields_types;

    auto sys_hdr_type = P4Scope::get_type_by_name(SYS_HDR_NAME);
    auto sys_hdr = sys_hdr_type->to<IR::Type_Struct>();
    if (!sys_hdr) {
        BUG("Unexpected system header %s", sys_hdr_type->static_type_name());
    }
    for (auto sf : sys_hdr->fields) {
        hdr_fields_names.push_back(sf->name.name);
        hdr_fields_types.emplace(sf->name.name, sf->type);
    }

    auto pkt_call = new IR::Member(new IR::PathExpression("pkt"), "extract");
    for (size_t i = 0; i < hdr_fields_names.size(); i++) {
        auto sf_name = hdr_fields_names.at(i);
        auto sf_type = hdr_fields_types[sf_name];
        if (auto sf_tp_s = sf_type->to<IR::Type_Stack>()) {
            auto mem = new IR::Member(new IR::PathExpression("hdr"), sf_name);
            size_t size = sf_tp_s->getSize();
            auto ele_tp_name = sf_tp_s->elementType;
            auto ele_tp =
                P4Scope::get_type_by_name(ele_tp_name->to<IR::Type_Name>()->path->name.name);
            if (ele_tp->is<IR::Type_Header>()) {
                for (size_t j = 0; j < size; j++) {
                    auto next_mem = new IR::Member(mem, "next");
                    components.push_back(gen_hdr_extract(pkt_call, next_mem));
                }
            } else if (auto hdru_tp = ele_tp->to<IR::Type_HeaderUnion>()) {
                for (size_t j = 0; j < size; j++) {
                    auto arr_ind = new IR::ArrayIndex(mem, new IR::Constant(j));
                    gen_hdr_union_extract(components, hdru_tp, arr_ind, pkt_call);
                }
            } else {
                BUG("wtf here %s", sf_type->node_type_name());
            }
        } else if (sf_type->is<IR::Type_Name>()) {
            auto mem = new IR::Member(new IR::PathExpression("hdr"), sf_name);
            auto hdr_field_tp =
                P4Scope::get_type_by_name(sf_type->to<IR::Type_Name>()->path->name.name);
            if (hdr_field_tp->is<IR::Type_HeaderUnion>()) {
                auto hdru_tp = hdr_field_tp->to<IR::Type_HeaderUnion>();
                auto sf = hdru_tp->fields.at(0);
                auto hdr_mem = new IR::Member(mem, sf->name);
                components.push_back(gen_hdr_extract(pkt_call, hdr_mem));
            } else {
                components.push_back(gen_hdr_extract(pkt_call, mem));
            }
        } else {
            BUG("wtf here %s", sf_type->node_type_name());
        }
    }

    // transition part
    // transition = new IR::PathExpression(new IR::Path(IR::ID("state_0")));
    /*    cstring next_state = randStr(6);
        gen_state(next_state);
        transition = new IR::PathExpression(next_state);*/

    transition = new IR::PathExpression("accept");

    auto ret = new IR::ParserState("parse_hdrs", components, transition);
    P4Scope::add_to_scope(ret);
    return ret;
}

IR::ListExpression *build_match_expr(IR::Vector<IR::Type> types) {
    IR::Vector<IR::Expression> components;
    for (auto tb : types) {
        IR::Expression *expr = nullptr;
        switch (getRndInt(0, 2)) {
            case 0: {
                // TODO: Figure out allowed expressions
                // if (P4Scope::check_lval(tb)) {
                if (false) {
                    cstring lval_name = P4Scope::pick_lval(tb);
                    expr = new IR::PathExpression(lval_name);
                } else {
                    expr = Expressions().genBitLiteral(tb);
                }
                break;
            }
            case 1: {
                // Range
                big_int max_range = (big_int)1U << tb->width_bits();
                // FIXME: disable large ranges for now
                max_range = min((big_int)1U << 32, max_range);
                big_int lower = getRndBigInt(0, max_range - 1);
                big_int higher = getRndBigInt(lower, max_range - 1);
                auto lower_expr = new IR::Constant(lower);
                auto higher_expr = new IR::Constant(higher);
                expr = new IR::Range(tb, lower_expr, higher_expr);
                break;
            }
            case 2: {
                // Mask
                auto left = Expressions().genBitLiteral(tb);
                auto right = Expressions().genBitLiteral(tb);
                expr = new IR::Mask(tb, left, right);
                break;
            }
        }
        components.push_back(expr);
    }
    return new IR::ListExpression(components);
}

void P4State::gen_state(cstring name) {
    IR::IndexedVector<IR::StatOrDecl> components;

    P4Scope::start_local_scope();

    // variable decls
    for (int i = 0; i < 5; i++) {
        auto var_decl = Declarations().genVariableDeclaration();
        components.push_back(var_decl);
    }
    // statements
    for (int i = 0; i < 5; i++) {
        auto ass = Statements().genAssignmentStatement();
        if (ass != nullptr) {
            components.push_back(ass);
        }
        break;
    }

    // expression
    IR::Expression *transition = nullptr;

    std::vector<int64_t> percent = {PCT.P4STATE_TRANSITION_ACCEPT, PCT.P4STATE_TRANSITION_REJECT,
                                    PCT.P4STATE_TRANSITION_STATE, PCT.P4STATE_TRANSITION_SELECT};

    P4Scope::end_local_scope();
    switch (randInt(percent)) {
        case 0: {
            transition = new IR::PathExpression("accept");
            break;
        }
        case 1: {
            transition = new IR::PathExpression("reject");
            break;
        }
        case 2: {
            cstring next_state = randStr(6);
            gen_state(next_state);
            transition = new IR::PathExpression(next_state);
            break;
        }
        case 3: {
            IR::Vector<IR::Expression> exprs;
            IR::Vector<IR::SelectCase> cases;
            size_t num_transitions = getRndInt(1, 3);
            size_t key_set_len = getRndInt(1, 4);

            IR::Vector<IR::Type> types;
            for (size_t i = 0; i <= key_set_len; i++) {
                auto tb = Expressions::genBitType(false);
                types.push_back(tb);
            }

            for (size_t i = 0; i < num_transitions; i++) {
                IR::Expression *match_set;
                // TODO: Do !always have a default
                if (i == (num_transitions - 1)) {
                    P4Scope::req.compile_time_known = true;
                    match_set = build_match_expr(types);
                    P4Scope::req.compile_time_known = false;
                } else {
                    match_set = new IR::DefaultExpression();
                }
                switch (getRndInt(0, 2)) {
                    case 0: {
                        cases.push_back(
                            new IR::SelectCase(match_set, new IR::PathExpression("accept")));
                        break;
                    }
                    case 1: {
                        cases.push_back(
                            new IR::SelectCase(match_set, new IR::PathExpression("reject")));
                        break;
                    }
                    case 2: {
                        cstring next_state = randStr(6);
                        gen_state(next_state);
                        auto sw_case =
                            new IR::SelectCase(match_set, new IR::PathExpression(next_state));
                        cases.push_back(sw_case);
                        break;
                    }
                }
            }
            P4Scope::req.require_scalar = true;
            IR::ListExpression *key_set = Expressions().genExpressionList(types, false);
            P4Scope::req.require_scalar = false;
            transition = new IR::SelectExpression(key_set, cases);
            break;
        }
    }

    // add to scope
    auto ret = new IR::ParserState(name, components, transition);
    P4Scope::add_to_scope(ret);
    state_list.push_back(ret);
}

void P4State::build_parser_tree() {
    state_list.push_back(P4State::gen_start_state());
    state_list.push_back(P4State::gen_hdr_states());
}

}  // namespace P4Smith

}  // namespace P4Tools
