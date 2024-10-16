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

#include "action_analysis.h"
#include "lib/log.h"
#include "resource.h"
#include "bf-p4c/common/slice.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/phv/phv_fields.h"
#include "lib/bitrange.h"
#include "lib/error.h"

using namespace P4;

namespace {

constexpr unsigned makeMask(unsigned size) {
    return (0xffffffffU) >> (sizeof(unsigned) * 8 - size);
}

bool setBitRangeFits(unsigned value, int numBits, int container_size) {
    BUG_CHECK(sizeof(unsigned) * 8 >= (unsigned)container_size,
                            "Variable can't hold container bits");
    // Pre-rotate set bits away from the wrap-ends.
    // N.B. This only works for encodable values, other values remain uncodable.
    constexpr int rotate = 4;
    BUG_CHECK(container_size/2 >= rotate && rotate >= numBits,
                            "We can't rotate set-bits away from wrap-ends");
    constexpr int bitsToCheck = 2;
    BUG_CHECK(bitsToCheck >= numBits-1, "Need to check more wrap-end bits");
    unsigned topBit = 1U << (container_size - 1);
    if ((value & topBit) || (value & 1)) {  // <---bitsToCheck
        value = ((value >> rotate) | (value << (container_size - rotate)))
                    & makeMask(container_size);
    }
    int range = bitvec(value).max().index() - bitvec(value).min().index() + 1;
    return range <= numBits;
}

enum class EncodeConstant {NotPossible, WithSignExtend, WithZeroExtend};
EncodeConstant canRotateConstant(unsigned value, int max_bits, int min_bits,
                        int container_size, int constant_size) {
    // Check if we can rotate a constant viz only m**_bits contiguous bits of variation.
    // First decide if zero/sign extension of the remaining bits may work.
    int setBits = bitvec(value).popcount();
    if (setBits <= max_bits) {  // Zero extend & check the set bit pattern.
        if (setBitRangeFits(value, max_bits, container_size))
            return EncodeConstant::WithZeroExtend;
    }
    int clearBits = constant_size - setBits;
    if (clearBits <= min_bits) {  // Sign extend & check the clear bit pattern.
        // invert bits to reuse functionality.
        unsigned inverted = ~value & makeMask(constant_size);
        if (setBitRangeFits(inverted, min_bits, container_size))
            return EncodeConstant::WithSignExtend;
    }
    return EncodeConstant::NotPossible;
}

/** A verification of the constant used in a ContainerAction to make sure that it can
 *  be used without being converted to action data. \p constant_size = -1 means
 *  the constant is non-contiguous.
 */
EncodeConstant valid_instruction_constant(unsigned value, int max_bits, int min_bits,
                                            int container_size, int constant_size = -1) {
    unsigned max_value = (1U << max_bits);
    unsigned min_value = (1U << min_bits);
    unsigned complement = makeMask((constant_size == -1)? container_size : constant_size);
    if (value < max_value)
        return EncodeConstant::WithZeroExtend;
    if (value > complement - min_value)
        return EncodeConstant::WithSignExtend;
    if (constant_size != -1) {  // Possible deposit-field instruction.
        return canRotateConstant(value, max_bits, min_bits, container_size, constant_size);
    }
    return EncodeConstant::NotPossible;
}
}   // namespace

namespace P4::Test {
// gtest accesses via the global Test::valid_instruction_constant()
// It would be better to refactor all the code into a public sub-API module.
bool valid_instruction_constant(unsigned value, int max_bits, int min_bits,
                                    int container_size, int constant_size) {
    auto valid = ::valid_instruction_constant(value, max_bits, min_bits,
                                    container_size, constant_size);
    return valid != EncodeConstant::NotPossible;
}
}

std::set<unsigned> ActionAnalysis::FieldAction::codesForErrorCases =
    { READ_AFTER_WRITES,
      REPEATED_WRITES,
      MULTIPLE_ACTION_DATA,
      BAD_CONDITIONAL_SET };

std::set<unsigned> ActionAnalysis::ContainerAction::codesForErrorCases =
    { MULTIPLE_CONTAINER_ACTIONS,
      READ_PHV_MISMATCH,
      ACTION_DATA_MISMATCH,
      CONSTANT_MISMATCH,
      TOO_MANY_PHV_SOURCES,
      IMPOSSIBLE_ALIGNMENT,
      ILLEGAL_OVERWRITE,
      MAU_GROUP_MISMATCH,
      PHV_AND_ACTION_DATA,
      MULTIPLE_SHIFTS,
      ATTACHED_OUTPUT_ILLEGAL_ALIGNMENT,
      BIT_COLLISION_SET,
      ILLEGAL_MOCHA_OR_DARK_WRITE };

/** Calculates a total container constant, given which constants wrote to which fields in the
 *  operation
 */


std::string ActionAnalysis::ActionParam::to_string() const {
    std::stringstream str;
    str << *this;
    return str.str();
}

const IR::Expression *ActionAnalysis::ActionParam::unsliced_expr() const {
    if (expr == nullptr)
        return expr;
    if (auto *sl = expr->to<IR::Slice>())
        return sl->e0;
    return expr;
}


std::string ActionAnalysis::FieldAction::to_string() const {
    std::stringstream str;
    str << *this;
    return str.str();
}

auto ActionAnalysis::FieldAction::container_write_type() const
    -> std::pair<container_overwrite_t, source_type_t> {
    auto source = source_type_t::OTHER;
    auto is_expr_const = [&](const IR::Expression *expr) {
        if (expr->is<IR::MAU::ActionDataConstant>()) {
            source = source_type_t::ACTION_DATA_CONSTANT;
            return true;
        }
        if (expr->is<IR::Constant>()) {
            source = source_type_t::CONSTANT;
            return true;
        }
        return false;
    };

    if (name == "add" || is_bitwise_overwritable()) {
        auto src1 = reads.at(0).expr;
        auto src2 = reads.at(1).expr;

        if (src2 == write.expr) {
            std::swap(src1, src2);
        }

        if (src1->equiv(*write.expr) && is_expr_const(src2)) {
            // Instruction in form `x = x + const` overwrites only field slices that are packed
            // with destination into more significant bits of a container.
            // Bitwise instructions in form `x = x op const` do not overwrite any other field slices
            // packed in the same container as destination.
            if (name == "add") return std::make_pair(container_overwrite_t::HIGHER_BITS, source);
            return std::make_pair(container_overwrite_t::DST_ONLY, source);
        }
    }

    return std::make_pair(container_overwrite_t::ALL_BITS, source);
}

std::string ActionAnalysis::ContainerAction::to_string() const {
    std::stringstream str;
    str << *this;
    return str.str();
}

unsigned ActionAnalysis::ConstantInfo::build_constant() {
    unsigned rv = 0;
    BUG_CHECK(!positions.empty(), "Building a constant over a container when no constants are "
              "contained within this container action");
    BUG_CHECK(container_size > 0, "ConstantInfo::container_size not set up");
    for (auto position : positions) {
        unsigned mask;
        if (position.range.size() == sizeof(unsigned) * 8)
            mask = static_cast<unsigned>(-1);
        else
            mask = (1U << position.range.size()) - 1;
        unsigned pre_shift = position.value & mask;
        rv |= (pre_shift << position.range.lo);
    }
    unsigned alt = rv | alignment.unused_container_bits.getrange(0, container_size);
    if (alt > rv && (1ULL << container_size) - alt < rv) {
        // using a negative value with extra set bits (for unused bits) will fit better
        // as a "small" constant in an instruction, so use that instead.
        rv = alt;
    }
    return rv;
}

/** Shifts a constant to the lower read bits point to determine if the constant can be used
 *  within a deposit-field
 */
unsigned ActionAnalysis::ConstantInfo::build_shiftable_constant() {
    unsigned rv = build_constant();
    bitvec rv_bv(rv);

    if (rv_bv.empty())
        return rv;

    return alignment.bitrange_contiguous()
        ? rv_bv.getrange(alignment.write_bits().min().index(), alignment.bitrange_contiguous_size())
        : rv;
}

/** Because the assembly only recognizes constants between -8..7 or their corresponding
 *  container size complement, this function converts all container constants (post-shifted)
 *  to a pre-shift value between -8 and 7.
 *  If the value is used in a deposit-field instruction, the value is the post-rotation result,
 *  implicitly encoding the required rotation (no longer between -8 and 7).
 *  The assembler will need to undo this rotation, so it can recreate the pre-rotational
 *  value of between -8 and 7, along with its rotation parameter.
 */
unsigned ActionAnalysis::ConstantInfo::valid_instruction_constant(int container_size) const {
    if (signExtend) {
        unsigned container_mask = makeMask(container_size);
        int constant_size = alignment.bitrange_cover_size();
        unsigned constant_mask = makeMask(constant_size);
        return container_mask & (~constant_mask | constant_value);
    }
    return constant_value;
}

void ActionAnalysis::initialize_phv_field(const IR::Expression *expr) {
    if (!phv.field(expr))
        return;

    if (isWrite()) {
        if (field_action.write_found) {
            BUG("Multiple write in a single instruction?");
        }
        ActionParam write(ActionParam::PHV, expr);
        field_action.setWrite(write);
    } else {
        field_action.reads.emplace_back(ActionParam::PHV, expr);
    }
}

void ActionAnalysis::initialize_action_data(const IR::Expression *expr) {
    Log::TempIndent indent;
    LOG5("Initialize action data for expr : " << expr << indent);
    field_action.reads.emplace_back(ActionParam::ACTIONDATA, expr);
    if (auto *ao = field_action.reads.back().unsliced_expr()->to<IR::MAU::AttachedOutput>()) {
        LOG5("Action Data is Attached Output");
        field_action.reads.back().speciality = classify_attached_output(ao); }
    if (field_action.reads.back().unsliced_expr()->is<IR::MAU::HashDist>()) {
        LOG5("Action Data is HASH_DIST");
        field_action.reads.back().speciality = ActionParam::HASH_DIST; }
    if (field_action.reads.back().unsliced_expr()->is<IR::MAU::StatefulCounter>()) {
        LOG5("Action Data is STFUL_COUNTER");
        field_action.reads.back().speciality = ActionParam::STFUL_COUNTER; }
    if (field_action.reads.back().unsliced_expr()->is<IR::MAU::RandomNumber>()) {
        LOG5("Action Data is RANDOM");
        field_action.reads.back().speciality = ActionParam::RANDOM; }
    if (field_action.reads.back().unsliced_expr()->is<IR::MAU::ConditionalArg>()) {
        LOG5("Action Data is Conditional");
        field_action.reads.back().is_conditional = true;
    }
}

ActionAnalysis::ActionParam::speciality_t
ActionAnalysis::classify_attached_output(const IR::MAU::AttachedOutput *ao) {
    auto *at = ao->attached;
    if (auto mtr = at->to<IR::MAU::Meter>()) {
        if (mtr->alu_output())
            return ActionParam::METER_ALU;
        else if (mtr->color_output())
            return ActionParam::METER_COLOR;
        BUG("%s: Unrecognized implementation %s on meter %s", mtr->srcInfo,
            mtr->implementation, mtr->name);
        return ActionParam::NO_SPECIAL;
    } else if (at->is<IR::MAU::StatefulAlu>()) {
        return ActionParam::METER_ALU;
    }
    BUG("%s: Unrecognizable Attached Output %s being used in an ALU operation",
        at->srcInfo, at->name);
}

/** Similar to phv.field, it returns the IR structure that corresponds to actiondata,
 *  If it is an MAU::ActionArg, HashDist, or AttachedOutput then the type is ACTIONDATA
 *  If it is an ActionDataConstant, then the type is CONSTANT
 */
const IR::Expression *ActionAnalysis::isActionParam(const IR::Expression *e,
        le_bitrange *bits_out, ActionParam::type_t *type) {
    le_bitrange bits = { 0, e->type->width_bits() - 1};
    if (auto *sl = e->to<IR::Slice>()) {
        bits.lo = sl->getL();
        bits.hi = sl->getH();
        e = sl->e0;;
        if (e->is<IR::MAU::ActionDataConstant>())
            BUG("No ActionDataConstant should be a member of a Slice");
    }
    if (e->is<IR::MAU::ActionArg>() || e->is<IR::MAU::ActionDataConstant>()
        || e->is<IR::MAU::AttachedOutput>() || e->is<IR::MAU::HashDist>()
        || e->is<IR::MAU::IXBarExpression>() || e->is<IR::MAU::RandomNumber>()
        || e->is<IR::MAU::StatefulCounter>()) {
        if (bits_out)
            *bits_out = bits;
        if ((e->is<IR::MAU::ActionArg>() || e->is<IR::MAU::AttachedOutput>()
             || e->is<IR::MAU::HashDist>() || e->is<IR::MAU::StatefulCounter>()
             || e->is<IR::MAU::IXBarExpression>() || e->is<IR::MAU::RandomNumber>()) && type)
            *type = ActionParam::ACTIONDATA;
        else if (e->is<IR::MAU::ActionDataConstant>() && type)
            *type = ActionParam::CONSTANT;
        return e;
    }
    return nullptr;
}

/**
 * sizeInBytes() and sizeInBits() are converted to constant after flexible
 * packing. The converted constant is constant-folded and strength-reduced (to
 * eliminate subtract operation). Action analysis needs to be aware of the
 * special treatment on 'sizeInBytes' and 'sizeInBits' function and not report
 * an error when a slice is applied to the output of the functions.
 */
const IR::Expression *ActionAnalysis::isStrengthReducible(const IR::Expression *e) {
    if (auto *sl = e->to<IR::Slice>())
        e = sl->e0;
    if (e->is<IR::MAU::TypedPrimitive>()) {
        if (e->to<IR::MAU::TypedPrimitive>()->name == "sizeInBytes" ||
            e->to<IR::MAU::TypedPrimitive>()->name == "sizeInBits") {
            return e; } }
    return nullptr;
}

const IR::MAU::ActionArg *ActionAnalysis::isActionArg(const IR::Expression *e,
    le_bitrange *bits_out) {
    le_bitrange bits = { 0, e->type->width_bits() - 1 };
    if (auto *sl = e->to<IR::Slice>()) {
        bits.lo = sl->getL();
        bits.hi = sl->getH();
        e = sl->e0;
    }

    if (auto aa = e->to<IR::MAU::ActionArg>()) {
        if (bits_out)
           *bits_out = bits;
        return aa;
    }
    return nullptr;
}


bool ActionAnalysis::preorder(const IR::MAU::Instruction *instr) {
    LOG4("ActionAnalysis preorder on instruction : " << *instr);
    field_action.clear();
    field_action.name = instr->name;
    field_action.instruction = instr;
    return true;
}

bool ActionAnalysis::preorder(const IR::MAU::ActionArg *arg) {
    LOG4("ActionAnalysis preorder on ActionArg : " << *arg);
    if (!findContext<IR::MAU::Instruction>())
        return false;

    initialize_action_data(arg);
    return false;
}

bool ActionAnalysis::preorder(const IR::MAU::ConditionalArg *ca) {
    LOG4("ActionAnalysis preorder on ConditionalArg : " << *ca);
    initialize_action_data(ca);
    return false;
}

// continue analyzing the node underneath the cast.
// An example is when a 1-bit action argument is casted to bool.
// action (bit<1> arg) { bool m = (bool) arg; }
bool ActionAnalysis::preorder(const IR::BFN::ReinterpretCast*) {
    return true;
}

bool ActionAnalysis::preorder(const IR::Constant *constant) {
    LOG4("ActionAnalysis preorder on const : " << *constant);
    field_action.reads.emplace_back(ActionParam::CONSTANT, constant);
    return false;
}

