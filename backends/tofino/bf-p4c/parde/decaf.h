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

#ifndef BF_P4C_PARDE_DECAF_H_
#define BF_P4C_PARDE_DECAF_H_

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/lib/assoc.h"
#include "bf-p4c/logging/pass_manager.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/parde/create_pov_encoder.h"
#include "bf-p4c/parde/parde_visitor.h"

/**
 * \defgroup DeparserCopyOpt DeparserCopyOpt
 * \ingroup parde
 * \brief decaf : a deparser optimization of copy assigned fields
 *
 * author: zhao ma
 *
 * The observation is that there is a high degree of data movement in many common
 * data-plane program patterns (e.g. tunneling, label switching), i.e. a large number
 * of fields participate in copy assignment only. These are the fields whose final
 * value is that of another such copied only field.
 *
 * Based on this observation, we devise an optimization to resolve data copies directly
 * in the deparser rather than using the MAU to move the data, costing normal %PHV
 * containers.
 *
 * Consider the program below:
 *
 *      action a1() { modify_field(data.a, data.b); }
 *      action a2() { modify_field(data.a, 0x0800); }
 *      action a3() { modify_field(data.a, data.c); }
 *      table t1 {
 *          reads { data.m : exact; }
 *          actions { a1; a2; a3; nop; }
 *      }
 *      action a4() { modify_field(data.b, data.a); }
 *      action a5() { modify_field(data.b, 0x0866); }
 *      table t2 {
 *          reads { data.k : exact; }
 *          actions { a4; a5; nop; }
 *      }
 *      control ingress {
 *          apply(t1);
 *          apply(t2);
 *      }
 *
 * The possible values for each field (and reaching action sequence) are:
 *
 *      data.a : v0: 0x0800:   t1.a2 -> *
 *               v1: data.b:   t1.a1 -> *
 *               v2: data.c:   t1.a3 -> *
 *               v3: data.a:  t1.nop -> *
 *      data.b : v0: 0x0800:  t1.a2 -> t2.a4
 *               v1: 0x0866:      * -> t2.a5
 *               v2: data.a: t1.nop -> t2.a4
 *               v3: data.c:  t1.a3 -> t2.a4
 *               v4: data.b:  t1.a1 -> t2.nop
 *      data.c : data.c (read-only)
 *
 * Let's assign a bit to each action, and another bit to each version for a
 * field's final value. The function between the action bits and version bits
 * can be represented as a truth table.
 *
 *      a1  a2  a3  a4  a5 | a_v0 a_v1 a_v2 a_v3  b_v0 b_v1 b_v2 b_v3 b_v4
 *      -------------------------------------------------------------------
 *      1   0   0   0   0  | 0    1    0    0     0    0    0    0    1
 *      0   1   0   0   0  | 1    0    0    0     0    0    0    0    1
 *      0   0   1   0   0  | 0    1    0    1     0    0    0    0    1
 *      0   1   0   1   0  | 1    0    0    0     1    0    0    0    0
 *      0   0   0   0   1  | 0    0    0    1     0    1    0    0    0
 *        ...
 *        ...
 *
 *  Conveniently, the truth table can be implemented in Tofino as a match action
 *  table (using static entries). The version bits are wired to the FD as POV bits
 *  for the deparser to disambiguate which version to use for each field to reconstruct
 *  final packet, and with each version allocated to its own FD entry. Finally, all
 *  versions of value can be parsed into tagalong containers.
 *
 * # TODO
 *
 * Tagalong containers, though abundant, are not unlimited. In addition, other resources
 * also need to be managed. The list below is all resources need to be managed in the order
 * of scarcity (most scarce to least). We need to make sure we don't over-fit any of these.
 * Given a list of fields that can be decaf'd, we need to establish a partial order between
 * any two fields such that one requires less resource than the other.
 *
 *   1. tagalong %PHV space (2k bits, half of normal %PHV)
 *   2. POV bits (128 bits per gress)
 *   3. %Parser constant extract (each state has 4xB, 2xH, 2xW constant extract bandwidth)
 *   4. %Table resources (logical ID, memory)
 *   5. FD entries (abundant)
 */

