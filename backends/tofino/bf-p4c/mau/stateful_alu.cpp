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

#include "stateful_alu.h"

#include <cmath>

#include "bf-p4c/common/asm_output.h"  // for generic formatting routines
#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/common/slice.h"
#include "bf-p4c/ir/ir_enums.h"
#include "bf-p4c/lib/safe_width.h"
#include "ir/pattern.h"
#include "ixbar_expr.h"
#include "lib/hex.h"

const Device::StatefulAluSpec &TofinoDevice::getStatefulAluSpec() const {
    static const Device::StatefulAluSpec spec = {/* .CmpMask = */ false,
                                                 /* .CmpUnits = */ {"lo"_cs, "hi"_cs},
                                                 /* .MaxSize = */ 32,
                                                 /* .MaxDualSize = */ 64,
                                                 /* .MaxPhvInputWidth = */ 32,
                                                 /* .MaxInstructions = */ 4,
                                                 /* .MaxInstructionConstWidth = */ 4,
                                                 /* .MinInstructionConstValue = */ -8,
                                                 /* .MaxInstructionConstValue = */ 7,
                                                 /* .OutputWords = */ 1,
                                                 /* .DivModUnit = */ false,
                                                 /* .FastClear = */ false,
                                                 /* .MaxRegfileRows = */ 4};
    return spec;
}

#if HAVE_JBAY
const Device::StatefulAluSpec &JBayDevice::getStatefulAluSpec() const {
    static const Device::StatefulAluSpec spec = {
        /* .CmpMask = */ true,
        /* .CmpUnits = */ {"cmp0"_cs, "cmp1"_cs, "cmp2"_cs, "cmp3"_cs},
        /* .MaxSize = */ 128,
        /* .MaxDualSize = */ 128,
        /* .MaxPhvInputWidth = */ 64,
        /* .MaxInstructions = */ 4,
        /* .MaxInstructionConstWidth = */ 4,
        /* .MinInstructionConstValue = */ -8,
        /* .MaxInstructionConstValue = */ 7,
        /* .OutputWords = */ 4,
        /* .DivModUnit = */ true,
        /* .FastClear = */ true,
        /* .MaxRegfileRows = */ 4};
    return spec;
}
#endif

/**
 * @brief This class detects a following pattern:
 *
 * if (expr1 && expr2) {
 *  dst = rvval1 | rvval2;
 * } else if (expr1) {
 *  dst = rvval1;
 * } else if (expr2) {
 *  dst = rvval2;
 * }
 *
 * This can be optimized into two instructions which are executed at the same
 * time and results are OR-ed together. This pattern is typical for the translation from
 * P4 14 to P4 16
 */
class SaluOredIf : public Inspector {
    // Top-level if statement
    const IR::Expression *top_if = nullptr;
    const IR::AssignmentStatement *top_st = nullptr;

    // Second level-if condition
    const IR::Expression *second_if = nullptr;
    const IR::AssignmentStatement *second_st = nullptr;

    // Third level-if condition
    const IR::Expression *third_if = nullptr;
    const IR::AssignmentStatement *third_st = nullptr;

    // Flag for the signalization of SALU pred
    bool is_saluOredIf = false;

    /**
     * @brief Analyze if the pattern seems fine regarding
     * captured data.
     */
    void analyze_pattern() {
        // 1] Check if cond1/2 is as same as in the case of AND-ed version
        auto land_node = top_if->to<IR::LAnd>();
        if (!land_node) return;
        if (!land_node->left->equiv(*second_if) || !land_node->right->equiv(*third_if)) return;
        // 2] Check that true bodies has the assignment statements only into the same variable
        if (!top_st || !second_st || !third_st) return;

        // 3] Check that the top-level body merges both statements using the OR operator and that
        // expr1 | expr2 parts are as same as in following if statements
        if (!top_st->left->equiv(*second_st->left) || !second_st->left->equiv(*third_st->left))
            return;

        // TODO: Maybe we can also support the permutation of both subtrees
        auto or_st = top_st->right->to<IR::BOr>();
        if (!or_st) return;
        if (!or_st->left->equiv(*second_st->right) || !or_st->right->equiv(*third_st->right))
            return;

        // All conditions are met
        is_saluOredIf = true;
    }

 public:
    bool preorder(const IR::IfStatement *if_node) override {
        // Catch the correct condition
        if (!top_if) {
            /* To match:
             * if (expr1 && expr2) {
             *  dst = rvval1 | rvval2;
             * } else if
             */
            LOG3("Trying to analyze: " << if_node);
            top_if = if_node->condition;
            top_st = if_node->ifTrue->to<IR::AssignmentStatement>();
            LOG4("top_if -> " << top_if << Log::endl << "top_st -> " << top_st);
            if (!if_node->ifFalse) {
                return false;
            }
            visit(if_node->ifFalse);
        } else if (!second_if) {
            /* To match:
             * } else if (expr1) {
             *  dst = rvval1;
             * } else if
             */
            second_if = if_node->condition;
            second_st = if_node->ifTrue->to<IR::AssignmentStatement>();
            LOG4("second_if -> " << second_if << Log::endl << "second_st -> " << second_st);
            if (!if_node->ifFalse) {
                return false;
            }
            visit(if_node->ifFalse);
        } else if (!third_if) {
            /* To match:
             * if (expr2) {
             *  dst = rvval2;
             * }
             *
             * Run the analysis because tree pattern seems fine
             */
            third_if = if_node->condition;
            third_st = if_node->ifTrue->to<IR::AssignmentStatement>();
            LOG4("third_if -> " << third_if << Log::endl << "third_st -> " << third_st);
            if (if_node->ifFalse) {
                // Not allowed shape, end the traversing
                return false;
            }
            analyze_pattern();
        } else {
            // We don't need to go deeper because it is not a required
            // shape.
            return false;
        }

        return false;
    }

    bool is_matched() const { return is_saluOredIf; }
};

cstring Device::StatefulAluSpec::cmpUnit(unsigned idx) const {
    if (idx < CmpUnits.size()) return CmpUnits.at(idx);
    return cstring("?" + std::to_string(idx) + "?");
}

static unsigned cmp_mask[4] = {0xaaaa, 0xcccc, 0xf0f0, 0xff00};

static unsigned eval_predicate(const IR::Expression *exp) {
    if (auto *land = exp->to<IR::LAnd>())
        return eval_predicate(land->left) & eval_predicate(land->right);
    if (auto *lor = exp->to<IR::LOr>())
        return eval_predicate(lor->left) | eval_predicate(lor->right);
    if (auto *lnot = exp->to<IR::LNot>()) return ~eval_predicate(lnot->expr);
    if (auto *reg = exp->to<IR::MAU::SaluCmpReg>()) return cmp_mask[reg->index];
    BUG("Invalid predicate expression %1%", exp);
    return 0;
}

// FIXME -- should use autogenerated Node::equiv, but that runs into problems
// with type InfInt on constants.  We ignore types here completely.
static bool equiv(const IR::Expression *a, const IR::Expression *b) {
    if (*a == *b) return true;
    if (typeid(*a) != typeid(*b)) return false;
    if (auto ma = a->to<IR::Member>()) {
        auto mb = b->to<IR::Member>();
        return ma->member == mb->member && equiv(ma->expr, mb->expr);
    }
    if (auto ua = a->to<IR::Operation::Unary>()) {
        auto ub = b->to<IR::Operation::Unary>();
        return equiv(ua->expr, ub->expr);
    }
    if (auto ea = a->to<IR::Operation::Binary>()) {
        auto eb = b->to<IR::Operation::Binary>();
        return equiv(ea->left, eb->left) && equiv(ea->right, eb->right);
    }
    if (auto ea = a->to<IR::Operation_Ternary>()) {
        auto eb = b->to<IR::Operation_Ternary>();
        return equiv(ea->e0, eb->e0) && equiv(ea->e1, eb->e1) && equiv(ea->e2, eb->e2);
    }
    if (auto pa = a->to<IR::PathExpression>()) {
        auto pb = b->to<IR::PathExpression>();
        return pa->path->name == pb->path->name;
    }
    if (auto ka = a->to<IR::Constant>()) {
        auto kb = b->to<IR::Constant>();
        return ka->value == kb->value;
    }
    if (auto ka = a->to<IR::BoolLiteral>()) {
        auto kb = b->to<IR::BoolLiteral>();
        return ka->value == kb->value;
    }
    if (auto ea = a->to<IR::MAU::IXBarExpression>()) {
        auto eb = b->to<IR::MAU::IXBarExpression>();
        return equiv(ea->expr, eb->expr);
    }
    if (auto sa = a->to<IR::MAU::SaluReg>()) {
        auto sb = b->to<IR::MAU::SaluReg>();
        return sa->equiv(*sb);
    }
    if (auto sa = a->to<IR::MAU::SaluRegfileRow>()) {
        auto sb = b->to<IR::MAU::SaluRegfileRow>();
        return sa->equiv(*sb);
    }
    return false;
}

static bool equiv(const IR::Vector<IR::Expression> *a, const IR::Vector<IR::Expression> *b) {
    if (a->size() != b->size()) return false;
    auto itb = b->begin();
    for (auto ela : *a) {
        if (!equiv(ela, *itb)) return false;
        ++itb;
    }
    return true;
}

bool IR::MAU::StatefulAlu::alu_output() const {
    for (auto act : Values(instruction))
        for (auto inst : act->action)
            if (inst->name == "output") return true;
    return false;
}

std::ostream &operator<<(std::ostream &out, CreateSaluInstruction::LocalVar::use_t u) {
    static const char *use_names[] = {"NONE", "ALUHI", "MEMLO", "MEMHI", "MEMALL", "REGFILE"};
    if (u < sizeof(use_names) / sizeof(use_names[0]))
        return out << use_names[u];
    else
        return out << "<invalid " << u << ">";
}
std::ostream &operator<<(std::ostream &out, CreateSaluInstruction::etype_t e) {
    static const char *names[] = {"NONE",  "MINMAX_IDX",   "IF",     "MINMAX_SRC",
                                  "VALUE", "OUTPUT_ALUHI", "OUTPUT", "MATCH"};
    if (size_t(e) < sizeof(names) / sizeof(names[0]))
        return out << names[e];
    else
        return out << "<invalid " << int(e) << ">";
}

static bool is_address_output(const IR::Expression *e) {
    if (auto *r = e->to<IR::MAU::SaluReg>()) return r->name == "address";
    return false;
}

static bool is_learn(const IR::Expression *e) {
    if (auto *r = e->to<IR::MAU::SaluReg>()) return r->name == "learn";
    return false;
}

/// Check a name to see if it is a reference to an argument of RegisterAction::apply,
/// or a reference to the local var we put in alu_hi.
/// or a reference to a copy of an in argument
/// If so, process it as an operand and return true
bool CreateSaluInstruction::applyArg(const IR::PathExpression *pe, cstring field) {
    Log::TempIndent indent;
    LOG4("applyArg(" << pe << ", " << field << ") etype = " << etype << indent);
    assert(dest == nullptr || !islvalue(etype));
    const IR::Expression *e = nullptr;
    auto argType = pe->type;
    int idx = 0, field_idx = 0;
    LocalVar *local = nullptr;
    if (locals.count(pe->path->name.name)) {
        local = &locals.at(pe->path->name.name);
        if (islvalue(etype)) dest = local;
        switch (local->use) {
            case LocalVar::NONE:
                // if this becomes a real instruction (not an elided copy), this
                // local will become alu_hi, so fall through
            case LocalVar::ALUHI:
            case LocalVar::MEMHI:
                field_idx = 1;
                break;
            case LocalVar::MEMLO:
            case LocalVar::MEMALL:
                break;
            case LocalVar::REGFILE:
                break;
            default:
                BUG("invalide use in CreateSaluInstruction::LocalVar %s %d", local->name,
                    local->use);
        }
    } else {
        if (!params) return false;
        if (islvalue(etype)) output_index = 0;
        for (auto p : params->parameters) {
            if (p->name == pe->path->name) {
                break;
            }
            if (islvalue(etype) && param_types->at(idx) == param_t::OUTPUT) ++output_index;
            ++idx;
        }
        if (size_t(idx) >= params->parameters.size()) return false;
        BUG_CHECK(size_t(idx) < param_types->size(), "param index out of range");
    }
    if (field && regtype->is<IR::Type_StructLike>() && argType->equiv(*regtype)) {
        BUG_CHECK(field_idx == 0 || (local && local->use == LocalVar::NONE),
                  "invalid reuse of local %s.%s", pe, field);
        field_idx = 0;
        for (auto f : regtype->to<IR::Type_StructLike>()->fields) {
            if (f->name == field) {
                argType = f->type;
                break;
            }
            ++field_idx;
        }
        if (field_idx > 1) {
            /* three or more fields in the register type will flag an error later */
            field_idx = 0;
        }
    }
    cstring name = field_idx ? "hi"_cs : "lo"_cs;
    switch (param_types->at(idx)) {
        case param_t::VALUE: /* inout value or local var */
            if (islvalue(etype)) {
                captureAssigstateProps();
                if (!dest) alu_write[field_idx] = true;
                etype = VALUE;
            }
            if (!opcode) opcode = "alu_a"_cs;
            if (etype == OUTPUT) {
                const char *pfx = "mem_"_cs;
                if (local) {
                    if (local->use == LocalVar::ALUHI) {
                        pfx = "alu_"_cs;
                    } else if (local->use == LocalVar::NONE) {
                        e = new IR::Constant(0);
                        break;
                    }
                } else if (alu_write[field_idx]) {
                    pfx = "alu_"_cs;
                }
                name = pfx + name;
            } else if (etype == MINMAX_SRC)
                name = "mem"_cs;
            if (local && local->use == LocalVar::REGFILE) {
                e = local->regfile;
                negate_regfile = negate;
            } else {
                e = new IR::MAU::SaluReg(pe->srcInfo, argType, name, field_idx > 0);
            }
            break;
        case param_t::OUTPUT: /* out rv; */
            if (islvalue(etype)) {
                captureAssigstateProps();
                etype = OUTPUT;
            } else {
                error(ErrorType::ERR_UNSUPPORTED, "Reading out param %s in %s not supported", pe,
                      action_type_name);
            }
            if (output_index > Device::statefulAluSpec().OutputWords)
                error(ErrorType::ERR_UNSUPPORTED, "Only %d stateful output%s supported",
                      Device::statefulAluSpec().OutputWords,
                      Device::statefulAluSpec().OutputWords > 1 ? "s" : "");
            if (!opcode) opcode = "output"_cs;
            break;
        case param_t::HASH: /* in digest */
            if (islvalue(etype)) {
                error(ErrorType::ERR_UNSUPPORTED, "Writing in param %s in %s not supported", pe,
                      action_type_name);
                return false;
            }
            e = new IR::MAU::SaluReg(pe->srcInfo, argType, "phv_" + name, field_idx > 0);
            break;
        case param_t::LEARN: /* in learn */
            if (islvalue(etype)) {
                error(ErrorType::ERR_UNSUPPORTED, "Writing in param %s in %s not supported", pe,
                      action_type_name);
                return false;
            }
            e = new IR::MAU::SaluReg(pe->srcInfo, argType, "learn"_cs, false);
            break;
        case param_t::MATCH: /* out match */
            if (!islvalue(etype)) {
                error(ErrorType::ERR_UNSUPPORTED, "Reading out param %s in %s not supported", pe,
                      action_type_name);
                return false;
            }
            etype = MATCH;
            if (!opcode) opcode = "#match"_cs;
            break;
        default:
            return false;
    }

    if (e) {
        if (negate) e = new IR::Neg(e);
        LOG4("applyArg operand: " << e);
        operands.push_back(e);
    }
    return true;
}