bool ActionAnalysis::preorder(const IR::MAU::ActionDataConstant *adc) {
    LOG4("ActionAnalysis preorder on ADConst : " << *adc);
    field_action.reads.emplace_back(ActionParam::ACTIONDATA, adc);
    return false;
}

bool ActionAnalysis::preorder(const IR::MAU::HashDist *hd) {
    LOG4("ActionAnalysis preorder on HashDist : " << *hd);
    field_action.reads.emplace_back(ActionParam::ACTIONDATA, hd, ActionParam::HASH_DIST);
    return false;
}

bool ActionAnalysis::preorder(const IR::MAU::IXBarExpression *) {
    BUG("bare IXBarExpression in action");
    return false;
}

bool ActionAnalysis::preorder(const IR::MAU::AttachedOutput *ao) {
    LOG4("ActionAnalysis preorder on AttachedOutput: " << *ao);
    auto speciality = classify_attached_output(ao);
    field_action.reads.emplace_back(ActionParam::ACTIONDATA, ao, speciality);
    return false;
}

bool ActionAnalysis::preorder(const IR::MAU::RandomNumber *rn) {
    LOG4("ActionAnalysis preorder on RandomNumber: " << *rn);
    field_action.reads.emplace_back(ActionParam::ACTIONDATA, rn, ActionParam::RANDOM);
    return false;
}

bool ActionAnalysis::preorder(const IR::MAU::StatefulAlu *) {
    return false;
}

bool ActionAnalysis::preorder(const IR::MAU::StatefulCall *) {
    return false;
}

bool ActionAnalysis::preorder(const IR::MAU::StatefulCounter *sc) {
    initialize_action_data(sc);
    return false;
}

bool ActionAnalysis::preorder(const IR::Slice *sl) {
    LOG4("ActionAnalysis preorder on Slice : " << *sl);
    if (phv.field(sl)) {
        initialize_phv_field(sl);
    } else if (isActionParam(sl)) {
        initialize_action_data(sl);
    } else if (isStrengthReducible(sl)) {
        // ignore, will be strength reduced
    } else {
        LOG1("ERROR: Slice is of IR structure not handled by ActionAnalysis: " << sl);
    }
    // Constants should not be in slices ever, they should just be either refactored into
    // action data, or already separately split constants with different values
    return false;
}

bool ActionAnalysis::preorder(const IR::Cast *) {
    BUG("No casts should ever reach this point in the Tofino backend");
}

bool ActionAnalysis::preorder(const IR::MAU::Primitive *prim) {
    LOG4("ActionAnalysis preorder on Primitive : " << *prim);
    BUG("%s: Primitive %s was not correctly converted in Instruction Selection", prim->srcInfo,
        prim);
    return false;
}

bool ActionAnalysis::preorder(const IR::Expression *expr) {
    LOG4("ActionAnalysis preorder on Expression : " << *expr);
    if (phv.field(expr)) {
        initialize_phv_field(expr);
    } else {
        BUG("IR structure not yet handled by the ActionAnalysis pass: %s", expr);
    }
    return false;
}

bool ActionAnalysis::preorder(const IR::Mux *mux) {
    error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
            "%1%\nConditions in an action must be simple comparisons of an action data "
            "parameter\nTry moving the test out of the action or making it part of the table key",
            mux->srcInfo);
    return false;
}

bool ActionAnalysis::preorder(const IR::Member *mem) {
    LOG4("ActionAnalysis postorder on member: " << mem);
    if (mem->expr->is<IR::MAU::AttachedOutput>()) return true;
    return preorder(static_cast<const IR::Expression *>(mem));
}

/** Responsible for adding the instruction into which containers they actually affect.
 *  Thus multiple field based actions can be added to the same container, and then evalauted
 *  later.
 */
void ActionAnalysis::postorder(const IR::MAU::Instruction *instr) {
    Log::TempIndent indent;
    LOG4("ActionAnalysis postorder on instruction : " << instr << indent);
    if (!field_action.write_found) {
        LOG1("ERROR: Nothing written in the instruction " << instr);
    }

    if (phv_alloc) {
        le_bitrange bits;
        const auto *field = phv.field(field_action.write.expr, &bits);
        PHV::FieldUse use(PHV::FieldUse::WRITE);
        int split_count = 0;
        field->foreach_alloc(bits, tbl, &use, [&](const PHV::AllocSlice& ) {
            split_count++;
        });

        const bool split = (split_count != 1);
        field->foreach_alloc(bits, tbl, &use,
                [&](const PHV::AllocSlice &alloc) {
            auto container = alloc.container();
            if (container_actions_map->find(container) == container_actions_map->end()) {
                ContainerAction cont_action(instr->name, tbl);
                container_actions_map->emplace(container, cont_action);
                LOG5("Adding container " << container << " for table " << tbl->name <<
                     " to container_action_map");
            }
            if (!split) {
                (*container_actions_map)[container].field_actions.push_back(field_action);
                LOG5("Adding field action " << field_action << " to container " << container);
            } else {
                FieldAction field_action_split;
                field_action_split.name = field_action.name;
                field_action_split.instruction = field_action.instruction;
                field_action_split.requires_split = true;
                auto *write_slice = MakeSliceDestination(field_action.write.expr,
                        alloc.field_slice().lo, alloc.field_slice().hi);
                ActionParam write_split(field_action.write.type, write_slice,
                                        field_action.write.speciality);
                field_action_split.setWrite(write_split);
                for (auto &read : field_action.reads) {
                    auto read_slice = MakeSliceSource(read.expr, alloc.field_slice().lo,
                            alloc.field_slice().hi, field_action.write.expr);
                    field_action_split.reads.emplace_back(read.type, read_slice, read.speciality);
                    field_action_split.reads.back().is_conditional = read.is_conditional;
                }
                (*container_actions_map)[container].field_actions.push_back(field_action_split);
                LOG5("Adding field action split " << field_action_split
                                << " to container " << container);
            }
        });
    } else {
        (*field_actions_map)[instr] = field_action;
    }
}


void ActionAnalysis::verify_conditional_set_without_phv(cstring action_name, FieldAction &fa) {
    int op_idx = 0;

    BUG_CHECK(fa.reads.size() == 2 || fa.reads.size() == 3, "Conditional set instruction not "
        "calculated correctly");

    if (fa.reads.size() == 3) {
        auto param = fa.reads[op_idx];
        BUG_CHECK(param.type == ActionParam::PHV, "Conditional write instruction PHV "
                                                  "field not built correctly");
        op_idx++;
    }

    auto arg_param = fa.reads[op_idx];
    BUG_CHECK(arg_param.type != ActionParam::PHV, "Conditional write action data field not built "
        "correctly");
    if (arg_param.speciality != ActionParam::NO_SPECIAL) {
       warning("Conditional set instruction parameter %s in instruction %s in action %s "
                 "must be from an action argument or constant", arg_param.to_string(),
                 fa.to_string(), action_name);
       has_warning = true;
       fa.error_code |= FieldAction::BAD_CONDITIONAL_SET;
       LOG4("Error Code Update: BAD_CONDITIONAL_SET");
       return;
    }
    op_idx++;

    auto cond_param = fa.reads[op_idx];
    BUG_CHECK(cond_param.is_conditional, "Conditional set parameter is not created correctly");
}

/** PHV allocation is not known by the time of this verification.  This check just guarantees
 *  that the action is even at all possible within Tofino.  If not, the compiler should just
 *  fail at this point.  To ensure that an action is possible in Tofino:
 *    - The operation must be able to run in parallel (i.e. no repeated lvalues, a write read
 *      later in the operation)
 *    - The operands must be in the same size
 *    - Only one action data per instruction
 *  If this check is post BackendCopyPropagation, we have gone from sequential to parallel.
 */
void ActionAnalysis::verify_P4_action_without_phv(cstring action_name) {
    Log::TempIndent indent;
    LOG4("Verifying P4 Action without phv for : " << action_name << indent);
    ordered_map<const PHV::Field *, bitvec> written_fields;

    for (auto& field_action_info : *field_actions_map) {
        auto &field_action = field_action_info.second;

        if (sequential) {
            // Check reads before writes for this, as field can be used in it's own instruction
            for (auto read : field_action.reads) {
                if (read.type != ActionParam::PHV) continue;
                le_bitrange read_bitrange = {0, 0};
                auto field = phv.field(read.expr, &read_bitrange);
                bitvec read_bits(read_bitrange.lo, read_bitrange.size());
                BUG_CHECK(field, "Cannot convert an instruction read to a PHV field reference");
                if (written_fields.find(field) == written_fields.end()) continue;
                if (written_fields[field].intersects(read_bits)) {
                    if (error_verbose) {
                        warning("Action %s has a read of a field %s "
                                          "after it already has been written",
                                  action_name, cstring::to_cstring(read));
                    }
                    field_action.error_code |= FieldAction::READ_AFTER_WRITES;
                    LOG4("Error Code Update: READ_AFTER_WRITES");
                    has_warning = true;
                }
            }
        }

        le_bitrange bits = {0, 0};
        auto field = phv.field(field_action.write.expr, &bits);
        bitvec write_bits(bits.lo, bits.size());
        BUG_CHECK(field, "Cannot convert an instruction write to a PHV field reference");
        if (written_fields.find(field) != written_fields.end()) {
            if (written_fields[field].intersects(write_bits)) {
                if (error_verbose) {
                    warning("Action %s has repeated lvalue %s", action_name, field->name);
                }
                field_action.error_code |= FieldAction::REPEATED_WRITES;
                LOG4("Error Code Update: REPEATED_WRITES");
                has_warning = true;
            }
            written_fields[field] |= write_bits;
        } else {
            written_fields[field] = write_bits;
        }

        int non_phv_count = 0;

        if (field_action.name == "conditionally-set") {
            verify_conditional_set_without_phv(action_name, field_action);
        } else {
            for (auto read : field_action.reads) {
                if (read.type == ActionParam::ACTIONDATA || read.type == ActionParam::CONSTANT)
                    non_phv_count++;
                if (non_phv_count > 1) {
                    if (error_verbose) {
                        warning("In action %s, the following instruction has multiple "
                                  "action data parameters: %s",
                                  action_name, cstring::to_cstring(field_action));
                    }
                    field_action.error_code |= FieldAction::MULTIPLE_ACTION_DATA;
                    LOG4("Error Code Update: MULTIPLE_ACTION_DATA");
                    has_warning = true;
                }
            }
        }

        if (!field_action.is_shift()) {
            for (auto read : field_action.reads) {
                if (read.size() != field_action.write.size()) {
                    if (error_verbose) {
                        warning("In action %s, write %s and read %s sizes do not match up",
                                  action_name, cstring::to_cstring(field_action.write),
                                  cstring::to_cstring(read));
                    }
                    field_action.error_code |= FieldAction::DIFFERENT_OP_SIZE;
                    LOG4("Error Code Update: DIFFERENT_OP_SIZE");
                    has_warning = true;
                }
            }
        }
    }

    // For certain error codes, we now should throw an error.
    for (auto& field_action_info : *field_actions_map) {
        auto &field_action = field_action_info.second;
        for (auto refCode : FieldAction::codesForErrorCases) {
            if ((field_action.error_code & refCode) != 0) {
                LOG5("Error set due to matching on error cases");
                has_error = true;
                break;
            }
        }
    }
}

/** The purpose of this function is to calculate the alignment of the write bits for the destination
  * of an invalidate instruction. The invalidate instruction is special in that it does not have any
  * source operands, and the alignment of the write bits must thus be set separately for this
  * particular instruction. The function will return false if the total_write_bits member of
  * @p cont_action cannot be set.
  */
bool ActionAnalysis::initialize_invalidate_alignment(const ActionParam &write,
                                                     ContainerAction &cont_action) {
    BUG_CHECK(cont_action.name == "invalidate", "Expected invalidate instruction");
    le_bitrange range;
    auto *field = phv.field(write.expr, &range);
    BUG_CHECK(field, "Write in invalidate instruction has no PHV location");

    int count = 0;
    le_bitrange write_bits;
    PHV::FieldUse use(PHV::FieldUse::WRITE);
    field->foreach_alloc(range, cont_action.table_context, &use,
                         [&](const PHV::AllocSlice &alloc) {
        count++;
        BUG_CHECK(alloc.container_slice().lo >= 0, "Invalid negative container bit");
        write_bits = alloc.container_slice();
    });

    BUG_CHECK(count == 1, "ActionAnalysis did not split up container by container");

    return cont_action.set_invalidate_write_bits(write_bits);
}

/** The purpose of this function is calculate the location of the alignments of both PHV
 *  and action data for a single FieldAction in a ContainerAction.  Before action data
 *  allocation (done by the ActionFormat structure), we just align the write_bits and
 *  read_bits.  After action data allocation, we pull directly from the structures that hold
 *  this information for values.
 *
 *  This function will return false if the parameters are not the same size, or if the
 *  PHV allocation is done in a way where a single read field is actually found over two
 *  PHV containers, and thus cannot be aligned by itself, i.e. a single write requires two
 *  reads.  This may be too tight, but is a good initial warning check.
 */
bool ActionAnalysis::initialize_alignment(const ActionParam &write, const ActionParam &read,
                                          const op_type_t read_src, ContainerAction &cont_action,
                                          cstring &error_message, PHV::Container container,
                                          cstring action_name) {
    if (cont_action.is_shift() && read.type == ActionParam::CONSTANT)
        return true;

    error_message = "In the ALU operation over container " + container.toString() +
                    " in action " + action_name + ", ";
    if (write.expr->type->width_bits() != read.expr->type->width_bits()
        && !cont_action.is_shift()) {
        error_message += "the number of bits in the write and read aren't equal";
        cont_action.error_code |= ContainerAction::DIFFERENT_READ_SIZE;
        LOG4("Error Code Update: DIFFERENT_READ_SIZE");
        if (read.type == ActionParam::ACTIONDATA)
            cont_action.adi.specialities.setbit(read.speciality);
        return false;
    }

    le_bitrange range;
    auto *field = phv.field(write.expr, &range);
    BUG_CHECK(field, "Write in an instruction has no PHV location");

    std::optional<PHV::AllocSlice> write_slice;
    const PHV::FieldUse use(PHV::FieldUse::WRITE);
    field->foreach_alloc(range, cont_action.table_context, &use, [&](const PHV::AllocSlice &alloc) {
        BUG_CHECK(alloc.container_slice().lo >= 0, "Invalid negative container bit");
        BUG_CHECK(!write_slice,
                  "ActionAnalysis did not split up container by container. "
                  "Field: %1%, alloc1: %2%, alloc2: %3%, table: %4%.",
                  field->name, *write_slice, alloc, cont_action.table_context->externalName());
        write_slice = alloc;
    });
    BUG_CHECK(write_slice, "ActionAnalysis cannot find allocation of field %1% for table %2%",
              field->name, cont_action.table_context->externalName());

    le_bitrange write_bits = write_slice->container_slice();
    bool initialized;
    if (read.is_conditional)
        return true;
    if (read.type == ActionParam::PHV) {
        initialized = init_phv_alignment(read, read_src, cont_action,
                                        write_bits, container, error_message);
    } else if (ad_alloc) {
        if (read.type == ActionParam::ACTIONDATA)
            initialized = init_ad_alloc_alignment(read, cont_action, write_bits, action_name,
                                                  container);
        else
            initialized = init_constant_alignment(read, cont_action, write_bits, action_name,
                                                  container);
    } else {
        initialized = init_simple_alignment(read, cont_action, write_bits);
    }

    if (!initialized)
        cont_action.set_mismatch(read.type);

    return initialized;
}

