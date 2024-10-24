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

#ifndef BACKENDS_TOFINO_BF_P4C_MAU_ACTION_FORMAT_H_
#define BACKENDS_TOFINO_BF_P4C_MAU_ACTION_FORMAT_H_

#include <array>
#include <iterator>
#include <map>

#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/mau/action_analysis.h"
#include "bf-p4c/mau/attached_info.h"
#include "bf-p4c/mau/ixbar_expr.h"
#include "bf-p4c/phv/phv.h"
#include "ir/ir.h"
#include "lib/bitops.h"
#include "lib/bitvec.h"
#include "lib/log.h"
#include "lib/ordered_set.h"
#include "lib/safe_vector.h"

namespace ActionData {

using namespace P4;
using ::P4::operator<<;  // avoid overload hiding issues

// MASK is currently not used, but maybe will be in order to reduce the size of the JSON
// If MASK is used, then the equiv_cond will have to change
enum ModConditionally_t { NONE, VALUE, /* MASK */ };

/**
 * The purpose of this class is the base abstract class for any parameter that can appear
 * as any section of bits on the action data bus.  This by itself does not coordinate to any
 * particular location in the bus or relationship to PHV, but is just information that can
 * be classified as ActionData
 */
class Parameter : public IHasDbPrint {
    /**
     * A modify_field_conditionally in p4-14, which is converted to a ternary operation in p4-16
     * is done in the following manner.  The instruction is converted to a bitmasked-set, with
     * the even action data bus location being the parameter, and the odd action data bus location
     * being the conditional mask.
     *
     *
     * bitmasked-set, described in uArch 15.1.3.8 Bit maskable set can be thought of as the
     * following: dest = (src1 & mask) | (src2 & ~mask)
     *
     * where the mask is directly stored on the RAM line, and is used on the action data bus.
     *
     * The mask is stored at the odd word, so in the case of a conditional set, the driver can
     * update the mask as it would any other parameter.  However, at the even word, it is not
     * just src1 stored, but src1 & mask.  The mask stored at the second word is used as
     * the background.  Thus, if the condition is true, src1 will equal the parameter, but if
     * the condition is false, src1 will be 0.
     *
     * This leads to the following issue.  Say you had a P4 action:
     *
     *    action a1(bit<8> p1, bool cond) {
     *        hdr.f1 = p1;
     *        hdr.f2 = cond ? p1 : hdr.f2;
     *    }
     *
     * While previously, the data space for parameter p1 could have been shared, because of the
     * condition, these have to be completely separate spaces on the RAM line, as the p1 in the
     * second instruction could sometimes be zero.  The rule is the following:
     *
     * Data is only equivalent when both the original parameter and the condition that is
     * controlling that parameter are equivalent.
     */
 protected:
    // Whether the value is controlled by a condition, or is part of conditional mask
    ModConditionally_t _cond_type = NONE;
    // The conditional that controls this value
    cstring _cond_name;

 public:
    // virtual const Parameter *shared_param(const Parameter *, int &start_bit) = 0;
    virtual ~Parameter() {}
    virtual int size() const = 0;
    virtual bool from_p4_program() const = 0;
    virtual cstring name() const = 0;
    virtual const Parameter *split(int lo, int hi) const = 0;
    virtual bool only_one_overlap_solution() const = 0;

    virtual bool is_next_bit_of_param(const Parameter *, bool same_alias) const = 0;
    virtual const Parameter *get_extended_param(uint32_t extension, const Parameter *) const = 0;
    virtual const Parameter *overlap(const Parameter *ad, bool guaranteed_one_overlap,
                                     le_bitrange *my_overlap, le_bitrange *ad_overlap) const = 0;
    virtual void dbprint(std::ostream &out) const = 0;

    virtual bool equiv_value(const Parameter *, bool check_cond = true) const = 0;
    virtual bool can_merge(const Parameter *param) const { return equiv_value(param); }
    virtual bool is_subset_of(const Parameter *param) const { return equiv_value(param); }
    virtual const Parameter *merge(const Parameter *param) const {
        if (param != nullptr) BUG_CHECK(can_merge(param), "Merging parameters that cannot merge");
        return this;
    }

    template <typename T>
    const T *to() const {
        return dynamic_cast<const T *>(this);
    }
    template <typename T>
    bool is() const {
        return to<T>() != nullptr;
    }

    bool is_cond_type(ModConditionally_t type) const { return type == _cond_type; }
    cstring cond_name() const { return _cond_name; }

    void set_cond(ModConditionally_t ct, cstring n) {
        _cond_type = ct;
        _cond_name = n;
    }

    void set_cond(const Parameter *p) {
        _cond_type = p->_cond_type;
        _cond_name = p->_cond_name;
    }

    bool equiv_cond(const Parameter *p) const {
        return _cond_type == p->_cond_type && _cond_name == p->_cond_name;
    }

    bool can_overlap_ranges(le_bitrange my_range, le_bitrange ad_range, le_bitrange &overlap,
                            le_bitrange *my_overlap, le_bitrange *ad_overlap) const;
};

/**
 * This class is to represent a slice of an IR::MAU::ActionArg, essentially any argument that
 * appears as a direct argument of an action and is used in an ALU operation:
 *
 * Let's take the following example:
 *
 * action act(bit<8> param1, bit<8> param2) {
 *     hdr.f1 = param1;
 *     hdr.f2 = param2;
 * }
 *
 * All valid bitranges of param1 and param2, e.g. param1[7:0] or param2[5:4] would be valid
 * instantiations of the Argument class
 */

class Argument : public Parameter {
    IR::ID _name;
    le_bitrange _param_field;

 public:
    cstring name() const override { return _name.name; }
    cstring originalName() const { return _name.originalName; }
    le_bitrange param_field() const { return _param_field; }

    Argument(IR::ID n, le_bitrange pf) : _name(n), _param_field(pf) {}
    virtual ~Argument() {}

    bool from_p4_program() const override { return true; }
    const Parameter *split(int lo, int hi) const override {
        le_bitrange split_range = {_param_field.lo + lo, _param_field.lo + hi};
        BUG_CHECK(_param_field.contains(split_range),
                  "Illegally splitting a parameter, as the "
                  "split contains bits outside of the range");
        auto *rv = new Argument(_name, split_range);
        rv->set_cond(this);
        return rv;
    }