bool CreateSaluInstruction::canBeIXBarExpr(const IR::Expression *e) {
    return CanBeIXBarExpr(
        e, [this](const IR::PathExpression *pe) -> bool { return !applyArg(pe, cstring()); });
}

bool CreateSaluInstruction::outputAluHi() {
    if (salu->dual) return false;
    // Can't output via ALU_HI without writing back the result if the register contains a single
    // value that uses both lo and hi stateful memory, which is possible on JB / CB / FTR.
    if (salu->width > Device::statefulAluSpec().MaxDualSize / 2) return false;
    if (locals.empty())
        locals.emplace("--output--"_cs, LocalVar("--output--"_cs, false, LocalVar::ALUHI));
    return locals.begin()->first == "--output--";
}

// clear all the state we do NOT want to carry between functions
void CreateSaluInstruction::clearFuncState() {
    param_types = nullptr;
    action = nullptr;
    params = nullptr;
    dest = nullptr;
    locals.clear();
    etype = NONE;
    negate = false;
    alu_write[0] = alu_write[1] = false;
    opcode = cstring();
    operands.clear();
    pred_operands.clear();
    output_index = -1;
    cmp_instr.clear();
    divmod_instr = nullptr;
    minmax_instr = nullptr;
    predicate = nullptr;
    onebit = nullptr;
    onebit_cmpl = false;
    outputs.clear();
    output_address_subword_predicate.clear();
    math = IR::MAU::StatefulAlu::MathUnit();
    math_function = nullptr;
    return_encoding = nullptr;
    assig_st = nullptr;
    assig_pred = nullptr;
    written_dest.clear();
    output_param_operands.clear();
    output_predicates.clear();
    or_targets.clear();
}

bool CreateSaluInstruction::preorder(const IR::Function *func) {
    static std::vector<param_t> empty_params;
    BUG_CHECK(!action && !params && !return_encoding, "Nested function?");
    IR::ID name = reg_action->name;
    if (func->name != "apply") {
        name.name += "$" + func->name;
        if (func->name == "overflow") {
            if (salu->overflow)
                error(ErrorType::ERR_UNEXPECTED, "%s: Conflicting overflow function for Register",
                      func->srcInfo);
            else
                salu->overflow = name.name;
        }
        if (func->name == "underflow") {
            if (salu->underflow)
                error(ErrorType::ERR_UNEXPECTED, "%s: Conflicting underflow function for Register",
                      func->srcInfo);
            else
                salu->underflow = name.name;
        }
    }
    const char *tail = action_type_name.c_str() + action_type_name.size();
    while (std::isdigit(tail[-1])) --tail;
    param_types =
        &function_param_types.at(std::make_pair(action_type_name.before(tail), func->name));
    LOG3("Creating action " << name << "[" << func->id << "] for stateful table " << salu->name
                            << Log::indent);
    LOG5(func);
    action = new IR::MAU::SaluAction(func->srcInfo, name, func);
    action->annotations = reg_action->annotations;
    for (auto pt : *param_types) {
        // maybe this flag should be 'needs_dleft_digest' rather than 'learn_action'?
        if (pt == param_t::HASH) {
            action->learn_action = true;
            break;
        }
    }
    salu->instruction.addUnique(name.name, action);
    params = func->type->parameters;
    int out_word = 0;
    for (auto i = 1U; i < params->parameters.size(); ++i) {
        if (auto rt = params->parameters.at(i)->type->to<IR::Type_Enum>()) {
            if (return_encoding)
                error(ErrorType::ERR_UNSUPPORTED,
                      "%1%: Multiple enum return values are not supported", func);
            action->return_encoding = return_encoding =
                new IR::MAU::SaluAction::ReturnEnumEncoding(rt);
            return_enum_word = out_word;
        }
        if (params->parameters.at(i)->direction == IR::Direction::Out) out_word++;
    }
    return true;
}

void CreateSaluInstruction::postorder(const IR::Function *func) {
    BUG_CHECK(params == func->type->parameters, "%1%: recursion failure", func);
    if (cmp_instr.size() > Device::statefulAluSpec().CmpUnits.size())
        error(ErrorType::ERR_OVERLIMIT,
              "%s: %s %s.%s needs %d comparisons but the device only has %d comparison units. To "
              "make the action compile, reduce the number of comparisons.",
              func->srcInfo, action_type_name, reg_action->name, func->name, cmp_instr.size(),
              Device::statefulAluSpec().CmpUnits.size());
    if (onebit) {
        insert_instruction(onebit);
        LOG3("  add " << *action->action.back());
    }
    for (auto &kv : output_address_subword_predicate) {
        // JBAY-2631: we now put the predicate controlling the subword bit into salu_mathtable,
        // but we hide that in the assembler, just specifying the predicate here as 'lmatch'
        auto &output = outputs[kv.first];
        BUG_CHECK(is_address_output(output->operands.back()), "not address output?");
        output->operands.push_back(new IR::MAU::SaluFunction(kv.second, "lmatch"_cs));
    }
    splitWideInstructions();
    assignOutputAlus();
    for (std::size_t i = 0; i < outputs.size(); ++i) {
        auto *instr = outputs[i];
        if (instr) {
            if (Device::statefulAluSpec().OutputWords > 1) {
                const auto it = action->output_param_to_alu.find(static_cast<int>(i));
                BUG_CHECK(it != action->output_param_to_alu.end(),
                          "No output ALU assigned to parameter %1%", i);
                instr->operands.insert(
                    instr->operands.begin() + instr->output_operand,
                    new IR::MAU::SaluReg(instr->operands.front()->type,
                                         "word" + std::to_string(it->second), false));
            }
            insert_instruction(instr);
            LOG3("  add " << *action->action.back());
        }
    }
    if (math.valid) {
        if (salu->math.valid && !(math == salu->math))
            error(ErrorType::ERR_UNSUPPORTED, "%s: math unit incompatible with %s",
                  reg_action->srcInfo, salu);
        else
            salu->math = math;
    }
    if (math_function && !math_function->expr)
        error(ErrorType::ERR_NOT_FOUND, "%s: math unit requires an input expression",
              reg_action->srcInfo);
    int alu_hi_use = salu->dual || salu->width > 64 ? 1 : 0;
    for (auto &local : Values(locals))
        if (local.use == LocalVar::ALUHI) alu_hi_use++;
    if (alu_hi_use > 1)
        error(ErrorType::ERR_OVERLIMIT, "%s: too many local variables in RegisterAction",
              func->srcInfo);
    if (return_encoding) {
        BUG_CHECK(::errorCount() > 0 || action->return_predicate_words & (1 << return_enum_word),
                  "%s salu return type mismatch", func->name);
        unsigned idx = 0;
        return_encoding->cmp_used = 0xffff;
        for (auto mask : cmp_mask) {
            if (idx >= cmp_instr.size() || cmp_instr[idx] == nullptr)
                return_encoding->cmp_used &= ~mask;
        }
        LOG4("  return_encoding->cmp_used = 0x" << hex(return_encoding->cmp_used));
    }
    if (action->action.empty()) {
        // Action body is empty, insert the nop instruction directly - we don't need
        // to merge any instructions.
        action->action.push_back(new IR::MAU::SaluInstruction("nop"_cs));
        warning(ErrorType::WARN_INVALID,
                "%1%: Expected stateful action '%2%' to have instructions "
                "assigned. Please verify the action is valid.",
                salu, action->name);
    }
    if (func->name == "apply") checkActions(func->srcInfo);
    LOG3_UNINDENT;
    clearFuncState();
}

/// @brief Check whether ALUs are being reused
///
/// Check whether an ALU (hi/lo) is being used mutliple times when it shouldn't be. An ALU half can
/// be used multiple times if a) the condidtions differ, or b) it's part of a bitwise-OR
/// computation.
void CreateSaluInstruction::checkActions(const Util::SourceInfo &srcInfo) {
    std::map<std::pair<cstring, const IR::Expression *>, const IR::MAU::SaluInstruction *>
        actions_by_dest_pred;
    for (const auto *act : action->action) {
        auto *dest = act->getOutput();
        if (!dest) continue;

        auto *dest_reg = dest->to<IR::MAU::SaluReg>();
        if (!dest_reg || dest_reg->is<IR::MAU::SaluCmpReg>() ||
            !(dest_reg->name == "lo" || dest_reg->name == "hi"))
            continue;

        auto dest_name = dest_reg->name;
        auto *pred = act->output_operand > 0 ? act->operands[0] : nullptr;
        std::set<const IR::Expression *> preds = {pred, nullptr};
        for (const auto *pred : preds) {
            auto dest_and_pred = std::make_pair(dest_name, pred);
            if (actions_by_dest_pred.count(dest_and_pred)) {
                const auto *dest = act->to<IR::MAU::SaluInstruction>()->getOutput();
                if (!or_targets.count(dest))
                    error(ErrorType::ERR_UNSUPPORTED,
                          "%sIn Stateful ALU, only two values can be computed per apply. "
                          "Value(s) written back to the register, including unchanged values "
                          "(even if not explicitly included in the P4 code), count as computed "
                          "values.",
                          srcInfo);
            }
        }
        auto dest_and_pred = std::make_pair(dest_name, pred);
        if (!actions_by_dest_pred.count(dest_and_pred)) {
            actions_by_dest_pred[dest_and_pred] = act;
        }
    }
}

void CreateSaluInstruction::doAssignment(const Util::SourceInfo &srcInfo) {
    auto *old_predicate = predicate;
    if (etype == IF && operands.empty() && pred_operands.size() == 1) {
        // output of conditional expression -- output constant 1 with the predicate
        predicate = pred_operands[0];
        if (old_predicate) predicate = new IR::LAnd(old_predicate, predicate);
        etype = OUTPUT;
        operands.push_back(new IR::Constant(1));
    }
    BUG_CHECK(operands.size() > (etype < OUTPUT_ALUHI), "%1%: recursion failure", srcInfo);
    if (safe_width_bits(operands.front()->type) > Device::statefulAluSpec().MaxSize)
        error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
              "%1%Wide operations not supported in "
              "Stateful ALU, will only operate on bottom %2% bits",
              srcInfo, Device::statefulAluSpec().MaxSize);
    if (dest && dest->use != LocalVar::ALUHI) {
        BUG_CHECK(etype == VALUE, "assert failure");
        LocalVar::use_t use = LocalVar::NONE;
        if (auto src = operands.back()->to<IR::MAU::SaluReg>()) {
            if (dest->pair) {
                use = LocalVar::MEMALL;
            } else if (src->hi) {
                use = LocalVar::MEMHI;
            } else {
                use = LocalVar::MEMLO;
            }
        } else if (operands.back()->is<IR::MAU::SaluRegfileRow>()) {
            use = LocalVar::REGFILE;
        } else {
            use = LocalVar::ALUHI;
        }
        if (use == LocalVar::NONE || (dest->use != LocalVar::NONE && dest->use != use))
            error("%s: Expression is too complex for Stateful ALU", srcInfo);
        dest->use = use;
        LOG3("local " << dest->name << " use " << dest->use);
    }
    if (!dest) {
        if (etype == OUTPUT_ALUHI) {
            etype = VALUE;
            auto *val = operands.at(0);
            operands.insert(operands.begin(),
                            new IR::MAU::SaluReg(val->srcInfo, val->type, "hi"_cs, true));
            createInstruction();
            operands.clear();
            operands.push_back(new IR::MAU::SaluReg(val->srcInfo, val->type, "alu_hi"_cs, true));
            etype = OUTPUT;
        }
        createInstruction();
    } else if (dest->use == LocalVar::ALUHI) {
        if (opcode == "alu_a" && Pattern(0).match(operands.back())) {
            // don't need to initialize the temp alu_hi to 0, it is that by default
            LOG3("  elide alu_a " << emit_vector(operands));
        } else {
            createInstruction();
        }
    }
    predicate = old_predicate;
    assignDone = true;
}

void CreateSaluInstruction::captureAssigstateProps() {
    // Capture the destination variable together with predicate assigned
    // to given written lvalue

    const Context *assignCtxt = nullptr;
    auto assig = findContext<IR::AssignmentStatement>(assignCtxt);
    // Check if the expression is not in an assignment or not the first (left) child
    // of the assignment
    if (!assig || assignCtxt->child_index != 0) return;

    // Backup statement & predicate for the checkWriteAfterWrite method
    assig_st = assig;
    assig_pred = predicate;
    LOG4("Captured assignment statement: " << assig_st);
    LOG4("Captured predicate statement: " << assig_pred);
}

