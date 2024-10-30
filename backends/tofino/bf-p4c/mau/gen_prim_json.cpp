/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bf-p4c/mau/gen_prim_json.h"

#include "bf-p4c/mau/mau_visitor.h"

Util::JsonObject *GeneratePrimitiveInfo::add_op_json(Util::JsonObject *prim, const std::string op,
                                                     const std::string type, cstring name) {
    auto op_json = new Util::JsonObject();
    op_json->emplace("type"_cs, type);
    op_json->emplace("name"_cs, name);
    prim->emplace(cstring(op), op_json);
    return op_json;
}

Util::JsonObject *GeneratePrimitiveInfo::add_stful_op_json(Util::JsonObject *prim,
                                                           const std::string op,
                                                           const std::string op_pfx,
                                                           const std::string type, cstring name) {
    auto op_json = new Util::JsonObject();
    op_json->emplace(cstring(op_pfx + "_type"), type);
    op_json->emplace(cstring(op_pfx + "_value"), name);
    prim->emplace(cstring(op), op_json);
    return op_json;
}

void GeneratePrimitiveInfo::add_hash_dist_json(Util::JsonObject *_primitive,
                                               const std::string prim_name,
                                               const std::string dst_type, const cstring dst_name,
                                               const IR::Expression *dst,
                                               const IR::MAU::HashDist *hd) {
    _primitive->emplace("name"_cs, prim_name);
    if (dst)
        validate_add_op_json(_primitive, "dst", dst);
    else
        add_op_json(_primitive, "dst", dst_type, dst_name);
    // FIXME: hash_name must be the meter field list name, which is
    // currently not available here.
    auto hash_name = "hash_" + dst_name;
    auto idx = add_op_json(_primitive, "idx", "hash", hash_name);
    BuildP4HashFunction builder(phv);
    hd->expr->apply(builder);
    P4HashFunction *func = builder.func();
    BUG_CHECK(func != nullptr, "%s: Could not generate hash function", hd->srcInfo);
    idx->emplace("algorithm"_cs, func->algorithm.name());
    Util::JsonArray *hash_inputs = new Util::JsonArray();
    for (auto input : func->inputs) {
        hash_inputs->append(canon_name(input->toString()));
    }
    _primitive->emplace("hash_inputs"_cs, hash_inputs);
}

void GeneratePrimitiveInfo::add_primitive(Util::JsonArray *primitives,
                                          Util::JsonObject *primitive) {
    if (!primitives) return;
    // TODO: Check if primitive already exists. For tables spanning
    // multiple stages, the same resource gets added as an attached resource
    // for every stage. This results in the resource primitives getting
    // duplicated. To avoid duplication only add when not present in
    // the array.
    // Util::JsonObject needs a '==' operator for comparison.
    bool found = false;
    // for (auto &prim : *primitives) {
    //     if (prim == primitive->to<Util::JsonObject>()) {
    //         found = true;
    //         break;
    //     }
    // }
    if (!found) primitives->append(primitive);
}