    bool only_one_overlap_solution() const override { return true; }

    bool is_next_bit_of_param(const Parameter *ad, bool) const override;

    const Parameter *get_extended_param(uint32_t extension, const Parameter *) const override {
        auto rv = new Argument(*this);
        rv->_param_field.hi += extension;
        return rv;
    }

    const Parameter *overlap(const Parameter *ad, bool guaranteed_one_overlap,
                             le_bitrange *my_overlap, le_bitrange *ad_overlap) const override;
    bool equiv_value(const Parameter *ad, bool check_cond = true) const override {
        const Argument *arg = ad->to<Argument>();
        if (arg == nullptr) return false;
        if (check_cond && !equiv_cond(ad)) return false;
        return _name == arg->_name && _param_field == arg->_param_field;
    }
    int size() const override { return _param_field.size(); }
    void dbprint(std::ostream &out) const override {
        out << "Arg:" << _name << " " << _param_field;
    }
};

/**
 * This class represents a section of IR::MAU::ActionDataConstant, or an IR::Constant that
 * cannot be created as the src1 operand of an ALU operation.  The constant instead must come
 * from Action Ram.
 *
 * Similar to IR::Constant, the constant has a value and a bit size.  Due to the size of the
 * constant theoretically being infinite, a bitvec is used to store the value (though in
 * theory, an mpz_class could have been used as well
 */
class Constant : public Parameter {
    bitvec _value;
    size_t _size;
    cstring _alias;
    le_bitrange range() const { return {0, static_cast<int>(_size) - 1}; }

 public:
    Constant(int v, size_t sz) : _value(v), _size(sz) {}
    Constant(bitvec v, size_t sz) : _value(v), _size(sz) {}

    const Parameter *split(int lo, int hi) const override;
    bool only_one_overlap_solution() const override { return false; }
    bool from_p4_program() const override { return !_alias.isNull(); }
    bool is_next_bit_of_param(const Parameter *ad, bool same_alias) const override;
    const Parameter *get_extended_param(uint32_t extension, const Parameter *ad) const override;
    const Parameter *overlap(const Parameter *ad, bool only_one_overlap_solution,
                             le_bitrange *my_overlap, le_bitrange *ad_overlap) const override;
    bool equiv_value(const Parameter *ad, bool check_cond = true) const override;
    bitvec value() const { return _value; }
    int size() const override { return _size; }
    cstring name() const override {
        if (_alias) return _alias;
        return "$constant"_cs;
    }
    cstring alias() const { return _alias; }
    void set_alias(cstring a) { _alias = a; }
    void dbprint(std::ostream &out) const override {
        out << "Const: 0x" << _value << " : " << _size << " bits";
    }
};

class Hash : public Parameter {
    P4HashFunction _func;

 public:
    explicit Hash(const P4HashFunction &f) : _func(f) {}

    int size() const override { return _func.size(); }
    cstring name() const override { return _func.name(); }
    const Parameter *split(int lo, int hi) const override;
    bool only_one_overlap_solution() const override { return false; }
    bool from_p4_program() const override { return true; }
    bool is_next_bit_of_param(const Parameter *ad, bool same_alias) const override;
    const Parameter *get_extended_param(uint32_t extension, const Parameter *ad) const override;
    const Parameter *overlap(const Parameter *ad, bool only_one_overlap_solution,
                             le_bitrange *my_overlap, le_bitrange *ad_overlap) const override;
    bool equiv_value(const Parameter *ad, bool check_cond = true) const override;
    void dbprint(std::ostream &out) const override { out << "Hash: " << _func; }
    P4HashFunction func() const { return _func; }
};

/**
 * Represents an output from the Random Number Generator.  Up to 32 bits possible per logical
 * table, as this is the size of the immediate bits maximum position.
 *
 * Because all Random Numbers across all actions can possibly be merged, a set of
 * random numbers (in the initial case a single entry) represent all of the data at those
 * particular bits.  Because the bits are always random, there is not lo-hi, only size.
 */
class RandomNumber : public Parameter {
    class UniqueAlloc {
        cstring _random;
        cstring _action;

     public:
        bool operator<(const UniqueAlloc &ua) const {
            if (_random != ua._random) return _random < ua._random;
            return _action < ua._action;
        }

        bool operator==(const UniqueAlloc &ua) const {
            return _random == ua._random && _action == ua._action;
        }

        bool operator!=(const UniqueAlloc &ua) const { return !(*this == ua); }

        cstring random() const { return _random; }
        cstring action() const { return _action; }
        UniqueAlloc(cstring rand, cstring act) : _random(rand), _action(act) {}
    };

    std::map<UniqueAlloc, le_bitrange> _rand_nums;
    std::string rand_num_names() const {
        std::stringstream str;
        str << "{ ";
        std::string sep = "";
        for (auto ua : _rand_nums) {
            str << sep << ua.first.random() << "$" << ua.first.action() << ua.second;
            sep = ", ";
        }
        str << " }";
        return str.str();
    }

    RandomNumber() {}
    void add_alloc(cstring rand, cstring act, le_bitrange range) {
        UniqueAlloc ua(rand, act);
        _rand_nums.emplace(ua, range);
    }

 public:
    int size() const override;
    cstring name() const override { return "random"_cs; }
    const Parameter *split(int lo, int hi) const override;
    bool from_p4_program() const override { return true; }
    bool only_one_overlap_solution() const override { return false; }
    bool is_next_bit_of_param(const Parameter *ad, bool same_alias) const override;
    const Parameter *get_extended_param(uint32_t extension, const Parameter *ad) const override;
    const Parameter *overlap(const Parameter *ad, bool only_one_overlap_solution,
                             le_bitrange *my_overlap, le_bitrange *ad_overlap) const override;
    bool equiv_value(const Parameter *ad, bool check_cond = true) const override;
    bool rand_nums_overlap_into(const RandomNumber *rn) const;

    bool is_subset_of(const Parameter *ad) const override;
    bool can_merge(const Parameter *ad) const override;
    const Parameter *merge(const Parameter *ad) const override;
    void dbprint(std::ostream &out) const override { out << "Random: " << rand_num_names(); }

    RandomNumber(cstring rand, cstring act, le_bitrange range) {
        UniqueAlloc ua(rand, act);
        _rand_nums.emplace(ua, range);
    }
};

class RandomPadding : public Parameter {
    int _size;