void CreateSaluInstruction::checkWriteAfterWrite() {
    // Destination variable with assigned predicate should be available
    // here. The code here needs to check all predicates for given assignment
    // statement. The report will be reported if and only if both expressions
    // can be fired in the same time --> can lead to a data corruption
    // If there are more operations with same variable, there are aggregated with OR.

    if (!assig_st) return;

    // Check if we already have some data for the destination
    auto lvalue_name = assig_st->left->toString();
    LOG4("Searching for the lvalue: " << lvalue_name);
    auto elem = written_dest.find(lvalue_name);
    if (elem == written_dest.end()) {
        bool zero_assigned =
            assig_st->right->is<IR::Constant>() && assig_st->right->to<IR::Constant>()->value == 0;
        bool false_assigned = assig_st->right->is<IR::BoolLiteral>() &&
                              assig_st->right->to<IR::BoolLiteral>()->value == false;
        bool uncond_zero_or_false = !assig_pred && (zero_assigned || false_assigned);
        // Ignore initial zero/false assignments like rv = 0; 0 | X is still X.
        if (!uncond_zero_or_false) {
            LOG4("lvalue doesn't find, storing for the next analysis: name="
                 << lvalue_name << ", pred=" << assig_pred);
            written_dest[lvalue_name].emplace_back(assig_pred, assig_st->srcInfo);
        }
        assig_st = nullptr;
        assig_pred = nullptr;
        return;
    }

    // We need to check if the current variable has an intersection
    // with any other predicate inside the vector. We need to insert
    // the record if it isn't already here
    bool found = false;
    for (auto assigProp : elem->second) {
        LOG4("Probing predicates: " << assigProp.predicate << " and " << assig_pred);
        if (assigProp.predicate == nullptr || assig_pred == nullptr ||  // Always executed
            assigProp.predicate->equiv(*assig_pred)) {                  // Predicates are same
            found = true;

            std::stringstream err;
            err << "Two or more assignments of " << lvalue_name << " inside the register "
                << "action " << action->name.originalName << " are not mutually exclusive "
                << "and thus cannot be implemented in Tofino Stateful ALU." << std::endl
                << "\tThese potentially sequential assignments have been detected here:"
                << std::endl
                // Print the info about the first block of code
                << assigProp.srcInfo.toPositionString() << std::endl
                << assigProp.srcInfo.toSourceFragment()
                // Print the info about the second block of code
                << assig_st->srcInfo.toPositionString() << std::endl
                << assig_st->srcInfo.toSourceFragment()
                << "Please, rewrite your code to make sure the value is assigned no more than"
                << " once in all cases.";
            error(ErrorType::ERR_UNSUPPORTED, "%s", err.str());
        }
    }

    if (!found) {
        LOG4("lvalue doesn't find, storing for the next analysis: name=" << lvalue_name << ", pred="
                                                                         << assig_pred);
        written_dest[lvalue_name].emplace_back(assig_pred, assig_st->srcInfo);
    }

    assig_st = nullptr;
    assig_pred = nullptr;
}

bool CreateSaluInstruction::preorder(const IR::AssignmentStatement *as) {
    BUG_CHECK(operands.empty(), "%1%: recursion failure", as);
    etype = NONE;
    dest = nullptr;
    opcode = cstring();
    visit(as->left, "left");
    BUG_CHECK(operands.size() == (etype < OUTPUT) || errorCount() > 0, "%1%: recursion failure",
              as->left);
    assignDone = false;
    if (islvalue(etype))
        error(ErrorType::ERR_UNSUPPORTED, "Can't assign to %s in RegisterAction", as->left);
    else
        visit(as->right);
    if (errorCount() == 0 && !assignDone) doAssignment(as->srcInfo);
    operands.clear();
    return false;
}

static const IR::Expression *negatePred(const IR::Expression *e) {
    if (auto a = e->to<IR::LAnd>())
        return new IR::LOr(e->srcInfo, negatePred(a->left), negatePred(a->right));
    if (auto a = e->to<IR::LOr>())
        return new IR::LAnd(e->srcInfo, negatePred(a->left), negatePred(a->right));
    if (auto a = e->to<IR::LNot>()) return a->expr;
    return new IR::LNot(e);
}

bool CreateSaluInstruction::preorder(const IR::IfStatement *s) {
    // This pass is used for the detection of special IF statement pattern
    // which can be optimized to the construction with less ASM instructions
    SaluOredIf saluOrIf;
    s->apply(saluOrIf);
    if (saluOrIf.is_matched()) {
        // In this case we want to skip the predicate analysis and go into the false
        // node directly because the SALU unit will be generated as stand-alone if blocks.
        split_ifs = true;
        visit(s->ifFalse);
        split_ifs = false;
        return false;
    }

    // Standard IF statement analysis
    BUG_CHECK(operands.empty() && pred_operands.empty(), "%1%: recursion failure", s);
    etype = IF;
    dest = nullptr;
    visit(s->condition, "condition");
    BUG_CHECK(operands.empty() && pred_operands.size() == 1, "%1%: recursion failure",
              s->condition);
    etype = NONE;
    auto old_predicate = predicate;
    auto new_predicate = pred_operands.at(0);
    pred_operands.clear();
    predicate = old_predicate ? new IR::LAnd(new_predicate, old_predicate) : new_predicate;
    visit(s->ifTrue, "ifTrue");
    // Don't modify the predicate if we are running the if splitting mode because that mode
    // means that we want to run IF statements in parallel. This is related to SaluOrdIf
    // optimization
    if (!split_ifs) {
        new_predicate = negatePred(new_predicate);
        predicate = old_predicate ? new IR::LAnd(new_predicate, old_predicate) : new_predicate;
        visit(s->ifFalse, "ifFalse");
    } else {
        // Restore the old predicate and run the analysis
        predicate = old_predicate;
        visit(s->ifFalse, "ifFalse");
    }

    predicate = old_predicate;

    return false;
}

bool CreateSaluInstruction::preorder(const IR::Mux *mux) {
    struct {
        etype_t etype;
        LocalVar *dest;
        IR::Vector<IR::Expression> operands;
    } save_state = {etype, dest, operands};
    etype = IF;
    dest = nullptr;
    operands.clear();
    visit(mux->e0, "e0");
    BUG_CHECK(operands.empty() && pred_operands.size() == 1, "%1%: recursion failure", mux->e0);
    auto old_predicate = predicate;
    auto new_predicate = pred_operands.at(0);
    pred_operands.clear();
    predicate = old_predicate ? new IR::LAnd(new_predicate, old_predicate) : new_predicate;
    etype = save_state.etype;
    dest = save_state.dest;
    operands = save_state.operands;
    visit(mux->e1, "e1");
    doAssignment(mux->srcInfo);
    new_predicate = negatePred(new_predicate);
    predicate = old_predicate ? new IR::LAnd(new_predicate, old_predicate) : new_predicate;
    etype = save_state.etype;
    dest = save_state.dest;
    operands = save_state.operands;
    visit(mux->e2, "e2");
    doAssignment(mux->srcInfo);
    predicate = old_predicate;
    return false;
}

void CreateSaluInstruction::doPrimary(const IR::Expression *e, const IR::PathExpression *pe,
                                      cstring field) {
    if (!pe || !applyArg(pe, field)) {
        if (negate) e = new IR::Neg(e);
        operands.push_back(e);
        LOG4("primary operand: " << operands.back());
    }
    auto *reg = operands.empty() ? nullptr : operands.back()->to<IR::MAU::SaluReg>();
    if (etype == IF && (!reg || reg->name != "learn") && e->type->is<IR::Type::Boolean>() &&
        !getParent<IR::Operation::Relation>()) {
        operands.push_back(new IR::Constant(0));
        setupCmp("neq"_cs);
        LOG4("  -- adding != 0 comparison");
    }
}

bool CreateSaluInstruction::preorder(const IR::PathExpression *pe) {
    doPrimary(pe, pe, cstring());
    return false;
}

bool CreateSaluInstruction::preorder(const IR::Member *mem) {
    doPrimary(mem, mem->expr->to<IR::PathExpression>(), mem->member);
    return false;
}

bool CreateSaluInstruction::preorder(const IR::StructExpression *se) {
    IR::Vector<IR::Expression> copy = operands;
    for (auto el : se->components) {
        operands = copy;
        visit(el->expression, "expression");
        doAssignment(el->srcInfo);
    }
    return false;
}

bool CreateSaluInstruction::preorder(const IR::Constant *c) {
    if (etype == IF && c->value == 0) return false;
    if (negate) c = (-*c).clone();
    LOG4("Constant operand: " << c);
    operands.push_back(c);
    return false;
}

bool CreateSaluInstruction::preorder(const IR::BoolLiteral *bl) {
    if (etype == IF && bl->value == 0) return false;
    auto *c = new IR::Constant(bl->srcInfo, bl->value ? negate ? -1 : 1 : 0);
    LOG4("Constant operand: " << c);
    operands.push_back(c);
    return false;
}

/* check that a slice is essentially just a cast to the correct size */
static bool checkSlice(const IR::Slice *sl, unsigned width) {
    return sl->getL() == 0 && sl->getH() == width - 1;
}

bool CreateSaluInstruction::preorder(const IR::Slice *sl) {
    bool keep_slice = false;
    bool save_negate = negate;
    if (etype == NONE) {
        if (!checkSlice(sl, salu->alu_width()))
            error(ErrorType::ERR_UNSUPPORTED, "%scan only output entire %d-bit ALU output from %s",
                  sl->srcInfo, salu->alu_width(), reg_action->toString());
    } else if (etype == MINMAX_IDX) {
        // Magic constant 4 is log_2(memory word size in bytes)
        if (!checkSlice(sl, 4 - minmax_width))
            error(ErrorType::ERR_UNSUPPORTED,
                  "%s%s index output can only write to bottom %d bits of output", sl->srcInfo,
                  minmax_instr->name, 4 - minmax_width);
    } else {
        negate = false;
        keep_slice = true;
    }
    if (etype == IF) {
        const IR::PathExpression *pe = nullptr;
        if (auto mem = sl->e0->to<IR::Member>()) {
            pe = mem->expr->to<IR::PathExpression>();
        } else {
            pe = sl->e0->to<IR::PathExpression>();
        }
        if (pe) {
            for (auto p : params->parameters) {
                if (p->name == pe->path->name) {
                    // slice of register value used in condition is not allowed
                    error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                          "%sslice of register value in condition is not supported", sl->srcInfo);
                }
            }
        }
    }
    auto num_ops = operands.size();
    visit(sl->e0, "e0");
    if (keep_slice) {
        if (num_ops + 1 != operands.size()) {
            // slice on top of an instruction -- can't mask the output
            error(ErrorType::ERR_UNSUPPORTED,
                  "%sExpression too complex for RegisterAction - result of an instruction cannot "
                  "be sliced",
                  sl->srcInfo);
            return false;
        } else if (operands.back() == sl->e0) {
            operands.back() = sl;
        } else {
            operands.back() = MakeSlice(operands.back(), sl->getL(), sl->getH());
        }
        if (save_negate) operands.back() = new IR::Neg(operands.back());
    }
    negate = save_negate;
    return false;
}

static double mul(double x) { return x; }
static double sqr(double x) { return x * x; }
static double rsqrt(double x) { return 1.0 / sqrt(x); }
static double div(double x) { return 1.0 / x; }
static double rsqr(double x) { return 1.0 / x * x; }

static double (*fn[2][3])(double) = {{sqrt, mul, sqr}, {rsqrt, div, rsqr}};
// fn_max is max(fn(x)) for the x value range we need ([4/8, 15/8] for shift == -1,
// [8/8, 15/8] for shift >= 0)
static double fn_max[2][3] = {{1.36930639376291528364, 1.875, 3.515625},
                              {1.41421356237309504880, 1.0, 1.0}};