/** This initializes the alignment of a particular PHV field.  It also guarantees that there
 *  is only one PHV read per PHV write.
 */
bool ActionAnalysis::init_phv_alignment(const ActionParam &read, const op_type_t read_src,
        ContainerAction &cont_action, le_bitrange write_bits, const PHV::Container container,
        cstring &error_message) {
    le_bitrange range;
    auto *field = phv.field(read.expr, &range);
    auto *tbl = cont_action.table_context;
    Log::TempIndent indent;
    LOG3("Init PHV Alignment for read expr:" << read.expr << ", field: " << field << indent);

    BUG_CHECK(field, "%1%: Operand %2% of instruction %3% operating on container %4% must be "
              "a PHV.", read.expr->srcInfo, read.expr, cont_action, container);

    PHV::FieldUse use(PHV::FieldUse::READ);
    std::set<PHV::Container> container_reads;
    field->foreach_alloc(range, tbl, &use, [&](const PHV::AllocSlice &alloc) {
        container_reads.insert(alloc.container());
        BUG_CHECK(alloc.container_slice().lo >= 0, "Invalid negative container bit");
    });

    if (container_reads.size() > MAX_PHV_SOURCES) {
        error_message += "an individual read phv is contained within more than 2 containers, and "
                         "is considered impossible";
        return false;
    }

    field->foreach_alloc(range, tbl, &use, [&](const PHV::AllocSlice &alloc) {
         le_bitrange read_bits = alloc.container_slice();
         int lo;
         int hi;
         if (!cont_action.is_shift()) {
             lo = write_bits.lo + (alloc.field_slice().lo - range.lo);
             hi = lo + read_bits.size() - 1;
         } else {
             lo = write_bits.lo;
             hi = write_bits.hi;
         }
         le_bitrange mini_write_bits(lo, hi);
         LOG4("Read Bits: " << read_bits << " Write Bits: " << write_bits);
         auto &init_phv_alignment = cont_action.initialization_phv_alignment;
         auto c = alloc.container();
         init_phv_alignment[c].emplace_back(mini_write_bits, read_bits, read_src);
         LOG4("Init PHV alignment for container " << c
                 << " - " << init_phv_alignment[alloc.container()]);
         if (container == c) {
            LOG5("Container is background source as it matches destination");
            cont_action.is_background_source = true;
         }
    });

    // Check if this is read in a form of "0 ++ <PHV_field>"
    if (auto concat = read.expr->to<IR::Concat>()) {
        auto constant = concat->left->to<IR::Constant>();
        if (constant != nullptr && (constant->value == 0)) {
            if (auto type = constant->type->to<IR::Type_Bits>()) {
                int resize_size = type->size;
                LOG4("Read expr is a Concat constant '" << type << "0 ++ PHV'");
                // Check if the resize bits are unallocated on top of the field
                // Note: there might be more alloc slices, just use and find the
                //       one corresponding to the MSBs of the field
                bool are_allocated = false;
                field->foreach_alloc(range, tbl, &use, [&](const PHV::AllocSlice &alloc) {
                    LOG5("Checking allocation on field alloc slice: " << alloc);
                    // We care about the alloc slice that represents the MSBs
                    if (alloc.field_slice().hi != field->size-1)
                        return;
                    int container_size = alloc.container().size();
                    int start_container_bit = alloc.container_slice().hi + 1;
                    // Check if there are enough bits left in the container
                    if (start_container_bit + resize_size > container_size) {
                        LOG5("Not enough bits left in container");
                        are_allocated = true;
                        return;
                    }
                    // Get all of the allocated bits for a slice
                    auto alloc_bits = phv.bits_allocated(alloc.container(), field, tbl, &use);
                    // Get the bits that should be unallocated
                    auto resize_bits = alloc_bits.getslice(start_container_bit, resize_size);
                    // Check if any of them is allocated
                    if (!resize_bits.empty()) {
                        LOG5("Not all bits are unallocated in container");
                        are_allocated = true;
                        return;
                    }
                });
                // If we found that there are not enough unallocated bits raise an error
                if (are_allocated) {
                    error_message += "resizing for the field " + field->name +
                                     " requires at least " + resize_size +
                                     " pad bits above the field"
                                     " (consider adding pa_solitary to the field)";
                    LOG3("PHV not completely initialized");
                    return false;
                }
                le_bitrange mini_write_bits((write_bits.hi - resize_size) + 1, write_bits.hi);
                cont_action.extra_resize_reads.add_alignment(mini_write_bits, mini_write_bits);
            }
        }
    }
    LOG3("PHV completely initialized");
    return true;
}

/**
 * For everything that is not yet handled by the new algorithm, (e.g. everything but hash), the
 * alignment for these particular fields is presumed to be directly aligned, as this is how
 * the allocation maximally can work.
 *
 * For Hash specifically, the algorithm now looks into the action format and finds the location
 * based on the UniqueLocationKey built from the parameters, similar to other control
 * plane based action data, and then determines the read bits of the ActionDataBus slot from
 * that point.
 */
bool ActionAnalysis::init_special_alignment(const ActionParam &read, ContainerAction &cont_action,
        le_bitrange write_bits, cstring action_name, PHV::Container container) {
    if (!(read.speciality == ActionParam::HASH_DIST || read.speciality == ActionParam::RANDOM))
        return init_simple_alignment(read, cont_action, write_bits);

    auto &action_format = tbl->resources->action_format;

    ActionData::Parameter *param = nullptr;
    if (read.speciality == ActionParam::HASH_DIST) {
        BuildP4HashFunction builder(phv);
        // Build the hash function from the expression
        auto hd = read.unsliced_expr()->to<IR::MAU::HashDist>();
        hd->apply(builder);
        P4HashFunction *func = builder.func();
        if (auto sl = read.expr->to<IR::Slice>())
            func->slice({static_cast<int>(sl->getL()), static_cast<int>(sl->getH())});

        param = new ActionData::Hash(*func);
    } else if (read.speciality == ActionParam::RANDOM) {
        auto rn = read.unsliced_expr()->to<IR::MAU::RandomNumber>();
        param = new ActionData::RandomNumber(rn->name, action_name, read.range());
    } else {
        return init_simple_alignment(read, cont_action, write_bits);
    }

    ActionData::UniqueLocationKey key(action_name, param, container, write_bits);

    const ActionData::ALUPosition *alu_pos = nullptr;
    const ActionData::ALUParameter *alu_param = action_format.find_param_alloc(key, &alu_pos);

    if (alu_pos == nullptr || alu_param == nullptr)
        return false;

    auto &adi = cont_action.adi;
    int bits_seen = 0;
    safe_vector<le_bitrange> slot_bits_brs = alu_param->slot_bits_brs(container);

    for (auto slot_bits : slot_bits_brs) {
        int write_hi = write_bits.hi - bits_seen;
        int write_lo = write_hi - slot_bits.size() + 1;
        le_bitrange mini_write_bits = { write_lo, write_hi };
        bits_seen += slot_bits.size();

        if (cont_action.counts[ActionParam::ACTIONDATA] == 0) {
            adi.alignment.add_alignment(mini_write_bits, slot_bits);
            cstring name = "$special"_cs;
            adi.initialize(name, alu_pos->loc == ActionData::IMMEDIATE, alu_pos->start_byte, 1);
            cont_action.counts[ActionParam::ACTIONDATA] = 1;
        } else if (static_cast<int>(alu_pos->start_byte) != adi.start ||
                   (alu_pos->loc == ActionData::IMMEDIATE) != adi.immediate) {
            cont_action.counts[ActionParam::ACTIONDATA]++;
        } else {
            adi.field_affects++;
            adi.alignment.add_alignment(mini_write_bits, slot_bits);
        }
        adi.specialities.setbit(read.speciality);
    }
    return true;
}

/** This initializes the alignment of action data, given that the action data allocation has
 *  taken place.  Action data allocation can take place before or after phv allocation, and
 *  thus the information in the ActionDataPlacement may not match up with the actual phv
 *  allocation.  Thus this function could potentially return false.
 */
bool ActionAnalysis::init_ad_alloc_alignment(const ActionParam &read, ContainerAction &cont_action,
        le_bitrange write_bits, cstring action_name, PHV::Container container) {
    if (read.speciality != ActionParam::NO_SPECIAL)
        return init_special_alignment(read, cont_action, write_bits, action_name, container);

    auto &action_format = tbl->resources->action_format;

    // Information on where fields are stored

    le_bitrange read_range;
    ActionParam::type_t type = ActionParam::ACTIONDATA;
    auto action_arg = isActionParam(read.expr, &read_range, &type);
    BUG_CHECK(action_arg != nullptr, "Action argument not converted correctly in the "
                                     "ActionAnalysis pass");

    // Find the location of the argument within the ActionData::Format::Use object
    ActionData::Parameter *param = nullptr;
    if (type == ActionParam::ACTIONDATA) {
        param = new ActionData::Argument(action_arg->to<IR::MAU::ActionArg>()->name, read.range());
    } else if (type == ActionParam::CONSTANT) {
        auto *adc = action_arg->to<IR::MAU::ActionDataConstant>();
        auto *ir_con = adc->constant;
        uint32_t constant_value = 0U;
        if (ir_con->fitsInt())
            constant_value = static_cast<uint32_t>(adc->constant->asInt());
        else if (ir_con->fitsUint())
            constant_value = static_cast<uint32_t>(adc->constant->asUnsigned());
        param = new ActionData::Constant(constant_value, read.size());
    }
    ActionData::UniqueLocationKey key(action_name, param, container, write_bits);

    const ActionData::ALUPosition *alu_pos = nullptr;
    const ActionData::ALUParameter *alu_param = action_format.find_param_alloc(key, &alu_pos);
    BUG_CHECK(alu_param, "Can't find %s in action format", read.expr);
    safe_vector<le_bitrange> slot_bits_brs = alu_param->slot_bits_brs(container);

    int bits_seen = 0;
    for (auto slot_bits : slot_bits_brs) {
        int write_hi = write_bits.hi - bits_seen;
        int write_lo = write_hi - slot_bits.size() + 1;
        le_bitrange mini_write_bits = { write_lo, write_hi };
        bits_seen += slot_bits.size();

        auto &adi = cont_action.adi;
        if (cont_action.counts[ActionParam::ACTIONDATA] == 0) {
            cstring alias = alu_pos->alu_op->alias();
            if (alias == nullptr)
                alias = alu_param->param->name();
            adi.alignment.add_alignment(mini_write_bits, slot_bits);
            adi.initialize(alias, alu_pos->loc == ActionData::IMMEDIATE, alu_pos->start_byte, 1);
            cont_action.counts[ActionParam::ACTIONDATA] = 1;
        } else if (static_cast<int>(alu_pos->start_byte) != adi.start ||
                   (alu_pos->loc == ActionData::IMMEDIATE) != adi.immediate) {
            cont_action.counts[ActionParam::ACTIONDATA]++;
        } else {
            adi.field_affects++;
            adi.alignment.add_alignment(mini_write_bits, slot_bits);
        }
        adi.specialities.setbit(read.speciality);
    }
    return true;
}


void ActionAnalysis::initialize_constant(const ActionParam &read,
        ContainerAction &cont_action, le_bitrange write_bits,
        safe_vector<le_bitrange> &read_bits_brs) {
    cont_action.ci.initialized = true;
    auto constant = read.expr->to<IR::Constant>();

    // FIXME: Could use a helper function on IR::Constant, but not pressing, though
    // for the purposes must fit within a 32 bit section
    // Constant can be from MINX_INT <= x <= MAX_UINT
    BUG_CHECK(constant->value >= INT_MIN && constant->value <= UINT_MAX, "%s: Constant "
              "value in an instruction not split correctly", constant->srcInfo);

    uint32_t constant_value = constant->value < 0 ?
        static_cast<uint32_t>(static_cast<int32_t>(constant->value)):
        static_cast<uint32_t>(constant->value);

    int bits_seen = 0;
    for (auto read_bits : read_bits_brs) {
        int write_hi = write_bits.hi - bits_seen;
        int write_lo = write_hi - read_bits.size() + 1;
        le_bitrange mini_write_bits = { write_lo, write_hi };
        bits_seen += read_bits.size();

        cont_action.ci.alignment.add_alignment(mini_write_bits, read_bits);
        uint32_t shift = write_bits.size() - bits_seen;
        uint32_t mask = read_bits.size() == 32 ? 0xffffffff
                                               : ((1U << read_bits.size()) - 1) << shift;
        uint32_t mini_constant_value = (constant_value & mask) >> shift;
        cont_action.ci.positions.emplace_back(mini_constant_value, mini_write_bits);
    }
}

bool ActionAnalysis::init_hash_constant_alignment(const ActionParam &read,
        ContainerAction &cont_action, le_bitrange write_bits, cstring action_name,
        PHV::Container container) {
    auto &action_format = tbl->resources->action_format;
    auto constant = read.expr->to<IR::Constant>();

    P4HashFunction func;
    func.inputs.push_back(constant);
    func.algorithm = IR::MAU::HashFunction::identity();
    func.hash_bits = { 0, constant->type->width_bits() - 1 };
    ActionData::Hash *hash = new ActionData::Hash(func);

    ActionData::UniqueLocationKey key(action_name, hash, container, write_bits);
    const ActionData::ALUPosition *alu_pos = nullptr;
    const ActionData::ALUParameter *alu_param = action_format.find_param_alloc(key, &alu_pos);

    if (alu_param == nullptr)
        return false;

    auto slot_bits_brs = alu_param->slot_bits_brs(container);
    initialize_constant(read, cont_action, write_bits, slot_bits_brs);
    return true;
}

/** Handles a IR::Constant within an Instruction to determine whether the constant will
 *  be converted to an ActionDataConstant or not.  If is the constant is to be converted,
 *  the alignment must be pulled out of the table placement algorithm.
 */
bool ActionAnalysis::init_constant_alignment(const ActionParam &read,
        ContainerAction &cont_action, le_bitrange write_bits, cstring action_name,
        PHV::Container container) {
    LOG5("init_constant_alignment: read:" << read << " write_bits:" << write_bits <<
         " action:" << action_name << " cont:" << container);
    auto &action_format = tbl->resources->action_format;
    auto constant = read.expr->to<IR::Constant>();

    // FIXME: Could use a helper function on IR::Constant, but not pressing, though
    // for the purposes must fit within a 32 bit section
    // Constant can be from MINX_INT <= x <= MAX_UINT
    BUG_CHECK(constant->fitsUint() || constant->fitsInt(), "%s: Constant "
              "value in an instruction not split correctly", constant->srcInfo);

    uint32_t constant_value = constant->value < 0 ?
        static_cast<uint32_t>(static_cast<int32_t>(constant->value)) :
        static_cast<uint32_t>(constant->value);

    // Tries to determine if the constant has an action data allocation in the
    // ActionData::Format::Use oject
    ActionData::Parameter *param = new ActionData::Constant(constant_value, read.size());
    ActionData::UniqueLocationKey key(action_name, param, container, write_bits);
    const ActionData::ALUParameter *alu_param = action_format.find_param_alloc(key, nullptr);
    if (alu_param == nullptr)
        return init_simple_alignment(read, cont_action, write_bits);

    auto slot_bits_brs = alu_param->slot_bits_brs(container);
    initialize_constant(read, cont_action, write_bits, slot_bits_brs);
    cont_action.counts[ActionParam::CONSTANT]++;
    return true;
}