/**
 * \ingroup DeparserCopyOpt
 */
struct Value {
    const PHV::Field *field = nullptr;
    const IR::Constant *constant = nullptr;

    Value() {}
    explicit Value(const PHV::Field *f) : field(f) {}
    explicit Value(const IR::Constant *c) : constant(c) {}

    explicit Value(const Value &other) : field(other.field), constant(other.constant) {
        if (field && constant) BUG("Value cannot be both field and constant");
    }

    Value &operator=(const Value &other) {
        field = other.field;
        constant = other.constant;
        if (field && constant) BUG("Value cannot be both field and constant");
        return *this;
    }

    std::string print() const {
        std::stringstream ss;
        if (field)
            ss << field->name;
        else if (constant)
            ss << "0x" << std::hex << constant << std::dec;
        return ss.str();
    }

    bool operator<(const Value &other) const {
        if (field && other.field)
            return field->id < other.field->id;
        else if (constant && other.constant)
            return constant->value < other.constant->value;
        else if (field && other.constant)
            return false;
        else if (constant && other.field)
            return true;

        BUG("Value is neither constant nor field?");
    }

    bool operator==(const Value &other) const {
        if (field != other.field) {
            return false;
        } else if (constant && other.constant) {
            if (!constant->equiv(*(other.constant))) return false;
        } else if (constant != other.constant) {
            return false;
        }

        return true;
    }
};

/**
 * \ingroup DeparserCopyOpt
 */
struct Assign {
    Assign(const IR::MAU::Instruction *instr, const PHV::Field *dst, const IR::Constant *src)
        : dst(dst), src(new Value(src)), instr(instr) {}

    Assign(const IR::MAU::Instruction *instr, const PHV::Field *dst, const PHV::Field *src)
        : dst(dst), src(new Value(src)), instr(instr) {}

    std::string print() const {
        std::stringstream ss;
        ss << dst->name << " <= " << src->print();
        return ss.str();
    }

    const PHV::Field *dst;
    const Value *src;

    const IR::MAU::Instruction *instr = nullptr;
};

/**
 * \ingroup DeparserCopyOpt
 */
class AssignChain : public std::vector<const Assign *> {
 public:
    void push_front(const Assign *assign) { insert(begin(), assign); }

    bool contains(const IR::MAU::Instruction *instr) const {
        for (auto &assign : *this) {
            if (assign->instr == instr) return true;
        }

        return false;
    }
};

/**
 * \ingroup DeparserCopyOpt
 */
struct FieldGroup : public ordered_set<const PHV::Field *> {
    FieldGroup() {}
    explicit FieldGroup(int i) : id(i) {}
    int id = -1;
};

/**
 * \ingroup DeparserCopyOpt
 */
struct CollectHeaderValidity : public Inspector, IHasDbPrint {
    const PhvInfo &phv;

    assoc::map<const PHV::Field *, const IR::Expression *> field_to_expr;

    ordered_map<const PHV::Field *, const PHV::Field *> field_to_valid_bit;

    assoc::map<const PHV::Field *, ordered_set<const IR::MAU::Action *>> validate_to_action;
    assoc::map<const PHV::Field *, ordered_set<const IR::BFN::ParserState *>> validate_to_extract;
    assoc::map<const PHV::Field *, ordered_set<const IR::MAU::Action *>> invalidate_to_action;

    explicit CollectHeaderValidity(const PhvInfo &phv) : phv(phv) {}

    const IR::Expression *get_valid_bit_expr(const PHV::Field *f) const {
        auto vld = field_to_valid_bit.at(f);
        return field_to_expr.at(vld);
    }

    // bool preorder(const IR::BFN::Extract* extract) override {
    //     // TODO
    //     return false;
    // }