bool CreateSaluInstruction::preorder(const IR::MAU::Primitive *prim) {
    cstring method;
    if (const char *p = prim->name.find('.')) {
        method = cstring(p + 1);
    }
    if (prim->name == "min" || prim->name == "max") {
        if (etype == VALUE || (etype == OUTPUT && outputAluHi())) {
            opcode = prim->name + (isSigned(prim->type) ? "s"_cs : "u"_cs);
            if (etype == OUTPUT) etype = OUTPUT_ALUHI;
            return true;
        }
        error(ErrorType::ERR_UNSUPPORTED, "%s%s must write back to memory", prim->srcInfo,
              prim->name);
    } else if (prim->name == "math_unit.execute" || prim->name == "MathUnit.execute") {
        BUG_CHECK(prim->operands.size() == 2, "typechecking failure");
        if (operands.size() > 0) {
            if (auto *reg = operands.at(0)->to<IR::MAU::SaluReg>()) {
                if (reg->hi)
                    error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                          "%1% can only output to "
                          "lower half of memory",
                          prim);
            }
        }
        visit(prim->operands.at(1), "math_input");
        operands.back() =
            new IR::MAU::SaluFunction(prim->srcInfo, operands.back(), "math_table"_cs);
        LOG4("Math Unit operand: " << operands.back());
        auto gref = prim->operands.at(0)->to<IR::GlobalRef>();
        auto mu = gref ? gref->obj->to<IR::Declaration_Instance>() : nullptr;
        BUG_CHECK(mu, "typechecking failure?");
        math.valid = true;
        unsigned i = mu->arguments->size() - 1;
        BUG_CHECK(i >= 1 && i <= 3, "typechecking failure");
        if (auto k = mu->arguments->at(0)->expression->to<IR::BoolLiteral>()) {
            math.exp_invert = k->value;
            if (auto k = mu->arguments->at(1)->expression->to<IR::Constant>())
                math.exp_shift = k->asInt();
            BUG_CHECK(i == 3, "typechecking failure");
        } else if (auto m = mu->arguments->at(0)->expression->to<IR::Member>()) {
            BUG_CHECK(i < 3, "typechecking failure");
            if (m->member == "MUL") {
                math.exp_shift = 0;
                math.exp_invert = false;
            } else if (m->member == "SQR") {
                math.exp_shift = 1;
                math.exp_invert = false;
            } else if (m->member == "SQRT") {
                math.exp_shift = -1;
                math.exp_invert = false;
            } else if (m->member == "DIV") {
                math.exp_shift = 0;
                math.exp_invert = true;
            } else if (m->member == "RSQR") {
                math.exp_shift = 1;
                math.exp_invert = true;
            } else if (m->member == "RSQRT") {
                math.exp_shift = -1;
                math.exp_invert = true;
            } else {
                BUG("Invalid MathUnit ctor arg %s", m);
            }
        } else {
            BUG("Invalid MathUnit ctor arg %s", mu->arguments->at(0));
        }
        if (mu->arguments->at(i)->expression->is<IR::ListExpression>() ||
            mu->arguments->at(i)->expression->is<IR::StructExpression>()) {
            BUG_CHECK(i >= 2, "typechecking failure");
            if (auto k = mu->arguments->at(i - 1)->expression->to<IR::Constant>())
                math.scale = k->asInt();
            else
                error(ErrorType::ERR_UNEXPECTED, "Expected %s to be a constant scale",
                      mu->arguments->at(i - 1)->expression);
            auto components = getListExprComponents(*mu->arguments->at(i)->expression);
            BUG_CHECK(components->size() <= 16, "Wrong number of MathUnit values");
            i = 15;
            for (auto e : *components) {
                if (auto k = e->to<IR::Constant>()) math.table[i] = k->asInt();
                --i;
            }
        } else if (auto k = mu->arguments->at(i)->expression->to<IR::Constant>()) {
            double val = k->asUint64();
            BUG_CHECK(i == 1 || i == 2, "typechecking failure");
            if (i == 2) {
                if ((k = mu->arguments->at(i - 1)->expression->to<IR::Constant>()))
                    val = k->asUint64() / val;
                else
                    error(ErrorType::ERR_UNEXPECTED, "Expected %s to be a constant value",
                          mu->arguments->at(i - 1)->expression);
            }
            double max = val * fn_max[math.exp_invert][math.exp_shift + 1];
            // choose the largest possible scale such that all table entries will be < 256
            math.scale = floor(log2(max)) - 7;
            if (math.scale > 31) {
                warning(ErrorType::ERR_OVERLIMIT, "%sMathUnit scale overflow %d, capping at 31",
                        mu->srcInfo, math.scale);
                math.scale = 31;
            }
            if (math.scale < -32) {
                warning(ErrorType::ERR_OVERLIMIT, "%sMathUnit scale underflow %d, capping at -32",
                        mu->srcInfo, math.scale);
                math.scale = -32;
            }
            val *= pow(2.0, -math.scale);
            for (i = math.exp_shift >= 0 ? 8 : 4; i < 16; ++i) {
                math.table[i] = rint(fn[math.exp_invert][math.exp_shift + 1](i / 8.0) * val);
                if (math.table[i] < 0)
                    math.table[i] = 0;
                else if (math.table[i] > 255)
                    math.table[i] = 255;
            }
        } else {
            error(ErrorType::ERR_UNEXPECTED, "Expected %s to be a %s",
                  mu->arguments->at(i)->expression, i > 1 ? "list expression" : "constant");
        }
    } else if (method == "address") {
        operands.push_back(new IR::MAU::SaluReg(prim->srcInfo, prim->type, "address"_cs, false));
        address_subword = 0;
        if (prim->operands.size() == 2) {
            auto k = prim->operands.at(1)->to<IR::Constant>();
            if (!k || (k->value != 0 && k->value != 1))
                error(ErrorType::ERR_UNEXPECTED, "%s argument must be 0 or 1", prim);
            else
                address_subword = k->asInt();
        }
    } else if (method == "predicate") {
        operands.clear();
        etype = IF;
        BUG_CHECK(pred_operands.empty(), "internal state failure");
        for (int i = 1; i < int(prim->operands.size()); ++i) {
            const char *problem = nullptr;
            visit(prim->operands.at(i), "arg");
            if (auto *b = prim->operands.at(i)->to<IR::BoolLiteral>()) {
                operands.clear();
                operands.push_back(new IR::Constant(0));
                setupCmp(b->value ? "equ"_cs : "neq"_cs);
            }
            if (pred_operands.size() > 1) {
                problem = "can only have one comparison per predicate argument";
            } else if (pred_operands.size() < 1) {
                problem = "can only compare one metadata field, one memory field and one constant";
            } else if (auto *cmpreg = pred_operands.at(0)->to<IR::MAU::SaluCmpReg>()) {
                if (cmpreg->index != i - 1)
                    problem =
                        "does not match with other comparisons in the ALU, try putting "
                        "this.predicate call first";
            } else {
                problem = "can only compare one metadata field, one memory field and one constant";
            }
            if (problem)
                error(ErrorType::ERR_UNSUPPORTED, "%spredicate too complex for stateful alu: %s",
                      prim->operands.at(i)->srcInfo, problem);
            pred_operands.clear();
        }
        operands.clear();
        etype = OUTPUT;
        operands.push_back(new IR::MAU::SaluReg(prim->type, "predicate"_cs, false));
        // auto psize = 1 << Device::statefulAluSpec().CmpUnits.size();
        // FIXME -- should shift up to the top 16 bits of the word to maximize space
        // for the comb_shift constant, but doing so break action bus allocation
        salu->pred_shift = 0;  // = 28 - psize;
        if (salu->pred_comb_shift >= 0) {
            if (comb_pred_width > salu->pred_shift + 4)
                error(ErrorType::ERR_UNSUPPORTED, "conflicting predicate output use in %s", salu);
        }
    } else if (method == "min8" || method == "max8" || method == "min16" || method == "max16") {
        minmax_width = (method == "min16" || method == "max16") ? 1 : 0;
        if (etype != OUTPUT) {
            error(ErrorType::ERR_UNSUPPORTED, "%s can only write to an output", prim);
            return false;
        }
        auto *saved_predicate = predicate;
        auto saved_output_index = output_index;
        predicate = nullptr;
        opcode = method;
        BUG_CHECK(prim->operands.size() >= 3, "typechecking failure");
        etype = MINMAX_SRC;
        visit(prim->operands[1], "source");
        etype = VALUE;
        visit(prim->operands[2], "mask");
        if (prim->operands.size() >= 5) visit(prim->operands[4], "postmod");
        if (minmax_instr) {
            if (!equiv(&operands, &minmax_instr->operands))
                error(ErrorType::ERR_OVERLIMIT,
                      "%s: only one min/max operation possible in a stateful alu", prim);
        } else {
            minmax_instr = createInstruction();
        }
        operands.clear();
        if (prim->operands.size() >= 4) {
            etype = MINMAX_IDX;
            dest = nullptr;
            opcode = cstring();
            visit(prim->operands[3], "index");
            BUG_CHECK(operands.size() == 0 || errorCount() > 0, "%1%: recursion failure",
                      prim->operands[3]);
            operands.push_back(new IR::MAU::SaluReg(prim->type, "minmax_index"_cs, false));
            createInstruction();
            operands.clear();
        }
        output_index = saved_output_index;
        predicate = saved_predicate;
        etype = OUTPUT;
        operands.push_back(new IR::MAU::SaluReg(prim->type, "minmax"_cs, false));
    } else if (prim->name == "RegisterParam.read") {
        cstring name;
        // Retrieve the name of the RegisterParam
        if (auto *gr = prim->operands.at(0)->to<IR::GlobalRef>())
            if (auto *di = gr->obj->to<IR::Declaration_Instance>()) name = di->name.name;
        if (!name)
            error(ErrorType::ERR_UNKNOWN, "%1%: cannot retrieve the name of RegisterParam", prim);
        const IR::MAU::SaluRegfileRow *regfile = nullptr;
        if (salu->regfile.count(name) > 0) regfile = salu->regfile[name];
        if (!regfile)
            error(ErrorType::ERR_UNKNOWN,
                  "%1%: no matching RegisterParam in the current stateful ALU", prim);
        if (etype == OUTPUT || etype == VALUE) visit(regfile, "prim");
        if (etype == VALUE && dest) {
            // Update the LocalVar so that we know it stores a value from the register file
            dest->use = LocalVar::REGFILE;
            dest->regfile = regfile;
        }
    } else {
        error(ErrorType::ERR_UNSUPPORTED, "%sUnsupported method used in Stateful ALU",
              prim->srcInfo);
    }
    return false;
}

bool CreateSaluInstruction::preorder(const IR::MAU::SaluRegfileRow *regfile) {
    const IR::Expression *e = regfile;
    if (negate) e = new IR::Neg(e);
    negate_regfile = negate;
    LOG4("Register file operand: " << e);
    operands.push_back(e);
    return false;
}

static std::map<cstring, cstring> negate_op = {
    {"equ"_cs, "neq"_cs},     {"neq"_cs, "equ"_cs},         {"geq.s"_cs, "lss.s"_cs},
    {"geq.u"_cs, "lss.u"_cs}, {"geq.uus"_cs, "lss.uus"_cs}, {"grt.s"_cs, "leq.s"_cs},
    {"grt.u"_cs, "leq.u"_cs}, {"grt.uus"_cs, "leq.uus"_cs}, {"leq.s"_cs, "grt.s"_cs},
    {"leq.u"_cs, "grt.u"_cs}, {"leq.uus"_cs, "grt.uus"_cs}, {"lss.s"_cs, "geq.s"_cs},
    {"lss.u"_cs, "geq.u"_cs}, {"lss.uus"_cs, "geq.uus"_cs},
};

const IR::Expression *CreateSaluInstruction::reuseCmp(const IR::MAU::SaluInstruction *cmp,
                                                      int idx) {
    if (operands.size() + 1 != cmp->operands.size()) return nullptr;
    for (unsigned i = 0; i < operands.size(); ++i)
        if (!equiv(operands.at(i), cmp->operands.at(i + 1))) return nullptr;
    auto name = Device::statefulAluSpec().cmpUnit(idx);
    if (!name.startsWith("cmp")) name = "cmp" + name;
    if (opcode == cmp->name) return new IR::MAU::SaluCmpReg(name, idx);
    if (negate_op.at(opcode) == cmp->name) return new IR::LNot(new IR::MAU::SaluCmpReg(name, idx));
    return nullptr;
}

void CreateSaluInstruction::setupCmp(cstring op) {
    int idx = 0;
    opcode = op;
    for (auto cmp : cmp_instr) {
        if (auto inst = reuseCmp(cmp, idx++)) {
            LOG4("Relation reuse pred_operand: " << inst);
            pred_operands.push_back(inst);
            operands.clear();
            return;
        }
    }
    cstring name = Device::statefulAluSpec().cmpUnit(idx);
    operands.insert(operands.begin(), new IR::MAU::SaluCmpReg(name, idx));
    cmp_instr.push_back(createInstruction());
    if (!name.startsWith("cmp")) name = "cmp" + name;
    pred_operands.push_back(new IR::MAU::SaluCmpReg(name, idx));
    LOG4("Relation pred_operand: " << pred_operands.back());
    operands.clear();
}

static std::map<cstring, cstring> negate_rel_op = {
    {"geq.s"_cs, "leq.s"_cs}, {"geq.u"_cs, "leq.u"_cs}, {"geq.uus"_cs, "leq.uus"_cs},
    {"grt.s"_cs, "lss.s"_cs}, {"grt.u"_cs, "lss.u"_cs}, {"grt.uus"_cs, "lss.uus"_cs},
    {"leq.s"_cs, "geq.s"_cs}, {"leq.u"_cs, "geq.u"_cs}, {"leq.uus"_cs, "geq.uus"_cs},
    {"lss.s"_cs, "grt.s"_cs}, {"lss.u"_cs, "grt.u"_cs}, {"lss.uus"_cs, "grt.uus"_cs},
};

static bool can_be_uus_cmp(const IR::Operation::Relation *rel, const IR::Type::Bits *type) {
    if (!type->isSigned) return false;
    if (rel->left->is<IR::Add>() || rel->right->is<IR::Add>() || rel->left->is<IR::Sub>() ||
        rel->right->is<IR::Sub>()) {
        /* Should also check that the operand is constant 0?  If it is not, then there is no
         * totally correct instruction */
        return true;
    }
    return false;
}

bool CreateSaluInstruction::preorder(const IR::Operation::Relation *rel, cstring op, bool eq) {
    if (etype == OUTPUT && operands.empty()) {
        // output a boolean condition directly -- change it into IF setting value to 1
        etype = IF;
    }
    if (etype == IF) {
        Pattern::Match<IR::Expression> e1, e2;
        Pattern::Match<IR::Constant> k;
        if (Device::statefulAluSpec().CmpMask && ((e1 & k) == e2).match(rel) && !k->fitsUint() &&
            !k->fitsInt()) {
            // FIXME -- wide "neq" can be done with tmatch too?
            opcode = "tmatch"_cs;
            visit(rel->left, "left");
            visit(rel->right, "right");
        } else {
            opcode = op;
            if (!eq) {
                const char *type_suffix = ".u";
                if (auto t = rel->left->type->to<IR::Type::Bits>()) {
                    type_suffix = t->isSigned ? ".s" : ".u";
                    if (can_be_uus_cmp(rel, t)) {
                        if (t->size < salu->source_width()) {
                            error(ErrorType::ERR_UNSUPPORTED,
                                  "%sCan't do %d-bit comparison in a %d-bit SALU", rel->srcInfo,
                                  t->size, salu->source_width());
                        } else if (t->size == salu->source_width()) {
                            type_suffix = ".uus";
                        }
                    }
                }
                opcode += type_suffix;
            }

            // Keep track of negated register file reference
            negate_regfile = false;

            visit(rel->left, "left");
            negate = !negate;
            visit(rel->right, "right");
            negate = !negate;

            // If a register file reference is negated, the comparison is transformed
            // so that the register file reference wouldn't be negated.
            // E.g. memory - PHV > const => memory - PHV - const > 0 => -memory + PHV + const < 0
            if (negate_regfile) {
                if (!eq) opcode = negate_rel_op[opcode];
                for (auto &op : operands) {
                    if (auto neg = op->to<IR::Neg>())
                        op = neg->expr;
                    else
                        op = new IR::Neg(op);
                }
            }
        }
        BUG_CHECK(etype == IF, "etype changed?");
        setupCmp(opcode);
    } else {
        error(ErrorType::ERR_UNSUPPORTED,
              "%sIn Stateful ALU, a comparison can only be used in a condition or in an assignment "
              "to an output parameter.",
              rel->srcInfo);
    }
    return false;
}

