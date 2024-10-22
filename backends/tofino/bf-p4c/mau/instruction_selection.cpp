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

#include "bf-p4c/mau/instruction_selection.h"

#include <sstream>

#include <boost/algorithm/string.hpp>

#include "action_analysis.h"
#include "bf-p4c/common/elim_unused.h"
#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/common/slice.h"
#include "bf-p4c/mau/stateful_alu.h"
#include "bf-p4c/mau/static_entries_const_prop.h"
#include "bf-p4c/mau/validate_actions.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/pattern.h"
#include "ixbar_expr.h"
#include "lib/bitops.h"
#include "lib/safe_vector.h"

bool UnimplementedRegisterMethodCalls::preorder(const IR::MAU::Primitive *prim) {
    auto dot = prim->name.find('.');
    auto objType = dot ? prim->name.before(dot) : cstring();
    cstring method = dot ? cstring(dot + 1) : prim->name;

    if (objType == "Register" && (method == "read" || method == "write")) {
        P4C_UNIMPLEMENTED(
            "%s: The method call of read and write on a Register is currently not "
            "supported in p4c.  Please use RegisterAction to describe any register "
            "programming.",
            prim->srcInfo);
    }
    return true;
}

const IR::Expression *ToFunnelShiftInstruction::preorder(IR::Cast *e) {
    const IR::Type *srcType = e->expr->type;
    const IR::Type *dstType = e->destType;

    if (srcType->is<IR::Type_Bits>() && dstType->is<IR::Type_Bits>()) {
        if (srcType->to<IR::Type_Bits>()->isSigned != dstType->to<IR::Type_Bits>()->isSigned)
            return e;

        if (srcType->width_bits() > dstType->width_bits()) {
            // This code block handle the funnel_shift_right() P4_14 operation.
            // P4_14 code:                funnel_shift_right(dest, src_hi, src_lo, shift)
            // is translated in P4_16 as: dest = (bit<dest>)Shr((src_hi ++ src_lo), shift)
            //
            // The midend decide if the above operation must be handled through slicing or using
            // funnel shifting. At this point, if we still have a format where the source
            // concatenation is larger than the destination, it mean the decision was to use funnel
            // shifting.
            if (auto *shift = e->expr->to<IR::Shr>()) {
                if (auto *concat = shift->left->to<IR::Concat>()) {
                    if (shift->right->to<IR::Constant>()->value ==
                        concat->right->type->width_bits()) {
                        return new IR::MAU::Instruction(
                            e->srcInfo, "set"_cs, {new IR::TempVar(shift->type), concat->left});
                    } else {
                        return new IR::MAU::Instruction(e->srcInfo, "funnel-shift"_cs,
                                                        {new IR::TempVar(shift->type), concat->left,
                                                         concat->right, shift->right});
                    }
                }
            } else if (auto *concat = e->expr->to<IR::Concat>()) {
                return new IR::MAU::Instruction(e->srcInfo, "funnel-shift"_cs,
                                                {new IR::TempVar(concat->type), concat->left,
                                                 concat->right, new IR::Constant(0)});
            }
        }
    }
    return e;
}

const IR::Expression *ConvertFunnelShiftExtern::preorder(IR::Primitive *prim) {
    if (prim->name != "funnel_shift_right") return prim;
    const auto *dst = prim->operands[0];
    const auto *src1 = prim->operands[1];
    const auto *src2 = prim->operands[2];
    const auto *n_shift = prim->operands[3];
    return new IR::MAU::Instruction(prim->srcInfo, "funnel-shift"_cs, {dst, src1, src2, n_shift});
}

bool HashGenSetup::CreateHashGenExprs::preorder(const IR::BFN::SignExtend *se) {
    if (!findContext<IR::MAU::Action>()) return false;
    if (CanBeIXBarExpr(se)) {
        per_prim_hge.push_back(se);
        auto hge = new IR::MAU::HashGenExpression(se->srcInfo, se->type, se,
                                                  IR::MAU::HashFunction::identity());
        self.hash_gen_injections[se] = hge;
        return false;
    }
    return true;
}

bool HashGenSetup::CreateHashGenExprs::preorder(const IR::Concat *c) {
    if (!findContext<IR::MAU::Action>()) return false;
    auto *prim = findContext<IR::MAU::Primitive>();
    auto k = c->left->to<IR::Constant>();
    if ((!prim || !prim->in_hash) && k && k->value == 0 && self.phv.field(c->right)) {
        // HACK -- avoid dealing with 0-prefix concats (zero extension) as the midend
        // now changes zero-extend casts into them, and trying to do them in the hash
        // breaks more things than it fixes
        return true;
    }
    if (CanBeIXBarExpr(c)) {
        per_prim_hge.push_back(c);
        auto hge = new IR::MAU::HashGenExpression(c->srcInfo, c->type, c,
                                                  IR::MAU::HashFunction::identity());
        self.hash_gen_injections[c] = hge;
    }
    return false;
}

bool HashGenSetup::CreateHashGenExprs::preorder(const IR::Expression *expr) {
    if (!findContext<IR::MAU::Action>()) return false;
    auto *prim = findContext<IR::MAU::Primitive>();
    if (prim && prim->in_hash && !isWrite() && CanBeIXBarExpr(expr)) {
        per_prim_hge.push_back(expr);
        auto hge = new IR::MAU::HashGenExpression(expr->srcInfo, expr->type, expr,
                                                  IR::MAU::HashFunction::identity());
        self.hash_gen_injections[expr] = hge;
        return false;
    }
    return true;
}

bool HashGenSetup::CreateHashGenExprs::preorder(const IR::Constant *) {
    // Never worth while to put a constant by itself through hash dist (though possible!),
    // so don't do it
    return false;
}

void HashGenSetup::CreateHashGenExprs::check_for_symmetric(const IR::Declaration_Instance *decl,
                                                           const IR::ListExpression *le,
                                                           IR::MAU::HashFunction hf,
                                                           LTBitMatrix *sym_keys) {
    auto tbl = findContext<IR::MAU::Table>();
    if (!tbl) return;

    safe_vector<const IR::Expression *> field_list;
    for (auto expr : le->components) {
        field_list.push_back(expr);
    }
    verifySymmetricHashPairs(self.phv, field_list, decl->annotations, tbl->gress, hf, sym_keys);
}

/**
 * @brief Construct a field list expression from a hash list and update the relevant properties.
 * @see HashGenSetup::CreateHashGenExprs::preorder.
 */
void load_hash_details(const BFN_Options &options, const IR::Expression *orig_hash_list,
                       int &hash_output_width, cstring &hash_name, IR::NameList const *&alg_names,
                       IR::MAU::FieldListExpression *&fle) {
    if (options.langVersion == CompilerOptions::FrontendVersion::P4_14) {
        if (orig_hash_list->is<IR::HashListExpression>()) {
            auto hle = orig_hash_list->to<IR::HashListExpression>();

            IR::ID fl_id(hle->fieldListNames->names[0]);
            fle = new IR::MAU::FieldListExpression(hle->srcInfo, hle->components, fl_id);
            hash_name = hle->fieldListCalcName;
            hash_output_width = hle->outputWidth;
            alg_names = hle->algorithms;
        } else if (orig_hash_list->is<IR::HashStructExpression>()) {
            auto hse = orig_hash_list->to<IR::HashStructExpression>();

            IR::ID fl_id(hse->fieldListNames->names[0]);
            fle = new IR::MAU::FieldListExpression(hse->srcInfo,
                                                   *getListExprComponents(*orig_hash_list), fl_id);
            hash_name = hse->fieldListCalcName;
            hash_output_width = hse->outputWidth;
            alg_names = hse->algorithms;
        } else {
            BUG("Hash not converted correctly in the midend");
        }
    } else {
        const IR::ListExpression *le = orig_hash_list->to<IR::ListExpression>();
        if (le == nullptr) {
            if (orig_hash_list->is<IR::StructExpression>()) {
                le = new IR::ListExpression(orig_hash_list->srcInfo,
                                            *getListExprComponents(*orig_hash_list));
            } else if (orig_hash_list->is<IR::Expression>()) {
                IR::Vector<IR::Expression> le_vec;
                le_vec.push_back(orig_hash_list);
                le = new IR::ListExpression(orig_hash_list->srcInfo, le_vec);
            }
        }

        BUG_CHECK(le != nullptr, "Hash.get calls must have a valid input or list of inputs");

        IR::ID fl_id("$field_list_1");
        fle = new IR::MAU::FieldListExpression(le->srcInfo, le->components, fl_id);
        fle->rotateable = true;
        fle->permutable = true;
        hash_name += ".configure";
    }
}

/**
 * The Hash.get function has different IR for P4-14 and P4-16.  This function converts all
 * of these to HashGenExpression, which currently is language dependent, as there is currently
 * no unified way of handling all of the expressivity of Hash in the p4-16 language yet through
 * our externs
 *
 * Any function that uses the Hash.get call is by definition a dynamic hash.  The rules are:
 *
 * 1.  All Hash.get come from an original Hash extern, which has a name.  (In p4-14, instead of
 *     using this compiler generated name, we use the field_list_calculation name)
 * 2.  Any Hash.get from the same Hash extern must have the same input field list, and
 *     currently only one field list, as this is the list that can be associated
 * 3.  P4-14 can allow only certain algorithms, the one that come through names, p4-16 can allow
 *     any hash algorithm necessary.  Everything is rotateable and permutable.
 */
bool HashGenSetup::CreateHashGenExprs::preorder(const IR::MAU::Primitive *prim) {
    per_prim_hge.clear();
    if (prim->name != "Hash.get") return true;

    cstring hash_name;
    IR::MAU::HashGenExpression *hge = nullptr;
    IR::MAU::FieldListExpression *fle = nullptr;
    IR::MAU::HashFunction algorithm;

    auto it = prim->operands.begin();
    // operand 0 is always the extern itself.
    BUG_CHECK(it != prim->operands.end(), "primitive %s does not have a reference to P4 extern",
              prim->name);
    auto glob = (*it)->to<IR::GlobalRef>();
    auto decl = glob->obj->to<IR::Declaration_Instance>();
    auto *orig_type = decl->type->to<IR::Type_Specialized>()->arguments->at(0);
    int hash_output_width = orig_type->width_bits();
    int bit_size = hash_output_width;

    if (self.options.langVersion == CompilerOptions::FrontendVersion::P4_14) {
    } else {
        hash_name = decl->controlPlaneName();
    }

    bool custom_hash = false;
    std::advance(it, 1);
    auto crc_poly = (*it)->to<IR::GlobalRef>();
    if (crc_poly) {
        custom_hash = true;
        if (!algorithm.convertPolynomialExtern(crc_poly))
            BUG("invalid hash algorithm %s", decl->arguments->at(1));
        std::advance(it, 1);
    }

    // otherwise, if not a custom hash from user, it is one of the hashes built into compiler.
    if (!custom_hash && !algorithm.setup(decl->arguments->at(0)->expression))
        BUG("invalid hash algorithm %s", decl->arguments->at(0));

    int remaining_op_size = std::distance(it, prim->operands.end());

    auto orig_hash_list = *it;
    if (remaining_op_size > 1) {
        auto base = std::next(it, 1);
        if (auto *constant = (*base)->to<IR::Constant>()) {
            if (constant->asInt() != 0)
                error(
                    "%1%: The initial offset for a hash calculation function has to be zero "
                    "instead of %2%",
                    prim, constant);
        }
        auto max = std::next(it, 2);
        if (auto *constant = (*max)->to<IR::Constant>()) {
            auto value = constant->asUint64();
            if (value != 0) {
                bit_size = bitcount(value - 1);
                if ((1ULL << bit_size) != value)
                    error(
                        "%1%: The hash offset must be a power of 2 in a hash calculation "
                        "instead of %2%",
                        prim, value);
            }
        }
    }

    const IR::NameList *alg_names = nullptr;
    load_hash_details(self.options, orig_hash_list, hash_output_width, hash_name, alg_names, fle);

    check_for_symmetric(decl, fle, algorithm, &fle->symmetric_keys);
    auto *type = IR::Type::Bits::get(bit_size);
    hge = new IR::MAU::HashGenExpression(prim->srcInfo, type, fle, IR::ID(hash_name), algorithm);
    // Symmetric is not supported with bf-utils dynamic hash library
    hge->dynamic = fle->symmetric_keys.empty();
    hge->any_alg_allowed = self.options.langVersion == CompilerOptions::FrontendVersion::P4_16;
    hge->hash_output_width = hash_output_width;
    hge->alg_names = alg_names;
    self.hash_gen_injections[prim] = hge;
    return false;
}

static const IR::Expression *removeSliceAndCast(const IR::Expression *expr) {
    if (auto *sl = expr->to<IR::Slice>()) return removeSliceAndCast(sl->e0);
    if (auto *c = expr->to<IR::Cast>()) return removeSliceAndCast(c->expr);
    return expr;
}

/* check which expression is better done in the hash for a primitive that writes to dest.
 * returns true if b is same or better, false if a is better
 */
bool HashGenSetup::CreateHashGenExprs::isBetter(const IR::Expression *dest, const IR::Expression *a,
                                                const IR::Expression *b) {
    if (a->equiv(*dest)) return true;
    if (b->equiv(*dest)) return false;
    if (a->is<IR::Constant>()) return true;
    if (b->is<IR::Constant>()) return false;
    return a->is<IR::Member>() || a->is<IR::TempVar>();
}

void HashGenSetup::CreateHashGenExprs::postorder(const IR::MAU::Primitive *prim) {
    if (per_prim_hge.size() > 1) {
        /* can only have one hash_gen expression per primitive, so we need to decide which
         * one is "best" and get rid of the rest. */
        auto *dest = removeSliceAndCast(prim->operands.at(0));
        const IR::Expression *best = nullptr;
        for (auto *expr : per_prim_hge) {
            if (!best) {
                best = expr;
                continue;
            }
            if (isBetter(dest, removeSliceAndCast(best), removeSliceAndCast(expr))) {
                self.hash_gen_injections.erase(best);
                best = expr;
            } else {
                self.hash_gen_injections.erase(expr);
            }
        }
    }
    per_prim_hge.clear();
}

bool HashGenSetup::ScanHashDists::preorder(const IR::Expression *expr) {
    if (self.hash_gen_injections.count(expr) == 0) return true;
    auto context = getContext();
    const IR::Node *curr_node = expr;
    while (context->node->is<IR::Slice>() || context->node->is<IR::Cast>()) {
        curr_node = context->node;
        context = context->parent;
    }

    auto highest_expr = curr_node->to<IR::Expression>();
    self.hash_dist_injections.insert(highest_expr);
    return false;
}

/**
 * Must introduce the instruction, in case someone writes the following:
 *
 *     action a {
 *         hash.get(blah);
 *     }
 *
 * as converting this to a HashGenExpression by itself will fail the Action check that
 * all base statements in the action are Primitives or their subclass.
 */
const IR::Expression *HashGenSetup::UpdateHashDists::postorder(IR::Expression *e) {
    auto rv = e;
    auto origExpr = getOriginal()->to<IR::Expression>();
    auto hgi = self.hash_gen_injections.find(origExpr);
    if (hgi != self.hash_gen_injections.end()) {
        rv = hgi->second;
    }

    auto hdi = self.hash_dist_injections.find(origExpr);
    if (hdi != self.hash_dist_injections.end()) {
        auto tv = new IR::TempVar(e->type);
        auto hd2 = new IR::MAU::HashDist(rv->srcInfo, rv->type, rv);
        auto inst = new IR::MAU::Instruction(e->srcInfo, "set"_cs, tv, hd2);
        rv = inst;
    }
    return rv;
}

const IR::MAU::Action *Synth2PortSetup::preorder(IR::MAU::Action *act) {
    clear_action();
    return act;
}
/** Converts any method calls relating to Synth2Port tables to the associated AttachedOutput
 *  if necessary, and once converted, saves this value to the primitive
 */