 public:
    int size() const override { return _size; }
    cstring name() const override { return "rand_padding"_cs; }
    const Parameter *split(int lo, int hi) const override;
    bool from_p4_program() const override { return false; }
    bool only_one_overlap_solution() const override { return false; }
    bool is_next_bit_of_param(const Parameter *ad, bool same_alias) const override;
    const Parameter *get_extended_param(uint32_t extensions, const Parameter *ad) const override;
    const Parameter *overlap(const Parameter *ad, bool only_one_overlap_solution,
                             le_bitrange *my_overlap, le_bitrange *ad_overlap) const override;
    bool equiv_value(const Parameter *ad, bool check_cond = true) const override;
    bool is_subset_of(const Parameter *ad) const override;
    bool can_merge(const Parameter *ad) const override;
    const Parameter *merge(const Parameter *ad) const override;
    void dbprint(std::ostream &out) const override {
        out << "RndPad: " << name() << " : " << size();
    }
    explicit RandomPadding(int s) : _size(s) {}
};

/**
 * A Meter Color Map RAM outputs an 8-bit color to bits 24..31 of immediate.  Only one meter
 * can currently be connected to a single table's logical table (though perhaps later maybe
 * more than one meter can be accessed).  Similar to random number, the entire 8-bits of
 * output is used by the meter color, thus nothing else can be allocated to those 8 bits.
 *
 * However, unlike Random, because only one meter can exist per logical table, there is no
 * need to complicate the merge and subset functions
 */
class MeterColor : public Parameter {
    cstring _meter_name;
    le_bitrange _range;

 public:
    le_bitrange range() const { return _range; }
    int size() const override { return _range.size(); }
    bool from_p4_program() const override { return true; }
    cstring name() const override { return _meter_name; }
    const Parameter *split(int lo, int hi) const override;
    bool only_one_overlap_solution() const override { return true; }
    bool is_next_bit_of_param(const Parameter *, bool same_alias) const override;
    const Parameter *get_extended_param(uint32_t extension, const Parameter *) const override;
    const Parameter *overlap(const Parameter *ad, bool only_one_overlap_solution,
                             le_bitrange *my_overlap, le_bitrange *ad_overlap) const override;
    void dbprint(std::ostream &out) const override {
        out << "MeterColor: " << _meter_name << _range;
    }
    bool equiv_value(const Parameter *, bool check_cond = true) const override;
    bool is_padding() const { return _meter_name == "$padding"; }

    bool is_subset_of(const Parameter *ad) const override;
    bool can_merge(const Parameter *ad) const override;
    const Parameter *merge(const Parameter *ad) const override;
    MeterColor(cstring mn, le_bitrange r) : _meter_name(mn), _range(r) {}
};

/**
 * As in the name, this represents data that comes from the Meter/Stateful ALU directly, not
 * meter color.  LPF, WRED, and StatefulAlu operations directly write to the home row
 * action bus when activated, and differ in meter color.
 *
 * Similar to Meter Color, the position of the actual parameter is fixed in hardware on where
 * the output will be appearing.  However, because nothing besides the Meter ALU output is on
 * that particular row, and is never packed with anything else, no padding is necessary.
 *
 * @sa attached_output.cpp and create_meter_alu_RamSection for more details
 */
class MeterALU : public Parameter {
    cstring _alu_user;
    le_bitrange _range;

 public:
    le_bitrange range() const { return _range; }
    int size() const override { return _range.size(); }
    bool from_p4_program() const override { return true; }
    cstring name() const override { return _alu_user; }
    bool only_one_overlap_solution() const override { return true; }
    const Parameter *split(int lo, int hi) const override;
    bool is_next_bit_of_param(const Parameter *, bool) const override;
    const Parameter *get_extended_param(uint32_t extension, const Parameter *) const override;
    const Parameter *overlap(const Parameter *ad, bool only_one_overlap_solution,
                             le_bitrange *my_overlap, le_bitrange *ad_overlap) const override;
    void dbprint(std::ostream &out) const override { out << "MeterALU: " << _alu_user << _range; }
    bool equiv_value(const Parameter *, bool check_cond = true) const override;
    MeterALU(cstring au, le_bitrange r) : _alu_user(au), _range(r) {}
};

struct ALUParameter {
    const Parameter *param;
    le_bitrange phv_bits;
    // @seealso ALUOperation::read_bits

    /* actually a rotate, not a shift -- the number of bits the destination PHV needs to
     * be rotated by to match up with this source.  So it turns out to be the number of bits
     * this parameter needs to left-rotate */
    int right_shift;

    safe_vector<le_bitrange> slot_bits_brs(PHV::Container cont) const;
    bool is_wrapped(PHV::Container cont) const { return slot_bits_brs(cont).size() > 1; }

    ALUParameter(const Parameter *p, le_bitrange pb) : param(p), phv_bits(pb), right_shift(0) {}

    friend std::ostream &operator<<(std::ostream &out, const ALUParameter &p) {
        return out << "ALU Param { " << p.param << ", phv bits : 0x" << p.phv_bits
                   << ", rotate_right_shift : " << p.right_shift << " } ";
    }
};

enum ALUOPConstraint_t { ISOLATED, BITMASKED_SET, DEPOSIT_FIELD, BYTE_ROTATE_MERGE };
std::ostream &operator<<(std::ostream &out, ALUOPConstraint_t c);

struct UniqueLocationKey {
    cstring action_name;
    const Parameter *param = nullptr;
    PHV::Container container;
    le_bitrange phv_bits;

 public:
    UniqueLocationKey() {}