bool CreateSaluInstruction::preorder(const IR::Cast *c) {
    if (safe_width_bits(c->type) > safe_width_bits(c->expr->type) && canBeIXBarExpr(c)) {
        operands.push_back(new IR::MAU::IXBarExpression(c));
        if (negate) operands.back() = new IR::Neg(operands.back());
        return false;
    }
    return true;
}

bool CreateSaluInstruction::preorder(const IR::BFN::SignExtend *c) {
    if (canBeIXBarExpr(c)) {
        operands.push_back(new IR::MAU::IXBarExpression(c));
        if (negate) operands.back() = new IR::Neg(operands.back());
        return false;
    }
    return true;
}

void CreateSaluInstruction::postorder(const IR::Cast *c) {
    if (etype == IF && c->type->to<IR::Type::Boolean>()) setupCmp("neq"_cs);
}

void CreateSaluInstruction::postorder(const IR::BFN::ReinterpretCast *c) {
    if (etype == IF && c->type->to<IR::Type::Boolean>()) setupCmp("neq"_cs);
}

inline void condition_error(const IR::Node *node, const char *operation) {
    error(ErrorType::ERR_UNSUPPORTED, "%sIn Stateful ALU, %s can only be used in a condition.",
          node->srcInfo, operation);
}
inline void only_operation_error(const IR::Node *node, const char *operation,
                                 const IR::MAU::StatefulAlu *salu) {
    CHECK_NULL(salu);
    if (!salu->dual)
        error(ErrorType::ERR_UNSUPPORTED,
              "%sIn Stateful ALU, %s can only be used if it is the only operation in a control "
              "flow.",
              node->srcInfo, operation);
    else
        error(ErrorType::ERR_UNSUPPORTED,
              "%s: In Stateful ALU for registers with two fields or a single %d bit field, %s can "
              "only be used when the result is written to the register.",
              node->srcInfo, Device::statefulAluSpec().MaxDualSize, operation);
}

void CreateSaluInstruction::postorder(const IR::LNot *e) {
    if (etype == IF) {
        if (operands.size() == 1 && is_learn(operands.back())) {
            operands.back() = new IR::LNot(e->srcInfo, operands.back());
            return;
        }
        BUG_CHECK(pred_operands.size() >= 1 || errorCount() > 0, "%1%: recursion failure", e);
        if (pred_operands.size() < 1) return;  // can only happen if there has been an error
        pred_operands.back() = negatePred(pred_operands.back());
        LOG4("LNot rewrite pred_opeands: " << pred_operands.back());
    } else {
        condition_error(e, "logical negation");
    }
}

void CreateSaluInstruction::postorder(const IR::LAnd *e) {
    if (etype == IF) {
        if (pred_operands.size() == 1) return;  // to deal with learn -- not always correct
        BUG_CHECK(pred_operands.size() >= 2 || errorCount() > 0, "%1%: recursion failure", e);
        if (pred_operands.size() < 2) return;  // can only happen if there has been an error
        auto r = pred_operands.back();
        pred_operands.pop_back();
        pred_operands.back() = new IR::LAnd(e->srcInfo, pred_operands.back(), r);
        LOG4("LAnd rewrite pred_opeands: " << pred_operands.back());
    } else {
        condition_error(e, "logical conjunction");
    }
}
void CreateSaluInstruction::postorder(const IR::LOr *e) {
    if (etype == IF) {
        if (pred_operands.size() == 1) return;  // to deal with learn -- not always correct
        BUG_CHECK(pred_operands.size() >= 2 || errorCount() > 0, "%1%: recursion failure", e);
        if (pred_operands.size() < 2) return;  // can only happen if there has been an error
        auto r = pred_operands.back();
        pred_operands.pop_back();
        pred_operands.back() = new IR::LOr(e->srcInfo, pred_operands.back(), r);
        LOG4("LOr rewrite pred_opeands: " << pred_operands.back());
    } else {
        condition_error(e, "logical disjunction");
    }
}

bool CreateSaluInstruction::preorder(const IR::Add *e) {
    checkAndReportComplexInstruction(e);
    if (etype == IF) return true;
    if (etype == VALUE || (etype == OUTPUT && outputAluHi())) {
        opcode = "add"_cs;
        if (etype == OUTPUT) etype = OUTPUT_ALUHI;
        return true;
    }
    only_operation_error(e, "addition", salu);
    return false;
}
bool CreateSaluInstruction::preorder(const IR::AddSat *e) {
    checkAndReportComplexInstruction(e);
    if (etype == VALUE || (etype == OUTPUT && outputAluHi())) {
        opcode = isSigned(e->type) ? "sadds"_cs : "saddu"_cs;
        if (etype == OUTPUT) etype = OUTPUT_ALUHI;
        return true;
    } else {
        only_operation_error(e, "saturating addition", salu);
        return false;
    }
}

bool CreateSaluInstruction::preorder(const IR::Sub *e) {
    checkAndReportComplexInstruction(e);
    if (etype == IF) {
        visit(e->left, "left");
        negate = !negate;
        visit(e->right, "right");
        negate = !negate;
        return false;
    }
    if (etype == VALUE || (etype == OUTPUT && outputAluHi())) {
        opcode = "sub"_cs;
        if (etype == OUTPUT) etype = OUTPUT_ALUHI;
        return true;
    }
    only_operation_error(e, "subtraction", salu);
    return false;
}
bool CreateSaluInstruction::preorder(const IR::SubSat *e) {
    checkAndReportComplexInstruction(e);
    if (etype == VALUE || (etype == OUTPUT && outputAluHi())) {
        opcode = isSigned(e->type) ? "ssubs"_cs : "ssubu"_cs;
        if (etype == OUTPUT) etype = OUTPUT_ALUHI;
        return true;
    } else {
        only_operation_error(e, "saturating subtraction", salu);
        return false;
    }
}

void CreateSaluInstruction::postorder(const IR::BAnd *e) {
    checkAndReportComplexInstruction(e);
    if (etype == VALUE || etype == OUTPUT_ALUHI) {
        if (e->left->is<IR::Cmpl>())
            opcode = "andca"_cs;
        else if (e->right->is<IR::Cmpl>())
            opcode = "andcb"_cs;
        else
            opcode = "and"_cs;
    } else if (etype == IF) {
        if (operands.size() < 2) return;  // can only happen if there has been an error
        if (opcode == "tmatch") return;   // separate operands
        auto r = operands.back();
        operands.pop_back();
        if (!r->is<IR::Constant>()) {
            if (!operands.back()->is<IR::Constant>())
                error("%s: mask operand must be a constant", r->srcInfo);
            std::swap(r, operands.back());
        }
        operands.back() = new IR::BAnd(e->srcInfo, operands.back(), r);
        LOG4("BAnd rewrite opeands: " << operands.back());
    } else {
        only_operation_error(e, "conjunction", salu);
    }
}

void CreateSaluInstruction::checkAndReportComplexInstruction(const IR::Operation_Binary *op) const {
    if (safe_width_bits(regtype) == 1) {
        error(ErrorType::ERR_UNSUPPORTED,
              "%sOnly simple assignments are supported for one-bit registers.", op->srcInfo);
        return;
    }

    if (!isComplexInstruction(op)) return;

    error(ErrorType::ERR_UNSUPPORTED,
          "You can only have more than one binary operator in a statement if "
          "the outer one is |%1%",
          op->srcInfo);
}
bool CreateSaluInstruction::isComplexInstruction(const IR::Operation_Binary *op) const {
    // The code checks if the passed binary operation is in the simple form: left <op> right
    // or if the sub-trees are not supported complex operations.
    //
    //  Examples:
    // ===========
    //
    //  1. (l1 <op1> r2) <op2> <whatever>;
    //  2. <whatever> <op1> (l1 <op2> l2)
    //
    // The method returns true if so, false otherwise
    //
    // IR::L* operators (LAnd, LOr, etc.) are working with predicates

    bool ret = false;
    for (auto oper : {op->left, op->right}) {
        ret |= oper->is<IR::Add>() | oper->is<IR::AddSat>();
        ret |= oper->is<IR::Sub>() | oper->is<IR::SubSat>();
        ret |= oper->is<IR::BAnd>() | oper->is<IR::BOr>() | oper->is<IR::BXor>();
        ret |= oper->is<IR::Div>() | oper->is<IR::Mod>();
        ret |= oper->is<IR::Neg>();
    }

    return ret;
}

bool CreateSaluInstruction::preorder(const IR::Neg *) {
    if (etype == OUTPUT && outputAluHi()) etype = OUTPUT_ALUHI;
    return true;
}

bool CreateSaluInstruction::preorder(const IR::BAnd *) {
    if (etype == OUTPUT && outputAluHi()) etype = OUTPUT_ALUHI;
    return true;
}

bool CreateSaluInstruction::preorder(const IR::BXor *) {
    if (etype == OUTPUT && outputAluHi()) etype = OUTPUT_ALUHI;
    return true;
}

bool CreateSaluInstruction::preorder(const IR::BOr *e) {
    if (etype == OUTPUT && outputAluHi()) etype = OUTPUT_ALUHI;
    if (isComplexInstruction(e)) {
        if (etype == VALUE && operands.size() == 1) {
            // Collect & dump data for the left subtree
            auto old_opcode = opcode;
            auto old_operands = operands;
            visit(e->left);
            doAssignment(e->left->srcInfo);
            auto *dest = action->action.back()->to<IR::MAU::SaluInstruction>()->getOutput();
            or_targets.emplace(dest);
            // Collect & dump data for the right subtree
            etype = VALUE;
            opcode = old_opcode;
            operands = old_operands;
            visit(e->right);
            doAssignment(e->right->srcInfo);
            dest = action->action.back()->to<IR::MAU::SaluInstruction>()->getOutput();
            or_targets.emplace(dest);
        } else if (etype == IF) {
            error(ErrorType::ERR_UNSUPPORTED, "%stoo complex bitwise operation used in a condition",
                  e->srcInfo);
        } else {
            // All other cases are too complex
            checkAndReportComplexInstruction(e);
        }
    }

    return true;
}
void CreateSaluInstruction::postorder(const IR::BOr *e) {
    if (isComplexInstruction(e)) {
        LOG3("Complex SALU instruction has been detected. Skipping the OR postorder analysis");
        return;
    }

    if (etype == VALUE || etype == OUTPUT_ALUHI) {
        if (e->left->is<IR::Cmpl>())
            opcode = "orca"_cs;
        else if (e->right->is<IR::Cmpl>())
            opcode = "orcb"_cs;
        else
            opcode = "or"_cs;
    } else {
        only_operation_error(e, "disjunction", salu);
    }
}
bool CreateSaluInstruction::preorder(const IR::Concat *e) {
    if (Pattern(0).match(e->left)) {
        // ElimCasts introduces these zero-extend concats for type casts we can ignore
        visit(e->right, "right");
        return false;
    }
    if (canBeIXBarExpr(e)) {
        operands.push_back(new IR::MAU::IXBarExpression(e));
        if (negate) operands.back() = new IR::Neg(operands.back());
        return false;
    }
    error(ErrorType::ERR_UNSUPPORTED,
          "%sBit concatenation result cannot be assigned to an output parameter", e->srcInfo);
    return false;
}
void CreateSaluInstruction::postorder(const IR::BXor *e) {
    checkAndReportComplexInstruction(e);
    if (etype == VALUE || etype == OUTPUT_ALUHI) {
        if (e->left->is<IR::Cmpl>() || e->right->is<IR::Cmpl>())
            opcode = "xnor"_cs;
        else
            opcode = "xor"_cs;
    } else {
        only_operation_error(e, "xor", salu);
    }
}
void CreateSaluInstruction::postorder(const IR::Neg *e) {
    // There is no opcode for unary negation, so we use subtraction instead. SUBR performs B - A
    // with A being the value to negate and B = 0.
    opcode = "subr"_cs;
    if (operands.size() <= 2) {
        BUG_CHECK(!operands.empty(), "No operands for unary negation in SALU action %1%", salu);
        const auto *negated = operands.back();
        operands.push_back(new IR::Constant(negated->srcInfo, negated->type, 0));
    } else {
        only_operation_error(e, "negation", salu);
    }
}
void CreateSaluInstruction::postorder(const IR::Cmpl *e) {
    static const std::map<cstring, cstring> complement = {
        {"alu_a"_cs, "nota"_cs}, {"alu_b"_cs, "notb"_cs}, {"andca"_cs, "orcb"_cs},
        {"andcb"_cs, "orca"_cs}, {"and"_cs, "nand"_cs},   {"nand"_cs, "and"_cs},
        {"nor"_cs, "or"_cs},     {"nota"_cs, "alu_a"_cs}, {"notb"_cs, "alu_b"_cs},
        {"orca"_cs, "andcb"_cs}, {"orcb"_cs, "andca"_cs}, {"or"_cs, "nor"_cs},
        {"sethi"_cs, "setz"_cs}, {"setz"_cs, "sethi"_cs}, {"xnor"_cs, "xor"_cs},
        {"xor"_cs, "xnor"_cs}};
    if (etype == OUTPUT && safe_width_bits(regtype) == 1) {
        onebit_cmpl = true;
        return;
    }
    if (complement.count(opcode))
        opcode = complement.at(opcode);
    else if (etype == OUTPUT || etype == OUTPUT_ALUHI) {
        if (outputAluHi()) {
            etype = OUTPUT_ALUHI;
        }
        // Switch opcode from "output" to "nota" so that the correct instruction is created for
        // bit complements in ALUHI, eg. ret = ~hdr.field;. If this bit complement is part of a
        // binary bitwise operation, eg. ORCA, the opcode will be changed again in the relevant
        // postorder method for the binary operation.
        opcode = "nota"_cs;
    } else if (etype != VALUE)
        only_operation_error(e, "bit complement", salu);
}
void CreateSaluInstruction::postorder(const IR::Concat *e) {
    if (operands.size() < 2) return;  // can only happen if there has been an error
    if (etype == VALUE || etype == MINMAX_SRC) return;  // done in preorder
    if (etype == IF) {
        auto r = operands.back();
        operands.pop_back();
        if (r->is<IR::Constant>()) {
            // FIXME -- dropped constant needed in hash function
            LOG4("concant dropping low bit constant " << r);
            return;
        }
        if (operands.back()->is<IR::Constant>()) {
            // FIXME -- dropped constant needed in hash function
            LOG4("concant dropping high bit constant " << operands.back());
            operands.back() = r;
            return;
        }
    }
    only_operation_error(e, "bit concatenation", salu);
}