bool ActionAnalysis::isReductionOr(ContainerAction &cont_action) const {
    if (cont_action.field_actions.size() != 1) return false;
    cstring red_key;
    if (!red_info.is_reduction_or(cont_action.field_actions[0].instruction, tbl, red_key))
        return false;
    if (tbl->stage() >= 0) {
        for (auto *t : red_info.tbl_reduction_or_group.at(red_key))
            if (t->stage() >= 0 && t->stage() < tbl->stage()) return false;
    }
    return true;
}

/** For action data before PHV allocation or constants.  Just guarantees that the write bits
 *  match up directly with the read bits
 */
bool ActionAnalysis::init_simple_alignment(const ActionParam &read,
         ContainerAction &cont_action, le_bitrange write_bits) {
    Log::TempIndent indent;
    LOG5("Init simple alignment for read param " << read
            << " and write bits "<< write_bits << indent);
    if (read.type == ActionParam::ACTIONDATA)
        BUG_CHECK(isActionParam(read.expr), "Action Data parameter not configured properly "
                                             "in ActionAnalysis pass");
    else if (read.type == ActionParam::CONSTANT)
        BUG_CHECK(read.expr->is<IR::Constant>(), "Constant parameter not configured properly "
                                                  "in ActionAnalysis pass");

    if (read.type == ActionParam::ACTIONDATA) {
        le_bitrange read_bits;
        cstring key;
        if (cont_action.is_deposit_field_variant || cont_action.convert_instr_to_deposit_field ||
            cont_action.convert_instr_to_byte_rotate_merge || cont_action.name == "set") {
            // if the instruction is or could be a deposit field or byte rotate, then we don't
            // need alignment for one of the operands.  We hack this by setting the read_bits
            // equal to the write bits.  This only works for one operand, and only for bytes on
            // a byte-rotate-merge, so there are certainly some cases where this is wrong
            read_bits = write_bits;
        } else if (read.speciality == ActionParam::NO_SPECIAL) {
            // if this is action data, we can 'align' it by adding padding, so as above we hack
            // the alignment to be the same as the destination
            read_bits = write_bits;
        } else if (isReductionOr(cont_action)) {
            // this is a reduction-or, so it will turn into a set (deposit-field)
            read_bits = write_bits;
        } else {
            read_bits = read.range();
        }
        cont_action.adi.alignment.add_alignment(write_bits, read_bits);
        cont_action.adi.initialized = true;
        cont_action.adi.specialities.setbit(read.speciality);
    } else if (read.type == ActionParam::CONSTANT) {
        safe_vector<le_bitrange> read_bits_brs = { write_bits };
        initialize_constant(read, cont_action, write_bits, read_bits_brs);
    }
    cont_action.counts[read.type]++;
    LOG5("Updating counts for " << read.get_type_string()
                << " = " << cont_action.counts[read.type]);
    return true;
}

/**
 * A PHV source can potentially appear both src1 and src2 of an instruction.  This function
 * splits the sources if a PHV alignment requirements have different right shift requirements
 */
void ActionAnalysis::build_phv_alignment(PHV::Container container, ContainerAction &cont_action) {
    Log::TempIndent indent;
    LOG3("Building phv alignment" << indent);
    for (auto entry : cont_action.initialization_phv_alignment) {
        std::map<int, safe_vector<Alignment>> alignment_per_right_shift;
        for (auto alignment : entry.second) {
            int right_shift = alignment.right_shift(container);
            alignment_per_right_shift[right_shift].emplace_back(alignment);
            LOG4("Adding alignment_per_right_shift : " << alignment
                    << ", right_shift: " << right_shift);
        }
        for (auto apr_entry : alignment_per_right_shift) {
            TotalAlignment ta;
            for (auto alignment : apr_entry.second) {
                ta.add_alignment(alignment.write_bits, alignment.read_bits);
                // For non commutative actions (e.g. sub) src operand order is fixed
                if (!cont_action.is_commutative() && cont_action.operands() == 2) {
                    ta.is_src1 = (alignment.read_src == SRC1);
                    LOG5("Setting alignment to SRC1 as operation is non commutative");
                }
            }
            cont_action.counts[ActionParam::PHV]++;
            LOG5("Updating PHV Alignment : " << entry.first << " : " << ta
                    << " counts for ActionParam::PHV " << cont_action.counts[ActionParam::PHV]);
            cont_action.phv_alignment.emplace(entry.first, ta);
        }
    }
}

void ActionAnalysis::determine_unused_bits(PHV::Container container,
                                           ContainerAction &cont_action) {
    ordered_set<const PHV::Field*> fieldsWritten;
    for (auto& field_action : cont_action.field_actions) {
        const PHV::Field* write_field = phv.field(field_action.write.expr);
        if (write_field == nullptr)
            BUG("Verify Overwritten: Action does not have a write?");
        fieldsWritten.insert(write_field); }

    PHV::FieldUse use(PHV::FieldUse::WRITE);
    bitvec container_occupancy = phv.bits_allocated(container, fieldsWritten,
                                                    cont_action.table_context, &use);
    bitvec unused_bits = bitvec(0, container.size()) - container_occupancy;
    LOG5("\tUnused bits for container " << container << " : " << unused_bits);

    if (cont_action.adi.initialized) {
        cont_action.adi.alignment.unused_container_bits = unused_bits;
        cont_action.adi.alignment.verbose = cont_action.verbose;
    }

    if (cont_action.ci.initialized) {
        cont_action.ci.alignment.unused_container_bits = unused_bits;
        cont_action.ci.alignment.verbose = verbose;
        cont_action.ci.container_size = container.size();
    }
    for (auto &ta : Values(cont_action.phv_alignment)) {
         ta.unused_container_bits = unused_bits;
         ta.verbose = verbose;
    }
}

/** After the container_actions_map structure is built, this analyzes each of the individual actions
 *  within a container, as well as the entire action to the container as an individual object.
 *  PHV allocation is assumed to be completed at this point.
 *
 *  The individual checks just verify that the reads can be correctly aligned to the write PHV.
 *     - For PHVs, this means that the total read PHV is completely capture in the write
 *     - For ActionData, whether the action data format is correctly aligned with the PHV
 *     - For Constants, TBD
 *
 *  The total container action also goes through a large verification step, which checks
 *  general constraints on total number of PHVs, ActionData and Constants used.  It'll then
 *  mark an instruction currently impossible or not yet implemented if it is either.
 */
void ActionAnalysis::verify_P4_action_with_phv(cstring action_name) {
    Log::TempIndent indent;
    if (verbose)
        LOG2("Verifying P4 action with phv for action " << action_name
                << " in table " << tbl->name << indent);
    for (auto &container_action : *container_actions_map) {
        auto &container = container_action.first;
        auto &cont_action = container_action.second;
        cont_action.verbose = verbose;

        if (verbose) {
            LOG2("container :" << container);
            LOG2("Field Actions:" << cont_action.field_actions);
        }
        cstring instr_name;
        bool same_action = true;
        BUG_CHECK(cont_action.field_actions.size() > 0, "Somehow a container action has no "
                                                        "field actions allocated to it");

        // Both conditionally-set and set can be combined in the same container action, as these
        // both can be converted into a bitmasked-set.  This is tracked as a to-bitmasked-set
        // name on the container action
        bool to_bitmasked_set = false;
        for (auto &field_action : cont_action.field_actions) {
            if (field_action.name == "conditionally-set")
                to_bitmasked_set = true;
        }

        instr_name = to_bitmasked_set ? "to-bitmasked-set"_cs : cont_action.field_actions[0].name;
        LOG5("Instruction name: " << instr_name << "------");

        for (auto &field_action : cont_action.field_actions) {
            if (instr_name == "to-bitmasked-set") {
                if (field_action.name != "set" && field_action.name != "conditionally-set") {
                    cont_action.error_code |= ContainerAction::MULTIPLE_CONTAINER_ACTIONS;
                    LOG4("Error Code Update: MULTIPLE_CONTAINER_ACTIONS");
                    same_action = false;
                }
            } else if (instr_name != field_action.name) {
                cont_action.error_code |= ContainerAction::MULTIPLE_CONTAINER_ACTIONS;
                LOG4("Error Code Update: MULTIPLE_CONTAINER_ACTIONS");
                same_action = false;
            }
        }

        if (!same_action && error_verbose) {
            warning("In action %s over container %s, the action has multiple operand types %s",
                      action_name, container.toString(), cstring::to_cstring(cont_action));
            has_warning = true;
        }

        if (!same_action) continue;

        cont_action.name = instr_name;
        // FIXME: total_init value set but unused
        bool total_init = true;
        for (auto &field_action : cont_action.field_actions) {
            auto &write = field_action.write;
            if (cont_action.name == "invalidate") {
                total_init &= initialize_invalidate_alignment(write, cont_action);
            }
            op_type_t read_src = SRC1;
            for (auto &read : field_action.reads) {
                LOG5("Initialize_alignment for field action: " << field_action << " total_init: " <<
                    total_init);
                cstring init_error_message;
                bool init = initialize_alignment(write, read, read_src, cont_action,
                        init_error_message, container, action_name);
                if (!init && error_verbose) {
                    warning("%1%: %2% %3%", tbl, init_error_message,
                              cstring::to_cstring(cont_action));
                    has_warning = true;
                }
                total_init &= init;
                read_src = (op_type_t)(static_cast<int>(read_src) + 1);
            }
        }
        build_phv_alignment(container, cont_action);
        determine_unused_bits(container, cont_action);

        cstring error_message;
        bool verify = cont_action.verify_possible(error_message, container, action_name, phv);
        if (!verify && error_verbose) {
            warning("%1%: %2% %3%", tbl, error_message, cstring::to_cstring(cont_action));
            has_warning = true;
        }
        check_constant_to_actiondata(cont_action, container);
        add_to_single_ad_params(cont_action);
    }

    for (auto &container_action : *container_actions_map) {
        auto &cont_action = container_action.second;
        check_single_ad_params(cont_action);
    }

    // Specifically for backtracking, as ActionFormat can be configured before PHV allocation,
    // and may not be correct.
    for (auto &container_action : *container_actions_map) {
        auto &cont_action = container_action.second;
        if ((cont_action.error_code & ContainerAction::ACTION_DATA_MISMATCH) != 0
            || (cont_action.error_code & ContainerAction::MULTIPLE_ACTION_DATA) != 0) {
            action_data_misaligned = true;
            break;
        }
    }

    // For certain error codes, we now should throw an error.
    for (auto& container_action : *container_actions_map) {
        auto& cont_action = container_action.second;
        for (auto refCode : ContainerAction::codesForErrorCases) {
            if ((cont_action.error_code & refCode) != 0) {
                LOG5("Error set due to matching on error cases");
                has_error = true;
                break;
            }
        }
    }
}


bitvec ActionAnalysis::ContainerAction::total_write() const {
    bitvec total_write_;
    for (auto tot_align_info : phv_alignment)
        total_write_ |= tot_align_info.second.direct_write_bits;
    total_write_ |= adi.alignment.direct_write_bits;
    total_write_ |= ci.alignment.direct_write_bits;

    return total_write_;
}


int ActionAnalysis::Alignment::right_shift(PHV::Container container) const {
    int rv = read_bits.lo - write_bits.lo;
    if (rv < 0)
        rv += container.size();
    return rv;
}

/**
 * The following functions are used during verify_alignment in order to determine which
 * source each parameter is as well as determine which bits are written by src1/src2
 *
 * The unused_container_bits are bits that are not live at the same time of any bits of
 * fields in the same container, and thus can be written to.
 *
 * For the deposit-field src1, the written bits must be a lo to hi that is contiguous
 * on the write bits.  For deposit-field src2, the opposite is true, as the write must
 * contain a single contiguous hole.
 *
 * For src1, this contiguous range can either container directly written bits, or unused
 * container bits that can be read
 */
bitvec ActionAnalysis::TotalAlignment::df_src1_mask() const {
    int sz = direct_write_bits.max().index() - direct_write_bits.min().index() + 1;
    bitvec write_mask(direct_write_bits.min().index(), sz);
    return (write_mask & unused_container_bits) | direct_write_bits;
}

/**
 * See comments above df_src1_mask for context.
 *
 * The goal of this function is to find what bits are to be written by the src2 of the deposit
 * field.  This looks for a single contiguous hole in the bits written, and returns
 * the reverse of this hole
 *
 * The algorithm looks for holes in the direct_write_bits.  These may actually not be the hole
 * for the deposit-field if those bits are entirely unused_container_bits.  Thus if the hole
 * is not all unused_container_bits, this is assumed to be the hole.
 *
 * If there are multiple holes, then the deposit-field mask is returned empty
 */
bitvec ActionAnalysis::TotalAlignment::df_src2_mask(PHV::Container container) const {
    bitvec all_write_bits = direct_write_bits | unused_container_bits;
    bitvec empty;

    if (all_write_bits == bitvec(0, container.size()))
        return direct_write_bits;
    int df_hole_start = 0;
    int df_hole_end = 0;

    int hole_start = direct_write_bits.ffz();
    bool hole_found = false;

    while (hole_start < static_cast<int>(container.size())) {
        int hole_end = direct_write_bits.ffs(hole_start);
        if (hole_end < 0)
            hole_end = container.size();
        bitvec hole_bv(hole_start, hole_end - hole_start);
        if ((hole_bv & unused_container_bits) != hole_bv) {
            if (hole_found)
                return empty;
            hole_found = true;
            df_hole_start = hole_start;
            df_hole_end = hole_end;
        }
        hole_start = direct_write_bits.ffz(hole_end);
    }

    bitvec reverse = bitvec(df_hole_start, df_hole_end - df_hole_start);
    return bitvec(0, container.size()) - reverse;
}

bool ActionAnalysis::TotalAlignment::contiguous() const {
    if (direct_write_bits.is_contiguous())
        return true;
    if (direct_write_bits.empty())
        return false;
    return df_src1_mask().is_contiguous();
}


/**
 * Determines if the source is to be wrapped around the container.  If the thing is wrapped,
 * the low position of the source is required to determine the location
 *
 * A deposit-field operation has source that can be rotated bit by bit.  This rotation can
 * rotate the data around the container.  The destination of a deposit-field src1 has to be
 * contiguous, but this doesn't mean that the source is contiguous.
 *
 * Say for example you were writing a 4 bit field, i.e. f1 = f2.   The allocation could be
 *
 * B0 [5..2] = f1[3..0]
 * B1 [7..6] = f2[1..0]
 * B1 [1..0] = f2[3..2]
 *
 * By right rotating B1 by 4 bits, this will shift the B1 into the correct place.  But notice that
 * the field f2 is non contiguous in it's own container.
 *
 * The instructions in the assembly file, when writing one slice to another slice are:
 *    - set dest_container(dest_bit_hi..dest_bit_lo), src_container(src_bit_hi..src_bit_lo)
 *
 * However, this is impossible to write with a source that is wrapped, as src_bit_hi is less
 * than src_bit_lo.  This is instead synthesized as a direct deposit-field instruction:
 *
 *     - deposit_field B0[5..2], B1(1), B0
 *
 * This funciton returns true if the instruction has to be synthesized directly as a deposit-field
 * because the source slice is not a lo to hi range.  It also can return a lo and hi value for
 * the IR::MAU::WrappedSlice
 *
 * 2 corner cases:
 *     1. The source is the entire container (thus if the source is shifted at all, it is wrapped)
 *     2. The source is a portion of a container (bitvec operations)
 */