const IR::Node *Synth2PortSetup::postorder(IR::MAU::Primitive *prim) {
    if (findContext<IR::MAU::SaluAction>()) return prim;

    auto act = findContext<IR::MAU::Action>();
    if (act == nullptr) return prim;

    auto tbl = findContext<IR::MAU::Table>();
    if (tbl == nullptr) return prim;

    auto dot = prim->name.find('.');
    auto objType = dot ? prim->name.before(dot) : cstring();
    const char *tail = objType.c_str() + objType.size();
    while (tail > objType && std::isdigit(tail[-1])) --tail;
    objType = objType.before(tail);
    IR::Node *rv = prim;

    const IR::GlobalRef *glob = nullptr;

    IR::MAU::MeterType meter_type = IR::MAU::MeterType::UNUSED;

    cstring method = dot ? cstring(dot + 1) : prim->name;
    if (objType.endsWith("Action") || objType == "SelectorAction") {
        bool direct_access = (prim->operands.size() == 1 && method == "execute") ||
                             objType == "DirectRegisterAction" || method == "execute_direct";
        glob = prim->operands.at(0)->to<IR::GlobalRef>();
        auto salu = glob->obj->to<IR::MAU::StatefulAlu>();
        if (method == "address") {
            if (getParent<IR::MAU::Action>()) {
                // dead use of the address, so discard it.
                return nullptr;
            }
            auto t = IR::Type::Bits::get(32);
            return new IR::MAU::StatefulCounter(prim->srcInfo, t, salu);
        }
        auto *salu_inst = salu->calledAction(tbl, act);
        BUG_CHECK(salu_inst != nullptr, "%s: Could not find called action for stateful memory in ",
                  prim->srcInfo, salu->name);
        auto salu_index = salu_inst->inst_code;
        if (salu_index < 0) {
            // FIXME -- should be allocating/setting this
            auto pos = salu->instruction.find(salu_inst->name);
            salu_index = std::distance(salu->instruction.begin(), pos);
        }
        switch (salu_index) {
            case 0:
                meter_type = IR::MAU::MeterType::STFUL_INST0;
                break;
            case 1:
                meter_type = IR::MAU::MeterType::STFUL_INST1;
                break;
            case 2:
                meter_type = IR::MAU::MeterType::STFUL_INST2;
                break;
            case 3:
                meter_type = IR::MAU::MeterType::STFUL_INST3;
                break;
            default:
                BUG("%s: An stateful instruction %s is outside the bounds of the stateful "
                    "memory in %s",
                    prim->srcInfo, salu_inst, salu->name);
        }

        // needed to setup the index and/or type properly
        stateful.push_back(prim);

        if (objType == "RegisterAction" && salu->direct != direct_access)
            error("%s: %sdirect access to %sdirect register", prim->srcInfo,
                  direct_access ? "" : "in", salu->direct ? "" : "in");

        int output_offsets[] = {0, 64, 32, 96};
        auto makeInstr = [&](const IR::Expression *dest, int param,
                             int output_alu) -> IR::MAU::Instruction * {
            BUG_CHECK(size_t(output_alu) < sizeof(output_offsets) / sizeof(output_offsets[0]),
                      "too many outputs");
            int bit = output_offsets[output_alu];
            int output_size = prim->type->width_bits();
            if ((salu_inst->return_predicate_words >> param) & 1) {
                // an output enum encoded in the 16-bit predicate
                BUG_CHECK(salu->pred_shift >= 0,
                          "Not outputting predicate even though "
                          "its being used");
                bit += 4 + salu->pred_shift;
                output_size = 1 << Device::statefulAluSpec().CmpUnits.size();
            }
            auto ao = new IR::MAU::AttachedOutput(IR::Type::Bits::get(bit + output_size), salu);
            int dest_size = dest->type->width_bits();
            if (output_size < dest_size) {
                created_instrs.push_back(new IR::MAU::Instruction(
                    prim->srcInfo, "set"_cs, MakeSlice(dest, output_size, dest_size - 1),
                    new IR::Constant(IR::Type_Bits::get(dest_size - output_size), 0)));
                dest = MakeSlice(dest, 0, output_size - 1);
            } else
                output_size = dest_size;
            return new IR::MAU::Instruction(prim->srcInfo, "set"_cs, dest,
                                            MakeSlice(ao, bit, bit + output_size - 1));
        };

        unsigned idx = (method == "execute" && !direct_access) ? 2 : 1;
        for (int output = 1; idx < prim->operands.size(); ++idx, ++output) {
            // Have to put these instructions at the highest level of the instruction
            const auto alu = salu_inst->output_param_to_alu.find(output);
            BUG_CHECK(alu != salu_inst->output_param_to_alu.end(),
                      "No output ALU assigned to parameter %1% in %2%", output, salu_inst->name);
            created_instrs.push_back(makeInstr(prim->operands[idx], output, alu->second));
        }
        rv = nullptr;
        if (prim->type->is<IR::Type::Void>()) {
        } else if (prim->type->is<IR::Type::Bits>() || prim->type->is<IR::Type_Enum>() ||
                   prim->type->is<IR::Type::Boolean>()) {
            const auto alu = salu_inst->output_param_to_alu.find(0);
            BUG_CHECK(alu != salu_inst->output_param_to_alu.end(),
                      "No output ALU assigned to paramater 0 in %1%", salu_inst->name);
            rv = makeInstr(new IR::TempVar(prim->type), 0, alu->second);
        } else {
            error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                  "%1%: %2% return type must be simple "
                  "(void, bit, bool or enum), not complex",
                  prim, objType);
        }
    } else if (prim->name == "Register.clear" || prim->name == "DirectRegister.clear") {
        glob = prim->operands.at(0)->to<IR::GlobalRef>();
        meter_type = IR::MAU::MeterType::STFUL_CLEAR;
        stateful.push_back(prim);  // needed to setup clear/busy values
        rv = nullptr;
    } else if (prim->name == "Counter.count") {
        glob = prim->operands.at(0)->to<IR::GlobalRef>();
        stateful.push_back(prim);  // needed to setup the index
        rv = nullptr;
    } else if (prim->name == "Lpf.execute" || prim->name == "Wred.execute" ||
               prim->name == "Meter.execute" || prim->name == "DirectLpf.execute" ||
               prim->name == "DirectWred.execute") {
        glob = prim->operands.at(0)->to<IR::GlobalRef>();
        auto mtr = glob->obj->to<IR::MAU::Meter>();
        BUG_CHECK(mtr != nullptr, "%s: Cannot find associated meter for the method call %s",
                  prim->srcInfo, *prim);
        stateful.push_back(prim);
        rv = new IR::MAU::Instruction(prim->srcInfo, "set"_cs, new IR::TempVar(prim->type),
                                      new IR::MAU::AttachedOutput(prim->type, mtr));
    } else if (prim->name == "DirectCounter.count") {
        glob = prim->operands.at(0)->to<IR::GlobalRef>();
        stateful.push_back(prim);
        rv = nullptr;
    } else if (prim->name == "DirectMeter.execute") {
        glob = prim->operands.at(0)->to<IR::GlobalRef>();
        auto mtr = glob->obj->to<IR::MAU::Meter>();
        stateful.push_back(prim);
        rv = new IR::MAU::Instruction(prim->srcInfo, "set"_cs, new IR::TempVar(prim->type),
                                      new IR::MAU::AttachedOutput(IR::Type::Bits::get(8), mtr));
    }

    if (glob != nullptr) {
        auto at_mem = glob->obj->to<IR::MAU::AttachedMemory>();
        auto u_id = at_mem->unique_id();
        if (per_flow_enables.count(u_id) > 0) {
            error("%s: An attached table can only be executed once per action", prim->srcInfo);
        }
        per_flow_enables.insert(u_id);
        if (meter_type != IR::MAU::MeterType::UNUSED) meter_types[u_id] = meter_type;
    }

    if (findContext<IR::MAU::Primitive>() != nullptr) return rv;
    // If instructions are at the top level of the instruction, then push these instructions
    // in after
    if (!created_instrs.empty()) {
        auto *rv_vec = new IR::Vector<IR::Node>();
        if (rv != nullptr) rv_vec->push_back(rv);
        for (auto instr : created_instrs) rv_vec->push_back(instr);
        created_instrs.clear();
        return rv_vec;
    }
    return rv;
}

const IR::MAU::Action *Synth2PortSetup::postorder(IR::MAU::Action *act) {
    for (auto stateful_prim : stateful) {
        auto at_mem =
            stateful_prim->operands.at(0)->to<IR::GlobalRef>()->obj->to<IR::MAU::AttachedMemory>();
        BUG_CHECK(at_mem, "%s: Stateful Call %s doesn't have a stateful object",
                  stateful_prim->srcInfo, stateful_prim);
        act->stateful_calls.emplace_back(stateful_prim->srcInfo, stateful_prim, at_mem);
    }

    // act->stateful.append(stateful);
    act->per_flow_enables.insert(per_flow_enables.begin(), per_flow_enables.end());
    act->meter_types.insert(meter_types.begin(), meter_types.end());
    return act;
}

template <class T>
static T *clone(const T *ir) {
    return ir ? ir->clone() : nullptr;
}

Visitor::profile_t DoInstructionSelection::init_apply(const IR::Node *root) {
    auto rv = MauTransform::init_apply(root);
    return rv;
}

IR::Member *DoInstructionSelection::genIntrinsicMetadata(gress_t gress, cstring header,
                                                         cstring field) {
    auto metadataName = cstring::to_cstring(gress) + "::" + header;
    auto *pipe = findContext<IR::BFN::Pipe>();
    if (!pipe) return nullptr;

    auto *meta = pipe->metadata[metadataName];
    if (!meta) {
        BUG("Unable to find metadata %s", metadataName);
        return nullptr;
    }
    if (auto *f = meta->type->getField(field))
        return new IR::Member(f->type, new IR::ConcreteHeaderRef(meta), field);
    BUG("No field %s in %s", field, metadataName);
    return nullptr;
}

const IR::GlobalRef *DoInstructionSelection::preorder(IR::GlobalRef *gr) {
    /* don't recurse through GlobalRefs as they refer to things elsewhere, not stuff
     * we want to turn into VLIW instructions */
    prune();
    return gr;
}

class DoInstructionSelection::SplitInstructions : public Transform {
    IR::Vector<IR::MAU::Primitive> &split;
    const IR::Expression *postorder(IR::MAU::Instruction *inst) override {
        if (inst->operands.empty()) return inst;
        if (auto *tv = inst->operands[0]->to<IR::TempVar>()) {
            if (getContext() != nullptr) {
                LOG3("splitting instruction " << inst);
                split.push_back(inst);
                return tv;
            }
        }
        return inst;
    }
    const IR::MAU::AttachedOutput *preorder(IR::MAU::AttachedOutput *ao) override {
        // don't recurse into attached tables read by instructions.
        prune();
        return ao;
    }

 public:
    explicit SplitInstructions(IR::Vector<IR::MAU::Primitive> &s) : split(s) {}
};

// Create a compiler generated table per lowest common table sequence having saturated subtract
// operation that require a constant to be applied to a metadata PHV. The generated metadata PHV
// can be shared among different saturated subtract as long as they refer to the same value and
// refer to a container of the same size. We can assume that typically multiple actions can lead to
// a saturated subtract to the same value under the same table sequence, e.g.:
//
//  action l3_switch(PortId_t port, bit<48> new_mac_da, bit<48> new_mac_sa) {
//      hdr.ethernet.dst_addr = new_mac_da;
//      hdr.ethernet.src_addr = new_mac_sa;
//      hdr.ipv4.ttl = hdr.ipv4.ttl |-| 1;
//      send(port);
//  }
//
//  table ipv4_host {
//      key = { hdr.ipv4.dst_addr : exact; }
//      actions = { send; drop; l3_switch; }
//  }
//
//  table ipv4_lpm {
//      key     = { hdr.ipv4.dst_addr : lpm; }
//      actions = { send; drop; l3_switch; }
//  }
//
// The two tables above will share the same "const_to_phv_8w1" metadata PHV to carry the constant
// value "1" and the compiler generated table will be added at the beginning of the table sequence
// that include them.
//
// It is also possible to have saturated subtract from two different table sequence, e.g.:
//
//  apply {
//      if (hdr.ipv4.isValid()) {
//          hdr.ipv4.total_len = hdr.ipv4.total_len |-| 10;
//          if (!meta.ipv4_checksum_err && hdr.ipv4.ttl > 1) {
//              if (!ipv4_host.apply().hit) {
//                  ipv4_lpm.apply();
//              }
//          }
//      }
//      if (hdr.vlan_tag[0].isValid()) {
//          hdr.ipv4.total_len = hdr.ipv4.total_len |-| 10;
//      }
//  }
//
// For this example, two compiler generated table will be added, one for each table sequence. The
// metadata PHV will have the same "const_to_phv_16w10" name but will be part of two different
// table "ingress_assign_const_to_phv_0" vs "ingress_assign_const_to_phv_1". Ultimately the
// metadata PHV selected should be the same because of overlay.

/* TODO: potential optimization:
 * consider a IR tree like this:
 *                (TableSeq a)
 *                  /     \
 *       (TableSeq b)      (TableSeq c)
 *         /    \             /    \
 *      SubSat1 ...          ...   SubSat2
 * In current solution, an action will be inserted into TableSeq b and an action will be inserted
 * into TableSeq C to initialize SubSat1 and SubSat2 respectively. However, it can be more optimal
 * if we only insert one action in TableSeq a to initialize SubSat1 and SubSat2 together.
 */
const IR::MAU::TableSeq *DoInstructionSelection::postorder(IR::MAU::TableSeq *ts) {
    static int id = 0;

    if (!const_to_phv.empty() && this->ts == ts) {
        LOG5("DoInstructionSelection Postorder adding temp variable to " << ts);

        gress_t gress = VisitingThread(this);

        auto action = new IR::MAU::Action("__assign_const_to_phv__");
        action->default_allowed = action->init_default = true;

        for (const auto inst : const_to_phv) {
            LOG3("Adding instruction " << inst.first << " to table");
            action->action.push_back(inst.second);
        }

        auto table_name = toString(gress) + "_assign_const_to_phv_" + cstring::to_cstring(id++);
        auto t = new IR::MAU::Table(table_name, gress);

        t->is_compiler_generated = true;
        t->actions[action->name] = action;
        t->match_table = new IR::P4Table(table_name.c_str(), new IR::TableProperties());

        ts->tables.insert(ts->tables.begin(), t);
        const_to_phv.clear();
        this->ts = nullptr;
    }

    return ts;
}

const IR::MAU::Action *DoInstructionSelection::postorder(IR::MAU::Action *af) {
    IR::Vector<IR::MAU::Primitive> split;
    // FIXME: This should be pulled out as a different pass
    for (auto *p : af->action) split.push_back(p->apply(SplitInstructions(split)));
    if (split.size() > af->action.size()) af->action = std::move(split);

    LOG5("Postorder " << af);
    this->af = nullptr;
    return af;
}

const IR::MAU::Action *DoInstructionSelection::preorder(IR::MAU::Action *af) {
    BUG_CHECK(findContext<IR::MAU::Action>() == nullptr, "Nested action functions");
    LOG2("DoInstructionSelection processing action " << af->name);
    LOG5(af);
    this->af = af;
    return af;
}

bool DoInstructionSelection::checkPHV(const IR::Expression *e) {
    if (auto *c = e->to<IR::BFN::ReinterpretCast>()) return checkPHV(c->expr);
    return phv.field(e);
}

bool DoInstructionSelection::checkActionBus(const IR::Expression *e) {
    if (auto *c = e->to<IR::BFN::ReinterpretCast>()) return checkActionBus(c->expr);
    if (auto *c = e->to<IR::BFN::SignExtend>()) return checkActionBus(c->expr);
    if (auto slice = e->to<IR::Slice>()) return checkActionBus(slice->e0);
    if (e->is<IR::Constant>()) return true;
    if (e->is<IR::BoolLiteral>()) return true;
    if (e->is<IR::MAU::ActionArg>()) return true;
    if (e->is<IR::MAU::HashDist>()) return true;
    if (e->is<IR::MAU::RandomNumber>()) return true;
    if (e->is<IR::MAU::AttachedOutput>()) return true;
    if (e->is<IR::MAU::StatefulCounter>()) return true;
    if (auto m = e->to<IR::Member>())
        if (m->expr->is<IR::MAU::AttachedOutput>()) return true;
    return false;
}

bool DoInstructionSelection::checkSrc1(const IR::Expression *e) {
    LOG3("Checking src1 : " << e);
    if (auto *c = e->to<IR::BFN::ReinterpretCast>()) return checkSrc1(c->expr);
    if (auto *c = e->to<IR::BFN::SignExtend>()) return checkSrc1(c->expr);
    if (auto slice = e->to<IR::Slice>()) return checkSrc1(slice->e0);
    if (checkActionBus(e)) return true;
    return phv.field(e);
}

bool DoInstructionSelection::checkConst(const IR::Expression *ex, long &value) {
    if (auto *k = ex->to<IR::Constant>()) {
        value = k->asLong();  // \TODO: long or 64-bit value?
        return true;
    } else {
        return false;
    }
}