    UniqueLocationKey(cstring an, const Parameter *p, PHV::Container cont, le_bitrange pb)
        : action_name(an), param(p), container(cont), phv_bits(pb) {}
};

class RamSection;

using ParameterPositions = std::map<int, const Parameter *>;

/**
 * This class is the representation of the src1 of a single ALU operation when the src1 comes
 * from the Action Data Bus.  Essentially a single PHV instruction could be represented as,
 * (somewhat):
 *      PHV Container number(phv_bits) = function(ADB SLOT number(slot_bits))
 *
 * This class represents the data contained within slot-hi to slot-lo.  What is known at
 * Action Format allocation are both which container the data is headed to and which bits
 * of the container are used.
 *
 * The purpose of the allocation is to both figure out the slot_bits ( which is just the PHV
 * bits barrel_shifted right by a certain amount), as well as a potential RAM location to store
 * this Action Data.
 */
class ALUOperation {
    // The location of the P4 parameters, i.e. what bits contain particular bitranges.  This
    // is knowledge required for the driver to pack the RAM
    safe_vector<ALUParameter> _params;
    // The bits that are to be written in the PHV Container
    bitvec _phv_bits;
    // The amount to barrel-shift right the phv_bits in order to know the associated slot_bits. One
    // can think of the phv_bits as the write_bits and the slot_bits as the read_bits.  In a
    // deposit-field instruction, the right rotate is actually the reverse, i.e. the right rotation
    // of the read bits in order to align them to the write bits
    int _right_shift;
    bool _right_shift_set = false;
    PHV::Container _container;
    // Information on how the data is used, in order to potentially pack in RAM space other
    // ALUOperations
    ALUOPConstraint_t _constraint;
    // Alias name for the assembly language
    cstring _alias;
    // Mask alias for the assembly language
    cstring _mask_alias;
    // Used for modify field conditionally
    safe_vector<ALUParameter> _mask_params;
    bitvec _mask_bits;
    // Explicitly needed for action parameters shared between actions, i.e. hash, rng
    cstring _action_name;

 public:
    void add_param(ALUParameter &ap) {
        _params.push_back(ap);
        _phv_bits.setrange(ap.phv_bits.lo, ap.phv_bits.size());
    }

    void add_mask_param(ALUParameter &ap) {
        _mask_params.push_back(ap);
        _mask_bits.setrange(ap.phv_bits.lo, ap.phv_bits.size());
    }

    bool contains_only_one_overlap_solution() const {
        for (auto param : _params) {
            if (param.param->only_one_overlap_solution()) return true;
        }
        return false;
    }

    bitvec phv_bits() const { return _phv_bits; }
    bitvec phv_bytes() const;
    bitvec mask_bits() const { return _mask_bits; }
    bitvec slot_bits() const {
        BUG_CHECK(_right_shift_set, "Unsafe call of slot bits in action format");
        if (_container)
            return _phv_bits.rotate_right_copy(0, _right_shift, _container.size());
        else
            return _phv_bits >> _right_shift;
    }

    bool valid() const { return static_cast<bool>(_container); }
    size_t size() const {
        return valid() ? _container.size() : static_cast<size_t>(PHV::Size::b32);
    }
    bool is_constrained(ALUOPConstraint_t cons) const { return _constraint == cons; }
    size_t index() const {
        BUG_CHECK(valid(), "Cannot call index on an invalid ALU operation");
        return ceil_log2(size()) - 3;
    }
    ALUOPConstraint_t constraint() const { return _constraint; }

    ALUOperation(PHV::Container cont, ALUOPConstraint_t cons)
        : _right_shift(0), _container(cont), _constraint(cons) {}

    const RamSection *create_RamSection(bool shift_to_lsb) const;
    const ALUOperation *add_right_shift(int right_shift, int *rot_alias_idx) const;
    cstring alias() const { return _alias; }
    cstring mask_alias() const { return _mask_alias; }
    cstring action_name() const { return _action_name; }
    void set_alias(cstring a) { _alias = a; }
    void set_mask_alias(cstring ma) { _mask_alias = ma; }
    void set_action_name(cstring an) { _action_name = an; }
    PHV::Container container() const { return _container; }
    const ALUParameter *find_param_alloc(UniqueLocationKey &key) const;
    ParameterPositions parameter_positions() const;
    std::string parameter_positions_to_string() const;
    cstring wrapped_constant() const;
    bitvec static_entry_of_arg(const Argument *arg, bitvec value) const;
    bitvec static_entry_of_constants() const;

    ///> \sa comments on create_meter_color_RamSection
    template <typename T>
    bool has_param() const {
        for (auto param : _params) {
            if (param.param->is<T>()) return true;
        }
        return false;
    }
    int hw_right_shift() const;
    const RamSection *create_meter_color_RamSection() const;
    const RamSection *create_meter_alu_RamSection() const;
    bool is_right_shift_from_hw() const { return has_param<MeterColor>() || has_param<MeterALU>(); }
    bool right_shift_set() const { return _right_shift_set; }

    friend std::ostream &operator<<(std::ostream &out, const ALUOperation &op) {
        Log::TempIndent indent;
        out << "ALU Operation { " << indent << Log::endl;
        out << "params:" << indent;
        for (auto &p : op._params) out << Log::endl << p;
        out << indent.pop_back() << Log::endl;
        out << "phv bits: " << op._phv_bits << ", right_shift : " << op._right_shift
            << ", right_shift_set: " << op._right_shift_set << ", container: " << op._container
            << ", op constraint: " << op._constraint << Log::endl;
        if (op._alias) out << "alias: " << op._alias << Log::endl;
        if (op._mask_alias) out << "mask alias: " << op._mask_alias << Log::endl;
        if (op._mask_params.size() > 0) {
            out << "mask params: " << Log::indent;
            for (auto &p : op._mask_params) out << Log::endl << p;
            out << Log::unindent << Log::endl;
        }
        out << "mask bits: " << op._mask_bits << ", action name: " << op._action_name;
        out << " }";
        return out;
    }
    friend std::ostream &operator<<(std::ostream &out, const ALUOperation *op) {
        if (op) out << *op;
        return out;
    }
    void dbprint_multiline() const {}
};

struct SharedParameter {
    const Parameter *param;
    int a_start_bit;
    int b_start_bit;

    SharedParameter(const Parameter *p, int a, int b) : param(p), a_start_bit(a), b_start_bit(b) {}
    friend std::ostream &operator<<(std::ostream &out, const SharedParameter &op) {
        out << "Param: " << *op.param << "A :" << op.a_start_bit << " B:" << op.b_start_bit;
        return out;
    }
};

using bits_iter_t = safe_vector<const Parameter *>::iterator;

/**
 * The next two classes hold information passed down recursively through any PackingConstraint
 * function call, in order to understand the size of the recursive_constraints vector and
 * how to interpret information.
 */
struct RotationInfo {
 private:
    bits_iter_t bits_begin;
    bits_iter_t bits_end;