bool ActionAnalysis::TotalAlignment::is_wrapped_shift(PHV::Container container, int *lo,
        int *hi) const {
    BUG_CHECK(contiguous(), "Wrapped Shift instruction can only be src1 operations");

    // implicit_read_bits might not yet be determined.  Therefore at this point,
    // the direct_read_bits may not be the full container, but within the addition of the
    // implicit_read_bits would be.
    if (read_bits().popcount() == static_cast<int>(container.size()) ||
        df_src1_mask().popcount() == static_cast<int>(container.size())) {
        if (right_shift == 0) {
            return false;
        } else {
            ///> lo is the bit of the read bits that needs to be aligned with bit 0
            ///> hi is the bit that needs to be aligned with container.size() - 1.
            if (lo)
                *lo = right_shift;
            if (hi)
                *hi = right_shift - 1;
            return true;
        }
    } else {
        // Implicit read bits might not yet be set, in verify_alignment
        // Left rotate df_src1_mask which is set by implicit write bits by right
        // shift value for dst -> src conversion
        int left_shift = (container.size() - right_shift) % container.size();
        bitvec curr_read_bits = df_src1_mask().rotate_right_copy(0, left_shift, container.size());
        if (curr_read_bits.is_contiguous())
            return false;

        bitvec reverse = bitvec(0, container.size()) - curr_read_bits;
        BUG_CHECK(reverse.is_contiguous(), "Wrapped shift reverse should be contiguous");

        if (lo)
            *lo = reverse.max().index() + 1;
        if (hi)
            *hi = reverse.min().index() - 1;
        return true;
    }
}

/**
 * Guarantees that a single alignment is aligned correctly.  The checks are:
 *    - Each individual field write by write are aligned at the same offset on their read
 *    - Sources are not both non-contiguous and non-aligned
 */
bool ActionAnalysis::TotalAlignment::verify_individual_alignments(PHV::Container &container) {
    Log::TempIndent indent;
    LOG4("ActionAnalysis::TotalAlignment::verify_individual_alignments on container : "
            << container << indent);
    bool right_shift_set = false;
    for (auto indiv_align : indiv_alignments) {
        int possible_right_shift = indiv_align.right_shift(container);
        LOG5("Checking indiv_alignment: " << indiv_align
                << ", possible_right_shift: " << possible_right_shift);
        if (right_shift_set && possible_right_shift != right_shift) {
            LOG4("Right shift not possible on alignment");
            return false;
        }
        right_shift = possible_right_shift;
        right_shift_set = true;
    }
    // For mocha and dark containers, individual writes need to be aligned with their sources.
    if (container.is(PHV::Kind::mocha) || container.is(PHV::Kind::dark)) {
        if (!aligned()) {
            LOG4("Individual writes not aligned on mocha or dark containers");
            return false;
        }
    }
    LOG4("Verified");
    return true;
}

/**
 * Src1 of a deposit-field does not have to be aligned, but must be contiguous after it
 * has been rotated
 */
bool ActionAnalysis::TotalAlignment::deposit_field_src1() const {
    return contiguous();
}

/**
 * Src2 of a deposit-field has to be be aligned, but not contiguous, as long as there is
 * only one hole within the deposit-field
 */
bool ActionAnalysis::TotalAlignment::deposit_field_src2(PHV::Container container) const {
    if (!aligned())
        return false;
    return !df_src2_mask(container).empty();
}


bool ActionAnalysis::TotalAlignment::is_byte_rotate_merge_src(PHV::Container container) const {
    if (container.is(PHV::Size::b8))
        return false;
    for (size_t i = 0; i < container.size(); i += 8) {
        if (direct_write_bits.getslice(i, 8).empty())
            continue;
        bitvec write_bits_per_byte = (direct_write_bits | unused_container_bits).getslice(i, 8);
        if (write_bits_per_byte.popcount() == 8)
            continue;
        return false;
    }
    return (right_shift % 8) == 0;
}


/**
 * Based on what bits that are written and unused, this is the possible source mask bit by bit.
 * Because of unused bits, the actual source mask can be a subset of this
 */
bitvec ActionAnalysis::TotalAlignment::brm_src_mask(PHV::Container container) const {
    bitvec rv;
    for (size_t i = 0; i < container.size(); i += 8) {
        bitvec write_bits_per_byte = (direct_write_bits | unused_container_bits).getslice(i, 8);
        if (write_bits_per_byte.popcount() == 8)
            rv.setrange(i, 8);
    }
    return rv;
}

/**
 * The byte mask appearing in the instruction
 */
bitvec ActionAnalysis::TotalAlignment::byte_rotate_merge_byte_mask(PHV::Container container) const {
    BUG_CHECK(is_src1, "byte_rotate_merge_byte_mask can only be called on src1");
    bitvec rv;
    bitvec write = write_bits();
    for (size_t i = 0; i < container.size(); i += 8) {
        bitvec write_bits_per_byte = write_bits().getslice(i, 8);
        BUG_CHECK(write_bits_per_byte.empty() || write_bits_per_byte.popcount() == 8, "Illegal "
            "call of byte rotate merge src mask");
        if (write_bits_per_byte.empty())
            continue;
        rv.setbit(i/8);
    }
    return rv;
}

/**
 * \sa verify_set_alignment for description of byte-rotate-merge
 */
bool ActionAnalysis::ContainerAction::is_byte_rotate_merge(PHV::Container container,
        TotalAlignment &ad_alignment) {
    if (container.is(PHV::Size::b8))
        return false;
    if (name == "to-bitmasked-set")
        return false;
    if (ad_sources()) {
        if (!ad_alignment.is_byte_rotate_merge_src(container))
            return false;
    }

    for (auto phv_ta : Values(phv_alignment)) {
        if (!phv_ta.is_byte_rotate_merge_src(container))
            return false;
    }

    convert_instr_to_byte_rotate_merge = true;
    if (read_sources() == 1)
        implicit_src2 = true;
    return true;
}

/**
 * Verifies that the instruction can be encoded as a deposit-field.  As mentioned above
 * verify_set_alignment, deposit field is the following:
 *
 * deposit-field:
 *     dest = ((src1 << shift) & mask) | (src2 & ~mask)
 *
 * The mask is a contiguous range, and has a single lo and hi.  Thus if src1 is a contiguous
 * range of data (which can go around the container boundary), then this is supportable.
 * The range has to be contiguous after the shift.
 *
 * In a deposit field, a source must at least either be aligned, or can be contiguous.  A
 * source can be both.  A source that is not aligned must be src1, and a source that is
 * not contiguous is src2.
 */
bool ActionAnalysis::ContainerAction::verify_deposit_field_variant(PHV::Container container,
        TotalAlignment &ad_alignment) {
    Log::TempIndent indent;
    LOG5("Verifying deposit field variant for container: " << container << indent);
    TotalAlignment *single_src_alignment = nullptr;
    if (ad_sources()) {
        single_src_alignment = &ad_alignment;
    } else if (phv_alignment.size() == 1) {
        single_src_alignment = &phv_alignment.begin()->second;
    }

    if (ad_sources()) {
        if (!ad_alignment.contiguous()) {
            LOG5("AD alignment is not contiguous");
            return false;
        }
    }

    int max_phv_non_aligned = ad_sources() ? 0 : 1;
    int max_phv_non_contiguous = read_sources() - max_phv_non_aligned;

    int phv_non_aligned = 0;
    int phv_non_contiguous = 0;
    for (auto phv_ta : Values(phv_alignment)) {
        LOG5("PHV alignment: " << phv_ta);
        if (!phv_ta.aligned() && !phv_ta.contiguous()) {
            LOG5("PHV is not aligned or contiguous");
            return false;
        }
        phv_non_contiguous += phv_ta.contiguous() ? 0 : 1;
        phv_non_aligned += phv_ta.aligned() ? 0 : 1;
    }

    LOG5("Read sources: " << read_sources() << ", single_src_alignment: "
            << (single_src_alignment ? *single_src_alignment : TotalAlignment()));
    if (read_sources() == 2) {
        convert_instr_to_deposit_field = true;
        LOG5("  A. Convert instr to deposit field");
    } else if (read_sources() == 1 && single_src_alignment) {
        if (!single_src_alignment->deposit_field_src1()) {
            /**
             * If the single PHV field in a deposit-field cannot be src1 due to non-contiguity,
             * but can be src2 because it is aligned
             */
            if (single_src_alignment->deposit_field_src2(container)) {
                convert_instr_to_deposit_field = true;
                LOG5("  B. Convert instr to deposit field with implicit source");
                implicit_src1 = true;
                max_phv_non_aligned = 0;
                max_phv_non_contiguous = 1;
            } else {
                return false;
            }
        /**
         * Generally in a single source set, this is translated to set C0(lo..hi), C1(lo..hi),
         * but when the source is wrapped, the only way for the assembler to understand is
         * an explicit deposit-field instruction.

         * The deposit field for a single sourced wrapped source will be the following:
         *
         * deposit-field C0(lo..hi), C1(lo), C0
         *
         * The assembler will not understand the C1 slice if lo > hi, but the deposit-field
         * instruction technically only requires the lo bit to determine the right shift
         */
        } else if (single_src_alignment->is_wrapped_shift(container)) {
            convert_instr_to_deposit_field = true;
            LOG5("  C. Convert instr to deposit field with implicit source");
            implicit_src2 = true;
        }
    } else {
        return false;
    }

    if (phv_non_aligned > max_phv_non_aligned)
        return false;
    if (phv_non_contiguous > max_phv_non_contiguous)
        return false;
    is_deposit_field_variant = true;
    LOG5("  D. Convert instr to deposit field variant");
    return true;
}

/**
 * Verifies that the set ContainerActions (which are translated from assignment operations)
 * are possible to be translated to an instruction.
 *
 * The following instructions are possible encodings of assignment statements.  Note that all
 * shifts are rotational, meaning that the container is rotated by this number of bits.
 *
 * deposit-field:
 *     dest = ((src1 << shift) & mask) | (src2 & ~mask)
 *
 * The mask is a contiguous range, and has a single lo and hi.  Thus if src1 is a contiguous
 * range of data (which can go around the container boundary), then this is supportable.
 * The range has to be contiguous after the shift.
 *
 * bitmasked-set:
 *     dest = (src1 & mask) | (src2 & ~mask)
 *
 * where the src1 must come from action data, and everything must be aligned.  The mask
 * is not required to be contiguous.  Instead the mask is stored as action data in the RAM
 * entry.
 *
 * byte-rotate-merge:
 *     dest = ((src1 << src1_shift) & mask) | ((src2 << src2_shift) & ~mask)
 *
 * The limitation is that src1_shift and src2_shift % 8 must == 0.  The mask is also a byte
 * mask, requiring the entire byte to come from the source.
 *
 * The purpose of this function is to verify that a ContainerAction can be translated
 * to one of these possible instructions, and will return false if the action is impossible
 * in the ALU.  This also determines if the action cannot be output as a set in the assembly
 * language.  With a set operation, src2 (background) = destination is implicit, but in the
 * case when this isn't true, the full field must be translated in the compiler, i.e:
 *     deposit-field C0(lo..hi), C1(lo..hi), C2
 *
 * Each of these operations have different constraints:
 *     - Bitmasked-set requires both sources to be aligned
 *     - Byte-Rotate-Merge has neither alignment or contiguous constraints, but requires
 *       the shifts to be factors of 8 and each byte to be sourced by only one source
 *     - Deposit-Field explained: \sa set_deposit_field_variant
 *
 * The worst case is that an instruction is encoded as a bitmasked-set, as this requires
 * double the action data that would be necessary in a different instruction.  Only when
 * something is required to be a bitmasked-set is this instruction used
 */
bool ActionAnalysis::ContainerAction::verify_set_alignment(PHV::Container container,
        TotalAlignment &ad_alignment) {
    Log::TempIndent indent;
    LOG4("Verify set alignment for container " << container
            << " AD Alignment: " << ad_alignment << indent);
    int non_aligned_phv_sources = 0;
    int non_contiguous_phv_sources = 0;
    int non_aligned_and_non_contiguous_sources = 0;

    for (auto phv_ta : Values(phv_alignment)) {
        non_aligned_phv_sources += phv_ta.aligned() ? 0 : 1;
        non_contiguous_phv_sources += phv_ta.contiguous() ? 0 : 1;
        non_aligned_and_non_contiguous_sources += phv_ta.aligned() || phv_ta.contiguous() ? 0 : 1;
    }

    int non_aligned_sources = non_aligned_phv_sources;
    int non_contiguous_sources = non_contiguous_phv_sources;

    if (ad_sources()) {
        non_aligned_sources += ad_alignment.aligned() ? 0 : 1;
        non_contiguous_sources += ad_alignment.contiguous() ? 0 : 1;
        non_aligned_and_non_contiguous_sources +=
            ad_alignment.aligned() || ad_alignment.contiguous() ? 0 : 1;
    }

    LOG5("Non aligned non contiguous sources: " << non_aligned_and_non_contiguous_sources
            << ", non aligned sources: " << non_aligned_sources
            << ", non aligned phv sources : " << non_aligned_phv_sources
            << ", ad sources: " << ad_sources()
            << ", ad alignment is contiguous: " << ad_alignment.contiguous()
            << ", non contiguous sources: " << non_contiguous_sources);
    // If a source is both not aligned and not contiguous, the only supportable is a
    // byte rotate merge
    if (non_aligned_and_non_contiguous_sources > 0)
        return is_byte_rotate_merge(container, ad_alignment);

    if (non_aligned_sources == 2 || non_contiguous_sources == 2)
        if (is_byte_rotate_merge(container, ad_alignment))
            return true;

    if (ad_sources() && non_aligned_phv_sources > 0)
        return is_byte_rotate_merge(container, ad_alignment);

    if (ad_sources())
        BUG_CHECK(non_aligned_phv_sources == 0, "Bug in alignment check for %s", to_string());

    if (ad_sources() && !ad_alignment.contiguous()) {
        if (is_byte_rotate_merge(container, ad_alignment))
            return true;
        convert_instr_to_bitmasked_set = true;
        LOG5("A. Converting instr to bitmasked set");
        return true;
    }

    if (name == "to-bitmasked-set") {
        if (non_aligned_sources == 0) {
            convert_instr_to_bitmasked_set = true;
            LOG5("B. Converting instr to bitmasked set");
            return true;
        } else {
            return false;
        }
    }

    return verify_deposit_field_variant(container, ad_alignment);
}

void ActionAnalysis::ContainerAction::determine_src1() {
    Log::TempIndent indent;
    LOG5("Determining src1" << indent);
    if (implicit_src1) {
        LOG5("Implicit src1 present");
        return;
    }
    bool src1_assigned = false;
    if (counts[ActionParam::CONSTANT] > 0) {
        ci.alignment.is_src1 = true;
        src1_assigned = true;
        LOG5("SRC1 (CONSTANT) present");
    }
    if (counts[ActionParam::ACTIONDATA] > 0) {
        adi.alignment.is_src1 = true;
        src1_assigned = true;
        LOG5("SRC1 (ACTIONDATA) present");
    }

    // Check is src1 is already assigned in the alignments
    if (!src1_assigned) {
        for (auto &tot_align_info : phv_alignment) {
            auto &tot_alignment = tot_align_info.second;
            if (tot_alignment.is_src1) {
                src1_assigned = true;
                LOG5("SRC1 already assigned (PHV)");
            }
        }
    }

    // If no src1 has been assigned, then PHV is the src1 information.
    // - If a PHV write and read bits are unaligned, then that PHV field is src1.
    // - If a PHV source is not contiguous, in a deposit-field, then it can't be a src1
    // - Otherwise either PHV source could be considered src1.
    if (!src1_assigned) {
        for (auto &tot_align_info : phv_alignment) {
            auto &tot_alignment = tot_align_info.second;
            if (src1_assigned) break;
            if (!tot_alignment.aligned()) {
                tot_alignment.is_src1 = true;
                src1_assigned = true;
                LOG5("SRC1 Assigned (Aligned): "
                    << tot_align_info.first << ":" << tot_alignment);
            }
        }
    }

    if (!src1_assigned) {
        for (auto &tot_align_info : phv_alignment) {
            auto &tot_alignment = tot_align_info.second;
            if (src1_assigned) break;
            if (!tot_alignment.contiguous()) continue;
            tot_alignment.is_src1 = true;
            src1_assigned = true;
            LOG5("SRC1 Assigned (Contiguous): "
                << tot_align_info.first << ":" << tot_alignment);
        }
    }

    if (!src1_assigned) {
        for (auto &tot_align_info : phv_alignment) {
            auto &tot_alignment = tot_align_info.second;
            tot_alignment.is_src1 = true;
            src1_assigned = true;
            LOG5("SRC1 Assigned: "
                << tot_align_info.first << ":" << tot_alignment);
            break;
        }
    }
}