bool DoInstructionSelection::equiv(const IR::Expression *a, const IR::Expression *b) {
    if (*a == *b) return true;
    if (typeid(*a) != typeid(*b)) return false;
    if (auto ca = a->to<IR::BFN::ReinterpretCast>()) {
        auto cb = b->to<IR::BFN::ReinterpretCast>();
        return ca->type == cb->type && equiv(ca->expr, cb->expr);
    }
    if (auto sa = a->to<IR::Slice>()) {
        if (auto sb = b->to<IR::Slice>()) {
            return sa->getL() == sb->getL() && sa->getH() == sb->getH() && equiv(sa->e0, sb->e0);
        }
    }
    return false;
}

void DoInstructionSelection::limitWidth(const IR::Expression *e) {
    // Verify that the operation width is less than the maximum container width.
    // Required for instructions that can't be split without rewriting the
    // instruction.
    auto bits = e->type->to<IR::Type_Bits>();
    unsigned short max_size =
        static_cast<unsigned short>(*Device::phvSpec().containerSizes().rbegin());
    if (bits && bits->width_bits() > max_size) {
        error(ErrorType::ERR_OVERLIMIT,
              "%1%: Saturating arithmetic operators may not exceed "
              "maximum PHV container width (%2%b)",
              e->srcInfo, max_size);
    }
}

const IR::Expression *DoInstructionSelection::postorder(IR::BoolLiteral *bl) {
    if (!findContext<IR::MAU::Action>()) return bl;
    return new IR::Constant(IR::Type::Bits::get(1), static_cast<int>(bl->value));
}

const IR::Expression *DoInstructionSelection::postorder(IR::BAnd *e) {
    if (!af) return e;
    auto *left = e->left, *right = e->right;
    std::string op = "and";
    auto *l = left->to<IR::MAU::Instruction>();
    auto *r = right->to<IR::MAU::Instruction>();
    if (l && l->name == "not" && r && r->name == "not") {
        left = l->operands[1];
        right = r->operands[1];
        op = "nor";
    } else if (l && l->name == "not") {
        left = l->operands[1];
        op = "andca";
    } else if (r && r->name == "not") {
        right = r->operands[1];
        op = "andcb";
    }
    return new IR::MAU::Instruction(e->srcInfo, cstring(op), new IR::TempVar(e->type), left, right);
}

const IR::Expression *DoInstructionSelection::postorder(IR::BOr *e) {
    if (!af) return e;
    auto *left = e->left, *right = e->right;
    std::string op = "or";
    auto *l = left->to<IR::MAU::Instruction>();
    auto *r = right->to<IR::MAU::Instruction>();
    if (l && l->name == "not" && r && r->name == "not") {
        left = l->operands[1];
        right = r->operands[1];
        op = "nand";
    } else if (l && l->name == "not") {
        left = l->operands[1];
        op = "orca";
    } else if (r && r->name == "not") {
        right = r->operands[1];
        op = "orcb";
    }
    return new IR::MAU::Instruction(e->srcInfo, cstring(op), new IR::TempVar(e->type), left, right);
}

const IR::Expression *DoInstructionSelection::postorder(IR::BXor *e) {
    if (!af) return e;
    auto *left = e->left, *right = e->right;
    std::string op = "xor";
    auto *l = left->to<IR::MAU::Instruction>();
    auto *r = right->to<IR::MAU::Instruction>();
    if (l && l->name == "not" && r && r->name == "not") {
        left = l->operands[1];
        right = r->operands[1];
    } else if (l && l->name == "not") {
        left = l->operands[1];
        op = "xnor";
    } else if (r && r->name == "not") {
        right = r->operands[1];
        op = "xnor";
    }
    return new IR::MAU::Instruction(e->srcInfo, cstring(op), new IR::TempVar(e->type), left, right);
}

const IR::Expression *DoInstructionSelection::postorder(IR::Cmpl *e) {
    if (!af) return e;
    if (auto *fold = ::clone(e->expr->to<IR::MAU::Instruction>())) {
        if (fold->name == "and")
            fold->name = "nand"_cs;
        else if (fold->name == "andca")
            fold->name = "orcb"_cs;
        else if (fold->name == "andcb")
            fold->name = "orca"_cs;
        else if (fold->name == "nand")
            fold->name = "and"_cs;
        else if (fold->name == "nor")
            fold->name = "or"_cs;
        else if (fold->name == "or")
            fold->name = "nor"_cs;
        else if (fold->name == "orca")
            fold->name = "andcb"_cs;
        else if (fold->name == "orcb")
            fold->name = "andca"_cs;
        else if (fold->name == "xnor")
            fold->name = "xor"_cs;
        else if (fold->name == "xor")
            fold->name = "xnor"_cs;
        else
            fold = nullptr;
        if (fold) return fold;
    }
    return new IR::MAU::Instruction(e->srcInfo, "not"_cs, new IR::TempVar(e->type), e->expr);
}

const IR::Expression *DoInstructionSelection::postorder(IR::Add *e) {
    if (!af) return e;
    /// HACK(hanw): I had to put in this hack to get p4-14 switch.p4 to compile after removing the
    /// cast, we need a follow-up PR to add the support for '++' operator in the backen.
    auto operand = e->right;
    if (auto concat = e->right->to<IR::Concat>()) {
        if (concat->left->is<IR::Constant>()) {
            operand = concat->right;
        }
    }
    return new IR::MAU::Instruction(e->srcInfo, "add"_cs, new IR::TempVar(e->type), e->left,
                                    operand);
}

const IR::Expression *DoInstructionSelection::postorder(IR::AddSat *e) {
    if (!af) return e;
    auto opName = "saddu"_cs;
    if (e->type->is<IR::Type_Bits>() && e->type->to<IR::Type_Bits>()->isSigned) opName = "sadds"_cs;
    limitWidth(e);
    return new IR::MAU::Instruction(e->srcInfo, opName, new IR::TempVar(e->type), e->left,
                                    e->right);
}

const IR::Expression *DoInstructionSelection::postorder(IR::SubSat *e) {
    if (!af) return e;
    auto opName = "ssubu";
    auto eLeft = e->left;
    auto eRight = e->right;
    auto bits = e->type->to<IR::Type_Bits>();
    if (bits && bits->isSigned) {
        opName = "ssubs";
        if (auto *k = eRight->to<IR::Constant>()) {
            // Dont convert to an add when constant is equal to maximum negative
            // value as the value will overflow upon negation.
            // e.g. For an 8 bit subtraction
            // ssubs b, a, -128 => sadds b, a, 128 (but 128 in 8 bits overflows and becomes -128).
            if (k->asLong() != -(1LL << (bits->width_bits() - 1))) {
                opName = "sadds";
                eLeft = (-*k).clone();
                eRight = e->left;
            }
        }
    }
    limitWidth(e);

    // Compiler generates an invalid instruction here for saturated unsigned
    // subtract with a constant value as src2 which is unsupported in
    // Tofino/JBAY.
    // P4:   hdr.ipv4.ttl = hdr.ipv4.ttl |-| 1;
    // BFA:  - ssubu hdr.ipv4.ttl, hdr.ipv4.ttl, 1
    //
    // Soln : Rewrite P4 by initializing the "1" through a field which makes it
    // a PHV which is supported as src2
    //
    // Unsupported alternate solutions:
    // 1. Use the ADD instruction: src1 (immediate -1) + src2 (unsigned PHV field)
    //    and then detect underflow from 0 to 255 in a subsequent stage and map back to 0.
    // 2. Instead of using an additional stage, use a table in this stage to detect TTL
    //    value of 0 to conditionally execute the subtract or not
    // Both these solutions require program transformations
    if (bits && !bits->isSigned && eRight->to<IR::Constant>()) {
        if (this->ts == nullptr) {
            this->ts = findContext<IR::MAU::TableSeq>();
        }
        cstring temp_name = "const_to_phv_"_cs + cstring::to_cstring(bits->width_bits()) + "w"_cs +
                            cstring::to_cstring(eRight->to<IR::Constant>()->asLong());

        const IR::MAU::Instruction *temp_inst;
        const IR::TempVar *temp_var;
        if (const_to_phv.count(temp_name)) {
            temp_inst = const_to_phv.at(temp_name);
            BUG_CHECK(!temp_inst->operands.empty(), "Incomplete temporary instruction");
            temp_var = temp_inst->operands[0]->to<IR::TempVar>();
        } else {
            temp_var = new IR::TempVar(IR::Type::Bits::get(bits->width_bits()), false, temp_name);
            temp_inst = new IR::MAU::Instruction(e->srcInfo, "set"_cs, temp_var, eRight);
            const_to_phv[temp_name] = temp_inst;
        }
        return new IR::MAU::Instruction(e->srcInfo, cstring(opName), new IR::TempVar(e->type),
                                        eLeft, temp_var);
    }

    return new IR::MAU::Instruction(e->srcInfo, cstring(opName), new IR::TempVar(e->type), eLeft,
                                    eRight);
}

const IR::Expression *DoInstructionSelection::postorder(IR::Sub *e) {
    if (!af) return e;
    if (auto *k = e->right->to<IR::Constant>())
        return new IR::MAU::Instruction(e->srcInfo, "add"_cs, new IR::TempVar(e->type),
                                        new IR::Constant(k->type, -k->value), e->left);
    return new IR::MAU::Instruction(e->srcInfo, "sub"_cs, new IR::TempVar(e->type), e->left,
                                    e->right);
}
const IR::Expression *DoInstructionSelection::postorder(IR::Neg *e) {
    if (!af) return e;
    return new IR::MAU::Instruction(e->srcInfo, "sub"_cs, new IR::TempVar(e->type),
                                    new IR::Constant(e->srcInfo, e->type, 0), e->expr);
}

const IR::Expression *DoInstructionSelection::postorder(IR::Shl *e) {
    if (!af) return e;
    if (!e->right->is<IR::Constant>())
        error("%s: shift count must be a constant in %s", e->srcInfo, e);
    return new IR::MAU::Instruction(e->srcInfo, "shl"_cs, new IR::TempVar(e->type), e->left,
                                    e->right);
}

const IR::Expression *DoInstructionSelection::postorder(IR::Shr *e) {
    if (!af) return e;
    if (!e->right->is<IR::Constant>())
        error("%s: shift count must be a constant in %s", e->srcInfo, e);
    std::string shr = "shru";
    if (e->type->is<IR::Type_Bits>() && e->type->to<IR::Type_Bits>()->isSigned) shr = "shrs";
    return new IR::MAU::Instruction(e->srcInfo, cstring(shr), new IR::TempVar(e->type), e->left,
                                    e->right);
}

const IR::Expression *DoInstructionSelection::postorder(IR::Operation_Relation *e) {
    if (!af) return e;
    // Skip the transform if it's in a Mux -- the Mux should handle it
    if (getParent<IR::Mux>()) return e;
    if (Device::hasCompareInstructions()) {
        auto isSigned =
            (e->left->type->is<IR::Type_Bits>() && e->left->type->to<IR::Type_Bits>()->isSigned) ||
            (e->right->type->is<IR::Type_Bits>() && e->right->type->to<IR::Type_Bits>()->isSigned);
        auto isWide = e->left->type->width_bits() > 32 || e->right->type->width_bits() > 32;
        auto opName = "";
        if (e->is<IR::Equ>()) {
            opName = isWide ? "eq64" : "eq";
        } else if (e->is<IR::Neq>()) {
            opName = isWide ? "neq64" : "neq";
        } else if (e->is<IR::Lss>()) {
            opName = isSigned ? "lts" : "ltu";
        } else if (e->is<IR::Leq>()) {
            opName = isSigned ? "leqs" : "lequ";
        } else if (e->is<IR::Grt>()) {
            opName = isSigned ? "gts" : "gtu";
        } else if (e->is<IR::Geq>()) {
            opName = isSigned ? "gteqs" : "gtequ";
        } else {
            error("%1%: Unknown relational operator", e, e->node_type_name());
        }
        return new IR::MAU::Instruction(e->srcInfo, cstring(opName), new IR::TempVar(e->type),
                                        e->left, e->right);
    } else {
        return e;
    }
}

const IR::Expression *DoInstructionSelection::postorder(IR::Mux *e) {
    if (auto r = e->e0->to<IR::Operation_Relation>()) {
        bool isMin = false;
        if (r->is<IR::Lss>() || r->is<IR::Leq>())
            isMin = true;
        else if (!r->is<IR::Grt>() && !r->is<IR::Geq>())
            return e;
        if (equiv(r->left, e->e2) && equiv(r->right, e->e1))
            isMin = !isMin;
        else if (!equiv(r->left, e->e1) || !equiv(r->right, e->e2))
            return e;
        cstring op = isMin ? "minu"_cs : "maxu"_cs;
        if (auto t = r->left->type->to<IR::Type::Bits>())
            if (t->isSigned) op = isMin ? "mins"_cs : "maxs"_cs;
        return new IR::MAU::Instruction(e->srcInfo, op, new IR::TempVar(e->type), e->e1, e->e2);
    }
    return e;
}

const IR::Slice *DoInstructionSelection::postorder(IR::Slice *sl) {
    if (auto expr = sl->e0->to<IR::Slice>()) {
        sl->e0 = expr->e0;
        sl->e1 = new IR::Constant(sl->getH() + expr->getL());
        sl->e2 = new IR::Constant(sl->getL() + expr->getL());
        BUG_CHECK(int(sl->getH()) < sl->e0->type->width_bits(), "invalid slice on slice");
    }
    return sl;
}

const IR::Expression *DoInstructionSelection::preorder(IR::BFN::SignExtend *c) {
    if (CanBeIXBarExpr(c))
        BUG("%s: Hash Dist object on concat %s not correctly converted", c->srcInfo, c->toString());
    return c;
}

const IR::Expression *DoInstructionSelection::preorder(IR::Concat *c) {
    if (auto k = c->left->to<IR::Constant>()) {
        if (k->value == 0) {
            // HACK -- avoid dealing with 0-prefix concats (zero extension) as the midend
            // now changes zero-extend casts into them, and trying to do them in the hash
            // breaks more things than it fixes
            return c;
        }
    }
    if (CanBeIXBarExpr(c))
        BUG("%s: Hash Dist object on concat %s not correctly converted", c->srcInfo, c->toString());
    return c;
}

static const IR::MAU::Instruction *fillInstDest(const IR::Expression *in,
                                                const IR::Expression *dest, int lo = -1,
                                                int hi = -1, bool cast = false) {
    if (auto *c = in->to<IR::BFN::ReinterpretCast>())
        // perhaps everything underneath should be cast to the same size
        return fillInstDest(c->expr, dest, c->destType->width_bits(), -1, true);
    if (auto *sl = in->to<IR::Slice>()) return fillInstDest(sl->e0, dest, sl->getL(), sl->getH());
    if (auto *c = in->to<IR::BFN::SignExtend>())
        return fillInstDest(c->expr, dest, c->destType->width_bits(), -1);
    auto *inst = in ? in->to<IR::MAU::Instruction>() : nullptr;
    auto *tv = inst ? inst->operands[0]->to<IR::TempVar>() : nullptr;
    auto *sl = inst ? inst->operands[0]->to<IR::Slice>() : nullptr;
    if (!tv) tv = sl ? sl->e0->to<IR::TempVar>() : nullptr;
    if (tv) {
        int id;
        if (sscanf(tv->name, "$tmp%d", &id) > 0 && id == tv->uid) --tv->uid;
        auto *rv = inst->clone();
        if (sl != nullptr) dest = MakeSlice(dest, 0, sl->type->width_bits() - 1);
        rv->operands[0] = dest;
        for (size_t i = 1; i < rv->operands.size(); i++) {
            if (lo == -1) break;
            if (cast) continue;
            // The last operand of a shift operation is the shift value. This constant must not be
            // sliced but returned as is.
            if ((inst->name != "shrs" && inst->name != "shru" && inst->name != "shl") ||
                (i != (rv->operands.size() - 1)))
                rv->operands[i] = MakeSlice(rv->operands[i], lo, hi);
        }
        return rv;
    }
    return nullptr;
}

static bool isDepositMask(long) {
    /* TODO */
    return false;
}
static const IR::MAU::Primitive *makeDepositField(IR::MAU::Primitive *prim, long) {
    /* TODO */
    return prim;
}