bool CreateSaluInstruction::divmod(const IR::Operation::Binary *e, cstring op) {
    if (etype == OUTPUT && operands.empty() && Device::statefulAluSpec().DivModUnit) {
        auto *saved_predicate = predicate;
        etype = VALUE;
        predicate = nullptr;
        opcode = "divmod"_cs;
        visit(e->left, "left");
        visit(e->right, "right");
        if (divmod_instr) {
            if (!equiv(&operands, &divmod_instr->operands))
                error(ErrorType::ERR_UNSUPPORTED,
                      "%s: only one div/mod operation possible in a Stateful ALU", e);
        } else {
            divmod_instr = createInstruction();
        }
        operands.clear();
        predicate = saved_predicate;
        etype = OUTPUT;
        operands.push_back(new IR::MAU::SaluReg(e->type, op, false));
    } else {
        only_operation_error(e, "div/mod operation", salu);
    }
    return false;
}

static bool canOutputDirectly(const IR::Expression *e) {
    if (auto sl = e->to<IR::Slice>()) e = sl->e0;
    if (auto m = e->to<IR::Member>()) return !m->expr->is<IR::TypeNameExpression>();
    if (e->is<IR::TempVar>()) return true;
    if (e->is<IR::MAU::SaluReg>()) return true;
    return false;
}

bool CreateSaluInstruction::outputEnumAsPredicate(const IR::Member *mem) {
    if (!mem || !return_encoding || return_encoding->return_type != mem->type) return false;
    unsigned mask = (1U << (1U << Device::statefulAluSpec().CmpUnits.size())) - 1;
    if (predicate) mask &= eval_predicate(predicate);
    for (auto &el : Values(return_encoding->tag_used)) el &= ~mask;
    return_encoding->tag_used[mem->member.name] |= mask;
    LOG3("  output " << mem << " in predicate 0x" << hex(mask));
    return true;
}

// The alus and outputs only operate on 32 bits at a time, but we allow wider operands for
// simple copies.  Those need to be split into multiple instructions operating 32 bits at
// a time;
void CreateSaluInstruction::splitWideInstructions() {
    IR::Vector<IR::MAU::SaluInstruction> code;
    std::array<const IR::Expression *, 4> words = {nullptr};
    for (auto *inst : action->action) {
        if (auto *dest = inst->getOutput()) {
            if (safe_width_bits(dest->type) > 32) {
                if (inst->name == "max8" || inst->name == "max16" || inst->name == "min8" ||
                    inst->name == "min16") {
                    // leave these alone
                } else if (inst->name == "add" || inst->name == "or" || inst->name == "sub" ||
                           inst->name == "xor") {
                    // FIXME --- these only work if the operation only affects the bottom 32
                    // bits of a larger value -- so when the second operand is a constant with
                    // the upper bits all 0s and can't over/underflow in the case of an add/sub
                    // We should probably check that and give an error, but for now we leave
                    // it alone
                } else if (inst->name == "alu_a") {
                    Log::TempIndent indent;
                    LOG4("split wide instruction " << *inst << indent);
                    unsigned i = dest->to<IR::MAU::SaluReg>()->hi ? 2 : 0;
                    auto *src = inst->operands.back();
                    for (int bit = 0; bit < safe_width_bits(dest->type); bit += 32, ++i) {
                        if (words[i]) {
                            error("%s%sConflicting wide operations in SALU", words[i]->srcInfo,
                                  src->srcInfo);
                        }
                        words[i] = MakeSlice(src, bit, bit + 31);
                        if ((i & 1) == 0) {
                            // only need actual instructions for non-flyover, as the
                            // flyover is implicit
                            auto *n = inst->clone();
                            if (i > 1)
                                n->operands.at(n->output_operand) = new IR::MAU::SaluReg(
                                    dest->srcInfo, words[i]->type, "hi"_cs, true);
                            n->operands.back() = MakeSlice(src, bit, bit + 63);
                            LOG4("add " << *n);
                            code.push_back(n);
                        }
                    }
                    continue;
                } else {
                    error("%sCan't do %s operation wider than 32 bits in SALU", inst->srcInfo,
                          inst->name);
                }
            }
        }
        code.push_back(inst);
    }
    action->action = code;
    constexpr std::array<int, 4> output_transpose = {0, 2, 1, 3};
    for (int i = outputs.size() - 1; i >= 0; --i) {
        if (!outputs[i]) continue;
        auto *src = outputs[i]->operands.back();
        if (safe_width_bits(src->type) <= 32) continue;
        LOG4("split wide output " << *outputs[i]);
        auto *reg = src->to<IR::MAU::SaluReg>();
        int j = output_transpose.at(i);
        if (reg && (reg->name == "alu_lo" || reg->name == "alu_hi")) {
            for (int bit = 0; bit < safe_width_bits(src->type); bit += 32, ++j) {
                int k = output_transpose.at(j);
                if (size_t(k) >= outputs.size()) outputs.resize(k + 1);
                if (k != i) {
                    if (outputs[k]) {
                        error("%s%sSALU wide output conflicts with other output", src->srcInfo,
                              outputs[k]->srcInfo);
                    }
                    auto *n = outputs[i]->clone();
                    n->operands.back() = words[j];
                    outputs[k] = n;
                }
            }
        } else {
            BUG("TBD");
        }
    }
}

void CreateSaluInstruction::assignOutputAlus() {
    if (Device::statefulAluSpec().OutputWords == 1) {
        action->output_param_to_alu[0] = 0;
        return;
    }

    for (int param = 0; param < Device::statefulAluSpec().OutputWords; ++param) {
        // In case any output parameter ends up uninitialized, we assign it to an output ALU that
        // outputs 0.
        output_param_operands.insert({param, nullptr});
    }

    std::set<int> used_alus;
    // First handle memory/phv slices because they might be constrained to specific ALUs.
    for (const auto &[param, operand] : output_param_operands) {
        if (operand) {
            if (const auto *slice = operand->to<IR::Slice>(); slice) {
                bool duplicate = false;
                for (const auto &[assigned_param, assigned_alu] : action->output_param_to_alu) {
                    const auto *assigned_op = output_param_operands.at(assigned_param);
                    if (assigned_op && slice->equiv(*assigned_op)) {
                        // Use the same output ALU for multiple output parameters if they return the
                        // same value.
                        action->output_param_to_alu[param] = assigned_alu;
                        duplicate = true;
                        break;
                    }
                }
                if (duplicate) {
                    continue;
                }

                // alu0/1 always use bits 31:0, alu2/3 - bits 63:32.
                // See JBay MAU Micro-Architecture 6.2.12.8 Alu-Output for details.
                const unsigned BOUNDARY = 32;
                std::vector<int> available_alus;
                bool ls_bits = true;
                if (slice->getH() < BOUNDARY) {
                    available_alus = {0, 1};
                } else if (slice->getL() >= BOUNDARY) {
                    available_alus = {2, 3};
                    ls_bits = false;
                } else {
                    ::fatal_error(ErrorType::ERR_INVALID,
                                  "Slices assigned to outputs of register actions cannot cross the "
                                  "%2%-bit boundary %1%",
                                  slice->srcInfo, BOUNDARY);
                }

                const auto alu = std::find_if(available_alus.begin(), available_alus.end(),
                                              [&](const int alu) { return !used_alus.count(alu); });
                if (alu != available_alus.end()) {
                    action->output_param_to_alu[param] = *alu;
                    used_alus.insert(*alu);
                } else {
                    // This is possible if too many output parameters are all assigned to either low
                    // or high bits of phv and stateful memory. However, returning values from PHV
                    // is not supported currently so it shouldn't happen.
                    std::stringstream msg;
                    msg << "Too many output parameters return the " << BOUNDARY
                        << (ls_bits ? " least" : " most")
                        << " significant bits of stateful memory or PHV from action "
                        << action->name << ". Reduce the number of such outputs to "
                        << available_alus.size() << ".%1%";
                    ::fatal_error(ErrorType::ERR_OVERLIMIT, msg.str().c_str(), action->srcInfo);
                }
            }
        }
    }
    // Handle other outputs
    for (const auto &[param, operand] : output_param_operands) {
        if (action->output_param_to_alu.count(param)) {
            continue;
        }

        for (int alu = 0; alu < Device::statefulAluSpec().OutputWords; ++alu) {
            if (!used_alus.count(alu)) {
                action->output_param_to_alu[param] = alu;
                used_alus.insert(alu);
                break;
            }
        }

        BUG_CHECK(
            action->output_param_to_alu.count(param),
            "Couldn't assign an output ALU even though the operand (%1%) imposes no constraints",
            operand);
    }
}

const IR::MAU::SaluInstruction *CreateSaluInstruction::setup_output() {
    if (outputs.size() <= size_t(output_index)) outputs.resize(output_index + 1);
    auto &output = outputs[output_index];
    int out_idx = -1;
    if (Device::statefulAluSpec().OutputWords > 1) {
        BUG_CHECK(operands.size() == 1ul, "Unexpected number of operands (%1%) for output",
                  operands.size());
        output_param_operands[output_index] = operands.front();
        out_idx = 0;
    }
    if (predicate) {
        operands.insert(operands.begin(), predicate);
        if (out_idx >= 0) ++out_idx;
    }
    if (is_address_output(operands.back()) && address_subword) {
        if (predicate && output_address_subword_predicate.count(output_index)) {
            auto &subword_predicate = output_address_subword_predicate[output_index];
            if (subword_predicate) subword_predicate = new IR::LOr(subword_predicate, predicate);
        } else {
            output_address_subword_predicate[output_index] = predicate;
        }
    }
    if (output) {
        if (!equiv(operands.back(), output->operands.back())) {
            error(ErrorType::ERR_UNSUPPORTED, "Incompatible outputs in RegisterAction: %s and %s\n",
                  output->operands.back(), operands.back());
            return nullptr;
        }
        if (output->operands.size() == operands.size()) {
            if (predicate)
                operands[0] = new IR::LOr(output->operands[0], operands[0]);
            else
                return output;
        } else if (predicate) {
            return output;
        }
    }
    return output = new IR::MAU::SaluInstruction("output"_cs, out_idx, &operands);
}