 public:
    RotationInfo(safe_vector<const Parameter *>::iterator bb,
                 safe_vector<const Parameter *>::iterator be)
        : bits_begin(bb), bits_end(be) {}

    int size() const { return bits_end - bits_begin; }
    void rotate(int bit_rotation);
    RotationInfo split(int first_bit, int sz) const;
};

struct LocalPacking {
    ///> How wide the packing structure is
    int bit_width;
    ///> The bits that are enabled
    bitvec bits_in_use;

    bool empty() const { return bits_in_use.empty(); }
    le_bitrange max_br_in_use() const {
        return {bits_in_use.min().index(), bits_in_use.max().index()};
    }
    LocalPacking(int bw, bitvec b) : bit_width(bw), bits_in_use(b) {}
    LocalPacking split(int first_bit, int sz) const;

    void dbprint(std::ostream &out) const { out << "bits: " << bit_width << " 0x" << bits_in_use; }
};

/**
 * Recursive Constraint indicating how data can be moved and rotated in the ActionDataRAMSection.
 * This is due to the multiple levels of action data headed to potentially different sizes of
 * ALU operations.  Please read the RamSection comments for further details.
 */
class PackingConstraint {
    ///> The ActionDataRAMSection can be barrel-shifted by any rotation where:
    ///> rotation % rotational_granularity == 0
    int rotational_granularity = -1;

    ///> Information on rotational ability of potential bytes/half-words underneath
    ///> the rotational_granularity
    safe_vector<PackingConstraint> recursive_constraints;

    bool under_rotate(LocalPacking &lp, le_bitrange open_range, int &final_bit,
                      int right_shift) const;
    bool over_rotate(LocalPacking &lp, le_bitrange open_range, int &final_bit,
                     int right_shift) const;

 public:
    PackingConstraint expand(int current_size, int expand_size) const;
    bool is_rotational() const { return rotational_granularity > 0; }
    bool can_rotate(int bit_width, int init_bit, int final_bit) const;
    void rotate(RotationInfo &ri, int init_bit, int final_bit);
    bool can_rotate_in_range(LocalPacking &lp, le_bitrange open_range, int &final_bit) const;
    int get_granularity() const { return rotational_granularity; }
    int bit_rotation_position(int bit_width, int init_bit, int final_bit, int init_bit_comp) const;
    safe_vector<PackingConstraint> get_recursive_constraints() const {
        return recursive_constraints;
    }

    PackingConstraint merge(const PackingConstraint &pc, const LocalPacking &lp,
                            const LocalPacking &pc_lp) const;

    PackingConstraint() {}
    PackingConstraint(int rg, safe_vector<PackingConstraint> &pc)
        : rotational_granularity(rg), recursive_constraints(pc) {}
    friend std::ostream &operator<<(std::ostream &out, const PackingConstraint &pc) {
        out << "Packing Constraint {" << " Rotational Granularity: " << pc.rotational_granularity
            << " Recursive Constraint (size): " << pc.recursive_constraints.size() << " } ";
        return out;
    }
};

enum SlotType_t { BYTE, HALF, FULL, SLOT_TYPES, DOUBLE_FULL = SLOT_TYPES, SECT_TYPES = 4 };
std::ostream &operator<<(std::ostream &, SlotType_t);
size_t slot_type_to_bits(SlotType_t type);
SlotType_t bits_to_slot_type(size_t bits);

using BusInputs = std::array<bitvec, SLOT_TYPES>;

/**
 * This class represents the positions of data in an entry of RAM.
 *
 * During the condense portion of the algorithm, each RamSection has not been assigned
 * a position on either an ActionData RAM, or a portion of immediate.  Rather each
 * RamSection can be thought of as a vector of bits that have a size, and that this
 * size constrains where this data can eventually appear on a RAM line.
 *
 * Note the following constraint on ALU operations:
 *
 *     - For an ALU operation PHV_Container[hi..lo] = f(ADB[adb_hi..adb_lo])
 *       the action data source must appear at most in a single action data bus location
 *
 * This adb_hi and adb_lo coordinate to a action_RAM_hi and action_RAM_lo, as data gets pulled
 * from the Action Data Table through the Action Data Bus and finally to an ALU.
 *
 * The constraint arising is:
 *     action_RAM_hi / size(PHV_Container) == action_RAM_lo / size(PHV_Container), integer division
 *
 * This guarantees that the data appears in the same action data bus slot.
 *
 * Thus the action_data_bits vector, which will be of size 8, 16, or 32 (or 64 explained later),
 * will eventually have to be packed in the Action RAM on a mod_offset of its size, as it can
 * be thought of as a source of RAM bits for an ALU operation.
 *
 * It is more extensible than the ALUOperation, as a section of RAM can be sent to
 * multiple ALUs.  Furthermore, portions of a ActionDataRAMSection can be sent to a single ALU.
 * Say for instance, the following happens.
 *
 * 32 bits of action data:
 *    The first byte can be sent to an 8 bit ALU.
 *    The third and fourth byte can be sent to a 16 bit ALU.
 *    The entire four bytes can be sent to a 32 bit ALU.
 *
 * This would capture 3 separate ALUOperation operations within an RamSection.
 * Furthermore, the constraints mentioned about action_RAM_lo and action_RAM_hi would have to
 * be captured for all 3 ALUOperation operations.  This is the purpose of the
 * PackingConstraint, which has recursive information on how data can possibly be moved in order
 * to more efficiently pack data.
 *
 * Lastly, because bitmasked-sets need to appear in even-odd pairs of ActionDataBus slots, the
 * algorithm currently requires the allocation of these to be contiguous, as contiguous RAM
 * bytes/half-words/full-words are much easier to enforce contiguity on the ActionDataBus.  This
 * leads to up to 64 bit (2 * 32 bit sources)
 */
class RamSection {
    ///> A vector of Parameter data bits that are of size 1/nullptr if that bit is not used
    safe_vector<const Parameter *> action_data_bits;
    ///> Recursive PackingConstraint on how data can rotate
    PackingConstraint pack_info;

    bool is_better_merge_than(const RamSection *comp) const;