void GeneratePrimitiveInfo::gen_action_json(const IR::MAU::Table *tbl, const IR::MAU::Action *act,
                                            Util::JsonObject *_action) {
    LOG3("GeneratePrimitiveInfo Act: " << canon_name(act->name));
    auto _primitives = new Util::JsonArray();
    for (auto call : act->stateful_calls) {
        // FIXME: Add info for hash_inputs, related to context.json schema 1.5.8
        bool is_hash_dist = false;
        const IR::MAU::HashDist *hd = nullptr;
        if (auto ci = call->index) {
            if ((hd = ci->to<IR::MAU::HashDist>()) != nullptr) {
                is_hash_dist = true;
            }
        }
        auto _primitive = new Util::JsonObject();
        auto *at = call->attached_callee;
        auto *salu = at->to<IR::MAU::StatefulAlu>();
        if (salu) {
            if (auto ci = call->index) {
                if (auto hd = ci->to<IR::MAU::HashDist>()) {
                    add_hash_dist_json(_primitive, "ExecuteStatefulAluFromHashPrimitive",
                                       "stateful", canon_name(at->name), nullptr, hd);
                } else {
                    _primitive->emplace("name"_cs, "ExecuteStatefulAluPrimitive");
                    add_op_json(_primitive, "dst", "stateful", canon_name(salu->name));
                    if (auto *k = ci->to<IR::Constant>()) {
                        add_op_json(_primitive, "idx", "immediate", k->toString());
                    } else if (auto *a = ci->to<IR::MAU::ActionArg>()) {
                        add_op_json(_primitive, "idx", "action_param", a->name.toString());
                    }
                }
            } else {
                _primitive->emplace("name"_cs, "ExecuteStatefulAluPrimitive");
                add_op_json(_primitive, "dst", "stateful", canon_name(salu->name));
            }
            if (auto *sact = salu->calledAction(tbl, act)) {
                auto *salu_details = new Util::JsonObject();
                salu_details->emplace("name"_cs, canon_name(sact->name));
                auto single_bit_mode = salu->source_width() == 1 ? true : false;
                salu_details->emplace("single_bit_mode"_cs, single_bit_mode);
                for (auto &sact_inst : sact->action) {
                    std::string inst_name = sact_inst->name.c_str();
                    auto *sact_update = new Util::JsonObject();
                    if (inst_name == "alu_a" || inst_name == "alu_b" || inst_name == "add" ||
                        inst_name == "sub") {
                        if (sact_inst->operands.size() == 2) {
                            auto src = sact_inst->operands[1];
                            auto src_string = src->toString();
                            if (auto *k = src->to<IR::Constant>()) {
                                sact_update->emplace("operand_1_type"_cs, "immediate");
                                sact_update->emplace("operand_1_value"_cs, k->toString());
                            } else if (auto *s = src->to<IR::MAU::SaluRegfileRow>()) {
                                sact_update->emplace("operand_1_type"_cs, "register_param");
                                sact_update->emplace("operand_1_value"_cs,
                                                     canon_name(s->externalName));
                            } else if (phv.field(src_string)) {
                                sact_update->emplace("operand_1_type"_cs, "phv");
                                sact_update->emplace("operand_1_value"_cs, src_string);
                            }
                        } else if (sact_inst->operands.size() == 3) {
                            auto src1 = sact_inst->operands[1];
                            auto src2 = sact_inst->operands[2];
                            std::string op = "";
                            if (inst_name == "add") op = "+";
                            if (inst_name == "sub") op = "-";
                            sact_update->emplace("operation"_cs, op);
                            if (auto *s = src1->to<IR::MAU::SaluReg>()) {
                                sact_update->emplace("operand_1_type"_cs, "memory");
                                sact_update->emplace("operand_1_value"_cs, "register_" + s->name);
                            } else if (auto *s = src1->to<IR::MAU::SaluRegfileRow>()) {
                                sact_update->emplace("operand_1_type"_cs, "register_param");
                                sact_update->emplace("operand_1_value"_cs,
                                                     canon_name(s->externalName));
                            }
                            if (auto *k = src2->to<IR::Constant>()) {
                                sact_update->emplace("operand_2_type"_cs, "immediate");
                                sact_update->emplace("operand_2_value"_cs, k->toString());
                            } else if (auto *s = src2->to<IR::MAU::SaluRegfileRow>()) {
                                sact_update->emplace("operand_2_type"_cs, "register_param");
                                sact_update->emplace("operand_2_value"_cs,
                                                     canon_name(s->externalName));
                            } else if (auto phv_field = phv.field(src2)) {
                                sact_update->emplace("operand_2_type"_cs, "phv");
                                sact_update->emplace("operand_2_value"_cs,
                                                     canon_name(phv_field->externalName()));
                            }
                        }
                        if (sact_inst->operands.size() > 1) {
                            if (auto *dst = sact_inst->operands[0]->to<IR::MAU::SaluReg>()) {
                                if (dst->name == "lo") {
                                    salu_details->emplace("update_lo_1_value"_cs, sact_update);
                                } else if (dst->name == "hi") {
                                    salu_details->emplace("update_hi_1_value"_cs, sact_update);
                                }
                            }
                        }
                    } else if (inst_name == "output") {
                        if (sact_inst->operands.size() == 1) {
                            if (auto dst = sact_inst->operands[0]->to<IR::MAU::SaluReg>()) {
                                sact_update->emplace("operand_1_type"_cs, "memory");
                                auto dst_name = (dst->name == "alu_lo")   ? "memory_lo"
                                                : (dst->name == "alu_hi") ? "memory_hi"
                                                                          : dst->name;
                                sact_update->emplace("operand_1_value"_cs, dst_name);
                                salu_details->emplace("output_value"_cs, sact_update);
                            }
                        }
                    }
                }
                _primitive->emplace("stateful_alu_details"_cs, salu_details);
            }
            add_primitive(_primitives, _primitive);
        }
        auto *meter = at->to<IR::MAU::Meter>();
        if (meter) {
            if (is_hash_dist) {
                add_hash_dist_json(_primitive, "ExecuteMeterFromHashPrimitive", "meter",
                                   canon_name(meter->name), nullptr, hd);
                add_primitive(_primitives, _primitive);
            } else {
                _primitive->emplace("name"_cs, "ExecuteMeterPrimitive");
                add_op_json(_primitive, "dst", "meter", canon_name(meter->name));
                /*
                if (auto pc = meter->pre_color_2) {
                    if (auto hd = pc->to<IR::MAU::HashDist>()) {
                        if (auto fl = hd->field_list) {
                            if (auto sl = fl->to<IR::Slice>()) {
                                if (auto e0 = sl->e0) {
                                    add_op_json(_primitive, "idx", "action_param",
                                            canon_name(e0->toString()));
                                    _primitives->append(_primitive);
                                }
                            }
                        }
                    }
                }
                */
            }
        }
        auto *counter = at->to<IR::MAU::Counter>();
        if (counter) {
            if (is_hash_dist) {
                add_hash_dist_json(_primitive, "CountFromHashPrimitive", "counter",
                                   canon_name(counter->name), nullptr, hd);
                add_primitive(_primitives, _primitive);
            } else {
                _primitive->emplace("name"_cs, "CountPrimitive");
                add_op_json(_primitive, "dst", "counter", canon_name(counter->name));
                add_primitive(_primitives, _primitive);
            }
        }
    }

    std::vector<std::string> modifyPrimVec = {"set"};
    std::vector<std::string> modifyCondPrimVec{"conditionally-set"};
    std::vector<std::string> invalidatePrimVec = {"invalidate"};
    std::vector<std::string> shiftPrimVec = {"shru", "shl", "shrs", "funnel-shift"};
    std::vector<std::string> directAluPrimVec = {
        "add",  "addc", "saddu", "sadds", "and",  "andca", "andcb", "nand", "nor",   "not",  "or",
        "orca", "orcb", "xnor",  "xor",   "maxu", "minu",  "sub",   "subc", "ssubu", "ssubs"};
    std::map<std::string, std::vector<std::string> *> prims = {
        {"ModifyFieldPrimitive", &modifyPrimVec},
        {"ModifyFieldConditionallyPrimitive", &modifyCondPrimVec},
        {"InvalidatePrimitive", &invalidatePrimVec},
        {"ShiftPrimitive", &shiftPrimVec},
        {"DirectAluPrimitive", &directAluPrimVec}};
    for (auto inst : act->action) {
        auto _primitive = new Util::JsonObject();
        std::string inst_name = inst->name.c_str();
        bool skip_prim_check = false;
        if ((inst_name == "set") && (inst->operands.size() == 2)) {
            auto dst = inst->operands[0];
            cstring dst_name = inst->operands[0]->toString();
            auto src = inst->operands[1];
            if (dst_name.endsWith("$valid")) {
                if (auto val = src->to<IR::Constant>()) {
                    if (val->value == 1) {
                        _primitive->emplace("name"_cs, "AddHeaderPrimitive");
                        skip_prim_check = true;
                    } else if (val->value == 0) {
                        _primitive->emplace("name"_cs, "RemoveHeaderPrimitive");
                        skip_prim_check = true;
                    }
                    add_op_json(_primitive, "dst", "header", canon_name(dst_name));
                }
            } else if (dst_name.endsWith("drop_ctl")) {
                if (auto val = src->to<IR::Constant>()) {
                    if (val->value == 1) {
                        _primitive->emplace("name"_cs, "DropPrimitive");
                        skip_prim_check = true;
                        add_op_json(_primitive, "dst", "phv", canon_name(dst_name));
                        add_op_json(_primitive, "src1", "immediate", "1"_cs);
                    }
                }
            } else if (auto hd = src->to<IR::MAU::HashDist>()) {
                add_hash_dist_json(_primitive, "SetFieldToHashIndexPrimitive", "",
                                   canon_name(dst_name), dst, hd);
                skip_prim_check = true;
            }
        }
        bool is_stful_dest = false;
        if (!skip_prim_check) {
            for (auto &p : prims) {
                auto pv = *p.second;
                if (std::any_of(pv.begin(), pv.end(), [&inst_name](std::string const &elem) {
                        return inst_name == elem;
                    })) {
                    _primitive->emplace("name"_cs, p.first);
                    _primitive->emplace("operation"_cs, inst_name);
                    if (inst->operands.size() == 2) {
                        auto src = inst->operands[1];
                        if (auto *ao = src->to<IR::MAU::AttachedOutput>()) {
                            if (ao->attached->to<IR::MAU::StatefulAlu>()) {
                                auto op0 = inst->operands[0];
                                auto dst_op = op0->toString();
                                if (auto *sl = op0->to<IR::Slice>()) dst_op = sl->e0->toString();
                                cstring dst = cstring::to_cstring(canon_name(dst_op));
                                // Add output_dst to previously added stateful
                                // primitive
                                for (auto _prim : *_primitives) {
                                    auto _prim_o = _prim->to<Util::JsonObject>();
                                    auto _prim_o_name = _prim_o->get("name"_cs);
                                    if (!_prim_o_name) continue;
                                    auto _prim_o_name_val = _prim_o_name->to<Util::JsonValue>();
                                    auto _prim_o_name_val_str = _prim_o_name_val->getString();
                                    std::string sprim_1 = "ExecuteStatefulAluPrimitive";
                                    std::string sprim_2 = "ExecuteStatefulAluFromHashPrimitive";
                                    bool stful_prim = ((_prim_o_name_val_str == sprim_1) ||
                                                       (_prim_o_name_val_str == sprim_2));
                                    if (!stful_prim) continue;
                                    auto _salu_d = _prim_o->get("stateful_alu_details"_cs);
                                    if (_salu_d) {
                                        auto _salu_d_o = _salu_d->to<Util::JsonObject>();
                                        if (_salu_d_o->count("output_dst"_cs)) {
                                            error(ErrorType::ERR_DUPLICATE,
                                                  "At most one stateful ALU operation "
                                                  "with a given address is allowed per action. "
                                                  "Writing to %s is not allowed here.",
                                                  op0);
                                            continue;
                                        }
                                        _salu_d_o->emplace("output_dst"_cs, canon_name(dst));
                                        is_stful_dest = true;
                                    }
                                }
                                break;
                            }
                        }
                    } else if (inst->operands.size() == 1) {
                        if (inst_name == "invalidate") {
                            auto dst = inst->operands[0]->to<IR::Expression>();
                            if (auto phv_field = phv.field(dst)) {
                                add_op_json(_primitive, "dst", "phv",
                                            canon_name(phv_field->externalName()));
                            }
                        }
                    }
                }
            }
            // Don't output operand json if instruction is a stateful destination,
            // this info goes within the stateful primitive evaluated above.

            if ((inst->operands.size() >= 2) && (!is_stful_dest)) {
                // Add dst operands
                auto dst = inst->operands[0]->to<IR::Expression>();
                le_bitrange bits = {0, 0};
                if (auto phv_field = phv.field(dst, &bits)) {
                    add_op_json(_primitive, "dst", "phv", canon_name(phv_field->externalName()));
                    add_op_json(_primitive, "dst_mask", "immediate",
                                std::to_string((1ULL << bits.size()) - 1));
                }

                if (inst->name == "conditionally-set") {
                    auto iteration = 0;
                    for (int idx = inst->operands.size() - 1; idx > 0; idx--) {
                        std::string src_name;
                        switch (iteration) {
                            case 0:
                                src_name = "cond";
                                break;
                            case 1:
                                src_name = "src1";
                                break;
                            case 2:
                                src_name = "src2";
                                break;
                            default:
                                BUG("Too many operands in a conditional-set");
                        }
                        auto src = inst->operands.at(idx);
                        if (iteration == 0) src = src->to<IR::MAU::ConditionalArg>()->orig_arg;
                        validate_add_op_json(_primitive, src_name, src);
                        iteration++;
                    }
                } else {
                    // Add src operands
                    auto idx = 0;
                    for (auto src : inst->operands) {
                        if (idx++ == 0) continue;  // skip 1st op which is dst
                        auto src_name = "src" + std::to_string(idx - 1);
                        validate_add_op_json(_primitive, src_name, src);
                    }
                }
            }
        }
        // FIXME: This name should be set above.
        // 'name' is a required attribute for primitives.
        if (_primitive->find("name"_cs) == _primitive->end()) {
            _primitive->emplace("name"_cs, "PrimitiveNameNeeded");
        }
        if (!is_stful_dest) add_primitive(_primitives, _primitive);
    }
    _action->emplace("primitives"_cs, _primitives);
    LOG5("Generated PrimitiveInfo : " << _primitives->toString());
}