void ActionAnalysis::TotalAlignment::set_implicit_bits_from_mask(bitvec mask,
        PHV::Container container) {
    LOG5("ActionAnalysis::TotalAlignment::set_implicit_bits_from_mask with mask "
            << mask << " and container " << container);
    bitvec cont_mask(0, container.size());
    implicit_write_bits |= (mask & unused_container_bits);
    // Left rotate implicit write bits by right shift value for dst -> src
    // conversion
    int left_shift = (container.size() - right_shift) % container.size();
    implicit_read_bits = implicit_write_bits.rotate_right_copy(0, left_shift, container.size());
    implicit_read_bits &= cont_mask;
    LOG5("  Setting implicit_read_bits : 0x" << std::hex << implicit_read_bits
         << ", implicit_write_bits : 0x" << implicit_write_bits << std::dec);
}

void ActionAnalysis::TotalAlignment::determine_df_implicit_bits(PHV::Container container) {
    LOG5("ActionAnalysis::TotalAlignment::determine_df_implicit_bits with container " << container);
    bitvec mask = is_src1 ? df_src1_mask() : df_src2_mask(container);
    set_implicit_bits_from_mask(mask, container);
}

void ActionAnalysis::TotalAlignment::determine_brm_implicit_bits(PHV::Container container,
        bitvec src1_mask) {
    bitvec mask = brm_src_mask(container);
    BUG_CHECK(is_src1 == src1_mask.empty(), "Src1 always needs to be done first");
    mask -= src1_mask;
    set_implicit_bits_from_mask(mask, container);
}



void ActionAnalysis::TotalAlignment::implicit_bits_full(PHV::Container container) {
    BUG_CHECK(right_shift == 0, "Whole container writes cannot have a shift");
    bitvec cont_mask = bitvec(0, container.size());
    implicit_write_bits = cont_mask - direct_write_bits;
    implicit_read_bits = cont_mask - direct_read_bits;
}

/**
 * The implicit bits are calculated as bits that aren't directly written or read from fields
 * in the PHV, but are written anyway by the operation.  Either fields can possibly be dead,
 * or padding, or just empty space within the allocation.
 *
 * This function runs at the end of verify_alignment, when src1 and src2 are possibly known
 */
void ActionAnalysis::ContainerAction::determine_implicit_bits(PHV::Container container,
        TotalAlignment &ad_alignment) {
    if (is_deposit_field_variant) {
        for (auto &ta : phv_alignment) {
            ta.second.determine_df_implicit_bits(container);
        }
        if (adi.initialized || ci.initialized)
            ad_alignment.determine_df_implicit_bits(container);
    } else if (convert_instr_to_bitmasked_set) {
        bitvec ad_write_bits = ad_alignment.write_bits();
        for (auto &ta : phv_alignment) {
            bitvec src2_write_bits = bitvec(0, container.size()) - ad_write_bits;
            ta.second.implicit_write_bits = src2_write_bits - ta.second.direct_write_bits;
        }
    } else if (convert_instr_to_byte_rotate_merge) {
        bitvec src1_mask;
        bitvec src2_mask;
        if (ad_sources()) {
            ad_alignment.determine_brm_implicit_bits(container, src1_mask);
            src1_mask = ad_alignment.write_bits();
        } else {
            for (auto &phv_ta : Values(phv_alignment)) {
                if (!phv_ta.is_src1) continue;
                phv_ta.determine_brm_implicit_bits(container, src1_mask);
                src1_mask = phv_ta.write_bits();
            }
        }

        if (implicit_src2) {
            src2_mask = bitvec(0, container.size()) - src1_mask;
        } else {
            for (auto &phv_ta : Values(phv_alignment)) {
                if (phv_ta.is_src1) continue;
                phv_ta.determine_brm_implicit_bits(container, src1_mask);
                src2_mask = phv_ta.write_bits();
            }
        }

        BUG_CHECK((src1_mask & src2_mask).empty() &&
                  (src1_mask | src2_mask).popcount() == static_cast<int>(container.size()) &&
                  (src1_mask | src2_mask).is_contiguous(), "Byte rotate merge implicit bits "
                  "incorrectly done");
    } else {
        for (auto &ta : phv_alignment) {
            ta.second.implicit_bits_full(container);
        }
        if (adi.initialized || ci.initialized)
            ad_alignment.determine_df_implicit_bits(container);
    }

    // The source can either only be action data or a constant at the moment, so currently,
    // this behavior is safe.  This is only used by determining constants to action data,
    // which will only be necessary if the action is constant only, and during
    // MergeInstructions, which at that point, everything will either be action data or
    // constants
    if (adi.initialized) {
        adi.alignment.implicit_write_bits = ad_alignment.implicit_write_bits;
        adi.alignment.implicit_read_bits = ad_alignment.implicit_read_bits;
    }

    if (ci.initialized) {
        ci.alignment.implicit_write_bits = ad_alignment.implicit_write_bits;
        ci.alignment.implicit_read_bits = ad_alignment.implicit_read_bits;
    }
}

bool ActionAnalysis::ContainerAction::verify_alignment(PHV::Container &container) {
    Log::TempIndent indent;
    LOG3("Verifying alignment on container " << container << indent);
    LOG4("ADI: " << adi.alignment);
    LOG4("CI: " << ci.alignment);
    TotalAlignment ad_alignment = adi.alignment | ci.alignment;
    if (ad_sources()) {
        LOG4("Checking action data sources " << ad_alignment);
        if (!ad_alignment.verify_individual_alignments(container)) {
            LOG3("Cannot verify individual alignments on Action Data");
            return false;
        }
    }

    if (adi.initialized)
        adi.alignment.right_shift = ad_alignment.right_shift;
    if (ci.initialized)
        ci.alignment.right_shift = ad_alignment.right_shift;


    for (auto &ta : phv_alignment) {
        LOG4("Checking phv source on container " << ta.first << " " << ta.second);
        if (!ta.second.verify_individual_alignments(container)) {
            LOG3("Cannot verify individual alignments on PHV");
            return false;
        }
    }

    if (is_from_set()) {
        LOG4("Checking set ");
        if (container.is(PHV::Kind::normal)) {
            if (!verify_set_alignment(container, ad_alignment)) {
                LOG3("Cannot verify set alignment");
                return false;
            }
        }
    } else {
        LOG4("Checking non set ");
        if (!ad_alignment.aligned()) {
            LOG3("Cannot verify non set alignment on Action Data");
            return false;
        }
        for (auto &ta : Values(phv_alignment)) {
            if (!ta.aligned()) {
                LOG3("Cannot verify non set alignment on PHV");
                return false;
            }
        }
    }
    determine_src1();
    ad_alignment.is_src1 = ci.alignment.is_src1 | adi.alignment.is_src1;

    determine_implicit_bits(container, ad_alignment);
    LOG3("Verified all alignments");
    return true;
}

// For container actions with counts[ActionParam::ACTIONDATA]>1 verify
// whether there are really multiple ACTIONDATA params
// E.g. in the case of an ACTIONDATA param being split into multiple slices
//        due to the destination metadata being sliced because of different overlays
//        and different initializations of these slices. In this case return false and
//        the source/dest slices will be merged into single slices or multioperand objects.
bool ActionAnalysis::ContainerAction::verify_multiple_action_data() const {
    bitvec read_bits, write_bits;
    bool first_act = true;
    const IR::Expression *prv_rexpr = nullptr, *prv_wexpr = nullptr;
    for (auto& fa : field_actions) {
        LOG4("   FieldAction " << fa.name << "  write_bits: " << fa.write.range() <<
             "   # of read_params: " << fa.reads.size());

        auto *fa_wexpr = fa.write.expr;
        if (auto *slice = fa_wexpr->to<IR::Slice>()) {
            fa_wexpr = slice->e0;
        }
        if (first_act) prv_wexpr = fa_wexpr;
        if (prv_wexpr != fa_wexpr) return true;

        // Limit multiple AD exception to single source instructions
        if (fa.reads.size() > 1) return true;
        for (const auto &r_param : fa.reads) {
            auto *r_expr = r_param.expr;
            if (auto *slice = r_expr->to<IR::Slice>()) {
                r_expr = slice->e0;
            }
            LOG4("\t " << *r_expr <<"   read_bits: "<< r_param.range());

            write_bits.setrange(fa.write.range().lo, fa.write.range().size());
            read_bits.setrange(r_param.range().lo, r_param.range().size());
            LOG4("  unified  write_bits: " <<  write_bits << "   read_bits: " << read_bits);

            if (first_act) {
                prv_rexpr = r_expr;
                first_act = false;
            }

            if (prv_rexpr != r_expr) return true;
        }
    }

    if (!write_bits.is_contiguous() || !read_bits.is_contiguous()) return true;

    return false;
}


bitvec ActionAnalysis::ContainerAction::specialities() const {
    bitvec rv;
    for (auto &fa : field_actions) {
        for (auto &read : fa.reads) {
            if (read.type != ActionParam::ACTIONDATA)
                continue;
            if (read.is_conditional)
                continue;
            rv.setbit(read.speciality);
        }
    }
    return rv;
}

/** For nearly all instructions, the ALU operation acts over all bits in the container.  The only
 *  instruction where this doesn't apply is the deposit-field instruction.  That instruction
 *  can have a portion masked.  Any other operation currently acts on the entire container, and
 *  all fields could be potentially affected.
 */
bool ActionAnalysis::ContainerAction::verify_overwritten(const PHV::Container container,
                                                         const PhvInfo &phv) {
    Log::TempIndent indent;
    LOG3("Verify_overwritten for container " << container << indent);
    ordered_set<const PHV::Field*> fieldsWritten;
    for (auto& field_action : field_actions) {
        const PHV::Field* write_field = phv.field(field_action.write.expr);
        if (write_field == nullptr)
            BUG("Verify Overwritten: Action does not have a write?");
        fieldsWritten.insert(write_field);
        LOG4("Adding written field " << write_field->name
                << " for expr " << field_action.write.expr);
    }

    PHV::FieldUse use(PHV::FieldUse::WRITE);
    // Collect bitvec of non (mutually/meta/dark) exclusive fieldslices on @container
    // for fields in @fieldWritten
    bitvec container_occupancy = phv.bits_allocated(container, fieldsWritten, table_context, &use);

    bitvec total_write_bits;
    for (auto &tot_align_info : phv_alignment) {
        total_write_bits |= tot_align_info.second.direct_write_bits;
    }

    LOG4("Overwrite masks - container:" << container_occupancy
            << " total_write: " << total_write_bits);
    LOG4("Overwrite masks - adi:" << adi.alignment.direct_write_bits
            << " ci: " << ci.alignment.direct_write_bits
            << " invalid: " << invalidate_write_bits);

    if (container_occupancy.empty()) {
        LOG3("container_occupancy is empty");
        return true;
    }

    total_write_bits |= adi.alignment.direct_write_bits;
    total_write_bits |= ci.alignment.direct_write_bits;
    if (name == "invalidate")
        total_write_bits |= invalidate_write_bits;

    bitvec preserved_bits = container_occupancy;
    for (const FieldAction& fa : field_actions) {
        le_bitrange dst_bits;
        auto dst_f = phv.field(fa.write.expr, &dst_bits);
        CHECK_NULL(dst_f);
        const auto write_type = fa.container_write_type();
        switch (write_type.first) {
            case ActionAnalysis::FieldAction::ALL_BITS: {
                preserved_bits.clear();
                break;
            }
            case ActionAnalysis::FieldAction::HIGHER_BITS: {
                bitvec container_w = phv.bits_allocated(container, dst_f, table_context, &use);
                preserved_bits &= bitvec(0, container_w.ffs() + dst_bits.lo);
                break;
            }
            case ActionAnalysis::FieldAction::DST_ONLY: {
                break;
            }
        }
        if (fa.name == "and" || fa.name == "xnor") {
            // AND/XNOR instructions may need additional bits to be set to 1 in the constant source,
            // in order to guarantee that fields that are packed into the same container but are
            // not ANDed/XNORed will not be modified. Because of these additional bits set to 1, the
            // constant source may no longer fit into an immediate constant, so we force it to
            // be changed to an action data constant.
            switch (write_type.second) {
                case FieldAction::CONSTANT:
                    error_code |= CONSTANT_TO_ACTION_DATA;
                    BFN_FALLTHROUGH;
                case FieldAction::ACTION_DATA_CONSTANT:
                    error_code |= UNRESOLVED_REPEATED_ACTION_DATA;
                    break;
                default:
                    break;
            }
        }
    }
    bool valid_overwrite = false;
    // If there is a set operation that sets values to a field that is in a mocha container packed
    // with other fields, then it is a invalid operation. Since mocha containers do not have ALUs
    // and value will be set to the whole container, other fields will be overwritten. However,
    // there is one exception, which is that if a set operation is assigning field a to field b, and
    // field a and field b are in the same mocha container and occupy same bits, then this set
    // operation is essentially a noop and will not overwrite other fields in that mocha container.
    if ((name == "set") && !container.is(PHV::Kind::normal)) {
        for (auto& fa : field_actions) {
            BUG_CHECK(fa.name == "set", "set container action contains non-set field action");
            BUG_CHECK(fa.reads.size() == 1, "set operation reads from multiple source");
            auto& read = fa.reads[0];
            if (read.type == ActionParam::PHV) {
                le_bitrange range;
                auto *field = phv.field(read.expr, &range);
                PHV::FieldUse use(PHV::FieldUse::READ);
                PHV::Container container_read;
                bool first_alloc_slice = true;
                bitvec read_range;
                field->foreach_alloc(range, table_context, &use, [&](const PHV::AllocSlice &alloc) {
                    BUG_CHECK(alloc.container_slice().lo >= 0, "Invalid negative container bit");
                    if (first_alloc_slice) {
                        container_read = alloc.container();
                        first_alloc_slice = false;
                    } else {
                        BUG_CHECK(container_read == alloc.container(),
                            "set operation assign value from two different containers");
                    }
                    le_bitrange slice = alloc.container_slice();
                    read_range.setrange(slice.lo, slice.size());
                });

                // same container and same bits
                if ((read_range == total_write_bits) && (container_read == container)) {
                    valid_overwrite = true;
                } else {
                    valid_overwrite = false;
                    break;
                }
            } else {
                valid_overwrite = false;
                break;
            }
        }
    }

    if (valid_overwrite) {
        return true;
    }

    LOG4("Overwrite masks - preserved:" << preserved_bits);

    LOG5("Total write bits: " << total_write_bits << ", preserved_bits: " << preserved_bits);
    if ((total_write_bits | preserved_bits) != container_occupancy) {
        LOG3("Total write bits or preserved bits not equal to container occupancy");
        return false;
    }

    if (static_cast<size_t>(total_write_bits.popcount()) != container.size()) {
        error_code |= PARTIAL_OVERWRITE;
        LOG4("Error Code Update: PARTIAL_OVERWRITE");
    }
    return true;
}