 public:
    ///> map of ALUOperation operations with their action data requirements allocated
    ///> in this RAMSection
    safe_vector<const ALUOperation *> alu_requirements;
    // Made public for unit tests.  Not sure if there is a way around this
    bitvec bits_in_use() const;
    safe_vector<le_bitrange> open_holes() const;
    const RamSection *expand_to_size(size_t expand_size) const;
    const RamSection *can_rotate(int init_bit, int final_bit) const;
    const RamSection *rotate_in_range(le_bitrange range) const;
    const RamSection *no_overlap_merge(const RamSection *ad) const;
    const RamSection *merge(const RamSection *ad) const;
    const RamSection *condense(const RamSection *) const;
    const RamSection *better_merge_cand(const RamSection *) const;
    void gather_shared_params(const RamSection *ad, safe_vector<SharedParameter> &shared_params,
                              bool only_one_overlap_solution) const;
    PackingConstraint get_pack_info() const { return pack_info; }

    bool is_data_subset_of(const RamSection *ad) const;
    bool contains(const RamSection *ad_small, int init_bit_pos, int *final_bit_pos) const;
    bool contains_any_rotation_from_0(const RamSection *ad_small, int init_bit_pos,
                                      int *final_bit_pos) const;
    bool contains(const ALUOperation *ad_alu, int *first_phv_bit_pos = nullptr) const;

    size_t size() const { return action_data_bits.size(); }
    size_t index() const { return ceil_log2(size()) - 3; }
    size_t byte_sz() const { return size() / 8; }
    ParameterPositions parameter_positions(bool same_alias = false) const;
    std::string parameter_positions_to_string(bool from_p4_program) const;
    explicit RamSection(int s) : action_data_bits(s, nullptr) {}
    RamSection(int s, PackingConstraint &pc) : action_data_bits(s, nullptr), pack_info(pc) {}
    void add_param(int bit, const Parameter *);
    void add_alu_req(const ALUOperation *rv) { alu_requirements.push_back(rv); }
    BusInputs bus_inputs() const;
    std::string get_action_data_bits_str() const {
        std::stringstream rv;
        // Merge action data bits to a slice if possible
        std::vector<std::pair<cstring, le_bitrange>> adb_params;
        for (auto *adb_param : action_data_bits) {
            if (!adb_param) continue;
            if (auto *arg_param = adb_param->to<Argument>()) {
                if (adb_params.size() != 0) {
                    auto &last_param = adb_params.back();
                    if (last_param.first == adb_param->name()) {
                        last_param.second |= arg_param->param_field();
                        continue;
                    }
                }
                adb_params.push_back(std::make_pair(adb_param->name(), arg_param->param_field()));
            }
        }
        rv << "Action Data : " << size() << "'b{";
        for (auto &a : adb_params) rv << " " << a.first << " : " << a.second;
        rv << " }";
        return rv.str();
    }

    friend std::ostream &operator<<(std::ostream &out, const RamSection &rs) {
        Log::TempIndent indent;
        out << Log::endl;
        out << "Ram Section {" << indent << Log::endl;
        out << rs.get_action_data_bits_str() << Log::endl;
        out << "Pack Info: " << indent << rs.pack_info << indent.pop_back() << Log::endl;
        out << "Alu Req: " << indent << rs.alu_requirements;
        out << " }";
        return out;
    }
    void dbprint_multiline() const {}
};

// Actual locations are ACTION_DATA_TABLE, IMMEDIATE & METER_ALU
// AD_LOCATIONS / ALL_LOCATIONS provide a way to loop over desired locations
// for (loc = 0; loc < AD_LOCATIONS; ++loc) to iterate over action data locations or
// for (loc = 0; loc < ALL_LOCATIONS; ++loc) to iterate over all location types
enum Location_t { ACTION_DATA_TABLE, IMMEDIATE, AD_LOCATIONS, METER_ALU = 2, ALL_LOCATIONS = 3 };
std::ostream &operator<<(std::ostream &, Location_t);

/**
 * Used to keep track of the coordination of RamSections and the byte offset
 * in the RAM, either ActionData or Immediate based on the allocation scheme
 */
struct RamSectionPosition {
    const RamSection *section;
    int byte_offset = -1;
    explicit RamSectionPosition(const RamSection *sect) : section(sect) {}
    size_t total_slots_of_type(SlotType_t slot_type) const;
    friend std::ostream &operator<<(std::ostream &out, const RamSectionPosition &rsp) {
        if (rsp.section)
            out << "Ram Section Position { " << *rsp.section << ", byte_offset: " << rsp.byte_offset
                << " } ";
        return out;
    }
    void dbprint_multiline() const {}
};

/**
 * The allocation of all RamSections for a single action.
 *
 * Please note the difference between slots/sections.  The slots represent the action data xbar
 * input slots, which come from the position of ALUOperation objects, while the section
 * denote the RamSections, built up of many ALUOperation objects.
 */
struct SingleActionPositions {
    cstring action_name;
    safe_vector<RamSectionPosition> all_inputs;

    SingleActionPositions(cstring an, safe_vector<RamSectionPosition> ai)
        : action_name(an), all_inputs(ai) {}

    size_t total_slots_of_type(SlotType_t slot_type) const;
    size_t bits_required() const;
    size_t adt_bits_required() const;
    std::array<int, SECT_TYPES> sections_of_size() const;
    std::array<int, SECT_TYPES> minmax_bit_req() const;
    std::array<int, SECT_TYPES> min_bit_of_contiguous_sect() const;
    std::array<bool, SECT_TYPES> can_be_immed_msb(int bits_to_move) const;
    void move_to_immed_from_adt(safe_vector<RamSectionPosition> &rv, size_t slot_sz,
                                bool move_min_max_bit_req);
    void move_other_sections_to_immed(int bits_to_move, SlotType_t minmax_sz,
                                      safe_vector<RamSectionPosition> &immed_vec);
    safe_vector<RamSectionPosition> best_inputs_to_move(int bits_to_move);
    friend std::ostream &operator<<(std::ostream &out, const SingleActionPositions &sap) {
        out << "Single Action Position { name: " << sap.action_name
            << ", all_inputs: " << Log::TempIndent() << sap.all_inputs << " } ";
        return out;
    }
};

using AllActionPositions = safe_vector<SingleActionPositions>;

/**
 * Information on the position of a single ALUOperation on a RAM line.  Due
 * to the constraint, that all of the action data in a single ALU operation has to appear
 * in a single Action Data Bus slot.  Due to the direct extraction through the homerow bus,
 * the following constraint must be satisfied:
 *
 *     - slot_bit_lo / container_size == slot_bit_hi / container_size (integer division)
 *
 * This means that the entirety of a single ALU operation starts a byte and is a particular
 * size.  The start can either be in ActionDataTable RAM or Match RAM (Immediate)
 *
 * Specifically for anything that comes from a Meter ALU, a third location has been provided
 */
struct ALUPosition {
    // The information on the P4 parameters within this region
    const ALUOperation *alu_op;
    // Whether the data is in ActionData, Immmediate, or a from a Meter ALU operation
    Location_t loc;
    // The byte offset within the allocation
    size_t start_byte;