    bool preorder(const IR::MAU::Instruction *instr) override {
        auto action = findContext<IR::MAU::Action>();

        if (instr->operands.size() != 2) return false;

        auto dst = instr->operands[0];
        auto f_dst = phv.field(dst);

        if (f_dst && f_dst->pov) {
            auto src = instr->operands[1];
            auto c = src->to<IR::Constant>();
            if (c) {
                if (c->equiv(IR::Constant(IR::Type::Bits::get(1), 1)))
                    validate_to_action[f_dst].insert(action);
                else if (c->equiv(IR::Constant(IR::Type::Bits::get(1), 0)))
                    invalidate_to_action[f_dst].insert(action);
            }
        }

        return false;
    }

    bool preorder(const IR::BFN::Emit *emit) override {
        auto pov_bit = emit->povBit->field;
        auto f_pov_bit = phv.field(pov_bit);
        field_to_expr[f_pov_bit] = pov_bit;

        if (auto ef = emit->to<IR::BFN::EmitField>()) {
            auto f = phv.field(ef->source->field);
            field_to_valid_bit[f] = f_pov_bit;
        } else if (auto ec = emit->to<IR::BFN::EmitChecksum>()) {
            auto f = phv.field(ec->dest->field);
            field_to_valid_bit[f] = f_pov_bit;
        } else {
            BUG("Unknown deparser emit type %1%", emit);
        }

        return false;
    }

    void end_apply() override {
        LOG4(*this);
        LOG1("=== DONE collecting header validity bits ===");
    }

    void dbprint(std::ostream &out) const {
        for (auto &kv : field_to_valid_bit)
            out << kv.first->name << " : " << kv.second->name << std::endl;
    }
};

/**
 * \ingroup DeparserCopyOpt
 *
 * We categorize fields that are referenced in the control flow as either strong or
 * weak. The strong fields are ones that participate in match, non-move instruction, action
 * data bus read, or other reasons (non packet defs, is part of a checksum update or
 * digest). The weak fields are, by exclusion, the ones that are not strong. Weak fields
 * also shall not have a transitive strong source. All weak fields are then decaf candidates.
 */
class CollectWeakFields : public MauInspector, BFN::ControlFlowVisitor, IHasDbPrint {
    const PhvInfo &phv;
    const PhvUse &uses;
    const FieldDefUse &defuse;
    const DependencyGraph &dg;

    FieldGroup strong_fields;

    ordered_map<const IR::MAU::Instruction *, const IR::MAU::Action *> instr_to_action;
    ordered_map<const IR::MAU::Action *, const IR::MAU::Table *> action_to_table;

 public:
    ordered_map<const PHV::Field *, ordered_set<const Assign *>> field_to_weak_assigns;
    FieldGroup read_only_weak_fields;
    std::map<gress_t, ordered_set<const IR::Constant *>> all_constants;

    CollectWeakFields(const PhvInfo &phv, const PhvUse &uses, const FieldDefUse &defuse,
                      const DependencyGraph &dg)
        : phv(phv), uses(uses), defuse(defuse), dg(dg) {
        joinFlows = true;
        visitDagOnce = false;
        BackwardsCompatibleBroken = true;
    }

    const IR::MAU::Action *get_action(const Assign *assign) const {
        auto action = instr_to_action.at(assign->instr);
        return action;
    }

    const IR::MAU::Table *get_table(const Assign *assign) const {
        auto action = instr_to_action.at(assign->instr);
        auto table = action_to_table.at(action);
        return table;
    }

    std::string print_assign_context(const Assign *assign) const {
        auto action = instr_to_action.at(assign->instr);
        auto table = action_to_table.at(action);

        std::stringstream ss;
        ss << "(" << table->name << " : " << action->name << ")";
        return ss.str();
    }

    std::string print_assign(const Assign *assign) const {
        std::stringstream ss;

        ss << assign->print();
        ss << " : ";
        ss << print_assign_context(assign);
        return ss.str();
    }

    void remove_weak_field(const PHV::Field *field) {
        field_to_weak_assigns.erase(field);
        read_only_weak_fields.erase(field);
        // TODO remove constants for field
    }

    void dbprint(std::ostream &out) const;

 private:
    bool filter_join_point(const IR::Node *) override { return true; }