const IR::MAU::SaluInstruction *CreateSaluInstruction::createInstruction() {
    const IR::MAU::SaluInstruction *rv = nullptr;
    auto k = operands.back()->to<IR::Constant>();
    bool skip_direct = false;
    const auto predicate_error = [&](const char *type) {
        error(ErrorType::ERR_UNSUPPORTED,
              "cannot output %2% from %1% because the predicate is output in another control "
              "flow",
              salu, type);
    };
    auto generate_alu_a = [this](const IR::Expression *val, const IR::Expression *predicate,
                                 const IR::MAU::SaluReg *reg, bool is_mem) {
        const cstring MEM_PREFIX = "mem_"_cs;

        // mem_lo/mem_hi can be used in output ALU instructions but not in state update ALU
        // instructions such as ALU_A. In state update ALU instructions, lo/hi should be
        // used instead, so we remove the prefix.
        if (is_mem) {
            const auto trimmed_name = reg->name.substr(MEM_PREFIX.size());
            val = new IR::MAU::SaluReg(reg->srcInfo, reg->type, trimmed_name, reg->hi);
        }

        const auto *dest_op = new IR::MAU::SaluReg(val->srcInfo, val->type, "hi"_cs, true);
        if (predicate) {
            insert_instruction(
                new IR::MAU::SaluInstruction("alu_a"_cs, 1, predicate, dest_op, val));
        } else {
            insert_instruction(new IR::MAU::SaluInstruction("alu_a"_cs, 0, dest_op, val));
        }
        LOG3("  add " << *action->action.back());
        return new IR::MAU::SaluReg(val->srcInfo, val->type, "alu_hi"_cs, true);
    };

    switch (etype) {
        case IF:
            insert_instruction(rv = new IR::MAU::SaluInstruction(opcode, 0, &operands));
            LOG3("  add " << *action->action.back());
            break;
        case VALUE:
        case MATCH:
            checkWriteAfterWrite();
            if (safe_width_bits(regtype) == 1) {
                BUG_CHECK(operands.size() == 2,
                          "one-bit register VALUE instruction should have two"
                          " operands only (output and a constant)");
                opcode = "clr_bit"_cs;
                if (predicate)
                    error(ErrorType::ERR_UNSUPPORTED,
                          "%scan't have condition in a RegisterAction<bit<1>>", predicate->srcInfo);
                else if (!k)
                    error(ErrorType::ERR_UNSUPPORTED,
                          "%scan't write non-constant value to Register<bit<1>>",
                          operands.back()->srcInfo);
                else if (k->value)
                    opcode = "set_bit"_cs;
                if (onebit_cmpl) opcode += 'c';
                /* For non-resilient hashes, all the bits must be updated whenever a port comes
                 * up or down and hence must use the adjust_total instructions */
                if (salu->selector && salu->selector->mode == IR::MAU::SelectorMode::FAIR)
                    opcode += "_at";
                rv = onebit = new IR::MAU::SaluInstruction(opcode);
            } else {
                int out_idx = 0;
                if (predicate) {
                    operands.insert(operands.begin(), predicate);
                    ++out_idx;
                }

                // mark divmod's output index, so that defuse analysis is correct
                if (opcode == "divmod") out_idx = -1;
                insert_instruction(rv = new IR::MAU::SaluInstruction(opcode, out_idx, &operands));
                LOG3("  add " << *action->action.back());
            }
            break;
        case OUTPUT:
            checkWriteAfterWrite();
            // Identify if we need to use the ALU_HI instead of direct output. Do this when either:
            //  - We have an existing output, and the output is already ALU_HI and we have something
            //    directly outputable.
            //  - We have a direct output, and we have something else that has to use the ALU. In
            //  this
            //    case, we need to convert the direct output to an ALU_HI.
            if (outputs.size() > size_t(output_index) && outputs[output_index] &&
                safe_width_bits(regtype) != 1 && !(k && k->value == 0)) {
                if (canOutputDirectly(operands.at(0))) {
                    auto &output = outputs[output_index];
                    skip_direct = !equiv(operands.back(), output->operands.back());
                } else if (outputAluHi()) {
                    const cstring MEM_PREFIX = "mem_"_cs;
                    const auto *reg = operands.at(0)->to<IR::MAU::SaluReg>();
                    const bool is_mem = reg && reg->name.startsWith(MEM_PREFIX.string());

                    auto *val = operands.at(0);

                    // mem_lo/mem_hi can be used in output ALU instructions but not in state update
                    // ALU instructions such as ALU_A. In state update ALU instructions, lo/hi
                    // should be used instead, so we remove the prefix.
                    if (is_mem) {
                        const auto trimmed_name = reg->name.substr(MEM_PREFIX.size());
                        val = new IR::MAU::SaluReg(reg->srcInfo, reg->type, trimmed_name, reg->hi);
                    }
                    auto *new_operand =
                        new IR::MAU::SaluReg(val->srcInfo, val->type, "alu_hi"_cs, true);

                    auto &output = outputs[output_index];
                    if (!equiv(new_operand, output->operands.back())) {
                        LOG3("  converting previous direct output to alu_a");

                        // use ALU_HI to drive the output as it is otherwise unused
                        auto *val = output->operands.back();
                        const auto *reg = val->to<IR::MAU::SaluReg>();
                        const bool is_mem = reg && reg->name.startsWith(MEM_PREFIX.string());

                        output->operands.back() =
                            generate_alu_a(val, output_predicates[output_index], reg, is_mem);
                    }
                }
            }

            if (safe_width_bits(regtype) == 1) {
                BUG_CHECK(operands.size() == 1,
                          "one-bit register OUTPUT instruction should have one"
                          " operand only (the output)");
                if (predicate) {
                    error(ErrorType::ERR_UNSUPPORTED, "%scan't have predicate on 1-bit instruction",
                          predicate->srcInfo);
                }
                opcode = onebit ? onebit->name : "read_bit"_cs;
                if (onebit_cmpl) opcode += "c";
                rv = onebit = new IR::MAU::SaluInstruction(opcode);
                operands.clear();
                operands.push_back(
                    new IR::MAU::SaluReg(IR::Type::Bits::get(1), "alu_lo"_cs, false));
            } else if (k && k->value == 0) {
                // 0 will be output if we don't drive it at all
                output_param_operands.insert({output_index, nullptr});
                break;
            } else if (canOutputDirectly(operands.at(0)) && !skip_direct) {
                if (auto *reg = operands.at(0)->to<IR::MAU::SaluReg>()) {
                    // explicit output of the predicate comes out shifted, so we need to
                    // let the rest of the compiler know when to unshift it.
                    if (reg->name == "predicate")
                        action->return_predicate_words |= 1 << output_index;
                }
                output_predicates[output_index] = predicate;
                // output it
            } else if (const bool directly = canOutputDirectly(operands.at(0)),
                       alu_hi = outputAluHi();
                       directly || alu_hi) {
                const cstring MEM_PREFIX = "mem_"_cs;
                const auto *reg = operands.at(0)->to<IR::MAU::SaluReg>();
                const bool is_mem = reg && reg->name.startsWith(MEM_PREFIX.string());

                // Prefer to output values from stateful memory via ALU_HI, because this way it's
                // possible for other sources to be assigned to the same output parameter with
                // different predicates.
                if (!is_mem && directly) {
                    // explicit output of the predicate comes out shifted, so we need to
                    // let the rest of the compiler know when to unshift it.
                    if (reg && reg->name == "predicate") {
                        action->return_predicate_words |= 1 << output_index;
                    }
                    // output it
                } else if (alu_hi) {
                    // use ALU_HI to drive the output as it is otherwise unused
                    operands.at(0) = generate_alu_a(operands.at(0), predicate, reg, is_mem);
                }
            } else if (k && (k->value & (k->value - 1)) == 0) {
                // use the predicate output shifted to the appropriate spot for a power of 2
                // constant no predicate means unconditional, which will output 1 unconditionally
                comb_pred_width = std::max(comb_pred_width, safe_width_bits(k->type));
                int shift = floor_log2(k->value);
                if (salu->pred_comb_shift >= 0 && salu->pred_comb_shift != shift)
                    predicate_error("constant");
                else
                    salu->pred_comb_shift = shift;
                if (salu->pred_shift >= 0) {
                    if (comb_pred_width > salu->pred_shift + 4) predicate_error("constant");
                } else {
                    salu->pred_shift = 28;
                }
                operands.at(0) = new IR::MAU::SaluReg(k->srcInfo, k->type, "predicate"_cs, false);
            } else if (outputEnumAsPredicate(operands.at(0)->to<IR::Member>())) {
                auto psize = 1 << Device::statefulAluSpec().CmpUnits.size();
                // FIXME -- should shift up to the top 16 bits of the word to maximize space
                // for the comb_shift constant, but doing so break action bus allocation
                salu->pred_shift = 0;  // = 28 - psize;
                if (salu->pred_comb_shift >= 0) {
                    if (comb_pred_width > salu->pred_shift + 4) predicate_error("enum");
                }
                operands.at(0) = new IR::MAU::SaluReg(
                    operands.at(0)->srcInfo, IR::Type::Bits::get(psize), "predicate"_cs, false);
                action->return_predicate_words |= 1 << output_index;
            } else {
                error(ErrorType::ERR_UNSUPPORTED, "can't output %1% from a RegisterAction",
                      operands.at(0));
            }
            rv = setup_output();
            break;
        default:
            BUG("Invalid etype");
            break;
    }
    return rv;
}

/**
 * @brief Compare two instructions if they are same and predicates are
 * not null.
 *
 * @param si1 First instruction to check
 * @param si2 Second instruction to check
 * @return true Both instructions are same, predicates are not same
 * @return false Instructions have different options
 */
static bool check_instructions(const IR::MAU::SaluInstruction *si1,
                               const IR::MAU::SaluInstruction *si2) {
    // Similar instructions differs in one field only (the predicate).
    LOG3("Probing instructions: " << si1 << " and " << si2);
    if (si1->name != si2->name || si1->output_operand != si2->output_operand ||
        si1->operands.size() != si2->operands.size())
        return false;
    // Start the check from the output operand index (typically after the predicate)
    for (size_t i = si1->output_operand; i < si1->operands.size(); i++)
        if (!si1->operands.at(i)->equiv(*si2->operands.at(i))) return false;
    return true;
}

void CreateSaluInstruction::insert_instruction(const IR::MAU::SaluInstruction *si_insert) {
    // We need to iterate over instructions inside the body and check if we can merge
    // instructions together. Two instructions are possible to merge if:
    // 1] Opcode and params are same
    // 2] The only difference is the predicate
    //
    // Instructions are merged together via the IR::BOr node where left and right
    // node is predicate of both values. We don't need to insert the BOr if predicate
    // expressions are same.
    bool insert = true;
    for (auto it = action->action.begin(); it != action->action.end(); it++) {
        auto si_orig = (*it)->to<IR::MAU::SaluInstruction>();
        BUG_CHECK(si_orig,
                  "SALU instruction is the only type which can be inserted into SALU assembler!");
        // We don't want to insert same instruction which is already ther
        if (si_orig->equiv(*si_insert)) {
            LOG3("Same instruction " << si_insert
                                     << " has been detected. It will not be inserted again.");
            insert = false;
            break;
        }

        bool similar_inst = check_instructions(si_orig, si_insert);
        if (similar_inst) {
            auto merged_pred = new IR::LOr(si_insert->operands.at(0), si_orig->operands.at(0));

            auto new_inst = si_insert->clone();
            new_inst->operands[0] = merged_pred;
            LOG3("Merging instruction " << si_orig << " and " << si_insert << " into " << new_inst);

            action->action.erase(it);
            action->action.insert(it, new_inst);
            insert = false;
            break;
        }
    }

    // Instruction cannot be merged a we can insert it here
    if (insert) action->action.push_back(si_insert);
}

bool CreateSaluInstruction::preorder(const IR::Declaration_Variable *v) {
    auto vt = v->type->to<IR::Type::Bits>();
    if (vt && vt->size == salu->source_width()) {
        locals.emplace(v->name, LocalVar(v->name, false));
    } else if (v->type == regtype) {
        locals.emplace(v->name, LocalVar(v->name, true));
    } else {
        error(ErrorType::ERR_UNSUPPORTED, "RegisterAction can't support local var %s", v);
    }
    return false;
}

std::map<std::pair<cstring, cstring>, std::vector<CreateSaluInstruction::param_t>>
    CreateSaluInstruction::function_param_types = {
        {{"DirectRegisterAction"_cs, "apply"_cs},
         {param_t::VALUE, param_t::OUTPUT, param_t::OUTPUT, param_t::OUTPUT, param_t::OUTPUT}},
        {{"RegisterAction"_cs, "apply"_cs},
         {param_t::VALUE, param_t::OUTPUT, param_t::OUTPUT, param_t::OUTPUT, param_t::OUTPUT}},
        {{"RegisterAction"_cs, "overflow"_cs},
         {param_t::VALUE, param_t::OUTPUT, param_t::OUTPUT, param_t::OUTPUT, param_t::OUTPUT}},
        {{"RegisterAction"_cs, "underflow"_cs},
         {param_t::VALUE, param_t::OUTPUT, param_t::OUTPUT, param_t::OUTPUT, param_t::OUTPUT}},
#ifdef HAVE_JBAY
        {{"LearnAction"_cs, "apply"_cs},
         {param_t::VALUE, param_t::HASH, param_t::LEARN, param_t::OUTPUT, param_t::OUTPUT,
          param_t::OUTPUT, param_t::OUTPUT}},
        {{"MinMaxAction"_cs, "apply"_cs},
         {param_t::VALUE, param_t::OUTPUT, param_t::OUTPUT, param_t::OUTPUT, param_t::OUTPUT}},
#endif
        {{"SelectorAction"_cs, "apply"_cs},
         {param_t::VALUE, param_t::OUTPUT, param_t::OUTPUT, param_t::OUTPUT, param_t::OUTPUT}}};

bool CreateSaluInstruction::preorder(const IR::Declaration_Instance *di) {
    BUG_CHECK(!reg_action, "%s: Nested extern", di->srcInfo);
    BUG_CHECK(di->properties.empty(), "direct from P4_14 salu blackbox no longer supported");
    reg_action = di;
    auto salu_regtype = regtype;
    auto type = di->type;
    if (auto st = type->to<IR::Type_Specialized>()) {
        type = st->baseType;
        // RegisterAction may use a register element type different from the Register
        // Compatibility is tough to ensure -- should we have warnings here?
        auto rt = st->arguments->at(0);
        if (!rt->is<IR::Type_Dontcare>()) regtype = rt;
    }
    action_type_name = type->toString();
    // don't visit constructor arguments...
    visit(di->initializer, "initializer");
    reg_action = nullptr;
    regtype = salu_regtype;
    return false;
}