 public:
    ALUPosition(const ALUOperation *ao, Location_t l, size_t sb)
        : alu_op(ao), loc(l), start_byte(sb) {}

    friend std::ostream &operator<<(std::ostream &out, const ALUPosition &pos) {
        out << "ALU Position" << Log::indent << Log::endl
            << "op: " << *pos.alu_op << Log::endl
            << "loc: " << pos.loc << Log::endl
            << "start_byte: " << pos.start_byte << Log::unindent;
        return out;
    }
};

using RamSec_vec_t = safe_vector<const RamSection *>;
using PossibleCondenses = safe_vector<RamSec_vec_t>;

/**
 * Structure to keep track of a single action allocation for it's action data for a single
 * possible layout.  The orig_inputs is anything that hasn't been yet allocated, and the
 * alloc_inputs is anything that has been allocated during the determination process
 */
struct SingleActionAllocation {
    SingleActionPositions *positions;
    BusInputs *all_action_inputs;
    BusInputs current_action_inputs = {{bitvec(), bitvec(), bitvec()}};
    safe_vector<RamSectionPosition> orig_inputs;
    safe_vector<RamSectionPosition> alloc_inputs;

    void clear_inputs() {
        orig_inputs.clear();
        alloc_inputs.clear();
    }
    void build_orig_inputs() {
        clear_inputs();
        orig_inputs.insert(orig_inputs.end(), positions->all_inputs.begin(),
                           positions->all_inputs.end());
    }

    void set_positions() {
        BUG_CHECK(orig_inputs.empty() && alloc_inputs.size() == positions->all_inputs.size(),
                  "Cannot set the positions of the action as not all have been assigned positions");
        positions->all_inputs.clear();
        positions->all_inputs.insert(positions->all_inputs.end(), alloc_inputs.begin(),
                                     alloc_inputs.end());
    }
    cstring action_name() const { return positions->action_name; }

    SingleActionAllocation(SingleActionPositions *sap, BusInputs *aai)
        : positions(sap), all_action_inputs(aai) {
        build_orig_inputs();
    }
};

// Really could be an Alloc1D of bitvec of size 2, but Alloc1D couldn't hold bitvecs?
using ModCondMap = std::map<cstring, safe_vector<bitvec>>;

class Format {
 public:
    static constexpr int IMMEDIATE_BITS = 32;
    static constexpr int ACTION_RAM_BYTES = 16;
    static constexpr int METER_COLOR_SIZE = 8;
    static constexpr int METER_COLOR_START_BIT = IMMEDIATE_BITS - METER_COLOR_SIZE;

    struct Use {
        std::map<cstring, safe_vector<ALUPosition>> alu_positions;
        ///> A bitvec per each SlotType / Location representing the use of each SlotType on
        ///> input.  Could be calculated directly from alu_positions
        std::array<BusInputs, AD_LOCATIONS> bus_inputs = {
            {{{bitvec(), bitvec(), bitvec()}}, {{bitvec(), bitvec(), bitvec()}}}};
        std::array<int, AD_LOCATIONS> bytes_per_loc = {{0, 0}};
        ///> A contiguous bits of action data in match overhead.  Could be calculated directly
        ///> from alu_positions
        bitvec immediate_mask;
        ///> Which FULL words are to be used in a bitmasked-set, as those have to be
        ///> contiguous on the action data bus words.  Could be calculated directly from
        ///> alu_positions
        bitvec full_words_bitmasked;

        std::map<cstring, ModCondMap> mod_cond_values;

        safe_vector<ALUPosition> locked_in_all_actions_alu_positions;
        BusInputs locked_in_all_actions_bus_inputs = {{bitvec(), bitvec(), bitvec()}};

        void determine_immediate_mask();
        void determine_mod_cond_maps();
        int immediate_bits() const { return immediate_mask.max().index() + 1; }
        const ALUParameter *find_locked_in_all_actions_param_alloc(
            UniqueLocationKey &loc, const ALUPosition **alu_pos_p) const;
        const ALUParameter *find_param_alloc(UniqueLocationKey &loc,
                                             const ALUPosition **alu_pos_p) const;

        cstring get_format_name(const ALUPosition &alu_pos, bool bitmasked_set = false,
                                le_bitrange *slot_bits = nullptr,
                                le_bitrange *postpone_range = nullptr) const;
        cstring get_format_name(SlotType_t slot_type, Location_t loc, int byte_offset,
                                le_bitrange *slot_bits = nullptr,
                                le_bitrange *postpone_range = nullptr) const;

        void clear() {
            alu_positions.clear();
            bus_inputs = {{{{bitvec(), bitvec(), bitvec()}}, {{bitvec(), bitvec(), bitvec()}}}};
            immediate_mask.clear();
            full_words_bitmasked.clear();
            mod_cond_values.clear();
            locked_in_all_actions_alu_positions.clear();
            locked_in_all_actions_bus_inputs = {{bitvec(), bitvec(), bitvec()}};
        }
        bool if_action_has_action_data(cstring action_name) const;
        bool if_action_has_action_data_table(cstring action_name) const;

        // For templated methods, the declaration has to be in the same location as the
        // implementation?  C++ is weird
        template <typename T>
        bool is_byte_offset(int byte_offset) const {
            bool found = false;
            bool is_template = false;
            for (auto &alu_position : locked_in_all_actions_alu_positions) {
                if (alu_position.start_byte != static_cast<unsigned>(byte_offset)) continue;
                auto param_positions = alu_position.alu_op->parameter_positions();
                BUG_CHECK(!param_positions.empty(), "An ALU operation somehow has no parameters");
                for (auto &param_pos : param_positions) {
                    bool template_param = param_pos.second->is<T>();
                    if (!found) {
                        found = true;
                        is_template = template_param;
                    } else {
                        BUG_CHECK(is_template == template_param,
                                  "A special parameter, i.e. "
                                  "HashDist, shares its packing with a different speciality");
                    }
                }
            }
            return is_template;
        }
        const RamSection *build_locked_in_sect() const;