    void flow_merge(Visitor &other_) override {
        CollectWeakFields &other = dynamic_cast<CollectWeakFields &>(other_);

        for (auto &kv : other.field_to_weak_assigns) {
            for (auto a : kv.second) field_to_weak_assigns[kv.first].insert(a);
        }

        strong_fields.insert(other.strong_fields.begin(), other.strong_fields.end());

        for (auto a : strong_fields) add_strong_field(a);

        for (auto &kv : other.instr_to_action) instr_to_action[kv.first] = kv.second;

        for (auto &kv : other.action_to_table) action_to_table[kv.first] = kv.second;
    }

    void flow_copy(::ControlFlowVisitor &other_) override {
        CollectWeakFields &other = dynamic_cast<CollectWeakFields &>(other_);
        field_to_weak_assigns = other.field_to_weak_assigns;
        strong_fields = other.strong_fields;
        instr_to_action = other.instr_to_action;
        action_to_table = other.action_to_table;
        // FIXME what about read_only_weak_fields and all_constants?  They're not merged
        // in flow_merge, so perhaps don't need to be copied?
    }

    CollectWeakFields *clone() const override { return new CollectWeakFields(*this); }

    Visitor::profile_t init_apply(const IR::Node *root) override {
        auto rv = Inspector::init_apply(root);

        field_to_weak_assigns.clear();
        strong_fields.clear();

        instr_to_action.clear();
        action_to_table.clear();

        return rv;
    }

    void end_apply() override {
        add_read_only_weak_fields();

        elim_by_strong_transitivity();

        elim_non_byte_aligned_fields();

        //        elim_if_too_few_weak_fields();

        get_all_constants();

        LOG3(*this);
        LOG1("=== DONE collecting weak fields ===");
    }

    void add_weak_assign(const PHV::Field *dst, const PHV::Field *field_src,
                         const IR::Constant *const_src, const IR::MAU::Instruction *instr);

    void add_strong_field(const PHV::Field *f, std::string reason = "");

    static bool other_elim_reason(const PHV::Field *f) {
        return !f->deparsed() ||  // we need the weak field's value to reach deparser
               f->pov || f->metadata || f->bridged || f->is_digest() || f->is_intrinsic();
    }

    bool preorder(const IR::MAU::TableKey *ixbar) override;
    bool preorder(const IR::MAU::StatefulAlu *) override { return false; }
    bool preorder(const IR::MAU::Instruction *instr) override;

    // returns true if all defs of src happen before dst
    bool all_defs_happen_before(const PHV::Field *src, const PHV::Field *dst);
    bool is_strong_by_transitivity(const PHV::Field *dst, const PHV::Field *src,
                                   assoc::set<const PHV::Field *> &visited);
    bool is_strong_by_transitivity(const PHV::Field *dst);

    void add_read_only_weak_fields();
    void get_all_constants();

    void elim_by_strong_transitivity();
    void elim_non_byte_aligned_fields();
};

/**
 * \ingroup DeparserCopyOpt
 *
 * Perform copy/constant propagation for the weak fields on table dependency.
 * For each weak field, what are the all possible values that are reachable at the
 * deparser?
 */
class ComputeValuesAtDeparser : public Inspector {
 public:
    std::vector<FieldGroup> weak_field_groups;

    ordered_map<const PHV::Field *, ordered_map<Value, ordered_set<AssignChain>>> value_to_chains;

    CollectWeakFields &weak_fields;

 public:
    explicit ComputeValuesAtDeparser(CollectWeakFields &weak_fields) : weak_fields(weak_fields) {}

    bool is_weak_assign(const IR::MAU::Instruction *instr) const;

    std::string print_assign_chain(const AssignChain &chain) const;
    std::string print_assign_chains(const ordered_set<AssignChain> &chains) const;
    std::string print_value_map(const FieldGroup &weak_field_group,
                                const ordered_set<AssignChain> &all_chains_in_group) const;

 private:
    Visitor::profile_t init_apply(const IR::Node *root) override;