void GeneratePrimitiveInfo::validate_add_op_json(Util::JsonObject *_primitive,
                                                 const std::string op_name,
                                                 const IR::Expression *exp) {
    if (auto phv_field = phv.field(exp)) {
        add_op_json(_primitive, op_name, "phv", canon_name(phv_field->externalName()));
    } else if (auto cnst = exp->to<IR::Constant>()) {
        add_op_json(_primitive, op_name, "immediate", cnst->toString());
    } else if (auto aarg = exp->to<IR::MAU::ActionArg>()) {
        add_op_json(_primitive, op_name, "action_param", canon_name(aarg->name.toString()));
    } else if (auto *sl = exp->to<IR::Slice>()) {
        if (auto e0 = sl->e0) {
            if (auto *ao = e0->to<IR::MAU::AttachedOutput>()) {
                if (auto stful = ao->attached->to<IR::MAU::StatefulAlu>()) {
                    if (op_name == "dst")
                        add_op_json(_primitive, op_name, "stateful", canon_name(stful->name));
                    else
                        add_op_json(_primitive, op_name, "phv", canon_name(stful->name));
                } else if (auto meter = ao->attached->to<IR::MAU::Meter>()) {
                    if (op_name == "src1")
                        add_op_json(_primitive, op_name, "action_param", canon_name(meter->name));
                    else
                        add_op_json(_primitive, op_name, "phv", canon_name(meter->name));
                }
            }
        }
    }
}