const IR::Node *DoInstructionSelection::postorder(IR::MAU::Primitive *prim) {
    if (!af) return prim;

    /*
     * Checks whether the expression is a reinterpret cast of 0 ++ expression,
     * i.e. a sign change of a zero-extended expression.
     * E.g.
     *      bit<M> a; bit<N> b = (int<N>)(bit<N>)a;  // where M < N
     * This statement is translated in midend to:
     *      bit<M> a; bit<N> b = (int<N>)((bit<N - M>)0 ++ a);
     */
    auto is_reinterpret_cast_concat_0 = [](const IR::Expression *e) {
        if (auto *rc = e->to<IR::BFN::ReinterpretCast>())
            if (auto *cc = rc->expr->to<IR::Concat>())
                if (auto *k = cc->left->to<IR::Constant>())
                    if (k->value == 0) return true;
        return false;
    };

    const IR::Expression *dest = prim->operands.size() > 0 ? prim->operands[0] : nullptr;
    LOG4("DoInstructionSelection::postorder on primitive " << prim->name);
    if (prim->name == "modify_field") {
        long mask;
        if ((prim->operands.size() | 1) != 3) {
            error(ErrorType::ERR_INVALID, "wrong number of operands to %1%", prim);
        } else if (!phv.field(dest)) {
            error(ErrorType::ERR_INVALID, "destination of %1% must be a field", prim);
        } else if (auto *rv = fillInstDest(prim->operands[1], dest)) {
            return rv;
        } else if (is_reinterpret_cast_concat_0(prim->operands[1])) {
            /*
             * Translates a primitive of the form:
             *   modify_field A, (int<M>)(0 ++ B) where A is bit<M> and B is bit<N>
             * To two instructions:
             *   set A[M-1:N], 0
             *   set A[N-1:0], B
             */
            auto *cc = prim->operands[1]->to<IR::BFN::ReinterpretCast>()->expr->to<IR::Concat>();
            auto *k = cc->left;
            auto *inst_phv = new IR::MAU::Instruction(
                prim->srcInfo, "set"_cs, MakeSlice(dest, 0, cc->right->type->width_bits() - 1),
                cc->right);
            auto *inst_const = new IR::MAU::Instruction(
                prim->srcInfo, "set"_cs,
                MakeSlice(dest, cc->right->type->width_bits(), dest->type->width_bits() - 1), k);
            return new IR::Vector<IR::MAU::Primitive>({inst_const, inst_phv});
        } else if (!checkSrc1(prim->operands[1])) {
            /**
             * Converting ternary operands into conditionally-set format:
             *
             *     conditionally-set destination, (optional background), source, conditional arg
             *
             * In examples:
             *     f1 = cond ? arg1 : f1 -> conditionally-set f1, arg1, cond1 (implicit background)
             *
             *     f1 = cond ? arg1 : f2 -> conditionally-set f1, f2, arg1, cond1
             */
            if (auto mux = prim->operands[1]->to<IR::Mux>()) {
                auto type = prim->operands[0]->type->to<IR::Type::Bits>();
                auto arg = mux->e0->to<IR::MAU::ActionArg>();
                if (!arg) {
                    error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                          "%1%\nConditions in an action must "
                          "be simple comparisons of an action data parameter\nTry moving the test "
                          "out of the action and into a control apply block, or making it part "
                          "of the table key",
                          prim->srcInfo);
                    return prim;
                }
                cstring cond_arg_name = "$cond_arg"_cs + std::to_string(synth_arg_num++);
                auto cond_arg = new IR::MAU::ConditionalArg(mux->e0->srcInfo, type, af->name,
                                                            cond_arg_name, arg);
                cstring instr_name = "conditionally-set"_cs;
                IR::MAU::Instruction *rv = nullptr;
                if (checkActionBus(mux->e1) && checkPHV(mux->e2)) {
                    rv = new IR::MAU::Instruction(prim->srcInfo, instr_name,
                                                  {prim->operands[0], mux->e2, mux->e1, cond_arg});
                } else if (checkActionBus(mux->e2) && checkPHV(mux->e1)) {
                    cond_arg->one_on_true = false;
                    rv = new IR::MAU::Instruction(prim->srcInfo, instr_name,
                                                  {prim->operands[0], mux->e1, mux->e2, cond_arg});
                    error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                          "%1%\nConditional assignment must "
                          "be reversed, as the non PHV parameter must be on the true branch for "
                          "support in the driver",
                          prim->srcInfo);
                } else {
                    error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                          "%1%\nConditional assignment is "
                          "too complicated to support in a single operation\nTry moving the test "
                          "out of the action and into a control apply block, or making it part "
                          "of the table key",
                          prim->srcInfo);
                }
                return rv;
            } else {
                error("%s: source of %s invalid", prim->srcInfo, prim->name);
            }
        } else if (prim->operands.size() == 2) {
            return new IR::MAU::Instruction(prim->srcInfo, "set"_cs, &prim->operands);
        } else if (!checkConst(prim->operands[2], mask)) {
            error(ErrorType::ERR_INVALID, "mask of %1% must be a constant", prim);
        } else if (dest && (1L << dest->type->width_bits() == mask + 1)) {
            return new IR::MAU::Instruction(prim->srcInfo, "set"_cs, dest, prim->operands[1]);
        } else if (isDepositMask(mask)) {
            return makeDepositField(prim, mask);
        } else {
            return new IR::MAU::Instruction(prim->srcInfo, "bitmasked-set"_cs, &prim->operands);
        }
        return prim;
    }

    // get rid of introduced tempvars
    for (auto &op : prim->operands) {
        if (auto *inst = op->to<IR::MAU::Instruction>()) {
            if (inst->name == "set" && inst->operands.at(0)->is<IR::TempVar>()) {
                BUG_CHECK(inst->operands.size() == 2, "invalid set");
                op = inst->operands.at(1);
            }
        }
    }

    if (prim->name == "Hash.get") {
        BUG("%s: Should have already converted %s in previous pass", prim->srcInfo,
            prim->toString());
    } else if (prim->name == "Random.get") {
        auto glob = prim->operands[0]->to<IR::GlobalRef>();
        auto decl = glob->obj->to<IR::Declaration_Instance>();
        auto rn = new IR::MAU::RandomNumber(prim->srcInfo, prim->type, decl->name);
        auto next_type = prim->type;
        return new IR::MAU::Instruction(prim->srcInfo, "set"_cs, new IR::TempVar(next_type), rn);
    } else if (prim->name == "invalidate") {
        return new IR::MAU::Instruction(prim->srcInfo, "invalidate"_cs, prim->operands[0]);
    } else if (prim->name == "min" || prim->name == "max") {
        if (prim->operands.size() != 2) {
            error(ErrorType::ERR_INVALID, "wrong number of operands to %1%", prim);
        } else if (!prim->type->is<IR::Type::Bits>()) {
            error(ErrorType::ERR_INVALID, "non-numeric operrands to %1%", prim);
        } else {
            cstring op = prim->name;
            op += prim->type->to<IR::Type::Bits>()->isSigned ? 's' : 'u';
            return new IR::MAU::Instruction(prim->srcInfo, op, new IR::TempVar(prim->type),
                                            prim->operands.at(0), prim->operands.at(1));
        }
    } else {
        LOG1("WARNING: unhandled in InstSel: " << *prim);
    }
    return prim;
}

static const IR::Type *stateful_type_for_primitive(const IR::MAU::Primitive *prim) {
    if (prim->name == "Counter.count" || prim->name == "DirectCounter.count")
        return IR::Type_Counter::get();
    if (prim->name == "Meter.execute" || prim->name == "DirectMeter.execute" ||
        prim->name == "Lpf.execute" || prim->name == "DirectLpf.execute" ||
        prim->name == "Wred.execute" || prim->name == "DirectWred.execute")
        return IR::Type_Meter::get();
    if (auto a = strstr(prim->name, "Action")) {
        if (a[6] == '.' || (std::isdigit(a[6]) && a[7] == '.')) return IR::Type_Register::get();
    }
    if (prim->name == "Register.clear" || prim->name == "DirectRegister.clear")
        return IR::Type_Register::get();
    BUG("Not a stateful primitive %s", prim);
}

static ssize_t index_operand(const IR::MAU::Primitive *prim) {
    if (prim->name.startsWith("Direct") || prim->name.endsWith(".clear"))
        return -1;
    else if (prim->name.startsWith("Counter") || prim->name.startsWith("Meter") ||
             prim->name.endsWith("Action.execute"))
        return 1;
    else if (strstr(prim->name, "Action."))
        return -1;
    else if (prim->name.startsWith("Lpf") || prim->name.startsWith("Wred"))
        return 2;
    return 1;
}

static size_t input_operand(const IR::MAU::Primitive *prim) {
    if (prim->name.startsWith("Lpf") || prim->name.startsWith("Wred") ||
        prim->name.startsWith("DirectLpf") || prim->name.startsWith("DirectWred"))
        return 1;
    else
        return -1;
}

static size_t precolor_operand(const IR::MAU::Primitive *prim) {
    if (prim->name.startsWith("DirectMeter") || prim->name.startsWith("Meter")) {
        if (auto tprim = prim->to<IR::MAU::TypedPrimitive>()) {
            for (auto o : tprim->op_names) {
                if (o.second == "color") return o.first;
            }
        }
    }

    return -1;
}

bool StatefulAttachmentSetup::Scan::preorder(const IR::MAU::Action *act) {
    self.remove_tempvars.clear();
    self.copy_propagated_tempvars.clear();
    if (act->stateful_calls.empty()) return false;

    for (auto call : act->stateful_calls) {
        auto prim = call->prim;
        BUG_CHECK(prim->operands.size() >= 1, "Invalid primitive %s", prim);
        auto gref = prim->operands[0]->to<IR::GlobalRef>();
        BUG_CHECK(gref, "No object named %s", prim->operands[0]);

        auto find_and_save_temp_var = [&](const IR::Expression *expr) {
            if (auto *tv = expr->to<IR::TempVar>()) {
                self.remove_tempvars.insert(tv->name);
                self.copy_propagated_tempvars.insert(tv->name);
            } else {
                // it is possible to have a slice of TempVar as the operand for a SALU call.
                if (auto *slice = expr->to<IR::Slice>()) {
                    if (auto tv = slice->e0->to<IR::TempVar>()) {
                        self.copy_propagated_tempvars.insert(tv->name);
                    }
                }
            }
        };

        if (prim->operands.size() >= 2) {
            find_and_save_temp_var(prim->operands[1]);
            if (auto *c = prim->operands[1]->to<IR::BFN::ReinterpretCast>()) {
                find_and_save_temp_var(c->expr);
            }
            if (auto *cc = prim->operands[1]->to<IR::Concat>()) {
                std::vector<const IR::Expression *> possible_vars;
                simpl_concat(possible_vars, cc);
                for (auto expr : possible_vars) {
                    find_and_save_temp_var(expr);
                }
            }
        }
    }
    return true;
}

bool StatefulAttachmentSetup::Scan::preorder(const IR::MAU::Instruction *) {
    self.saved_tempvar = nullptr;
    self.saved_hashdist = nullptr;
    return true;
}

bool StatefulAttachmentSetup::Scan::preorder(const IR::TempVar *tv) {
    if (self.copy_propagated_tempvars.count(tv->name)) self.saved_tempvar = tv;
    return true;
}

bool StatefulAttachmentSetup::Scan::preorder(const IR::MAU::HashDist *hd) {
    self.saved_hashdist = hd;
    return true;
}

void StatefulAttachmentSetup::Scan::postorder(const IR::MAU::Instruction *instr) {
    if (self.saved_tempvar && self.saved_hashdist) {
        if (self.copy_propagated_tempvars.count(self.saved_tempvar->name)) {
            self.stateful_alu_from_hash_dists[self.saved_tempvar->name] = self.saved_hashdist;
        }
        if (self.remove_tempvars.count(self.saved_tempvar->name)) {
            self.remove_instr.insert(instr);
        }
    }
}

void StatefulAttachmentSetup::Scan::postorder(const IR::MAU::Primitive *prim) {
    const IR::MAU::AttachedMemory *obj = nullptr;
    use_t use = IR::MAU::StatefulUse::NO_USE;
    auto dot = prim->name.find('.');
    cstring method = dot ? cstring(dot + 1) : prim->name;
    while (dot && dot > prim->name && std::isdigit(dot[-1])) --dot;
    auto objType = dot ? prim->name.before(dot) : cstring();
    if (objType.endsWith("Action")) {
        obj = prim->operands.at(0)->to<IR::GlobalRef>()->obj->to<IR::MAU::StatefulAlu>();
        BUG_CHECK(obj, "invalid object");
        if (method == "execute") {
            if (objType == "DirectRegisterAction")
                use = IR::MAU::StatefulUse::DIRECT;
            else
                use = IR::MAU::StatefulUse::INDIRECT;
        } else if (method == "execute_log") {
            use = IR::MAU::StatefulUse::LOG;
        } else if (method == "enqueue") {
            use = IR::MAU::StatefulUse::FIFO_PUSH;
        } else if (method == "dequeue") {
            use = IR::MAU::StatefulUse::FIFO_POP;
        } else if (method == "push") {
            use = IR::MAU::StatefulUse::STACK_PUSH;
        } else if (method == "pop") {
            use = IR::MAU::StatefulUse::STACK_POP;
        } else if (method == "sweep") {
            use = IR::MAU::StatefulUse::FAST_CLEAR;
        } else {
            BUG("Unknown %s method %s in: %s", objType, method, prim);
        }
    } else if (method == "execute") {
        obj = prim->operands.at(0)->to<IR::GlobalRef>()->obj->to<IR::MAU::Meter>();
        BUG_CHECK(obj, "invalid object");
        use = objType.startsWith("Direct") ? IR::MAU::StatefulUse::DIRECT
                                           : IR::MAU::StatefulUse::INDIRECT;
    } else if (method == "count") {
        obj = prim->operands.at(0)->to<IR::GlobalRef>()->obj->to<IR::MAU::Counter>();
        BUG_CHECK(obj, "invalid object");
        use = objType.startsWith("Direct") ? IR::MAU::StatefulUse::DIRECT
                                           : IR::MAU::StatefulUse::INDIRECT;
    } else if (method == "clear") {
        obj = prim->operands.at(0)->to<IR::GlobalRef>()->obj->to<IR::MAU::StatefulAlu>();
        BUG_CHECK(obj, "invalid object");
        use = IR::MAU::StatefulUse::FAST_CLEAR;
        bitvec clear_value;
        uint32_t busy_value = 0;
        if (prim->operands.size() > 1) {
            auto *v = prim->operands.at(1)->to<IR::Constant>();
            if (!v) {
                error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "clear value %1% must be a constant",
                      prim->operands.at(1));
            } else {
                clear_value.putrange(0, 64, static_cast<uint64_t>(v->value));
                clear_value.putrange(64, 64,
                                     static_cast<uint64_t>(static_cast<big_int>(v->value >> 64)));
            }
        }
        if (prim->operands.size() > 2) {
            auto *v = prim->operands.at(2)->to<IR::Constant>();
            if (!v) {
                error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "busy value %1% must be a constant",
                      prim->operands.at(2));
            } else {
                busy_value = v->asUnsigned();
            }
        }
        auto &info = self.table_clears[findContext<IR::MAU::Table>()];
        if (info) {
            if (obj != info->attached)
                error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "Clearing %1% and %2% in one table",
                      info->attached, obj);
            if (info->clear_value != clear_value || info->busy_value != busy_value)
                error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, "Inconsistent clear arguments in %1%",
                      prim);
        } else {
            info = &self.salu_clears[obj];
            if (info->attached) {
                if (info->clear_value != clear_value || info->busy_value != busy_value)
                    error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                          "Inconsistent clear arguments "
                          "for %1%",
                          info->attached);
            }
            info->attached = obj;
        }
        info->clear_value = clear_value;
        info->busy_value = busy_value;
    }
    if (obj) {
        auto *act = findContext<IR::MAU::Action>();
        use_t &prev_use = self.action_use[act][obj];
        if (prev_use != IR::MAU::StatefulUse::NO_USE && prev_use != use)
            error("Inconsistent use of %s in action %s", obj, act);
        prev_use = use;
    }
}

IR::MAU::HashDist *StatefulAttachmentSetup::create_hash_dist(const IR::Expression *expr,
                                                             const IR::MAU::Primitive *prim) {
    auto hash_field = expr;
    if (auto c = expr->to<IR::BFN::ReinterpretCast>()) hash_field = c->expr;

    int size = hash_field->type->width_bits();
    auto *hge = new IR::MAU::HashGenExpression(prim->srcInfo, IR::Type::Bits::get(size), hash_field,
                                               IR::MAU::HashFunction::identity());

    auto *hd = new IR::MAU::HashDist(hge->srcInfo, hge->type, hge);
    return hd;
}

/**
 * Find or generate a Hash Distribution unit, given what IR::Node is in the primitive
 */