    ordered_set<const Value *> get_all_weak_srcs(const PHV::Field *field,
                                                 assoc::set<const PHV::Field *> &visited);

    ordered_set<const Value *> get_all_weak_srcs(const PHV::Field *field);

    // Group fields that have transitive assignment to one another. Basically members
    // in a group can only take on the value of other members in the group, or constant.
    void group_weak_fields();

    std::vector<const IR::MAU::Table *> get_next_tables(
        const IR::MAU::Table *curr_table,
        const ordered_map<const IR::MAU::Table *, ordered_set<const Assign *>> &table_to_assigns);

    ordered_set<AssignChain> enumerate_all_assign_chains(
        const IR::MAU::Table *curr_table,
        const ordered_map<const IR::MAU::Table *, ordered_set<const Assign *>> &table_to_assigns);

    ordered_map<const PHV::Field *, Value> propagate_value_on_assign_chain(
        const AssignChain &chain);

    bool compute_all_reachable_values_at_deparser(const FieldGroup &weak_field_group);
};

/**
 * \ingroup DeparserCopyOpt
 */
struct VersionMap {
    ordered_map<const PHV::Field *, const IR::TempVar *> default_version;

    ordered_map<const PHV::Field *, ordered_map<Value, const IR::TempVar *>> value_to_version;

    ordered_set<const IR::TempVar *> get_all_version_bits() const {
        ordered_set<const IR::TempVar *> rv;

        for (auto v : default_version) rv.insert(v.second);

        for (auto &fvv : value_to_version) {
            for (auto &vv : fvv.second) rv.insert(vv.second);
        }

        return rv;
    }
};

/**
 * \ingroup DeparserCopyOpt
 *
 * Given a set of weak fields, and their reaching values/action chains at
 * the deparser, we construct tables to synthesize the POV bits needed
 * to deparse the right version at runtime.
 */
class SynthesizePovEncoder : public MauTransform, IHasDbPrint {
    const CollectHeaderValidity &pov_bits;
    const CollectWeakFields &weak_fields;
    const ComputeValuesAtDeparser &values_at_deparser;

    std::map<gress_t, std::vector<IR::MAU::Table *>> tables_to_insert;

    assoc::map<const FieldGroup *, std::vector<const IR::Expression *>> group_to_vld_bits;

    assoc::map<const FieldGroup *, std::vector<const IR::TempVar *>> group_to_ctl_bits;

    ordered_map<const FieldGroup *, VersionMap> group_to_version_map;

 public:
    ordered_map<const IR::MAU::Action *, const IR::TempVar *> action_to_ctl_bit;
    ordered_map<const IR::MAU::Instruction *, const IR::TempVar *> assign_to_version_bit;

    ordered_map<const PHV::Field *, ordered_map<Value, const IR::TempVar *>> value_to_pov_bit;

    ordered_map<const PHV::Field *, const IR::TempVar *> default_pov_bit;

    explicit SynthesizePovEncoder(const CollectHeaderValidity &pov_bits,
                                  const ComputeValuesAtDeparser &values_at_deparser)
        : pov_bits(pov_bits),
          weak_fields(values_at_deparser.weak_fields),
          values_at_deparser(values_at_deparser) {}

    void dbprint(std::ostream &out) const;

 private:
    gress_t get_gress(const FieldGroup &group) {
        auto it = group.begin();
        return (*it)->gress;
    }

    std::vector<const IR::Expression *> get_valid_bits(const FieldGroup &group);

    ordered_set<const IR::MAU::Action *> get_all_actions(const FieldGroup &group);

    std::vector<const IR::TempVar *> create_action_ctl_bits(
        const ordered_set<const IR::MAU::Action *> &actions);

    bool have_same_vld_ctl_bits(const FieldGroup *a, const FieldGroup *b);

    bool have_same_action_chain(const AssignChain &a, const AssignChain &b);

    bool have_same_assign_chains(const PHV::Field *p, const Value &pv, const PHV::Field *q,
                                 const Value &qv);

    bool have_same_hdr_vld_bit(const PHV::Field *p, const PHV::Field *q);