        safe_vector<const ALUPosition *> all_alu_positions() const {
            safe_vector<const ALUPosition *> rv;
            for (auto &act : alu_positions)
                for (auto &pos : act.second) rv.push_back(&pos);
            return rv;
        }
        friend std::ostream &operator<<(std::ostream &out, const Use &use);
    };

 private:
    PhvInfo &phv;
    const IR::MAU::Table *tbl;
    const ReductionOrInfo &red_info;
    safe_vector<Use> *uses = nullptr;
    SplitAttachedInfo &att_info;
    int calc_max_size = 0;
    std::map<cstring, RamSec_vec_t> init_ram_sections;

    std::array<int, AD_LOCATIONS> bytes_per_loc = {{0, 0}};
    safe_vector<BusInputs> action_bus_input_bitvecs;
    safe_vector<AllActionPositions> action_bus_inputs;

    // Responsible for holding hash, (eventually rng, meter color, stateful counters)
    RamSec_vec_t locked_in_all_actions_sects;
    BusInputs locked_in_all_actions_input_bitvecs;
    AllActionPositions locked_in_all_actions_inputs;

    void create_argument(ALUOperation &alu, ActionAnalysis::ActionParam &read,
                         le_bitrange container_bits, const IR::MAU::ConditionalArg *ca);
    void create_constant(ALUOperation &alu, const IR::Expression *read, le_bitrange container_bits,
                         int &constant_alias_index, const IR::MAU::ConditionalArg *ca);
    void create_hash(ALUOperation &alu, ActionAnalysis::ActionParam &read,
                     le_bitrange container_bits);
    void create_hash_constant(ALUOperation &alu, ActionAnalysis::ActionParam &read,
                              le_bitrange container_bits);
    void create_random_number(ALUOperation &alu, ActionAnalysis::ActionParam &read,
                              le_bitrange container_bits, cstring action_name);
    void create_random_padding(ALUOperation &alu, le_bitrange padding_bits);
    void create_meter_color(ALUOperation &alu, ActionAnalysis::ActionParam &read,
                            le_bitrange container_bits);
    void create_mask_argument(ALUOperation &alu, ActionAnalysis::ActionParam &read,
                              le_bitrange container_bits);
    void create_mask_constant(ALUOperation &alu, bitvec value, le_bitrange container_bits,
                              int &constant_alias_index);

    bool fix_bitwise_overwrite(ALUOperation &alu,
                               const ActionAnalysis::ContainerAction &cont_action,
                               const size_t container_size, const bitvec &total_write_bits,
                               int &constant_alias_index, int &alias_required_reads);

    void create_alu_ops_for_action(ActionAnalysis::ContainerActionsMap &ca_map,
                                   cstring action_name);
    bool analyze_actions(FormatType_t format_type);

    void initial_possible_condenses(PossibleCondenses &condenses, const RamSec_vec_t &ram_sects);
    void incremental_possible_condenses(PossibleCondenses &condense, const RamSec_vec_t &ram_sects);

    bitvec bytes_in_use(BusInputs &all_inputs) const {
        return all_inputs[BYTE] | all_inputs[HALF] | all_inputs[FULL];
    }
    bool is_better_condense(RamSec_vec_t &ram_sects, const RamSection *best, size_t best_skip1,
                            size_t best_skip2, const RamSection *comp, size_t comp_skip1,
                            size_t comp_skip2);
    void condense_action(cstring action_name, RamSec_vec_t &ram_sects);
    void shrink_possible_condenses(PossibleCondenses &pc, RamSec_vec_t &ram_sects,
                                   const RamSection *ad, size_t i_pos, size_t j_pos);
    void set_ram_sect_byte(SingleActionAllocation &single_action_alloc,
                           bitvec &allocated_slots_in_action, RamSectionPosition &ram_sect,
                           int byte_position);
    bitvec adt_iteration(SlotType_t slot_type, int &iteration);
    void alloc_adt_slots_of_size(SlotType_t slot_size, SingleActionAllocation &single_action_alloc,
                                 int max_bytes_required);
    void alloc_immed_slots_of_size(SlotType_t size, SingleActionAllocation &single_action_alloc,
                                   int max_bytes_required);
    void verify_placement(SingleActionAllocation &single_action_alloc);

    void determine_single_action_input(SingleActionAllocation &single_action_alloc,
                                       int max_bytes_required);
    bool determine_next_immediate_bytes(bool immediate_forced);
    bool determine_bytes_per_loc(bool &initialized, IR::MAU::Table::ImmediateControl_t imm_ctrl);

    void assign_action_data_table_bytes(AllActionPositions &all_bus_inputs,
                                        BusInputs &total_inputs);
    void assign_immediate_bytes(AllActionPositions &all_bus_inputs, BusInputs &total_inputs,
                                int max_bytes_needed);
    void assign_RamSections_to_bytes();

    const ALUOperation *finalize_locked_shift_alu_operation(const RamSection *ram_sect,
                                                            const ALUOperation *init_ad_alu,
                                                            int right_shift);
    const ALUOperation *finalize_alu_operation(const RamSection *ram_sect,
                                               const ALUOperation *init_ad_alu, int right_shift,
                                               int section_byte_offset, int *rot_alias_idx);
    void build_single_ram_sect(RamSectionPosition &ram_sect, Location_t loc,
                               safe_vector<ALUPosition> &alu_positions, BusInputs &verify_inputs,
                               int *rot_alias_idx);
    void build_locked_in_format(Use &use);
    void build_potential_format(bool immediate_forced);

 public:
    void set_uses(safe_vector<Use> *u) { uses = u; }
    void allocate_format(IR::MAU::Table::ImmediateControl_t imm_ctrl, FormatType_t format_type);
    Format(PhvInfo &p, const IR::MAU::Table *t, const ReductionOrInfo &ri, SplitAttachedInfo &sai)
        : phv(p), tbl(t), red_info(ri), att_info(sai) {}
};

}  // namespace ActionData

#endif /* BACKENDS_TOFINO_BF_P4C_MAU_ACTION_FORMAT_H_ */