/** Ensure that a read field is the only field within that container
 */
bool ActionAnalysis::ContainerAction::verify_only_read(const PhvInfo &phv, int num_source) {
    Log::TempIndent indent;
    LOG3("Verifying only read for souces: " << num_source << indent);
    ordered_set<const PHV::Field*> fieldsRead;
    int src_cnt = 0;
    for (auto& field_action : field_actions) {
        const PHV::Field* read_field = nullptr;
        for (auto& read : field_action.reads) {
            if (read.type == ActionParam::PHV) {
                src_cnt++;
                read_field = phv.field(read.expr);
                fieldsRead.insert(read_field); } } }
    LOG4("Fields Read: " << fieldsRead);
    BUG_CHECK(src_cnt == num_source, "Shift operation with improper number of sources, expected "
              "%d but got %d", num_source, src_cnt);

    PHV::FieldUse use(PHV::FieldUse::READ);
    for (auto &tot_align_info : phv_alignment) {
        auto container = tot_align_info.first;
        auto &total_alignment = tot_align_info.second;
        bitvec container_occupancy = phv.bits_allocated(container, fieldsRead, table_context, &use);
        LOG4("Phv alignment on container " << container << ": " << total_alignment
                << " container_occupancy: " << container_occupancy);
        if (total_alignment.direct_read_bits != container_occupancy) {
            LOG3("Alignment read bits not equal to container_occupancy");
            return false;
        }
    }
    return true;
}

void ActionAnalysis::add_to_single_ad_params(ContainerAction &cont_action) {
    Log::TempIndent indent;
    LOG4("Adding to single/multiple AD params" << indent);
    const IR::MAU::ActionArg *aa = nullptr;
    for (auto &field_action : cont_action.field_actions) {
        for (auto &param : field_action.reads) {
            le_bitrange aa_range = { 0, 0 };
            aa = isActionArg(param.expr, &aa_range);
            if (aa == nullptr)
                continue;
            auto pair = std::make_pair(aa->name, aa_range);
            if (single_ad_params.count(pair) > 0) {
                multiple_ad_params.insert(pair);
                LOG5("Adding to multiple AD Param: " << pair.first << ":" << pair.second);
            } else {
                single_ad_params.insert(pair);
                LOG5("Adding to single AD Param: " << pair.first << ":" << pair.second);
            }
        }
    }
}

void ActionAnalysis::check_single_ad_params(ContainerAction &cont_action) {
    if (cont_action.field_actions.size() != 1)
        return;
    const IR::MAU::ActionArg *aa = nullptr;
    for (auto &param : cont_action.field_actions[0].reads) {
        le_bitrange aa_range = { 0, 0 };
        aa = isActionArg(param.expr, &aa_range);
        if (aa == nullptr)
            continue;
        auto ad_params = multiple_ad_params.count(std::make_pair(aa->name, aa_range));
        LOG5("Multiple AD Params Count : " << ad_params
            << ", aa: " << aa->name << ", range: " << aa_range);
        if (ad_params > 0) {
            cont_action.error_code |= ContainerAction::UNRESOLVED_REPEATED_ACTION_DATA;
            LOG4("Error Code Update: UNRESOLVED_REPEATED_ACTION_DATA");
        }
    }
}

/** A check to guarantee that the use of constant is legal in the action within a container.
 *  The constant does not have to be converted to action data if:
 *
 *  the constant is used in a set instruction
 *      - if the constant covers the whole container, when the container is 8 or 16 bit size
 *      - if the constant covers the whole container, the container is a 32 bit size, and
 *        the constant is between -(2^19) <= value <= 2^19 - 1
 *      - if the constant doesn't cover the whole container, and the constant is between
 *        -8 <= value <= 7
 *
 *  the constant is used in another instruction, where the constant convers the whole container,
 *  and the constant is between -8 <= value <= 7
 *
 *  This range also applies to the complement of the range i.e. an 8 bit container converts to
 *  value >= 255 - 7 OR value < -255 + 8
 *
 *  These ranges are due to instruction formats of load-consts, and one of the src fields in
 *  every instruction
 *
 *  A constant must be converted to action data if it doesn't meet the requirements, or:
 *      - the container also requires action data
 *      - multiple constants are used
 */
void ActionAnalysis::check_constant_to_actiondata(ContainerAction &cont_action,
        PHV::Container container) {
    Log::TempIndent indent;
    LOG4("Checking constant to action data : " << container << indent);
    auto &counts = cont_action.counts;
    if (counts[ActionParam::ACTIONDATA] > 1 && ad_alloc) {
        bool mult_ad = cont_action.verify_multiple_action_data();

        if (mult_ad) {
            cont_action.error_code |= ContainerAction::MULTIPLE_ACTION_DATA;
            LOG4("Error Code Update: MULTIPLE_ACTION_DATA");
        } else {
            counts[ActionParam::ACTIONDATA] = 1;
        }
    }

    if (counts[ActionParam::CONSTANT] == 0)
        return;

    if (!cont_action.ci.initialized)
        BUG("Constant not setup by the program correctly");

    if (cont_action.specialities().getbit(ActionParam::HASH_DIST)) {
        cont_action.error_code |= ContainerAction::CONSTANT_TO_HASH;
        LOG4("Error Code Update: CONSTANT_TO_HASH");
        return;
    }

    if (counts[ActionParam::ACTIONDATA] > 0 && counts[ActionParam::CONSTANT] > 0) {
        cont_action.error_code |= ContainerAction::CONSTANT_TO_ACTION_DATA;
        LOG4("A. Error Code Update: CONSTANT_TO_ACTION_DATA");
        return;
    }
    unsigned constant_value = 0U;

    // Due to the number of sources differing between Tofino and JBay in an action, respectively
    // 16 and 20, the range for instruction constants is different between architectures.
    // For Tofino it is -8..7 but for JBay it is -4..7
    int const_src_min = CONST_SRC_MAX;
#ifdef HAVE_JBAY
    if (Device::currentDevice() == Device::JBAY)
        const_src_min = JBAY_CONST_SRC_MIN;
#endif /* HAVE_JBAY */

    if (cont_action.convert_instr_to_bitmasked_set ||
        cont_action.convert_instr_to_byte_rotate_merge) {
        // Bitmasked-set or Byte-rotate-merge must be converted to action data
        cont_action.error_code |= ContainerAction::CONSTANT_TO_ACTION_DATA;
        LOG4("B. Error Code Update: CONSTANT_TO_ACTION_DATA");
        return;
    } else if (container.is(PHV::Kind::mocha)) {
        constant_value = cont_action.ci.build_constant();
        auto valid = valid_instruction_constant(constant_value, CONST_SRC_MAX,
                                                const_src_min, container.size());
        if (valid == EncodeConstant::NotPossible) {
             cont_action.error_code |= ContainerAction::CONSTANT_TO_ACTION_DATA;
             LOG4("C. Error Code Update: CONSTANT_TO_ACTION_DATA");
             return;
        }
        cont_action.ci.signExtend = valid == EncodeConstant::WithSignExtend;
    } else if (cont_action.name == "set" &&
               cont_action.ci.alignment.direct_write_bits.popcount()
                                            == static_cast<int>(container.size()) &&
               container.is(PHV::Kind::normal)) {
        // Converting to load_const instruction

        constant_value = cont_action.ci.build_constant();
        if (constant_value >= (1U << LOADCONST_MAX)) {
            cont_action.error_code |= ContainerAction::CONSTANT_TO_ACTION_DATA;
            LOG4("D. Error Code Update: CONSTANT_TO_ACTION_DATA");
            return;
        }
    } else if (cont_action.name != "set") {
        // Using the constant source directly in a non deposit-field operation
        constant_value = cont_action.ci.build_constant();
        auto valid = valid_instruction_constant(constant_value, CONST_SRC_MAX,
                                                const_src_min, container.size());
        if (valid == EncodeConstant::NotPossible) {
            cont_action.error_code |= ContainerAction::CONSTANT_TO_ACTION_DATA;
            LOG4("E. Error Code Update: CONSTANT_TO_ACTION_DATA");
            return;
        }
        cont_action.ci.signExtend = valid == EncodeConstant::WithSignExtend;
    } else {
        // Set or deposit-field
        constant_value = cont_action.ci.build_shiftable_constant();
        int constant_size = cont_action.ci.alignment.bitrange_contiguous_size();
        auto valid = valid_instruction_constant(constant_value, CONST_SRC_MAX, const_src_min,
                                                container.size(), constant_size);
        if (valid == EncodeConstant::NotPossible) {
            cont_action.error_code |= ContainerAction::CONSTANT_TO_ACTION_DATA;
            LOG4("F. Error Code Update: CONSTANT_TO_ACTION_DATA");
        } else {
            cont_action.ci.signExtend = valid == EncodeConstant::WithSignExtend;
        }
        if (constant_value > ((1U << CONST_SRC_MAX) - 1)) {
            cont_action.error_code |= ContainerAction::REFORMAT_CONSTANT;
            LOG4("Error Code Update: REFORMAT_CONSTANT");
        }
    }
    cont_action.ci.constant_value = constant_value;
    LOG4("Constant Value: " << cont_action.ci.constant_value);
}

void ActionAnalysis::ContainerAction::move_source_to_bit(safe_vector<int> &bit_uses,
        ActionAnalysis::TotalAlignment &ta) {
    for (auto alignment : ta.indiv_alignments) {
        for (int bit = alignment.write_bits.lo; bit <= alignment.write_bits.hi; bit++) {
            bit_uses[bit]++;
        }
    }
}


/** This checks to make sure that all bits are operated correctly, i.e. if the operation is
 *  a set, every bit in the write is either set once or not at all or e.g. add, subtract, or,
 *  each write bit has  either two read bits, or no read bits affecting it (overwrite
 *  constraints are checked in a different verification)
 */
bool ActionAnalysis::ContainerAction::verify_source_to_bit(int operands,
        PHV::Container container) {
    LOG5("Verifying source to bit for container " << container << ", operands: " << operands);
    /**
     * When combining a conditional-set with another set, the number of operands is not perfect
     * here, as the operands can be either one or two bits, i.e.
     *
     * conditional-set f1, f2, arg1, cond-arg
     * set f3, arg2
     *
     * How many sources, the condtional-set would have either two sources, but the normal
     * set would have one operand.  Potentially can be rewritten
     */
    if (name == "to-bitmasked-set")
        return true;

    safe_vector<int> bit_uses(container.size(), 0);

    for (auto &phv_ta : phv_alignment) {
        move_source_to_bit(bit_uses, phv_ta.second);
    }

    move_source_to_bit(bit_uses, adi.alignment);
    move_source_to_bit(bit_uses, ci.alignment);
    move_source_to_bit(bit_uses, extra_resize_reads);

    LOG5("  Bit uses: " << bit_uses);
    for (size_t i = 0; i < container.size(); i++) {
        if (!(bit_uses[i] == operands || bit_uses[i] == 0))
            return false;
    }

    LOG5("  Verified");
    return true;
}

/** Each PHV ALU can only pull from a local group of PHVs in an operation.  This guarantees that
 * this clustering constraint is met.
 */
bool ActionAnalysis::ContainerAction::verify_phv_mau_group(PHV::Container container) {
    auto write_container_id = Device::phvSpec().containerToId(container);
    auto write_group = Device::phvSpec().mauGroup(write_container_id);
    for (auto phv_ta : phv_alignment) {
        auto read_container = phv_ta.first;
        auto read_container_id = Device::phvSpec().containerToId(read_container);
        auto read_group = Device::phvSpec().mauGroup(read_container_id);
        if (write_group != read_group)
            return false;
    }
    return true;
}

/**
 * Verifies that the use of a speciality argument is valid
 *
 * For all cases:
 *     - At most one speciality is currently allowed per ALU operation (i.e. cannot combine
 *       HashDist with Control Plane or RNG)
 *
 * For NO_SPECIAL (anything still applies)
 *
 * For any non-HASH
 *     - Only one argument is currently allowed, as InstructionAdjustment is not able to
 *       handle all of the corner cases
 *
 * For METER_ALU
 *     - The parameter headed to an ALU operation must correctly fit within a single
 *       action bus slot
 *
 * For HASH:
 *     - Multiple Hash parameters as well as constants are allowed in the Hash.  In the
 *       future, PHV could be allowed in the Hash as well.
 */
bool ActionAnalysis::ContainerAction::verify_speciality(cstring &error_message,
         PHV::Container container, cstring action_name) {
    int ad_params = 0;
    ActionParam *speciality_read = nullptr;
    const IR::Expression *param = nullptr;
    for (auto &field_action : field_actions) {
        for (auto &read : field_action.reads) {
            if (read.type == ActionParam::ACTIONDATA || read.type == ActionParam::CONSTANT) {
                auto *expr = read.expr;
                if (auto *slice = expr->to<IR::Slice>()) {
                    expr = slice->e0;
                }
                if (expr != param) {
                    param = expr;
                    ad_params++;
                }
            }

            if (read.speciality != ActionParam::NO_SPECIAL) {
                speciality_read = &read;
            }
        }
    }

    BUG_CHECK(adi.specialities == specialities(), "Speciality alignments are not coordinated");
    if (specialities().empty())
        return true;

    bool impossible_to_merge = adi.specialities.popcount() > 1;
    impossible_to_merge |= !(specialities().getbit(ActionParam::HASH_DIST) ||
                             specialities().getbit(ActionParam::NO_SPECIAL))
                             && ad_params > 1;

    if (impossible_to_merge) {
        error_message += "In the ALU operation over container " + container.toString() +
                         " in action " + action_name +
                         ", the packing is "
                         "too complicated due to a too complex container instruction with a "
                         "speciality action data combined with other action data.";
        error_code |= MULTIPLE_SPECIALITIES;
        return false;
    }


    if (speciality_read && speciality_read->speciality == ActionParam::METER_ALU) {
        int lo = -1;  int hi = -1;
        if (auto sl = speciality_read->expr->to<IR::Slice>()) {
            lo = sl->getL();
            hi = sl->getH();
        } else {
            lo = 0;
            hi = speciality_read->size() - 1;
        }
        if (lo / container.size() != hi / container.size()) {
            error_message += "the alignment of the output of the meter ALU forces the required "
                             "data for the ALU operation to go to multiple action data bus "
                             "slots, while the ALU operation can only pull from one.";
            error_code |= ATTACHED_OUTPUT_ILLEGAL_ALIGNMENT;
            LOG4("Error Code Update: ATTACHED_OUTPUT_ILLEGAL_ALIGNMENT");
            return false;
        }
    }
    return true;
}

/** Due to shifts being fairly unique when it comes to action constraints, a separate function
 *  is required. The following constraints are verified:
 *     - Only one field_action is allowed per container (Not technically a constraint, but
 *       difficult to verify this otherwise
 *     - No action data or constants are allowed as a shift, only one PHV
 *     - The writes and reads are the only fields within their containers (Again not technically
 *       a constraint, but difficult to verify as well)
 */