bool CheckStatefulAlu::preorder(IR::MAU::StatefulAlu *salu) {
    large_constants.clear();

    auto &device = Device::statefulAluSpec();
    const IR::Type *regtype = nullptr;
    if (salu->selector) {
        // selector action
        BUG_CHECK(salu->width == 1 && salu->dual == false, "wrong size for selector action");
        regtype = IR::Type::Bits::get(1);
    } else {
        BUG_CHECK(salu->reg && salu->reg->type->to<IR::Type_Specialized>(), "invalid SALU");
        regtype = getType(salu->reg->type->to<IR::Type_Specialized>()->arguments->at(0));
    }
    auto bits = regtype->to<IR::Type::Bits>();
    if (auto str = regtype->to<IR::Type_Struct>()) {
        auto nfields = str->fields.size();
        bool sameTypes = false;
        if (nfields > 0) bits = getType(str->fields.at(0)->type)->to<IR::Type::Bits>();
        if (nfields > 1) {
            auto *secondBits = getType(str->fields.at(1)->type);
            if (bits && secondBits) sameTypes = bits->equiv(*secondBits);
        }
        if (nfields < 1 || !bits || nfields > 2 || (nfields > 1 && !sameTypes)) {
            bits = nullptr;
        }
        if (bits) {
            salu->dual = nfields > 1;
            if (bits->size == 1) bits = nullptr;
        }
    }
    if (bits) {
        if (bits->size == 1 && !salu->dual) {
            // ok
        } else if (bits->size < 8) {
            // too small
            bits = nullptr;
        } else if (bits->size > device.MaxSize) {
            if (bits->size == device.MaxDualSize && !salu->dual) {
                // we can support 1x1x  double the max width by using dual mode.
                salu->dual = true;
            } else {
                // too big
                bits = nullptr;
            }
        } else if (bits->size & (bits->size - 1)) {
            // not a power of two
            bits = nullptr;
        }
    }
    if (!bits) {
        std::stringstream detail;
        detail << "    Supported Register element types:\n       ";
        for (int sz = 8; sz < device.MaxDualSize; sz += sz)
            detail << " bit<" << sz << "> int<" << sz << ">";
        detail << "\n        structs containing one or two fields of one of the above types";
        detail << "\n        bit<1> bit<" << device.MaxDualSize << ">\n";
        error(ErrorType::ERR_UNSUPPORTED, "Unsupported Register element type %s for %s\n%s",
              regtype, salu->reg, detail.str());
        return false;
    }

    // For a 1bit SALU, the driver expects a set and clr instr. Check if these
    // instr are already present, if not add them. Test - p4factory stful.p4 -
    // TestOneBit
    if (bits->size == 1) {
        ordered_set<cstring> set_clr{"set_bit"_cs, "clr_bit"_cs};
        if (salu->selector) {
            set_clr = ordered_set<cstring>{"set_bit_at"_cs, "clr_bit_at"_cs};
            if (salu->selector->mode == IR::MAU::SelectorMode::RESILIENT) {
                set_clr.insert("set_bit"_cs);
                set_clr.insert("clr_bit"_cs);
            }
        }
        for (auto &salu_action : salu->instruction) {
            auto &salu_action_instr = salu_action.second;
            if (salu_action_instr) {
                for (auto &salu_instr : salu_action_instr->action) {
                    LOG4("SALU " << salu->name << " already has " << salu_instr->name
                                 << " instruction");
                    std::string name = std::string(salu_instr->name);
                    /* make sure that if the "bitc" variant instruction exists then
                     * we do not add the non-c variant. */
                    auto pos = name.find("bitc"_cs);
                    if (pos != std::string::npos) name.erase(pos + 3, 1);
                    set_clr.erase(name);
                }
            }
        }
        for (auto sc : set_clr) {
            if (sc == "") continue;
            LOG4("SALU " << salu->name << " adding " << sc << " instruction");
            auto instr_action = new IR::MAU::SaluAction(IR::ID(sc + "_alu$0"));
            salu->instruction.addUnique(sc + "_alu$0", instr_action);
            auto set_clr_instr = new IR::MAU::SaluInstruction(sc);
            instr_action->action.push_back(set_clr_instr);
        }
    }

    const IR::MAU::SaluAction *first = nullptr;
    ;
    for (auto salu_action : Values(salu->instruction)) {
        auto chain = salu_action->annotations->getSingle("chain_address"_cs);
        if (first) {
            if (salu->chain_vpn != (chain != nullptr))
                error(ErrorType::ERR_UNSUPPORTED, "Inconsistent chaining for %s and %s", first,
                      salu_action);
        } else {
            salu->chain_vpn = chain != nullptr;
            first = salu_action;
        }
        // Validate that each operand are not larger than the stateful ALU can support based on
        // the register size selected.
        for (const IR::MAU::Primitive *act_prim : salu_action->action) {
            // Min and Max instruction can operate on 128 bit input even if the stateful ALU
            // operate in 8 or 16 bit mode. Just ignore this corner case for now in this
            // preemptive error reporting.
            if (act_prim->name.startsWith("min") || act_prim->name.startsWith("max")) break;
            for (const IR::Expression *op : act_prim->operands) {
                int salu_size = bits->size;
                if (salu_size < 8) salu_size = 8;
                if (safe_width_bits(op->type) > salu_size)
                    error(ErrorType::ERR_UNSUPPORTED,
                          "%sBecause you declared the register %s to store the type %s, the SALU "
                          "for the associated RegisterAction()s is configured in %d-bit mode. As a "
                          "result, it can only access header/metadata fields of the type bit<%d>",
                          op->srcInfo, salu->reg, regtype, salu_size, salu_size);
            }
        }
    }

    lmatch_usage.clear();
    lmatch_usage.salu = salu;
    salu->apply(lmatch_usage);
    return true;
}

void CheckStatefulAlu::postorder(IR::MAU::SaluInstruction *si) {
    if (Device::currentDevice() == Device::TOFINO &&
        (si->name.endsWith(".u") || si->name.endsWith(".uus"))) {
        // For the unsigned compare, the inputs from memory and phv are treated as unsigned
        // and are zero-extended to 34b, but the constant input is always treated as a signed
        // number and is sign-extended. On Tofino 1, the constant regfile is only 32 bits wide.
        for (const auto *op : si->operands) {
            const auto *constant = op->to<IR::Constant>();
            if (constant && (constant->value > INT_MAX || constant->value < INT_MIN))
                error(ErrorType::ERR_UNSUPPORTED,
                      "%sconstant value %s is out of range [%d; %d] for stateful ALU", op->srcInfo,
                      constant->value.str(), INT_MIN, INT_MAX);
        }
    }
}

bool CheckStatefulAlu::preorder(IR::MAU::Primitive *prim) {
    if (!findContext<IR::MAU::SaluAction>()) {
        return true;
    }

    const auto &device = Device::statefulAluSpec();
    for (const auto *op : prim->operands) {
        if (const auto *constant = op->to<IR::Constant>();
            constant && (constant->value < device.MinInstructionConstValue ||
                         constant->value > device.MaxInstructionConstValue)) {
            large_constants.insert(constant->value);
        }
    }
    return true;
}

void CheckStatefulAlu::postorder(IR::MAU::StatefulAlu *salu) {
    const auto &device = Device::statefulAluSpec();
    const std::size_t constants = large_constants.size();
    const std::size_t params = salu->regfile.size();

    const std::size_t needed_file_rows = constants + params;
    if (needed_file_rows > static_cast<std::size_t>(device.MaxRegfileRows)) {
        std::stringstream msg;
        msg << "Register actions associated with %1%: [ ";
        for (const auto *salu_action : Values(salu->instruction)) {
            msg << salu_action->name.name << " ";
        }
        msg << "] do not fit on the device. Actions use ";
        if (constants > 0) {
            msg << constants << " large constants ";
            if (params > 0) {
                msg << "and ";
            }
        }
        if (params > 0) {
            msg << params << " register parameters ";
            if (constants > 0) {
                msg << "for a total of " << needed_file_rows << " register action parameter slots ";
            }
        }
        msg << "but the device has only " << device.MaxRegfileRows
            << " register action parameter slots. To make the actions fit, reduce the number of ";
        if (constants > 0) {
            msg << "large constants";
            if (params > 0) {
                msg << " or ";
            } else {
                msg << ".";
            }
        }
        if (params > 0) {
            msg << "register parameters.";
        }
        error(ErrorType::ERR_OVERLIMIT, msg.str().c_str(), salu);
    }
}

void CheckStatefulAlu::AddressLmatchUsage::clear() {
    lmatch_operand = nullptr;
    inuse_mask = lmatch_inuse_mask = 0;
}

unsigned CheckStatefulAlu::AddressLmatchUsage::regmasks[] = {0xaaaa, 0xcccc, 0xf0f0, 0xff00};
unsigned CheckStatefulAlu::AddressLmatchUsage::eval_cmp(const IR::Expression *e) {
    if (auto *cmp = e->to<IR::MAU::SaluCmpReg>()) {
        return regmasks[cmp->index];
    } else if (auto *And = e->to<IR::LAnd>()) {
        return eval_cmp(And->left) & eval_cmp(And->right);
    } else if (auto *Or = e->to<IR::LOr>()) {
        return eval_cmp(Or->left) | eval_cmp(Or->right);
    } else if (auto *Not = e->to<IR::LNot>()) {
        return ~eval_cmp(Not->expr);
    } else {
        BUG("Invalid predicate");
    }
}

bool CheckStatefulAlu::AddressLmatchUsage::preorder(const IR::MAU::SaluAction *) {
    inuse_mask = 0;
    return true;
}
bool CheckStatefulAlu::AddressLmatchUsage::preorder(const IR::MAU::SaluCmpReg *cmp) {
    inuse_mask |= regmasks[cmp->index];
    return true;
}

/* Check to see if its safe to replace the lmatch expression 'b' with 'a', given the set
 * of registers in use for the action 'b' -- that is will 'a' evaluate to the same value
 * as 'b' if every register NOT in reguse is false. */
bool CheckStatefulAlu::AddressLmatchUsage::safe_merge(const IR::Expression *a,
                                                      const IR::Expression *b, unsigned inuse) {
    return (eval_cmp(a) & inuse) == (eval_cmp(b) & inuse);
}

bool CheckStatefulAlu::AddressLmatchUsage::preorder(const IR::MAU::SaluFunction *fn) {
    if (fn->name != "lmatch") return false;
    if (lmatch_operand) {
        if (safe_merge(lmatch_operand, fn->expr, inuse_mask)) {
            return false;
        } else if (safe_merge(fn->expr, lmatch_operand, lmatch_inuse_mask)) {
            lmatch_operand = fn->expr;
            lmatch_inuse_mask = inuse_mask;
        } else {
            error(ErrorType::ERR_UNSUPPORTED, "Conflicting address calls in RegisterActions on %s",
                  salu->reg);
        }
    } else {
        lmatch_operand = fn->expr;
        lmatch_inuse_mask = inuse_mask;
    }
    return false;
}
bool CheckStatefulAlu::preorder(IR::MAU::SaluFunction *fn) {
    if (fn->name != "lmatch") return false;
    fn->expr = lmatch_usage.lmatch_operand;
    return false;
}

bool IR::MAU::SaluAction::ReturnEnumEncoding::operator<=(
    const IR::MAU::SaluAction::ReturnEnumEncoding &a) const {
    if (return_type != a.return_type) return false;
    for (auto &tag : tag_used) {
        if (tag.second == 0) continue;
        if (!a.tag_used.count(tag.first)) return false;
        if ((tag.second & ~a.tag_used.at(tag.first)) != 0) return false;
    }
    return (cmp_used & ~a.cmp_used) == 0;
}

const IR::MAU::SaluAction::ReturnEnumEncoding *IR::MAU::SaluAction::ReturnEnumEncoding::merge(
    const IR::MAU::SaluAction::ReturnEnumEncoding *a) const {
    if (*a <= *this) return this;
    if (*this <= *a) return a;
    auto *rv = new ReturnEnumEncoding(*this);
    for (auto &el : a->tag_used) rv->tag_used[el.first] |= el.second;
    unsigned inuse = 0;
    for (auto &el : tag_used) {
        if ((inuse & el.second) != 0) return nullptr;  // can't merge
        inuse |= el.second;
    }
    rv->cmp_used |= a->cmp_used;
    return rv;
}

bool FixupStatefulAlu::FindEncodings::preorder(const IR::MAU::SaluAction *act) {
    if (!act->return_encoding) return false;
    auto &info = self.encodings[act->return_encoding->return_type];
    info.actions.insert(act);
    if (!info.enum_name) {
        info.enum_name = act->return_encoding->return_type->name;
        info.encoding = act->return_encoding;
    } else if (auto *m = info.encoding->merge(act->return_encoding)) {
        info.encoding = m;
    } else {
        error(ErrorType::ERR_UNSUPPORTED,
              "can't merge return type enum encoding for %1% with %2%,"
              " try giving them different types",
              act, *info.actions.begin());
    }
    return false;
}

const IR::MAU::SaluAction *FixupStatefulAlu::UpdateEncodings::preorder(IR::MAU::SaluAction *act) {
    if (act->return_encoding)
        act->return_encoding = self.encodings.at(act->return_encoding->return_type).encoding;
    prune();
    return act;
}

const IR::Operation::Relation *FixupStatefulAlu::UpdateEncodings::preorder(
    IR::Operation::Relation *rel) {
    Pattern::Match<IR::Expression> e;
    Pattern::Match<IR::Member> k;
    bool iseq = true;
    if (!(e == k).match(rel) && (iseq = false, !(e != k).match(rel))) return rel;
    auto enum_type = k->type->to<IR::Type_Enum>();
    if (!enum_type) return rel;
    // At this point we know we have a test of something against an enum tag that has been
    // encoded as a SALU predicate output, so need to change it into the appropriate mask
    // test
    BUG_CHECK(self.encodings.count(enum_type), "%1% can't find encoding for %2%", rel, enum_type);
    auto encoding = self.encodings.at(enum_type).encoding;
    unsigned mask = encoding->tag_used.at(k->member.name);
    if (iseq)
        mask ^= (1U << self.pred_type_size) - 1;
    else
        rel = new IR::Equ(rel->srcInfo, rel->type, rel->left, rel->right);
    rel->left = new IR::BAnd(self.pred_type, e, new IR::Constant(self.pred_type, mask));
    rel->right = new IR::Constant(self.pred_type, 0);
    prune();
    return rel;
}

const IR::Expression *FixupStatefulAlu::UpdateEncodings::preorder(IR::Member *exp) {
    auto enum_type = exp->type->to<IR::Type_Enum>();
    if (!enum_type || !self.encodings.count(enum_type)) return exp;
    if (!exp->expr->is<IR::TypeNameExpression>()) return exp;
    auto encoding = self.encodings.at(enum_type).encoding;
    const IR::MAU::Primitive *prim;
    if (getParent<IR::AssignmentStatement>() || getParent<IR::BFN::SavedRVal>()) {
        // ok
    } else if ((prim = getParent<IR::MAU::Primitive>()) && prim->name == "modify_field") {
        // ok
    } else {
        error(ErrorType::ERR_UNSUPPORTED, "%1%: unexpected enum reference", exp);
        return exp;
    }
    unsigned val = encoding->tag_used.at(exp->member.name);
    val ^= val & (val - 1);  // clear all but lowest set bit
    return new IR::Constant(self.pred_type, val);
}

const IR::BFN::ParserRVal *FixupStatefulAlu::UpdateEncodings::postorder(IR::BFN::SavedRVal *e) {
    if (auto k = e->source->to<IR::Constant>()) return new IR::BFN::ConstantRVal(e->srcInfo, k);
    return e;
}

const IR::Expression *FixupStatefulAlu::UpdateEncodings::preorder(IR::Expression *exp) {
    if (exp->type->is<IR::Type_Enum>()) exp->type = self.pred_type;
    return exp;
}