bool GeneratePrimitiveInfo::preorder(const IR::MAU::Table *tbl) {
    auto tname = tbl->match_table ? tbl->match_table->externalName() : tbl->name;
    LOG3("Generating primitive info for table: " << tname);

    bool alpm_preclassifier = tbl->name.endsWith("preclassifier");

    if (tbl->actions.empty()) return true;
    if (tbl->always_run == IR::MAU::AlwaysRun::ACTION) return true;

    auto _table = new Util::JsonObject();
    auto _actions = new Util::JsonArray();
    for (auto act : Values(tbl->actions)) {
        auto _action = new Util::JsonObject();
        _action->emplace("name"_cs, canon_name(act->name));
        gen_action_json(tbl, act, _action);
        _actions->append(_action);
    }

    _table->emplace("name"_cs, canon_name(tname));
    if (alpm_preclassifier) {
        auto _match_attr = new Util::JsonObject();
        auto _pre_classifier = new Util::JsonObject();
        _pre_classifier->emplace("actions"_cs, _actions);
        _match_attr->emplace("pre_classifier"_cs, _pre_classifier);
        _table->emplace("match_attributes"_cs, _match_attr);
    } else {
        _table->emplace("actions"_cs, _actions);
    }

    _tables->append(_table);
    return true;
}

Visitor::profile_t GeneratePrimitiveInfo::init_apply(const IR::Node *root) {
    _tables->clear();
    return MauInspector::init_apply(root);
}

void GeneratePrimitiveInfo::end_apply() {
    // Write to primitive json file
    auto _pNode = new Util::JsonObject();
    _pNode->emplace("tables"_cs, _tables);
    _primNode = *_pNode;
}
