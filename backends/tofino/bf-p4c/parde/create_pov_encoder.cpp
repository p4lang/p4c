/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#include "bf-p4c/parde/create_pov_encoder.h"

static const IR::Entry *create_static_entry(unsigned key_size, unsigned match,
                                            const IR::MAU::Action *action,
                                            const ordered_set<const IR::TempVar *> &outputs,
                                            unsigned action_param) {
    IR::Vector<IR::Expression> components;

    for (unsigned i = 0; i < key_size; i++) {
        auto bit_i = (match >> i) & 1;
        components.insert(components.begin(), new IR::Constant(bit_i));
    }

    auto keys = new IR::ListExpression(components);

    auto params = new IR::ParameterList;

    int i = 0;
    for (auto o : outputs) {
        std::string arg_name = "x" + std::to_string(i++);
        auto p = new IR::Parameter(arg_name.c_str(), IR::Direction::In, o->type);
        params->push_back(p);
    }

    auto type = new IR::Type_Action(new IR::TypeParameters, params);

    auto args = new IR::Vector<IR::Argument>();
    for (unsigned i = 0; i < outputs.size(); i++) {
        auto bit_i = (action_param >> i) & 1;
        auto a = new IR::Argument(new IR::Constant(bit_i));
        args->push_back(a);
    }

    auto method = new IR::PathExpression(type, new IR::Path(action->name));
    auto call = new IR::MethodCallExpression(type, method, args);
    auto entry = new IR::Entry(true, nullptr, keys, call, false);

    return entry;
}

IR::MAU::Table *create_pov_encoder(gress_t gress, cstring tableName, cstring action_name,
                                   const MatchAction &match_action) {
    static int id = 0;
    std::string table_name = toString(gress) + tableName + std::to_string(id++);
    LOG1("\ncreating a table " << table_name << " for match action:");
    LOG3(match_action.print());
    auto encoder = new IR::MAU::Table(table_name, gress);
    encoder->is_compiler_generated = true;

    auto p4_name = table_name + "_" + cstring::to_cstring(gress);
    encoder->match_table = new IR::P4Table(p4_name, new IR::TableProperties());

    int i = 0;
    for (auto in : match_action.keys) {
        auto ixbar_read = new IR::MAU::TableKey(in, IR::ID("exact"));
        ixbar_read->p4_param_order = i++;
        encoder->match_key.push_back(ixbar_read);
    }

    unsigned key_size = match_action.keys.size();

    IR::Vector<IR::Entry> static_entries;
    // set match_action.output w/ action param
    auto generated_action = new IR::MAU::Action(action_name);
    encoder->actions[generated_action->name] = generated_action;

    i = 0;
    for (auto o : match_action.outputs) {
        std::string arg_name = "x" + std::to_string(i++);

        auto arg = new IR::MAU::ActionArg(IR::Type::Bits::get(1), generated_action->name,
                                          arg_name.c_str());
        auto instr = new IR::MAU::Instruction("set"_cs, {o, arg});

        generated_action->action.push_back(instr);
        generated_action->args.push_back(arg);
    }

    for (auto &ma : match_action.match_to_action_param) {
        auto static_entry = create_static_entry(key_size, ma.first, generated_action,
                                                match_action.outputs, ma.second);
        static_entries.push_back(static_entry);
    }

    auto nop = new IR::MAU::Action("__nop_"_cs);
    encoder->actions[nop->name] = nop;
    nop->default_allowed = nop->init_default = true;

    encoder->entries_list = new IR::EntriesList(static_entries);
    LOG5(encoder);
    return encoder;
}