const IR::MAU::HashDist *StatefulAttachmentSetup::find_hash_dist(const IR::Expression *expr,
                                                                 const IR::MAU::Primitive *prim) {
    const IR::MAU::HashDist *hd = expr->to<IR::MAU::HashDist>();
    if (!hd) {
        auto tv = expr->to<IR::TempVar>();
        const IR::Slice *slice = nullptr;

        if (!tv) {
            if ((slice = expr->to<IR::Slice>())) {
                tv = slice->e0->to<IR::TempVar>();
            }
        }
        if (tv != nullptr && stateful_alu_from_hash_dists.count(tv->name)) {
            hd = stateful_alu_from_hash_dists.at(tv->name);
            if (slice) {
                // If a slice of TempVar is used, then create a slice of HashGenExpression
                auto *new_slice = MakeSlice(hd->expr, slice->getL(), slice->getH());
                return new IR::MAU::HashDist(hd->srcInfo, new_slice);
            }
        } else if (phv.field(expr)) {
            hd = create_hash_dist(expr, prim);
        }
    }
    return hd;
}

/**
 * Simplify concatenation expression for index operand.
 * Current limitation:
 * - only one sub-expression in concatenation expression can be a field.
 * - other sub-expression must be constant.
 */
void StatefulAttachmentSetup::Scan::simpl_concat(std::vector<const IR::Expression *> &slices,
                                                 const IR::Concat *expr) {
    if (expr->left->is<IR::Constant>()) {
        // ignore
    } else if (auto lhs = expr->left->to<IR::Concat>()) {
        simpl_concat(slices, lhs);
    } else {
        slices.push_back(expr->left);
    }
    if (expr->right->is<IR::Constant>()) {
        // ignore
    } else if (auto rhs = expr->right->to<IR::Concat>()) {
        simpl_concat(slices, rhs);
    } else {
        slices.push_back(expr->right);
    }
}

/**
 * Determine the index operand for a particular action.  This is to determine the address
 * for the stateful call, which is required for the context JSON.
 *
 * Also currently validates that the address is able to be understood, i.e. if it
 * can be part of a HashDist, Constant, etc.
 *
 * FIXME: a hash.get is not yet translated to a HashDist before this.  The only way to
 * do this is to initialize a temporary variable as this, and use this as an address
 */
void StatefulAttachmentSetup::Scan::setup_index_operand(const IR::Expression *index_expr,
                                                        const IR::MAU::Synth2Port *synth2port,
                                                        const IR::MAU::Table *tbl,
                                                        const IR::MAU::StatefulCall *call) {
    if (!synth2port) return;
    const IR::Expression *simpl_expr = nullptr;
    std::vector<const IR::Expression *> expressions;
    if (index_expr->is<IR::Concat>()) {
        simpl_concat(expressions, index_expr->to<IR::Concat>());

        if (expressions.size() == 1) {
            simpl_expr = expressions.at(0);
        } else {
            error("Complex expression is not yet supported: %1%", index_expr);
            return;
        }
    } else {
        simpl_expr = index_expr;
        if (auto sl = index_expr->to<IR::Slice>()) {
            // corner case -- action arg used both for index and elsewhere in P4_14 which
            // is inferred as larger than the index size (due to the other use).  We end up
            // with a slice on the index which we want to ignore.
            if (sl->e0->is<IR::MAU::ActionArg>() && sl->getL() == 0 &&
                (2 << sl->getH()) >= synth2port->size) {
                simpl_expr = sl->e0;
            }
        }
    }

    bool both_hash_and_index = false;
    auto index_check = std::make_pair(synth2port, tbl);
    if (auto hd = self.find_hash_dist(simpl_expr, call->prim)) {
        HashDistKey hdk = std::make_pair(synth2port, tbl);
        if (self.update_hd.count(hdk)) {
            auto hd_comp = self.update_hd.at(hdk);
            BuildP4HashFunction builder(self.phv);
            hd->apply(builder);
            P4HashFunction *a_func = builder.func();
            hd_comp->apply(builder);
            P4HashFunction *b_func = builder.func();
            if (!a_func->equiv(b_func)) {
                error("%s: Incompatible attached indexing between actions in table %s",
                      call->prim->srcInfo, tbl->name);
            }
        }
        self.update_hd[hdk] = hd;
        simpl_expr = hd;
        if (self.addressed_by_index.count(index_check)) both_hash_and_index = true;
        self.addressed_by_hash.insert(index_check);
    } else if (!simpl_expr->is<IR::Constant>() && !simpl_expr->is<IR::MAU::ActionArg>()) {
        error("%s: The index is too complex for the primitive to be handled.", call->prim->srcInfo);
    } else {
        if (self.addressed_by_hash.count(index_check)) both_hash_and_index = true;
        self.addressed_by_index.insert(index_check);
    }

    if (both_hash_and_index) {
        error(
            "%s: The attached %s is addressed by both hash and index in %s, "
            "which cannot be supported.",
            call->prim->srcInfo, synth2port, tbl);
    }

    StatefulCallKey sck = std::make_pair(call, tbl);
    self.update_calls[sck] = simpl_expr;
}

/** This pass was specifically created to deal with adding the HashDist object to different
 *  stateful objects.  On one particular case, execute_stateful_alu_from_hash was creating
 *  two separate instructions, a TempVar = hash function call, and an execute stateful call
 *  addressed by this TempVar.  This pass combines these instructions into one instruction,
 *  and correctly saves the HashDist IR into these attached tables
 */
void StatefulAttachmentSetup::Scan::postorder(const IR::MAU::Table *tbl) {
    for (auto act : Values(tbl->actions)) {
        for (auto call : act->stateful_calls) {
            auto prim = call->prim;
            // typechecking should have verified
            BUG_CHECK(prim->operands.size() >= 1, "Invalid primitive %s", prim);
            auto gref = prim->operands[0]->to<IR::GlobalRef>();
            // typechecking should catch this too
            BUG_CHECK(gref, "No object named %s", prim->operands[0]);
            auto synth2port = gref->obj->to<IR::MAU::Synth2Port>();

            bool already_attached = false;
            for (auto back_at : tbl->attached) {
                if (synth2port == back_at->attached) {
                    already_attached = true;
                    break;
                }
            }

            if (!already_attached) {
                BUG("%s not attached to %s", synth2port->name, tbl->name);
            }

            auto type = stateful_type_for_primitive(prim);
            if (!synth2port || synth2port->getType() != type) {
                // typechecking is unable to check this without a good bit more work
                error("%s: %s is not a %s", prim->operands[0]->srcInfo, gref->obj, type);
            }

            if (!synth2port) continue;

            auto index_op = index_operand(prim);
            if (index_op < 0) {
                continue;
            }

            if (static_cast<int>(prim->operands.size()) > index_op)
                setup_index_operand(prim->operands[index_op], synth2port, tbl, call);
            else
                error(
                    "%s: Indirect attached object %s requires an index to address, as it "
                    "isn't directly addressed from the match entry",
                    prim->srcInfo, synth2port->name);
        }
    }
}

/**
 * Save the index operand into the StatefulCall IR Node
 */
const IR::MAU::StatefulCall *StatefulAttachmentSetup::Update::preorder(
    IR::MAU::StatefulCall *call) {
    auto *tbl = findOrigCtxt<IR::MAU::Table>();
    auto *orig_call = getOriginal()->to<IR::MAU::StatefulCall>();

    StatefulCallKey sck = std::make_pair(orig_call, tbl);
    if (auto expr = ::get(self.update_calls, sck)) call->index = expr;

    auto prim = call->prim;
    BUG_CHECK(prim->operands.size() >= 1, "Invalid primitive %s", prim);
    auto gref = prim->operands[0]->to<IR::GlobalRef>();
    auto attached = gref->obj->to<IR::MAU::AttachedMemory>();
    auto act = findOrigCtxt<IR::MAU::Action>();
    use_t use = self.action_use[act][attached];
    if (!(use == IR::MAU::StatefulUse::NO_USE || use == IR::MAU::StatefulUse::DIRECT ||
          use == IR::MAU::StatefulUse::INDIRECT)) {
        BUG_CHECK(call->index == nullptr,
                  "%s: Primitive cannot both have index and use "
                  "counter index: %s",
                  prim->srcInfo, prim);
        call->index = new IR::MAU::StatefulCounter(prim->srcInfo, prim->type, attached);
    }

    prune();
    return call;
}

/**
 * Save the Hash Distribution unit and the type of the stateful ALU counter
 */
const IR::MAU::BackendAttached *StatefulAttachmentSetup::Update::preorder(
    IR::MAU::BackendAttached *ba) {
    auto *tbl = findOrigCtxt<IR::MAU::Table>();
    HashDistKey hdk = std::make_pair(ba->attached, tbl);
    if (auto hd = ::get(self.update_hd, hdk)) {
        ba->hash_dist = hd;
    }
    use_t use = IR::MAU::StatefulUse::NO_USE;
    for (auto act : Values(tbl->actions)) {
        auto use2 = self.action_use[act][ba->attached];
        if (use2 != IR::MAU::StatefulUse::NO_USE) {
            if (use != IR::MAU::StatefulUse::NO_USE && use != use2) {
                error("inconsistent use of %s in table %s", ba->attached, tbl);
                break;
            }
            use = use2;
        }
    }
    ba->use = use;
    return ba;
}

const IR::MAU::StatefulAlu *StatefulAttachmentSetup::Update::preorder(IR::MAU::StatefulAlu *salu) {
    auto *orig = getOriginal<IR::MAU::StatefulAlu>();
    if (self.salu_clears.count(orig)) {
        auto &info = self.salu_clears.at(orig);
        salu->clear_value = info.clear_value;
        // truncate the value to width bits, then replicate back to 128 bits
        salu->clear_value.clrrange(salu->width, ~0);
        for (size_t w = salu->width; w < 128; w = w * 2)
            salu->clear_value |= salu->clear_value << w;
        salu->busy_value = info.busy_value;
    }
    prune();
    return salu;
}

const IR::MAU::Instruction *StatefulAttachmentSetup::Update::preorder(IR::MAU::Instruction *inst) {
    if (self.remove_instr.count(getOriginal())) return nullptr;
    return inst;
}

Visitor::profile_t MeterSetup::Scan::init_apply(const IR::Node *root) {
    self.update_lpfs.clear();
    self.update_pre_colors.clear();
    self.pre_color_types.clear();
    self.standard_types.clear();

    return MauInspector::init_apply(root);
}

bool MeterSetup::Scan::preorder(const IR::MAU::Instruction *) { return false; }

void MeterSetup::Scan::find_input(const IR::MAU::Primitive *prim) {
    int input_index = input_operand(prim);
    if (input_index == -1) return;

    auto gref = prim->operands[0]->to<IR::GlobalRef>();
    if (!gref) return;
    auto mtr = gref->obj->to<IR::MAU::Meter>();
    BUG_CHECK(mtr, "%s: Operand in LPF execute is not a meter", prim->srcInfo);
    auto input = prim->operands[input_index];
    auto *field = self.phv.field(input);
    if (!field) {
        error("%1%: Not a phv field in the lpf execute: %2%", prim->srcInfo, input);
        return;
    }

    if (self.update_lpfs.count(mtr) == 0) {
        self.update_lpfs[mtr] = input;
        return;
    }

    auto *act = findContext<IR::MAU::Action>();
    if (!act) return;
    auto *tbl = findContext<IR::MAU::Table>();
    if (!tbl) return;
    auto *other_field = self.phv.field(self.update_lpfs.at(mtr));
    if (!other_field) return;

    ERROR_CHECK(field == other_field,
                "%s: The call of this lpf.execute in action %s has "
                "a different input %s than another lpf.execute on %s in the same table %s. "
                "The other input is %s.",
                prim->srcInfo, act->name, field->name, mtr->name, tbl->name, other_field->name);
}

/** This determines the field to be used for the pre-cplor.  It will also mark a meter as
 *  color-aware or color-blind.
 */
void MeterSetup::Scan::find_pre_color(const IR::MAU::Primitive *prim) {
    auto act = findContext<IR::MAU::Action>();
    if (act == nullptr) return;
    auto gref = prim->operands[0]->to<IR::GlobalRef>();
    if (!gref) return;
    auto mtr = gref->obj->to<IR::MAU::Meter>();
    int pre_color_index = precolor_operand(prim);
    BUG_CHECK(!(mtr == nullptr && pre_color_index != -1),
              "%s: Operation requiring pre-color is "
              "not a meter",
              prim->srcInfo);

    if (mtr == nullptr) return;

    // A pre-color only appears as an extra parameter on direct/indirect meters.  If the meter
    // either doesn't have a pre-color or is an lpf/wred, then the type is normal
    if (pre_color_index == -1 || static_cast<int>(prim->operands.size() - 1) < pre_color_index) {
        self.standard_types[act] = mtr->unique_id();
        return;
    }

    auto pre_color = prim->operands[pre_color_index];
    // pre_color = convert_cast_to_slice(pre_color);
    auto *field = self.phv.field(pre_color);
    if (!field) {
        // The pre-color must come from phv.
        error(ErrorType::ERR_UNEXPECTED,
              "The pre-color must come from phv. Please have a separate"
              " table that writes the precolor to meter metadata in an earlier stage. It makes "
              "sure that the precolor comes from PHV.");
        return;
    }

    BUG_CHECK(field, "%s: Not a phv field in the lpf execute: %s", prim->srcInfo, field->name);

    if (self.update_pre_colors.count(mtr) == 0) {
        self.update_pre_colors[mtr] = pre_color;
    }

    auto *other_field = self.phv.field(self.update_pre_colors.at(mtr));
    CHECK_NULL(other_field);
    ERROR_CHECK(field == other_field,
                "%s: The meter execute with a pre-color in action %s has "
                "a different pre-color %s than another meter execute on %s.  This other "
                "pre-color is %s.",
                prim->srcInfo, act->name, field->name, mtr->name, other_field->name);
    self.pre_color_types[act] = mtr->unique_id();
}

/** Linking the input for an LPF found in the action call with the IR node.  The Scan pass finds
 *  the PHV field, and the Update pass updates the AttachedMemory object
 */
bool MeterSetup::Scan::preorder(const IR::MAU::Primitive *prim) {
    find_input(prim);
    find_pre_color(prim);
    return false;
}

void MeterSetup::Update::update_input(IR::MAU::Meter *mtr) {
    auto orig_meter = getOriginal()->to<IR::MAU::Meter>();
    bool should_have_input =
        mtr->implementation.name == "lpf" || mtr->implementation.name == "wred";
    bool has_input = self.update_lpfs.count(orig_meter) > 0;
    if (has_input != should_have_input) {
        ERROR_CHECK(should_have_input && !has_input,
                    "%s: %s meter %s never provided an input "
                    "through an action",
                    mtr->srcInfo, mtr->implementation.name, mtr->name);
        ERROR_CHECK(has_input && !should_have_input,
                    "%s: meter %s does not require an input, "
                    "but is provided one through an action",
                    mtr->srcInfo, mtr->name);
    }
    if (!(has_input && should_have_input)) return;
    mtr->input = self.update_lpfs.at(orig_meter);
}

void MeterSetup::Update::update_pre_color(IR::MAU::Meter *mtr) {
    auto orig_meter = getOriginal()->to<IR::MAU::Meter>();
    bool has_pre_color = self.update_pre_colors.count(orig_meter) > 0;

    if (has_pre_color) {
        auto pre_color = self.update_pre_colors.at(orig_meter);
        auto hge = new IR::MAU::HashGenExpression(pre_color->srcInfo, IR::Type::Bits::get(2),
                                                  pre_color, IR::MAU::HashFunction::identity());
        auto hd = new IR::MAU::HashDist(hge->srcInfo, hge->type, hge);
        mtr->pre_color = hd;
    }
}

bool MeterSetup::Update::preorder(IR::MAU::Meter *mtr) {
    update_input(mtr);
    update_pre_color(mtr);
    return true;
}

bool MeterSetup::Update::preorder(IR::MAU::Action *act) {
    auto orig_act = getOriginal()->to<IR::MAU::Action>();
    if (self.standard_types.count(orig_act) > 0) {
        act->meter_types[self.standard_types.at(orig_act)] = IR::MAU::MeterType::COLOR_BLIND;
    } else if (self.pre_color_types.count(orig_act) > 0) {
        act->meter_types[self.pre_color_types.at(orig_act)] = IR::MAU::MeterType::COLOR_AWARE;
    }
    return true;
}

struct CheckInvalidate : public Inspector {
    explicit CheckInvalidate(const PhvInfo &phv) : phv(phv) {}

    void postorder(const IR::MAU::Primitive *prim) override {
        if (prim->name == "invalidate") {
            auto *f = phv.field(prim->operands[0]);
            if (!f) {
                std::string hdrInvalid = "";
                if (prim->operands[0]->is<IR::ConcreteHeaderRef>()) {
                    hdrInvalid += "\nTo invalidate a header, use .setInvalid()";
                }
                error("%s: invalid operand %s for primitive %s: not a field.%s", prim->srcInfo,
                      prim->operands[0], prim, hdrInvalid.c_str());
                return;
            }
            if (!f->is_invalidate_from_arch()) {
                error("%s: invalid operand %s for primitive %s", prim->srcInfo, f->name, prim);
            }
        }
    }