    const IR::TempVar *find_equiv_version_bit_for_value(const PHV::Field *f, unsigned version,
                                                        const Value &value,
                                                        const VersionMap &version_map);

    const IR::TempVar *find_equiv_version_bit_for_value(const PHV::Field *f,
                                                        const FieldGroup &group, unsigned version,
                                                        const Value &value);

    const IR::TempVar *create_version_bit_for_value(const PHV::Field *f, const FieldGroup &group,
                                                    unsigned version, const Value &value);

    const VersionMap &create_version_map(const FieldGroup &group);

    unsigned encode_assign_chain(const AssignChain &chain,
                                 const ordered_set<const IR::MAU::Action *> &all_actions);
    bool is_valid(const PHV::Field *f, const std::vector<const IR::Expression *> &vld_bits_onset);

    MatchAction *create_match_action(const FieldGroup &group);

    IR::MAU::TableSeq *preorder(IR::MAU::TableSeq *seq) override;

    unsigned num_coalesced_match_bits(const FieldGroup &a, const FieldGroup &b);

    bool is_trivial_group(const FieldGroup &group);

    void resolve_trivial_group(const FieldGroup &group);

    std::pair<std::map<gress_t, std::vector<FieldGroup>>,
              std::map<gress_t, std::vector<FieldGroup>>>
    coalesce_weak_field_groups();

    Visitor::profile_t init_apply(const IR::Node *root) override;

    void end_apply() override {
        LOG3(*this);
        LOG1("=== DONE synthesizing decaf POV encoder ===");
    }
};

/**
 * \ingroup DeparserCopyOpt
 *
 * Certain weak fields may have constant values. These constants need to be extracted
 * by the parser in Tofino. In JBay, the deparser comes with 8 bytes of constants we
 * can use, the rest still need to be extracted in the parser.
 */
class CreateConstants : public PardeTransform, IHasDbPrint {
    const ComputeValuesAtDeparser &values_at_deparser;
    unsigned cid = 0;

 public:
    std::map<gress_t, ordered_map<const IR::Constant *, std::vector<uint8_t>>> const_to_bytes;

    // these need to be written from the parser
    std::map<gress_t, ordered_map<uint8_t, const IR::TempVar *>> byte_to_temp_var;

    // these can be directly be sourced from the deparser
    std::map<gress_t, ordered_set<uint8_t>> deparser_bytes;

    explicit CreateConstants(const ComputeValuesAtDeparser &values_at_deparser)
        : values_at_deparser(values_at_deparser) {}

    void dbprint(std::ostream &out) const;

    bool is_inserted(const IR::TempVar *constant) const {
        for (auto &kv : byte_to_temp_var) {
            for (auto &bt : kv.second) {
                if (bt.second == constant) return true;
            }
        }
        return false;
    }

 private:
    void create_temp_var_for_parser_constant_bytes(
        gress_t gress, const ordered_set<const IR::Constant *> &constants);

    Visitor::profile_t init_apply(const IR::Node *root) override;

    void end_apply() override {
        LOG3(*this);
        LOG1("=== DONE inserting parser constants ===");
    }

    void insert_init_consts_state(IR::BFN::Parser *parser);

    IR::BFN::Parser *preorder(IR::BFN::Parser *parser) override;
};

/**
 * \ingroup DeparserCopyOpt
 *
 * Replace all move instructions that weak fields are involved in by a single
 * bit set to the $ctl bits for each action that the move instructions are in.
 * Typically, these are the tunnel encap/decap actions.
 */
class RewriteWeakFieldWrites : public MauTransform {
    const ComputeValuesAtDeparser &values_at_deparser;
    const SynthesizePovEncoder &synth_pov_encoder;

    assoc::map<const IR::MAU::Action *, const IR::MAU::Action *> curr_to_orig;

    assoc::map<const IR::MAU::Action *, const IR::MAU::Instruction *> orig_action_to_new_instr;

 public:
    RewriteWeakFieldWrites(const ComputeValuesAtDeparser &values_at_deparser,
                           const SynthesizePovEncoder &synth_pov_encoder)
        : values_at_deparser(values_at_deparser), synth_pov_encoder(synth_pov_encoder) {}