bool ActionAnalysis::ContainerAction::verify_shift(cstring &error_message,
                                                   PHV::Container container,
                                                   const PhvInfo &phv) {
    if (field_actions.size() > 1) {
        error_code |= MULTIPLE_SHIFTS;
        LOG4("Error Code Update: MULTIPLE_SHIFTS");
        error_message += "p4c cannot support multiple shift instructions in one container";
        return false;
    }

    if (has_ad_or_constant()) {
        error_code |= ILLEGAL_ACTION_DATA;
        LOG4("Error Code Update: ILLEGAL_ACTION_DATA");
        error_message += "no action data or constant field is allowed in a shift function";
        return false;
    }

    int max_source = is_funnel_shift() ? 2 : 1;
    if (counts[ActionParam::PHV] > max_source) {
        error_code |= TOO_MANY_PHV_SOURCES;
        LOG4("Error Code Update: TOO_MANY_PHV_SOURCES");
        error_message += "this shift operation can't have more than " +
            cstring::to_cstring(max_source) + " PHV source(s)";
        return false;
    }

    LOG4("VERIFY SHIFT");
    total_overwrite_possible = verify_overwritten(container, phv);
    total_overwrite_possible |= verify_only_read(phv, max_source);
    LOG5("Total overwrite possible: " << (total_overwrite_possible ? "Y" : "N"));
    if (!total_overwrite_possible) {
        error_code |= ILLEGAL_OVERWRITE;
        LOG4("Error Code Update: ILLEGAL_OVERWRITE");
        error_message += "either a read or write in a shift operation is not the only field "
                         "within a container";
    }
    return true;
}


bool ActionAnalysis::ContainerAction::verify_mocha_and_dark(cstring &error_message,
        PHV::Container container) {
    if (container.is(PHV::Kind::normal))
        return true;
    std::string cont_type_name = container.is(PHV::Kind::mocha) ? "mocha" : "dark";
    int sources_needed = ad_sources() + counts[ActionParam::PHV];
    if (!(sources_needed == 1 && name == "set")) {
        error_code |= ILLEGAL_MOCHA_OR_DARK_WRITE;
        LOG4("Error Code Update: ILLEGAL_MOCHA_OR_DARK_WRITE");
        error_message += cont_type_name + " containers can only perform assignment operations "
            "from a single source";
        return false;
    }

    if (container.is(PHV::Kind::dark) && ad_sources() > 0) {
        error_code |= ILLEGAL_MOCHA_OR_DARK_WRITE;
        LOG4("Error Code Update: ILLEGAL_MOCHA_OR_DARK_WRITE");
        error_message += "dark containers can only be sourced from PHV containers";
        return false;
    }
    return true;
}

/** The goal of this function is to validate a container operation given the allocation
 *  the write fields and the sources of the particular operation.  The following checks are:
 *    - If the ALU operation requires too many sources
 *    - If the ALU operation has the wrong number of operands per bit
 *    - If the number of operands matches instruction number
 *    - If the PHV clustering algorithm is incorrect
 *    - If the alignment of the bits are allowed
 *    - If an overwriting operation overwrites the entire container
 *
 *  The reason these constraints exists are due to the Action ALU Instruction Set Architecture.
 *  If any of these are not true, then this instruction is not possible on Tofino.
 */
bool ActionAnalysis::ContainerAction::verify_possible(cstring &error_message,
                                                      PHV::Container container,
                                                      cstring action_name,
                                                      const PhvInfo &phv) {
    error_message = "In the ALU operation over container " + container.toString() +
                    " in action " + action_name + ", ";

    if (!phv.trivial_alloc()) {
        bool phv_group_correct = verify_phv_mau_group(container);
        if (!phv_group_correct) {
            error_code |= MAU_GROUP_MISMATCH;
            LOG4("Error Code Update: MAU_GROUP_MISMATCH");
            error_message += "a read phv is in an incompatible PHV group";
            return false;
        }
    }

    if (is_shift()) {
        return verify_shift(error_message, container, phv);
    }

    if (!verify_speciality(error_message, container, action_name)) {
        return false;
    }

    int actual_ad = ad_sources();
    int sources_needed = counts[ActionParam::PHV] + actual_ad;
    LOG3("Sources needed: " << sources_needed << "[ PHV: "
        << counts[ActionParam::PHV] << ", AD: " << actual_ad << " ]");

    if (sources_needed > 2) {
        if (actual_ad) {  // If action data as a source
            error_code |= PHV_AND_ACTION_DATA;
            LOG4("Error Code Update: PHV_AND_ACTION_DATA");
            error_message += "Action " + action_name + " writes fields using the same assignment "
                "type but different source operands (both action parameter and phv)";
        } else {          // no action data sources
            error_code |= TOO_MANY_PHV_SOURCES;
            LOG4("Error Code Update: TOO_MANY_PHV_SOURCES");
            error_message += "over 2 PHV sources for the ALU operation are required, thus rendering"
                " the action impossible";
        }
        return false;
    }

    if (!verify_mocha_and_dark(error_message, container))
        return false;

    bool source_to_bit_correct = verify_source_to_bit(operands(), container);
    if (!source_to_bit_correct) {
        if (is_from_set()) {
            error_code |= BIT_COLLISION_SET;
            LOG4("Error Code Update: BIT_COLLISION_SET");
        } else {
            error_code |= BIT_COLLISION;
            LOG4("Error Code Update: BIT_COLLISION");
        }
        error_message += "every write bit does not have a corresponding "
                         + cstring::to_cstring(operands()) + " or 0 read bits.";
        return false;
    }

    if (!is_from_set() && sources_needed != operands()) {
        // Some 2/3 operand instructions can have a source which is an uninitialized read. In this
        // case phv may allocate src/dst operands to the same PHV container slice
        // E.g.
        // Container H17(0..14) is occupied by both Field eg_md.txpgid & eg_md.pgid_16x3.cntrA_data
        // - or eg_md.txpgid, eg_md.txpgid, eg_md.pgid_16x2.cntrA_data
        // If eg_md.txpgid is an uninitialized read, then this phv allocation is valid, the
        // container is basically OR'ing itself which is a Noop and ideally should be eliminated
        //
        // However when code reaches here, it equates sources needed (PHV) as 1 and operands
        // required as 2 and flags an operand mismatch. This eventually causes the merge instruction
        // to fail and can create an "ALU ops cannot operate on slices" error.
        //
        // A new EliminateNoopInstructions (InstructionAdjustment) pass is now added to identify and
        // remove these operations as they are effectively Noops. This pass is invoked at the
        // beginning of InstructionAdjustment which should prevent the code from reaching here with
        // the invalid instruction
        error_code |= OPERAND_MISMATCH;
        LOG4("Error Code Update: OPERAND_MISMATCH");
        error_message += ("the number of operands(" + std::to_string(operands())
            + ") does not match the number of sources(" + std::to_string(sources_needed) + ")");
        return false;
    }

    bool aligned = verify_alignment(container);
    if (!aligned) {
        error_code |= IMPOSSIBLE_ALIGNMENT;
        LOG4("Error Code Update: IMPOSSIBLE_ALIGNMENT");
        error_message += "the alignment of fields within the container renders the action "
                         "impossible";
        return false;
    }

    bool check_overwrite = !(is_from_set() && container.is(PHV::Kind::normal));
    check_overwrite |= (is_from_set() && read_sources() == 2
                        && (!(convert_instr_to_deposit_field && is_background_source)));

    if (check_overwrite) {
        LOG4("Checking overwrite");
        total_overwrite_possible = verify_overwritten(container, phv);
        LOG5("Total overwrite possible: " << (total_overwrite_possible ? "Y" : "N"));
        if (!total_overwrite_possible) {
            error_code |= ILLEGAL_OVERWRITE;
            LOG4("Error Code Update: ILLEGAL_OVERWRITE");
            error_message += "the container is not completely overwritten when the operand is "
                             "over the entire container";
            return false;
        }
    }
    if (convert_instr_to_bitmasked_set || convert_instr_to_byte_rotate_merge) {
        error_code |= PARTIAL_OVERWRITE;
        LOG4("Error Code Update: PARTIAL_OVERWRITE");
    }

    return true;
}

void ActionAnalysis::postorder(const IR::MAU::Action *act) {
    Log::TempIndent indent;
    LOG3("ActionAnalysis postorder on action: " << act << indent);
    if (phv_alloc)
        verify_P4_action_with_phv(act->name);
    else
        verify_P4_action_without_phv(act->name);

    LOG5(*this);
}

std::ostream &operator<<(std::ostream &out, const ActionAnalysis::Alignment& a) {
    out << "align [ R: " << a.read_bits << ", W: " << a.write_bits << " ]";
    return out;
}

std::ostream &operator<<(std::ostream &out, const ActionAnalysis::ActionParam &ap) {
    out << "ActionParam: " << ap.expr
        << ", type: " << ap.get_type_string()
        << ", speciality: " << ap.get_speciality_string()
        << ", is cond: " << ap.is_conditional;
    return out;
}


std::ostream &operator<<(std::ostream &out, const ActionAnalysis::FieldAction &fa) {
    out << fa.name << " ";
    out << fa.write;
    for (auto &read : fa.reads)
        out << ", " << read;
    return out;
}

const std::vector<cstring> ActionAnalysis::ContainerAction::error_code_string_t = {
    // "NO_PROBLEM", // Set to 0 so not indexed
    "MULTIPLE_CONTAINER_ACTIONS"_cs,
    "READ_PHV_MISMATCH"_cs,
    "ACTION_DATA_MISMATCH"_cs,
    "CONSTANT_MISMATCH"_cs,
    "TOO_MANY_PHV_SOURCES"_cs,
    "IMPOSSIBLE_ALIGNMENT"_cs,
    "CONSTANT_TO_ACTION_DATA"_cs,
    "MULTIPLE_ACTION_DATA"_cs,
    "ILLEGAL_OVERWRITE"_cs,
    "BIT_COLLISION"_cs,
    "OPERAND_MISMATCH"_cs,
    "UNHANDLED_ACTION_DATA"_cs,
    "DIFFERENT_READ_SIZE"_cs,
    "MAU_GROUP_MISMATCH"_cs,
    "PHV_AND_ACTION_DATA"_cs,
    "PARTIAL_OVERWRITE"_cs,
    "MULTIPLE_SHIFTS"_cs,
    "ILLEGAL_ACTION_DATA"_cs,
    "REFORMAT_CONSTANT"_cs,
    "UNRESOLVED_REPEATED_ACTION_DATA"_cs,
    "ATTACHED_OUTPUT_ILLEGAL_ALIGNMENT"_cs,
    "CONSTANT_TO_HASH"_cs,
    "ILLEGAL_MOCHA_OR_DARK_WRITE"_cs,
    "BIT_COLLISION_SET"_cs,
};

void ActionAnalysis::ContainerAction::set_mismatch(ActionParam::type_t type) {
    if (type == ActionParam::PHV) {
        error_code |= READ_PHV_MISMATCH;
        LOG4("Error Code Update: READ_PHV_MISMATCH");
    } else if (type == ActionParam::ACTIONDATA) {
        error_code |= ACTION_DATA_MISMATCH;
        LOG4("Error Code Update: ACTION_DATA_MISMATCH");
    } else {
        error_code |= CONSTANT_MISMATCH;
        LOG4("Error Code Update: CONSTANT_MISMATCH");
    }
}

std::ostream &operator<<(std::ostream &out, const ActionAnalysis::ContainerAction &ca) {
    Log::TempIndent indent;
    out << "Container Action : " << ca.name << indent;
    out << Log::endl;

    out << "Field Actions : ";
    for (auto &fa : ca.field_actions) {
        out << fa << "; " << Log::endl;
    }

    out << "Error_code: " << std::hex << ca.error_code << std::dec;
    out << " - [ ";
    int i = 0;
    unsigned error_code = ca.error_code;
    while (error_code>>i) {
        if ((error_code>>i) & 0x1) {
            out << " " << ActionAnalysis::ContainerAction::error_code_string_t[i];
        }
       i++;
       if (i>=(int)ActionAnalysis::ContainerAction::error_code_string_t.size()) break;
    }
    out << " ]" << Log::endl;

    out << "Convert to [ Bitmasked Set : " << ca.convert_instr_to_bitmasked_set
        << " Deposit Field : "             << ca.convert_instr_to_deposit_field
        << " Byte Rotate Merge : "         << ca.convert_instr_to_byte_rotate_merge
        << " ]"                            << Log::endl;

    auto adi_specs = ca.adi.specialities;
    out << "Action Data ["
        << " HASH_DIST: "     << adi_specs.getbit(ActionAnalysis::ActionParam::HASH_DIST)
        << " METER COLOR: "   << adi_specs.getbit(ActionAnalysis::ActionParam::METER_COLOR)
        << " RANDOM: "        << adi_specs.getbit(ActionAnalysis::ActionParam::RANDOM)
        << " METER_ALU: "     << adi_specs.getbit(ActionAnalysis::ActionParam::METER_ALU)
        << " STFUL_COUNTER: " << adi_specs.getbit(ActionAnalysis::ActionParam::STFUL_COUNTER)
        << " ]"               << Log::endl;

    out << "Total Overwrite Possible: "  << (ca.total_overwrite_possible ? "Y" : "N")
        << " Deposit Field Variant: "    << (ca.is_deposit_field_variant  ? "Y" : "N")
        << " Implicit [ SRC1: "          << (ca.implicit_src1 ? "Y" : "N")
        << " SRC2: "                     << (ca.implicit_src2  ? "Y" : "N") << " ]"
        << " Impossible: "               << (ca.impossible ? "Y" : "N")
        << " Unhandled Action: "         << (ca.unhandled_action ? "Y" : "N")
        << " Constant To AD: "           << (ca.constant_to_ad ? "Y" : "N");

    return out;
}

std::ostream &operator<<(std::ostream &out, const ActionAnalysis::TotalAlignment &ta) {
    out << "Total Alignment : ["
        << " DRB : " << ta.direct_write_bits
        << " DWB : " << ta.direct_write_bits
        << " IRB : " << ta.implicit_read_bits
        << " IWB : " << ta.implicit_write_bits
        << " UCB : " << ta.unused_container_bits
        << " ]";
    out << " contiguous: " << (ta.contiguous() ? "Y" : "N");
    out << " aligned: " << (ta.aligned() ? "Y" : "N");
    out << " is_src1: " << (ta.is_src1 ? "Y" : "N");
    for (int i = 0; i < (int)ta.indiv_alignments.size(); i++) {
        if (i == 0) out << Log::endl;
        out << " - " << ta.indiv_alignments[i];
        if (i < ((int)ta.indiv_alignments.size() - 1))
            out << Log::endl;
    }
    return out;
}

std::ostream &operator<<(std::ostream &out, const ActionAnalysis &aa) {
    Log::TempIndent indent;
    out << "Action Analysis: "
        << " table: " << aa.get_table()->name << indent << Log::endl;
    if (auto *cam = aa.get_container_actions_map()) {
        out << "[ ";
        for (auto [cont, cont_act] : *cam)
            out << cont << ":" << cont_act;
        out << " ]";
    }
    out << " phv_alloc: " << (aa.get_phv_alloc() ? "Y" : "N")
        << " ad_alloc: " << (aa.get_ad_alloc() ? "Y" : "N")
        << " allow_unalloc: " << (aa.get_allow_unalloc() ? "Y" : "N")
        << " sequential: " << (aa.get_sequential() ? "Y" : "N")
        << " action_data_misaligned: " << (aa.get_action_data_misaligned() ? "Y" : "N")
        << " verbose: " << (aa.get_verbose() ? "Y" : "N")
        << " error_verbose: " << (aa.get_error_verbose() ? "Y" : "N")
        << " warning: " << (aa.warning_found() ? "Y" : "N")
        << " error: " << (aa.error_found() ? "Y" : "N");
    return out;
}