    const PhvInfo &phv;
};

/** The execution of Counters, Meters, and Stateful ALUs can be per action.  In the following
 *  example, a table has two actions a1 and a2, and an associated counter cnt.  But only
 *  one of the actions has an associated execution.
 *
 *  a1(index) { cnt.execute(index); }
 *  a2() { }
 *
 *  The addresses for counters/meter/stateful alus (as well as all other addresses) have
 *  a per_flow_enable bit.  If this pfe == true, then the associated counter/meter/stateful
 *  alu will run, and if pfe == false, then the table will not run.  This per flow enable
 *  bit is described in section 6.4.3.5.2 Per Entry Enable
 *
 *  The per flow enable bit can be defaulted to true in all addresses.  This can happen
 *  only if all actions run the associated object.  However, if the object is not executed
 *  in all hit actions, then the per flow enable bit must come from match overhead.
 *
 *  A similar concept to per flow enable bit is meter type.  The meter address, which goes
 *  to meters, stateful alus, and selectors, has a 3 bit type associated with the address.
 *  This type is described in section 6.4.4.11. Address Distribution Non-Address Bit
 *  Encodings.
 *
 *  For selectors, there is only one type.  For Meters, a meter can either use a pre-color
 *  value or not.  For Stateful ALUs, one of up to 4 stateful ALUs instructions can be used.
 *  Similar to per flow enable, the meter type can be defaulted, if the object executed
 *  has an identical meter type for all actions.  If the meter type is different per action,
 *  however, the meter type has to come from overhead.
 *
 *  Examine the following example: a table with two actions a1 and a2, and an associated
 *  meter.
 *
 *  a1 (index) { meter.execute(index, pre_color); }
 *  a2 (index) { meter.execute(index);  // no pre_color }
 *
 *  Now both actions execute a meter, so the per_flow_enable can be defaulted on.  However,
 *  because actions have a different meter_type, one with color awareness and one without
 *  color awareness, the meter type has to come from overhead.
 *
 *  When a table is associated with a gateway, a pfe must come from overhead, though that is
 *  not known until table placement, and is not associated with this analysis.  Currently
 *  this needs some work during TableFormat and TablePlacement, and does not yet fully
 *  work.
 */
bool SetupAttachedAddressing::InitializeAttachedInfo::preorder(const IR::MAU::BackendAttached *ba) {
    auto at_mem = ba->attached;
    if (!(at_mem->is<IR::MAU::Counter>() || at_mem->is<IR::MAU::Meter>() ||
          at_mem->is<IR::MAU::StatefulAlu>())) {
        return false;
    }

    AttachedActionCoord aac;
    aac.am = at_mem;
    auto tbl = findContext<IR::MAU::Table>();
    auto &attached_info = self.all_attached_info[tbl];
    attached_info[at_mem->unique_id()] = aac;
    return false;
}

struct StatefulConflict {
    cstring act_name;
    const IR::MAU::Action *act;
    const IR::MAU::StatefulCall *calls[2];

    StatefulConflict(cstring act_name, const IR::MAU::Action *act, const IR::MAU::StatefulCall *a,
                     const IR::MAU::StatefulCall *b)
        : act_name(act_name), act(act), calls{a, b} {}

    template <typename T>
    void fill_conflict_names(T &set) const {
        for (auto &call : calls) set.emplace(call->attached_callee->toString());
    }

    void emitError() const {
        // note: source info objects are not printed as part of the message, they only give the
        // context below the error message -- we use them for the last two arguments to avoid
        // polluting the error message and to give the right context
        error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
              "The action %1% indexes %2% with %3% but it also indexes %4% with %5%.%6%%7%", act,
              calls[0]->attached_callee->toString(), calls[0]->index->toString(),
              calls[1]->attached_callee->toString(), calls[1]->index->toString(),
              calls[0]->getSourceInfo(), calls[1]->getSourceInfo());
    }
};

bool SetupAttachedAddressing::ScanTables::preorder(const IR::MAU::Table *tbl) {
    std::vector<StatefulConflict> state_issues;
    std::list<std::string> stats_issues, meter_issues;
    auto create_issue_item = [](const cstring &action, const cstring &used,
                                const cstring &not_used) {
        std::stringstream str;
        str << "The action " << action << " uses " << used << " but does not use " << not_used
            << ".";
        return str.str();
    };

    for (auto &action : tbl->actions) {
        auto &act_name = action.first;
        auto *act = action.second;

        if (act->miss_only()) continue;

        const IR::MAU::AttachedMemory *stats_enabled = nullptr;
        const IR::MAU::AttachedMemory *stats_disabled = nullptr;
        const IR::MAU::AttachedMemory *meter_enabled = nullptr;
        const IR::MAU::AttachedMemory *meter_disabled = nullptr;

        auto &attached_info = self.all_attached_info[tbl];
        for (auto &kv : attached_info) {
            auto &uid = kv.first;
            auto &aac = kv.second;

            if (act->per_flow_enables.count(uid) == 0) {
                aac.all_per_flow_enabled = false;
                if (aac.am->usesStatsBus()) stats_disabled = aac.am;
                if (aac.am->usesMeterBus()) meter_disabled = aac.am;
            } else {
                if (aac.am->usesStatsBus()) stats_enabled = aac.am;
                if (aac.am->usesMeterBus()) meter_enabled = aac.am;
            }
            if (uid.has_meter_type()) {
                if (act->meter_types.count(uid) == 0) continue;
                if (!aac.meter_type_set) {
                    aac.meter_type = act->meter_types.at(uid);
                    aac.meter_type_set = true;
                } else if (aac.meter_type != act->meter_types.at(uid)) {
                    aac.all_same_meter_type = false;
                }
            }
        }
        if (stats_enabled && stats_disabled) {
            stats_issues.push_back(
                create_issue_item(act_name, stats_enabled->toString(), stats_disabled->toString()));
        }
        if (meter_enabled && meter_disabled) {
            meter_issues.push_back(
                create_issue_item(act_name, meter_enabled->toString(), meter_disabled->toString()));
        }
        const IR::MAU::StatefulCall *stats_call = nullptr, *meter_call = nullptr;
        for (auto *sc : act->stateful_calls) {
            const IR::MAU::StatefulCall **prev;
            if (sc->attached_callee->usesStatsBus())
                prev = &stats_call;
            else if (sc->attached_callee->usesMeterBus())
                prev = &meter_call;
            else
                continue;
            if (*prev && (sc->index ? (*prev)->index ? !sc->index->equiv(*(*prev)->index) : true
                                    : (*prev)->index != nullptr)) {
                state_issues.emplace_back(act_name, act, sc, *prev);
            }
            *prev = sc;
        }
    }

    const char common_msg[] =
        "%1%: There are issues with the following indirect externs:\n  %2%\n"
        "The Tofino architecture requires all indirect externs to be addressed "
        "with the same expression across all actions they are used in.\n"
        "You can also try to distribute individual indirect externs into separate tables.";
    if (stats_issues.size() > 0) {
        error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, common_msg, tbl,
              boost::algorithm::join(stats_issues, "\n  "));
    }
    if (meter_issues.size() > 0) {
        error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, common_msg, tbl,
              boost::algorithm::join(meter_issues, "\n  "));
    }
    if (state_issues.size() > 0) {
        std::set<std::string> error_exts;
        for (const auto &entry : state_issues) entry.fill_conflict_names(error_exts);

        error(ErrorType::ERR_UNSUPPORTED_ON_TARGET, common_msg, tbl,
              boost::algorithm::join(error_exts, ", ") + " (see following errors for details).");
        for (const auto &entry : state_issues) entry.emitError();
    }

    return true;
}

/** In the case of direct tables, or tables where there isn't any match overhead, but have
 *  an associated address through hash, then a per flow enable bit must always be defaulted
 *  on.  (The hash may not apply for JBay, but unsupported).
 *
 *  The reason for this is that unlike an indirect address, which can be different per
 *  entry, the address is pulled from the same place per direct entry, and the associated
 *  pfe must be identical for all entries.
 *
 *  The meter type must also be identical for these types of tables, as the meter type
 *  must be pulled from after the shift of the address, which is impossible in a direct
 *  entry as the address is at the MSB of the payload.  This pass verifies that this
 *  behavior is correct.
 */
bool SetupAttachedAddressing::VerifyAttached::preorder(const IR::MAU::BackendAttached *ba) {
    auto at_mem = ba->attached;
    if (!(at_mem->is<IR::MAU::Counter>() || at_mem->is<IR::MAU::Meter>() ||
          at_mem->is<IR::MAU::StatefulAlu>())) {
        return false;
    }

    auto tbl = findContext<IR::MAU::Table>();
    BUG_CHECK(tbl != nullptr, "No associated table found for attached table - %1%", ba);

    bool direct = at_mem->direct;
    bool from_hash = ba->hash_dist != nullptr;

    bool keyless = true;
    for (auto key : tbl->match_key) {
        if (key->for_match()) {
            keyless = false;
            break;
        }
    }

    bool singular_functionality = (direct || (from_hash && keyless));

    auto &aac = self.all_attached_info.at(tbl).at(at_mem->unique_id());
    if (singular_functionality) {
        std::string problem =
            direct ? "direct attached objects" : "objects attached to a hash action";

        std::string solution = direct ? "" : "either have match data or ";
        std::string meter_type_problem = at_mem->is<IR::MAU::Meter>()
                                             ? "color aware per flow meters"
                                             : "multiple stateful ALU actions";
        ERROR_CHECK(
            aac.all_per_flow_enabled,
            "%s: Attached object %s in table %s is executed "
            "in some actions and not executed in others.  However, %s must be enabled in all "
            "hit actions.  If you require this functionality, consider converting this to "
            "%sindirect addressing",
            ba->srcInfo, at_mem->name, tbl->name, problem, solution);
        ERROR_CHECK(aac.all_same_meter_type,
                    "%s: Attached object %s in table %s requires "
                    "multiple different types of meter addressing due to %s.  However %s must "
                    "have the same type in all hit actions.  If you require this functionality "
                    "consider converting this to %sindirect addressing",
                    ba->srcInfo, at_mem->name, tbl->name, meter_type_problem, problem, solution);
    }
    self.attached_coord[ba] = aac;
    return false;
}

void SetupAttachedAddressing::UpdateAttached::simple_attached(IR::MAU::BackendAttached *ba) {
    auto at_mem = ba->attached;
    if (at_mem->direct)
        ba->addr_location = IR::MAU::AddrLocation::DIRECT;
    else
        ba->addr_location = IR::MAU::AddrLocation::OVERHEAD;
    if (at_mem->is<IR::MAU::Selector>()) {
        ba->pfe_location = IR::MAU::PfeLocation::OVERHEAD;
        ba->type_location = IR::MAU::TypeLocation::DEFAULT;
    } else {
        ba->pfe_location = IR::MAU::PfeLocation::DEFAULT;
    }
}

bool SetupAttachedAddressing::UpdateAttached::preorder(IR::MAU::BackendAttached *ba) {
    auto at_mem = ba->attached;
    if (!(at_mem->is<IR::MAU::Counter>() || at_mem->is<IR::MAU::Meter>() ||
          at_mem->is<IR::MAU::StatefulAlu>())) {
        simple_attached(ba);
        return false;
    }

    auto orig_ba = getOriginal()->to<IR::MAU::BackendAttached>();
    auto &aac = self.attached_coord.at(orig_ba);

    IR::MAU::PfeLocation pfe_loc = IR::MAU::PfeLocation::DEFAULT;
    IR::MAU::TypeLocation type_loc = IR::MAU::TypeLocation::DEFAULT;

    if (!aac.all_per_flow_enabled) pfe_loc = IR::MAU::PfeLocation::OVERHEAD;
    if (!aac.all_same_meter_type) type_loc = IR::MAU::TypeLocation::OVERHEAD;

    if (at_mem->direct) {
        ba->addr_location = IR::MAU::AddrLocation::DIRECT;
    } else if (ba->hash_dist != nullptr) {
        ba->addr_location = IR::MAU::AddrLocation::HASH;
    } else if (!(ba->use == IR::MAU::StatefulUse::NO_USE ||
                 ba->use == IR::MAU::StatefulUse::DIRECT ||
                 ba->use == IR::MAU::StatefulUse::INDIRECT)) {
        ba->addr_location = IR::MAU::AddrLocation::STFUL_COUNTER;
    } else {
        ba->addr_location = IR::MAU::AddrLocation::OVERHEAD;
    }

    ba->pfe_location = pfe_loc;
    if (at_mem->is<IR::MAU::StatefulAlu>() || at_mem->is<IR::MAU::Meter>())
        ba->type_location = type_loc;
    return false;
}

/**
 * By the time this pass is run, all relevant information about the different stateful
 * calls has been moved to other parts of the IR.
 *
 * The primitive, however, will potentially keep information around that could potentially
 * be erroneously used by PHV, i.e. temporary variables that were converted to hash
 * distribution IR nodes.  In order to not have PHV allocation make space for these Temporary
 * variables, these primitives must be discarded.
 *
 * If other information is potentially relevant, it must be saved in other IR nodes
 * before this pass
 */
bool NullifyAllStatefulCallPrim::preorder(IR::MAU::StatefulCall *sc) {
    sc->prim = nullptr;
    return false;
}

/** The purpose of BackendCopyPropagation is to propagate reads written in previous sets.
 *  This will only work for set operations, i.e. the following action:
 *
 *      set b, a
 *      set c, b
 *      set d, c
 *
 *  should afterwards be:
 *
 *     set b, a
 *     set c, a
 *     set d, a
 *
 *  The point of this is to parallelize the action as much as possible in order to guarantee
 *  that the action can be performed within a single stage.
 *
 *  Currently the algorithm does not copy propagate forward any non-set operation.  The following
 *  would be considered to be too difficult to copy_propagate
 *
 *      xor c, b, a
 *      add e, c, d
 *
 *  as it is not a guarantee that any action can possibly be handled within a single stage.
 *  At some point, this could be loosened
 *
 *      xor c, b, a
 *      set d, c
 *
 *  could be:
 *
 *      xor c, b, a,
 *      xor d, b, a
 *
 *  Another example not currently wall reads are not previously written, and that the writes
 *  are simple, i.e.
 *
 *     set f1[7:0], f2
 *     set f3, f1[3:0]
 *
 *  would be considered too difficult to copy propagate correctly in the short term, as this
 *  itself would require the instructions to first be split into the minimal field slice in
 *  order to correctly copy propagate.  Perhaps this can be done at a later time
 *
 *  However, instruction with mutually exclusive bitranges would be completely acceptable.
 *  Note that no instructions are eliminated, as this should be the job of ElimUnused
 */
const IR::MAU::Action *BackendCopyPropagation::preorder(IR::MAU::Action *a) {
    visitOnce();
    copy_propagation_replacements.clear();
    return a;
}

const IR::Node *BackendCopyPropagation::preorder(IR::MAU::Instruction *instr) {
    instr->copy_propagated.clear();
    // In order to maintain the sequential property, the reads have to be visited before the writes
    split_set = nullptr;
    for (ssize_t i = instr->operands.size() - 1; i >= 0; i--) {
        elem_copy_propagated = false;
        visit(instr->operands[i], "operands", i);
        instr->copy_propagated[i] = elem_copy_propagated;
    }
    prune();
    if (split_set) {
        return split_set;
    }
    return instr;
}
/** Mark @p instr->operands[1] as the most recent replacement for instr->operands[0] when @p instr
 * is a set instruction.  Otherwise, remove @p instr->operands[0] from the set of copy propagation
 * candidates.
 */