 private:
    void end_apply() override { LOG1("=== DONE rewriting weak fields ==="); }

    const IR::Node *preorder(IR::MAU::Action *action) override;

    bool cache_instr(const IR::MAU::Action *orig_action, const IR::MAU::Instruction *new_instr);

    const IR::Node *postorder(IR::MAU::Instruction *instr) override;
};

/**
 * \ingroup DeparserCopyOpt
 *
 * Rewrite deparser to insert new emits created for each of the weak field's
 * reachable value at the deparser. Each value emit is predicated by the synthesized
 * %POV bit created in the SynthesizePovEncoder pass.
 */
class RewriteDeparser : public DeparserModifier {
    const PhvInfo &phv;
    const SynthesizePovEncoder &synth_pov_encoder;
    const CreateConstants &create_consts;

 public:
    ordered_set<cstring> must_split_fields;

    RewriteDeparser(const PhvInfo &phv, const SynthesizePovEncoder &synth_pov_encoder,
                    const CreateConstants &create_consts)
        : phv(phv), synth_pov_encoder(synth_pov_encoder), create_consts(create_consts) {}

 private:
    void end_apply() override { LOG1("=== DONE rewriting deparser ==="); }

    void add_emit(IR::Vector<IR::BFN::Emit> &emits, const IR::Expression *source,
                  const IR::TempVar *pov_bit);

    void add_emit(IR::Vector<IR::BFN::Emit> &emits, uint8_t value, const IR::TempVar *pov_bit);

    const IR::Expression *find_emit_source(const PHV::Field *field,
                                           const IR::Vector<IR::BFN::Emit> &emits);

    ordered_set<ordered_set<const IR::Expression *>> get_all_disjoint_pov_bit_sets();

    std::vector<const IR::BFN::Emit *> coalesce_disjoint_emits(
        const std::vector<const IR::BFN::Emit *> &disjoint_emits);

    void coalesce_emits_for_packing(
        IR::BFN::Deparser *deparser,
        const ordered_set<ordered_set<const IR::Expression *>> &disjoint_pov_sets);

    void infer_deparser_no_repeat_constraint(
        IR::BFN::Deparser *deparser,
        const ordered_set<ordered_set<const IR::Expression *>> &disjoint_pov_sets);

    bool preorder(IR::BFN::Deparser *deparser) override;
};

/**
 * \ingroup DeparserCopyOpt
 * \brief Top level PassManager.
 */
class DeparserCopyOpt : public Logging::PassManager {
    CollectHeaderValidity collect_hdr_valid_bits;
    CollectWeakFields collect_weak_fields;
    ComputeValuesAtDeparser values_at_deparser;
    SynthesizePovEncoder synth_pov_encoder;
    CreateConstants create_consts;
    RewriteWeakFieldWrites rewrite_weak_fields;

 public:
    RewriteDeparser rewrite_deparser;

    DeparserCopyOpt(const PhvInfo &phv, PhvUse &uses, const FieldDefUse &defuse,
                    const DependencyGraph &dg)
        : Logging::PassManager("decaf"_cs, Logging::Mode::AUTO),
          collect_hdr_valid_bits(phv),
          collect_weak_fields(phv, uses, defuse, dg),
          values_at_deparser(collect_weak_fields),
          synth_pov_encoder(collect_hdr_valid_bits, values_at_deparser),
          create_consts(values_at_deparser),
          rewrite_weak_fields(values_at_deparser, synth_pov_encoder),
          rewrite_deparser(phv, synth_pov_encoder, create_consts) {
        addPasses({
            &uses, &collect_hdr_valid_bits, &collect_weak_fields, &values_at_deparser,
            &synth_pov_encoder,    // mau-transform
            &create_consts,        // parde-transform
            &rewrite_weak_fields,  // mau-transform
            &rewrite_deparser      // dep-modifier
        });
    }
};

#endif /* BF_P4C_PARDE_DECAF_H_ */