void BackendCopyPropagation::update(const IR::MAU::Instruction *instr, const IR::Expression *e) {
    if (findContext<IR::MAU::SaluAction>()) {
        // don't propagate within or out of SALU actions
        return;
    }
    auto act = findContext<IR::MAU::Action>();
    if (act == nullptr) {
        return;
    }

    le_bitrange bits = {0, 0};
    auto field = phv.field(e, &bits);
    if (field == nullptr) {
        return;
    }

    bool replaced = false;
    // If a value is previously written, and previously had a copy propagation,
    // replace this value with a new value, if in a set, or clear the value
    auto it = copy_propagation_replacements[field].begin();
    while (it != copy_propagation_replacements[field].end()) {
        if (it->dest_bits == bits) {
            if (instr->name == "set") {
                it->read = instr->operands[1];
                it++;
            } else {
                it = copy_propagation_replacements[field].erase(it);
            }
            replaced = true;
            // overlapping ranges for copy propagation are too difficult to handle for the
            // first pass. Just try to handle overlapping set instr now.
        } else if ((!it->dest_bits.intersectWith(bits).empty()) && (instr->name != "set")) {
            error(
                "%s: Currently the field %s[%d:%d] in action %s is assigned in a "
                "way too complex for the compiler to currently handle.  Please consider "
                "simplifying this action around this parameter",
                instr->srcInfo, field->name, bits.hi, bits.lo, act->name);
            replaced = true;
            it++;
        } else {
            // Treat overlapping "set" instruction as non-overlapping in BackendCopyPropagation pass
            // and let the EliminateAllButLastWrite pass to handle it.
            it++;
        }
    }

    if (!replaced && instr->name == "set") {
        LOG5("BackendCopyProp: saving " << instr->operands[0] << " = " << instr->operands[1]);
        copy_propagation_replacements[field].emplace_back(bits, instr->operands[1]);
    }
}

const IR::Expression *BackendCopyPropagation::FieldImpact::getSlice(bool isSalu, le_bitrange bits) {
    const IR::Expression *rv;
    BUG_CHECK(dest_bits.contains(bits), "%s is not a superset of %s", dest_bits, bits);
    if (isSalu && read->is<IR::MAU::HashDist>()) {
        // FIXME -- SALU uses hash directly, not via hash dist; need refactoring
        // of IXBarExpression/HashGenExpression to make this more sane
        auto *hd = read->to<IR::MAU::HashDist>();
        rv = MakeSlice(hd->expr, bits.shiftedByBits(-dest_bits.lo));
        rv = new IR::MAU::IXBarExpression(rv);
    } else {
        rv = MakeSlice(read, bits.shiftedByBits(-dest_bits.lo));
    }
    return rv;
}

/** @returns the copy propagation candidate for @p e if @p e can be replaced
 * (setting \@elem_copy_propagated to true), or @p e if @p e cannot be replaced
 * (setting \@elem_copy_propagated to false).
 */
const IR::Expression *BackendCopyPropagation::propagate(const IR::MAU::Instruction *instr,
                                                        const IR::Expression *e) {
    auto act = findContext<IR::MAU::Action>();
    if (act == nullptr) {
        prune();
        return e;
    }

    le_bitrange bits = {0, 0};
    auto field = phv.field(e, &bits);
    if (field == nullptr) {
        return e;
    }
    bool isSalu = findContext<IR::MAU::SaluAction>() != nullptr;
    bitvec mask_bits(bits.lo, bits.size());

    prune();
    // If a read is possibly replaced with a copy propagated value, replace this value
    for (auto replacement : copy_propagation_replacements[field]) {
        if (isSalu && replacement.read->is<IR::MAU::ActionArg>()) {
            // can't access action args in the SALU
            continue;
        }
        if (replacement.dest_bits.contains(bits)) {
            elem_copy_propagated = true;
            auto rv = replacement.getSlice(isSalu, bits);
            LOG4("BackendCopyProp: " << e << " -> " << rv);
            return rv;
        } else if (replacement.dest_bits.intersectWith(bits).empty()) {
            continue;
        } else if (instr->name == "set") {
            if (!split_set) split_set = new IR::Vector<IR::MAU::Primitive>;
            le_bitrange range = replacement.dest_bits.intersectWith(bits);
            split_set->push_back(new IR::MAU::Instruction(
                instr->srcInfo, "set"_cs,
                MakeSlice(instr->operands.at(0), range.shiftedByBits(-bits.lo)),
                replacement.getSlice(isSalu, range)));
            mask_bits.clrrange(range.lo, range.size());
            LOG4("BackendCopyProp: created slice: " << split_set->back());
        } else {
            error(
                "%s: Currently the field %s[%d:%d] in action %s is read in a way "
                "too complex for the compiler to currently handle.  Please consider "
                "simplifying this action around this parameter",
                instr->srcInfo, field->name, bits.hi, bits.lo, act->name);
        }
    }
    if (split_set) {
        while (!mask_bits.empty()) {
            int lo = mask_bits.ffs();
            le_bitrange range(lo, mask_bits.ffz(lo) - 1);
            split_set->push_back(new IR::MAU::Instruction(
                instr->srcInfo, "set"_cs,
                MakeSlice(instr->operands.at(0), range.shiftedByBits(-bits.lo)),
                MakeSlice(instr->operands.at(1), range.shiftedByBits(-bits.lo))));
            mask_bits.clrrange(range.lo, range.size());
            LOG4("BackendCopyProp: created slice: " << split_set->back());
        }
    }
    return e;
}

const IR::Expression *BackendCopyPropagation::preorder(IR::Expression *expr) {
    auto instr = findContext<IR::MAU::Instruction>();
    if (!instr) {
        prune();
    } else if (isWrite()) {
        prune();
        update(instr, expr);
    } else {
        return propagate(instr, expr);
    }
    return expr;
}

const IR::MAU::Action *BackendCopyPropagation::postorder(IR::MAU::Action *action) {
    // Instructions in actions are now parallel
    action->parallel = true;
    return action;
}

namespace {

class VerifyInstructionParams : public Inspector {
 private:
    const PhvInfo *phv;
    VerifyParallelWritesAndReads::FieldWrites *writes;
    const IR::MAU::Instruction *instruction;
    bool is_write;
    int param_index;

 public:
    explicit VerifyInstructionParams(const PhvInfo *phv_,
                                     VerifyParallelWritesAndReads::FieldWrites *writes_,
                                     const IR::MAU::Instruction *instruction_, bool is_write_,
                                     int param_index_);

    /* -- avoid copying */
    VerifyInstructionParams &operator=(VerifyInstructionParams &&) = delete;

    /* -- visitor interface */
    bool preorder(const IR::MAU::HashDist *hd_) override;
    bool preorder(const IR::MAU::HashGenExpression *hge_) override;
    bool preorder(const IR::ListExpression *le_) override;
    bool preorder(const IR::StructExpression *se_) override;
    bool preorder(const IR::Expression *expr_) override;

 private:
    bool isParallel(const PHV::Field *field, const le_bitrange &bits);
};

VerifyInstructionParams::VerifyInstructionParams(const PhvInfo *phv_,
                                                 VerifyParallelWritesAndReads::FieldWrites *writes_,
                                                 const IR::MAU::Instruction *instruction_,
                                                 bool is_write_, int param_index_)
    : phv(phv_),
      writes(writes_),
      instruction(instruction_),
      is_write(is_write_),
      param_index(param_index_) {}

bool VerifyInstructionParams::preorder(const IR::MAU::HashDist *) { return true; }

bool VerifyInstructionParams::preorder(const IR::MAU::HashGenExpression *) { return true; }

bool VerifyInstructionParams::preorder(const IR::ListExpression *) { return true; }

bool VerifyInstructionParams::preorder(const IR::StructExpression *) { return true; }

bool VerifyInstructionParams::preorder(const IR::Expression *expr_) {
    le_bitrange bits_ = {0, 0};
    const auto *field_(phv->field(expr_, &bits_));
    if (field_ == nullptr) {
        /* -- if the expression is not a reference to a PHV field, enter inside */
        return true;
    }

    if (!isParallel(field_, bits_)) {
        // Overlapping set will be handled in the EliminateAllButLastWrite pass
        if (instruction->name != "set") {
            const auto *act_(findContext<IR::MAU::Action>());
            error(ErrorType::ERR_UNSUPPORTED,
                  "%1%: action spanning multiple stages. "
                  "Operations on operand %3% (%4%[%5%..%6%]) in action %2% require multiple "
                  "stages for a single action. We currently support only single stage actions. "
                  "Please rewrite the action to be a single stage action.",
                  instruction, act_, (param_index + 1), field_->name, bits_.lo, bits_.hi);
        }
    }
    return false;
}

bool VerifyInstructionParams::isParallel(const PHV::Field *field, const le_bitrange &bits) {
    if (is_write) {
        bool append = true;
        // Ensures that the writes of ranges of field bits are either completely identical
        // or over mutually exclusive regions of that field, as those are too difficult
        // to deal with
        for (auto write_bits : (*writes)[field]) {
            // Because EliminateAllButLastWrite has to come after this, due to the write
            // appearing in potentially reads
            if (bits == write_bits) {
                append = false;
            } else if (!bits.intersectWith(write_bits).empty()) {
                return false;
            }
        }
        if (append) (*writes)[field].push_back(bits);
    } else {
        for (auto write_bits : (*writes)[field]) {
            if (!bits.intersectWith(write_bits).empty()) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace

/** The purpose of this pass is to verify that an action is able to be performed in parallel
 *  Because the semantics of P4 are that instructions are sequential, and the actions in
 *  Tofino are parallel, this guarantees that an action is possible as a single action.
 *
 *  The following example would be marked as non-parallel
 *
 *     set f2, f1
 *     set f3, f2
 *
 *  as f2 is previously written and then read, which would require a two stage parameter.
 *
 *  Fields will be skipped if they have been copy propagated.  This is necessary if the P4
 *  programmer intended on swapping fields, i.e.
 *
 *      set $tmp, f2
 *      set f2, f1
 *      set f1, $tmp
 *
 *  BackendCopyPropagation will have the following output:
 *
 *      set $tmp, f2,
 *      set f2, f1,
 *      set f1, f2
 *
 *  The f2 read in the final instruction will be marked as having been copy propagated.  By
 *  using this marking, the algorithm skips over any things that have been copy propagated
 *  as the pre-change value is the value desired.
 *
 *  Note the following instruction will also be caught:
 *
 *      set f1[7:0], f2
 *      set f1[11:4], f3
 *
 *  as these instructions not parallelizable.
 *
 *  In the future, an action could possibly be split across multiple stages, given that
 *  the following is implemented:
 *
 *      1. Splitting a complex actions into multiple actions that are single stage.
 *      2. Chaining the next table with these action splitting
 *      3. Somehow update the context JSON, in the case that this requires either stateful
 *         outputs or action parameters.  This 3rd step is significantly more complicated
 *
 *   Let say a table t1 has actions a1-a3.  a2 and a3 are single-stage actions, while a1
 *   is the following:
 *        xor $tmp1, f1, f2
 *        add f4, f3, $tmp1
 *
 *   a1 would be split into:
 *
 *   action a1_0 { xor $tmp1, f1, f2 }
 *   action a1_1 { add f4, f3, $tmp1 }
 *
 *   the table apply then must be converted into
 *   if (t1.apply().action_run) {
 *       a1_0 { a1_1(); }
 *   }
 *
 *   The splitting of an action is relatively easy.  The chaining could have many corner
 *   cases, but would be only compiler.  However, if instead of f3, the compiler had
 *   an action data parameter, an API would be required for the splitting of this table
 *   to write to two logical tables, which definitely does not yet have driver support.
 *
 */

/** Determines if any values of this particular instruction have previously been written
 *  within this action, and if so, throw an error
 */
void VerifyParallelWritesAndReads::postorder(const IR::MAU::Instruction *instr) {
    for (ssize_t i = instr->operands.size() - 1; i >= 0; i--) {
        // Skip copy propagated values
        if (instr->copy_propagated[i]) continue;
        instr->operands[i]->apply(
            VerifyInstructionParams(&phv, &writes, instr, instr->isOutput(i), i),
            getChildContext());
    }
}

bool VerifyParallelWritesAndReads::preorder(const IR::MAU::Action *) {
    writes.clear();
    return true;
}

/** The purpose of this pass is to parallelize the passes by removing all writes to the last
 *  from a field, i.e.
 *
 *      set f1, f2
 *      xor f1, f3, f4
 *      set f1, f5
 *
 *  will be translated into:
 *
 *      set f1, f5
 *
 *  because in a parallel sense, this is the only result that matters.
 *
 *  This pass has to follow VerifyParallelWritesAndReads, due to the following example:
 *
 *      add f1, f2, f3
 *      add f1, f1, f4
 *
 *  This could not be considered parallel, as long as the check happens before the elimination
 */
bool EliminateAllButLastWrite::Scan::preorder(const IR::MAU::Action *) {
    last_instr_map.clear();
    self.has_overlap_set.clear();
    self.fs_bits.clear();
    return true;
}

bool EliminateAllButLastWrite::Scan::preorder(const IR::MAU::Instruction *instr) {
    if (instr->operands.size() == 0) return false;

    auto write = instr->operands[0];
    le_bitrange bits = {0, 0};
    auto field = self.phv.field(write, &bits);
    if (field == nullptr) {
        auto *act = findContext<IR::MAU::Action>();
        BUG_CHECK(act != nullptr, "No associated action found for instruction - %1%", instr);
        error("%s: A write of an instruction in action %s is not a PHV field", instr->srcInfo,
              act->name);
        return false;
    }
    PHV::FieldSlice fs(field, bits);
    // Ensures that the last instruction relating to this instruction is marked
    last_instr_map[fs] = instr;

    // Handle overlapping set
    if (instr->name == "set") {
        for (auto ele_bits : self.fs_bits[field]) {
            if (ele_bits == bits) return false;  // stop if fs_bits has this phv bitrange.
            if (!ele_bits.intersectWith(bits).empty()) self.has_overlap_set[field] = true;
        }
        self.fs_bits[field].push_back(bits);  // add new phv bitrange in fs_bits
    }

    return false;
}

void EliminateAllButLastWrite::Scan::postorder(const IR::MAU::Action *a) {
    self.last_instr_per_action_map[a] = last_instr_map;
}

const IR::MAU::Action *EliminateAllButLastWrite::Update::preorder(IR::MAU::Action *act) {
    current_af = getOriginal()->to<IR::MAU::Action>();
    return act;
}

const IR::Node *EliminateAllButLastWrite::Update::preorder(IR::MAU::Instruction *instr) {
    auto orig_instr = getOriginal()->to<IR::MAU::Instruction>();
    auto last_instr_map = self.last_instr_per_action_map[current_af];
    prune();

    if (instr->operands.size() == 0) return instr;

    auto write = instr->operands[0];
    le_bitrange bits = {0, 0};
    auto field = self.phv.field(write, &bits);
    if (field == nullptr) {
        error("%s: A write of an instruction in action %s is not a PHV field", instr->srcInfo,
              current_af->name);
        return instr;
    }
    PHV::FieldSlice fs(field, bits);
    BUG_CHECK(last_instr_map.count(fs) != 0,
              "A write was not found in the inspect pass, but "
              "was found in the update pass");
    // Remove if not the last instruction
    if (last_instr_map.at(fs) != orig_instr) {
        return nullptr;
    }

    // handle overlapping set instr if has_overlp_set is true in current action
    if ((instr->name == "set") && (self.has_overlap_set[field] == true)) {
        IR::Vector<IR::MAU::Primitive> *split_overlap_set = nullptr;
        bitvec mask_bits(bits.lo, bits.size());
        mask_bits.setrange(bits.lo, bits.size());

        auto it = self.fs_bits[field].begin();
        // clear the bits of all the follwoing last set instructions
        while (it != self.fs_bits[field].end()) {
            // remove the current last set instr as it won't overwrite its following set instrs.
            if (*it == bits) {
                it = self.fs_bits[field].erase(it);
                continue;
            }
            mask_bits.clrrange((*it).lo, (*it).size());
            it++;
            // If all of the phv bits of the current set instr be overwritten by the following
            // set instrs. remove it.
            if (mask_bits.empty()) return nullptr;
        }
        // create new set instructions for current overlapped set instruction.
        while (!mask_bits.empty()) {
            int lo = mask_bits.ffs();
            le_bitrange range(lo, mask_bits.ffz(lo) - 1);

            if (!split_overlap_set) split_overlap_set = new IR::Vector<IR::MAU::Primitive>;
            split_overlap_set->push_back(new IR::MAU::Instruction(
                instr->srcInfo, "set"_cs,
                MakeSlice(instr->operands.at(0), range.shiftedByBits(-bits.lo)),
                MakeSlice(instr->operands.at(1), range.shiftedByBits(-bits.lo))));
            mask_bits.clrrange(range.lo, range.size());
            LOG4("EliminateAllButLastWrite: created slice for overlaping set: "
                 << split_overlap_set->back());
        }
        return split_overlap_set;
    }

    return instr;
}

/** This pass transforms arithmetic compare operations. Aritmetic compares
 *  write to an entire PHV, not just single bits.
 *
 *  See the description in the header file.
 *
 *  The Scan pass identifies fields used in compare operations that should be
 *  transformed.
 *
 *  Clear local state when entering an MAU action.
 */
bool ArithCompareAdjustment::Scan::preorder(const IR::MAU::Action *) {
    comp_targets.clear();
    comp_or_set_zero_writes.clear();
    other_targets.clear();
    return true;
}

/** Identify comparison instructions or set 0 instructions that might be paired
 *  with comparison instructions.
 */
bool ArithCompareAdjustment::Scan::preorder(const IR::MAU::Instruction *instr) {
    if (instr->operands.size() == 0) return false;

    int max_width = 0;
    const PHV::Field *write_field = nullptr;
    le_bitrange write_bits = {0, 0};
    for (auto op : instr->operands) {
        le_bitrange bits = {0, 0};
        auto field = self.phv.field(op, &bits);

        if (!write_field && !field) {
            auto act = findContext<IR::MAU::Action>();
            BUG_CHECK(act != nullptr, "No associated action found for instruction - %1%", instr);
            error("%sA write of an instruction in action %s is not a PHV field", instr->srcInfo,
                  act->name);
            return false;
        }

        if (field && !write_field) {
            write_field = field;
            write_bits = bits;
        }

        // This is safe for set/compare ops. The value may be wrong for other
        // instructions, but in that case it is not used.
        max_width = std::max(max_width, op->type->width_bits());
    }

    // Record details of the instruction
    if (self.is_compare(instr->name)) {
        comp_targets[write_field] = max_width;

        bitvec &zero_bits = comp_or_set_zero_writes[write_field];
        zero_bits.setrange(write_bits.lo, write_bits.size());
    } else if (instr->name == "set") {
        auto src = instr->operands[1]->to<IR::Constant>();
        if (src && src->fitsInt() && src->asInt() == 0) {
            bitvec &zero_bits = comp_or_set_zero_writes[write_field];
            zero_bits.setrange(write_bits.lo, write_bits.size());
        }
    } else {
        other_targets.emplace(write_field);
    }

    return false;
}

/** Identify which fields are elegible for transformation after processing all
 *  instructions in an action. Elegible fields:
 *    - Are the target of a comparison operation.
 *    - All bits of the field are written to by either the comparison (LSB) or
 *      by set 0 operations.
 */
void ArithCompareAdjustment::Scan::postorder(const IR::MAU::Action *a) {
    std::set<const PHV::Field *> action_comp_adj_fields;
    for (auto target : comp_targets) {
        auto field = target.first;
        auto width = target.second;

        if (other_targets.count(field)) continue;
        if (field->size > 1) {
            bitvec &set_bits = comp_or_set_zero_writes[field];
            if (set_bits != bitvec(0, field->size)) continue;
        }

        LOG4("Marking field for comparison transformation: " << field->name);
        action_comp_adj_fields.emplace(field);
        self.comp_adj_field_width_map.emplace(field, width);
        self.comp_adj_name_width_map.emplace(field->name, width);
    }

    self.comp_adj_fields_per_action_map[a] = action_comp_adj_fields;
}

const IR::MAU::Action *ArithCompareAdjustment::Update::preorder(IR::MAU::Action *act) {
    current_af = getOriginal()->to<IR::MAU::Action>();
    comp_adj_fields = self.comp_adj_fields_per_action_map[current_af];
    return act;
}

const IR::MAU::Action *ArithCompareAdjustment::Update::postorder(IR::MAU::Action *act) {
    current_af = nullptr;
    comp_adj_fields.clear();
    return act;
}

/** Modify instructions as follows:
 *    1. Comparisons: adjust the destination field width to match the source widths.
 *    2. Sets: delete any set 0 instructions to a comparison target field in
 *       the same action as the comparison. The comparison sets all bits except
 *       the LSB to 0.
 */
const IR::MAU::Instruction *ArithCompareAdjustment::Update::preorder(IR::MAU::Instruction *instr) {
    if (instr->operands.size() == 0) return instr;

    auto write = instr->operands[0];
    le_bitrange bits = {0, 0};
    auto field = self.phv.field(write, &bits);
    if (field == nullptr) return instr;

    if (!comp_adj_fields.count(field)) return instr;

    LOG3("Modifying/deleting instruction: " << instr);

    if (instr->name == "set") return nullptr;

    int width = self.comp_adj_field_width_map[field];
    if (write->type->width_bits() != width) {
        auto slice_op = write->to<IR::Slice>();
        if (slice_op) {
            write = instr->operands[0] = slice_op->e0;
        }
        if (write->type->width_bits() != width) {
            auto dst = write->clone();
            dst->type = IR::Type_Bits::get(width);
            instr->operands[0] = dst;
        }
    }
    return instr;
}

/** Update metadata field widths
 */
const IR::Metadata *ArithCompareAdjustment::Update::preorder(IR::Metadata *h) {
    prune();

    auto type_new = adjust_type_struct(h->type, h->name.name);
    if (type_new != h->type) h->type = type_new;
    return h;
}

/** Update field widths
 */
const IR::ConcreteHeaderRef *ArithCompareAdjustment::Update::preorder(IR::ConcreteHeaderRef *chr) {
    // Don't prune -- allow to propagate into ref

    if (!chr->type->is<IR::Type_Struct>()) return chr;

    auto type = chr->type->to<IR::Type_Struct>();
    auto type_new = adjust_type_struct(type, chr->ref->name.name);
    if (type != type_new) chr->type = type_new;
    return chr;
}

/** Update field widths if the member is a compare target field
 */
const IR::Node *ArithCompareAdjustment::Update::preorder(IR::Member *m) {
    auto name = m->toString();
    if (!self.comp_adj_name_width_map.count(name)) return m;

    int orig_width = m->type->width_bits();
    int target_width = self.comp_adj_name_width_map[name];
    if (orig_width != target_width) {
        if (getContext()->node->is<IR::MAU::Instruction>())
            LOG3("Changing field width of " << name << " in " << getContext()->node);
        else
            LOG3("Changing field width of " << name << " in " << getContext()->node->toString());

        m->type = IR::Type_Bits::get(target_width);
        return new IR::Slice(m, orig_width - 1, 0);
    }

    return m;
}

/** Adjust field widths in a Type_StructLike object
 *
 *  Returns a new object if the struct is modified, otherwise returns the
 *  original object.
 */
const IR::Type_StructLike *ArithCompareAdjustment::Update::adjust_type_struct(
    const IR::Type_StructLike *ts, cstring prefix) {
    bool changed = false;
    auto rv = ts->clone();
    rv->fields.clear();
    for (auto f : ts->fields) {
        cstring fname = prefix + '.' + f->name.name;
        auto it = self.comp_adj_name_width_map.find(fname);
        if (it != self.comp_adj_name_width_map.end() && f->type->width_bits() != it->second) {
            int width = it->second;
            auto fc = f->clone();
            fc->type = IR::Type_Bits::get(width);
            rv->fields.push_back(fc);
            changed = true;
        } else {
            rv->fields.push_back(f);
        }
    }
    return changed ? rv : ts;
}

/**
 * modify_field_with_hash_based_offset is not yet converted correctly in the converters,
 * as the slicing operation portion, i.e. the max size of the instruction is not correct.
 *
 * This leads to mismatched sizes in HashDist Instructions, which used to be done by
 * ConvertCastToSlice, though that pass no longer exists.  In order for ActionAnalysis
 * to function on hash dist parameters, all operands in hash based instructions must
 * be the same size.  This pass guarantees this.
 */
bool GuaranteeHashDistSize::Scan::preorder(const IR::MAU::Instruction *) {
    contains_hash_dist = false;
    return true;
}

bool GuaranteeHashDistSize::Scan::preorder(const IR::MAU::HashDist *) {
    contains_hash_dist = true;
    return false;
}

void GuaranteeHashDistSize::Scan::postorder(const IR::MAU::Instruction *instr) {
    if (!contains_hash_dist) return;

    bool size_set = false;
    bool all_same_size = true;
    int size = 0;
    for (auto op : instr->operands) {
        if (!size_set) {
            size = op->type->width_bits();
            size_set = true;
        } else {
            int check_size = op->type->width_bits();
            if (check_size != size) all_same_size = false;
        }
    }

    if (all_same_size) return;

    if (instr->name != "set")
        error(
            "%1%: Currently cannot handle a non-assignment hash function in an action "
            "where the hash size is not the same size as the write operand",
            instr);
    else
        self.hash_dist_instrs.insert(instr);
}

const IR::Node *GuaranteeHashDistSize::Update::postorder(IR::MAU::Instruction *instr) {
    auto orig_instr = getOriginal()->to<IR::MAU::Instruction>();
    if (self.hash_dist_instrs.count(orig_instr) == 0) return instr;

    BUG_CHECK(instr->name == "set", "Incorrectly adjusting a non-set hash instruction");

    auto *rv_vec = new IR::Vector<IR::Node>();
    auto write = instr->operands[0];
    auto read = instr->operands[1];

    int write_size = write->type->width_bits();
    int read_size = read->type->width_bits();

    if (write_size > read_size) {
        rv_vec->push_back(new IR::MAU::Instruction(instr->srcInfo, instr->name,
                                                   MakeSlice(write, 0, read_size - 1), read));
        auto zero = new IR::Constant(IR::Type::Bits::get(write_size - read_size), 0);
        rv_vec->push_back(new IR::MAU::Instruction(
            instr->srcInfo, instr->name, MakeSlice(write, read_size, write_size - 1), zero));
    } else if (write->type->width_bits() < read->type->width_bits()) {
        rv_vec->push_back(new IR::MAU::Instruction(instr->srcInfo, instr->name, write,
                                                   MakeSlice(read, 0, write_size)));
    } else {
        BUG("Incorrectly adjusting a non-set hash instruction");
    }
    return rv_vec;
}

/**
 * Remove a slice of an ActionArg in an instruction, if the argument was the same size
 * as the slice.  The naming in the assembly output assumes that a slice will only happen
 * when the portion of the argument is not the full width of the argument.
 *
 * This was to deal with issue583.p4
 */
const IR::Node *RemoveUnnecessaryActionArgSlice::preorder(IR::Slice *sl) {
    if (!findContext<IR::MAU::Instruction>()) return sl;
    auto aa = sl->e0->to<IR::MAU::ActionArg>();
    if (aa == nullptr) return sl;
    if (sl->type->width_bits() == aa->type->width_bits() && sl->getL() == 0) return aa;
    return sl;
}

// Simplify "actionArg != 0 ? f0 : f1" to "actionArg ? f0 : f1"
const IR::Node *SimplifyConditionalActionArg::postorder(IR::Mux *mux) {
    Pattern::Match<IR::MAU::ActionArg> aa;
    if ((0 != aa).match(mux->e0)) mux->e0 = aa;
    return mux;
}

/**
 * Some instructions may be impossible to execute in one stage. This pass detects several
 * such cases and splits the problematic instructions into two tables.
 */
class ExpandInstructions : public MauTransform {
    const PhvInfo &phv_i;
    const ReductionOrInfo &red_info;
    std::vector<IR::MAU::Instruction *> m_precompute;
    std::set<const IR::Expression *> m_preload;

    /**
     * For every instruction, Tofino can read only up to one non-PHV operand (action data,
     * constants, hash dists etc.) This function detects these invalid instructions and splits
     * them into two steps (tables): in first, newly created table, one of the non-PHV operands is
     * pre-loaded into a temporary PHV container, which is then read by the original instruction.
     *
     * For example:
     *
     * tbl:
     *      add(ingress::hdr.mul32_64.res,
     *          hash_dist(Ingress.copy_32_0.configure($field_list_1 : {ingress::hdr.mul32_64.b})),
     *          hash_dist(Ingress.copy_32_1.configure($field_list_1 : {ingress::hdr.mul32_64.a})));
     *
     * is rewritten into:
     *
     * tbl_preload:
     *      set($tmp3,
     *          hash_dist(Ingress.copy_32_0.configure($field_list_1 : {ingress::hdr.mul32_64.b})));
     * tbl:
     *      add(ingress::hdr.mul32_64.res,
     *          $tmp3,
     *          hash_dist(Ingress.copy_32_1.configure($field_list_1 : {ingress::hdr.mul32_64.a})));
     *
     */
    void check_action_data_bus_params(const IR::MAU::Action *act) {
        auto tbl = findContext<IR::MAU::Table>();
        BUG_CHECK(tbl, "Action doesn't have any table context!");
        ActionAnalysis::FieldActionsMap field_actions_map;
        ActionAnalysis aa(phv_i, false, false, tbl, red_info, false, false);
        aa.set_field_actions_map(&field_actions_map);
        act->apply(aa);
        // Capture action inputs which are not PHV related. Only one non-PHV action input is allowed
        for (auto fa_element : field_actions_map) {
            auto field_instruction = fa_element.first;
            auto &field_action = fa_element.second;

            if (!(field_action.error_code & ActionAnalysis::FieldAction::MULTIPLE_ACTION_DATA)) {
                continue;
            }

            std::vector<ActionAnalysis::ActionParam> expression_candidates;
            for (const auto &read : field_action.reads) {
                if (read.type != ActionAnalysis::ActionParam::CONSTANT &&
                    read.speciality != ActionAnalysis::ActionParam::HASH_DIST) {
                    // Constants and hash_dists can be pre-loaded into a PHV container
                    // in separate table. Action data or stateful objects on the other hand
                    // cannot due their link to this specific table.
                    continue;
                }

                expression_candidates.push_back(read);
            }

            if (expression_candidates.empty()) {
                LOG1("Cannot split invalid instruction "
                     << *field_instruction
                     << " into two tables, because all source operands are action data "
                     << "or stateful objects.");
                continue;
            }
            BUG_CHECK(expression_candidates.size() <= 2,
                      "More than two non-PHV element splitting is not being supported.");

            // Let's take the first read expression from the action
            auto expr_rew = expression_candidates[0];
            LOG2("Number of required non-PHV arguments is bigger than one for action "
                 << act->name.name << ", instruction: " << field_instruction << std::endl
                 << " * preload operand " << expr_rew << " into PHV container in separate table.");
            m_preload.insert(expr_rew.expr);
        }
    }

    IR::Node *preorder(IR::MAU::Action *act) {
        check_action_data_bus_params(act);
        return act;
    }

    IR::Node *postorder(IR::Expression *expr) {
        auto orig_expr = getOriginal<IR::Expression>();
        BUG_CHECK(orig_expr, "Couldn't find original IR node");

        auto it = m_preload.find(orig_expr);
        if (it == m_preload.end()) return expr;
        m_preload.erase(it);

        auto temp_var = new IR::TempVar(orig_expr->type);
        auto preload_inst = new IR::MAU::Instruction("set"_cs, {temp_var, orig_expr});
        m_precompute.push_back(preload_inst);

        return temp_var;
    }

    IR::Node *postorder(IR::MAU::Table *tbl) override {
        if (m_precompute.empty()) {
            return tbl;
        }

        auto precompute_action = new IR::MAU::Action("$precompute");
        precompute_action->action.append(m_precompute);
        m_precompute.clear();
        precompute_action->default_allowed = true;
        precompute_action->init_default = true;

        auto t = new IR::MAU::Table(tbl->name + "$precompute", VisitingThread(this));
        t->is_compiler_generated = true;
        t->match_table = new IR::P4Table(t->name.c_str(), new IR::TableProperties());
        t->actions[precompute_action->name] = precompute_action;

        return new IR::Vector<IR::MAU::Table>({t, tbl});
    }

 public:
    ExpandInstructions(const PhvInfo &p, const ReductionOrInfo &ri) : phv_i(p), red_info(ri) {}
};

/** EliminateAllButLastWrite has to follow VerifyParallelWritesAndReads.  Look at the example
 *  above EliminateAllButLastWrite
 */
InstructionSelection::InstructionSelection(const BFN_Options &options, PhvInfo &phv,
                                           const ReductionOrInfo &ri)
    : PassManager{
          new CheckInvalidate(phv),  // Instructions in actions are sequential.
          new UnimplementedRegisterMethodCalls, new ConvertFunnelShiftExtern,
          new ToFunnelShiftInstruction, new HashGenSetup(phv, options), new Synth2PortSetup(phv),
          new SimplifyConditionalActionArg(), new DoInstructionSelection(phv),
          new StatefulAttachmentSetup(phv), new MeterSetup(phv),
          // new DLeftSetup,
          new SetupAttachedAddressing, new NullifyAllStatefulCallPrim, new GuaranteeHashDistSize,
          new CollectPhvInfo(phv), new StaticEntriesConstProp(phv), new BackendCopyPropagation(phv),
          new VerifyParallelWritesAndReads(phv),  // Instructions in actions are now parallel
          new EliminateAllButLastWrite(phv), new ArithCompareAdjustment(phv),
          new RemoveUnnecessaryActionArgSlice, new ExpandInstructions(phv, ri),
          new CollectPhvInfo(phv), new ValidateActions(phv, ri, false, false, false)} {}
