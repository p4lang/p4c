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

#include "decaf.h"

#include <bitset>

#include "bf-p4c/common/table_printer.h"
#include "bf-p4c/device.h"

template <typename T>
static std::vector<T> to_vector(const ordered_set<T> &data) {
    std::vector<T> vec;

    for (auto d : data) vec.push_back(d);

    return vec;
}

template <typename T>
static std::vector<std::vector<T>> enumerate_all_subsets(const std::vector<T> &inputs) {
    std::vector<std::vector<T>> rv = {{}};

    for (auto &n : inputs) {
        std::vector<std::vector<T>> temp = rv;

        for (auto &t : temp) {
            t.push_back(n);
            rv.push_back(t);
        }
    }

    return rv;
}

std::string print_field_group(const FieldGroup &group) {
    std::stringstream ss;
    TablePrinter tp(ss, {"Group " + std::to_string(group.id), "Bits"}, TablePrinter::Align::LEFT);

    unsigned total_field_bits = 0;
    for (auto f : group) {
        tp.addRow({std::string(f->name), std::to_string(f->size)});
        total_field_bits += f->size;
    }

    tp.addSep();
    tp.addRow({"Total", std::to_string(total_field_bits)});
    tp.print();
    return ss.str();
}

std::string print_field_groups(const std::vector<FieldGroup> &groups) {
    std::stringstream ss;
    for (auto &group : groups) ss << print_field_group(group) << std::endl;
    return ss.str();
}

void CollectWeakFields::add_weak_assign(const PHV::Field *dst, const PHV::Field *field_src,
                                        const IR::Constant *const_src,
                                        const IR::MAU::Instruction *instr) {
    BUG_CHECK(((field_src && !const_src) || (!field_src && const_src)),
              "Expect source to be either a PHV field or constant");

    if (field_src && field_src == dst) {
        LOG4("src is dst");
        return;
    }

    auto assign = const_src ? new Assign(instr, dst, const_src) : new Assign(instr, dst, field_src);

    field_to_weak_assigns[dst].insert(assign);
}

void CollectWeakFields::add_strong_field(const PHV::Field *f, std::string reason) {
    strong_fields.insert(f);
    field_to_weak_assigns.erase(f);
    if (reason != "") LOG2(f->name << " is not a candidate (" << reason << ")");
}

bool CollectWeakFields::preorder(const IR::MAU::TableKey *ixbar) {
    auto f = phv.field(ixbar->expr);
    add_strong_field(f, "ixbar match");
    return false;
}

bool CollectWeakFields::preorder(const IR::MAU::Instruction *instr) {
    auto table = findContext<IR::MAU::Table>();
    auto action = findContext<IR::MAU::Action>();

    instr_to_action[instr] = action;
    action_to_table[action] = table;

    auto dst = instr->operands[0];
    auto f_dst = phv.field(dst);

    if (!f_dst) {
        LOG2(dst << " is not a candidate (non-PHV field)");
        return false;
    }

    if (strong_fields.count(f_dst)) {
        return false;
    }

    // "a = b ++ c", p4_16/stf/ixbar_expr3.p4
    // "a" can still be weak, but need per slice weak field support TODO
    if (dst->is<IR::Slice>()) {
        add_strong_field(f_dst, "dst is slice");
        return false;
    }

    if (!f_dst->deparsed()) {
        add_strong_field(f_dst, "not deparsed");
        return false;
    }

    if (instr->name != "set") {
        for (auto op : instr->operands) {
            auto f_op = phv.field(op);
            if (f_op) add_strong_field(f_op, "non-move instr");
        }
        return false;
    }

    auto src = instr->operands[1];
    auto f_src = phv.field(src);

    if (!src->to<IR::Constant>() && (!f_src || src->is<IR::Slice>())) {
        add_strong_field(f_dst, "non-PHV src|slice");
        return false;
    }

    // We eliminate fields that are checksummed in the deparser, because the
    // checksum engine can only refer to each PHV container once (no multiple
    // references of the same field). Therefore we do need the field's updated
    // value to reach the deparser in a normal container.

    if (f_dst->is_checksummed()) {
        add_strong_field(f_dst, "checksummed");
        return false;
    }

    if (other_elim_reason(f_dst) || (f_src && other_elim_reason(f_src))) {
        add_strong_field(f_dst, "pov|metadata|digest");
        return false;
    }

    if (f_src && !f_src->parsed()) {
        add_strong_field(f_dst, "src not parsed");
        return false;
    }

    add_weak_assign(f_dst, f_src, src->to<IR::Constant>(), instr);

    return false;
}

bool CollectWeakFields::all_defs_happen_before(const PHV::Field *src, const PHV::Field *dst) {
    auto src_defs = defuse.getAllDefs(src->id);
    auto dst_defs = defuse.getAllDefs(dst->id);

    for (auto sd : src_defs) {
        for (auto dd : dst_defs) {
            auto ts = sd.first->to<IR::MAU::Table>();
            auto td = dd.first->to<IR::MAU::Table>();
            if (ts && td) {
                if (dg.happens_phys_before(td, ts)) {
                    return false;
                }
            }
        }
    }

    LOG4("all defs of " << src->name << " happen before " << dst->name);

    return true;
}

bool CollectWeakFields::is_strong_by_transitivity(const PHV::Field *dst, const PHV::Field *src,
                                                  assoc::set<const PHV::Field *> &visited) {
    if (visited.count(src)) return false;

    visited.insert(src);

    if (read_only_weak_fields.count(src)) return false;

#if 0
// This requires more work:
// [x] stack_valid.p4
// [x] simple_l3_mirror.p4
// [v] decaf_4.p4

    if (all_defs_happen_before(src, dst))
        return false;
#endif

    if (strong_fields.count(src)) return true;

    if (!field_to_weak_assigns.count(src)) return true;

    for (auto assign : field_to_weak_assigns.at(src)) {
        if (assign->src->field) {
            if (is_strong_by_transitivity(dst, assign->src->field, visited)) return true;
        }
    }

    return false;
}

bool CollectWeakFields::is_strong_by_transitivity(const PHV::Field *dst) {
    assoc::set<const PHV::Field *> visited;

    for (auto assign : field_to_weak_assigns.at(dst)) {
        if (assign->src->field) {
            if (is_strong_by_transitivity(dst, assign->src->field, visited)) return true;
        }
    }

    return false;
}

void CollectWeakFields::add_read_only_weak_fields() {
    for (auto &kv : phv.get_all_fields()) {
        auto f = &(kv.second);

        if (!uses.is_used_mau(f))  // already a tagalong field
            continue;

        if (strong_fields.count(f)) continue;

        if (field_to_weak_assigns.count(f)) continue;

        if (other_elim_reason(f)) continue;

        LOG4("add read-only field " << f->name);

        read_only_weak_fields.insert(f);
    }
}

void CollectWeakFields::get_all_constants() {
    for (auto &kv : field_to_weak_assigns) {
        for (auto &assign : kv.second) {
            auto gress = kv.first->gress;
            if (assign->src->constant) {
                bool new_constant = true;

                for (auto uc : all_constants[gress]) {
                    if (uc->equiv(*(assign->src->constant))) {
                        new_constant = false;
                        break;
                    }
                }

                if (new_constant) all_constants[gress].insert(assign->src->constant);
            }
        }
    }
}

void CollectWeakFields::elim_by_strong_transitivity() {
    // elim all fields that have a strong source (directly or transitively)

    ordered_set<const PHV::Field *> to_delete;

    for (auto &kv : field_to_weak_assigns) {
        auto f = kv.first;
        if (is_strong_by_transitivity(f)) to_delete.insert(f);
    }

    for (auto f : to_delete) add_strong_field(f, "has strong src");
}

void CollectWeakFields::elim_non_byte_aligned_fields() {
    // For non byte-aligned fields, we need to check all fields in a byte
    // share same decaf pov bit in order to pack them (TODO). Elim these
    // for now.

    ordered_set<const PHV::Field *> to_delete;

    for (auto f : read_only_weak_fields) {
        if (f->size % 8 != 0 || (f->alignment && !((*(f->alignment)).isByteAligned())))
            to_delete.insert(f);
    }

    for (auto f : to_delete) {
        LOG2(f->name << " is not a candidate (non byte-aligned)");
        read_only_weak_fields.erase(f);
    }

    to_delete.clear();

    for (auto &kv : field_to_weak_assigns) {
        auto f = kv.first;
        if (f->size % 8 != 0 || (f->alignment && !((*(f->alignment)).isByteAligned())))
            to_delete.insert(f);
    }

    for (auto f : to_delete) {
        LOG2(f->name << " is not a candidate (non byte-aligned)");
        field_to_weak_assigns.erase(f);
    }
}

void CollectWeakFields::dbprint(std::ostream &out) const {
    unsigned total_field_bits = 0;

    out << "\ndecaf candidate fields:" << std::endl;
    TablePrinter tp(out, {"Field", "Bits", "Property"}, TablePrinter::Align::LEFT);

    for (auto f : read_only_weak_fields) {
        total_field_bits += f->size;
        tp.addRow({std::string(f->name), std::to_string(f->size), "(read-only)"});
    }

    for (auto &kv : field_to_weak_assigns) {
        auto dst = kv.first;
        total_field_bits += dst->size;
        tp.addRow({std::string(dst->name), std::to_string(dst->size), "(copy-assigned)"});
    }

    tp.addSep();
    tp.addRow({"Total", std::to_string(total_field_bits), ""});
    tp.print();

    out << "\nconstants:" << std::endl;

    if (all_constants.empty()) {
        out << "-none-" << std::endl;
        return;
    }

    TablePrinter tp2(out, {"#", "Const", "Gress"}, TablePrinter::Align::LEFT);

    unsigned i = 0;
    for (auto &kv : all_constants) {
        for (auto c : kv.second) {
            std::stringstream as_hex;
            as_hex << "0x" << std::hex << c << std::dec;
            tp2.addRow({std::to_string(i++), as_hex.str(), kv.first == INGRESS ? "I" : "E"});
        }
    }
    tp2.print();
}

bool ComputeValuesAtDeparser::is_weak_assign(const IR::MAU::Instruction *instr) const {
    for (auto &fv : value_to_chains) {
        for (auto &vc : fv.second) {
            for (auto &chain : vc.second) {
                if (chain.contains(instr)) return true;
            }
        }
    }

    return false;
}

std::string ComputeValuesAtDeparser::print_assign_chain(const AssignChain &chain) const {
    std::stringstream ss;
    for (auto assign : chain) {
        ss << "    ";
        ss << weak_fields.print_assign(assign);
    }
    return ss.str();
}

std::string ComputeValuesAtDeparser::print_assign_chains(
    const ordered_set<AssignChain> &chains) const {
    std::stringstream ss;

    TablePrinter tp(ss, {"#", "Assign Sequence"}, TablePrinter::Align::LEFT);

    unsigned i = 0;
    for (auto &chain : chains) {
        tp.addBlank();

        bool is_first = true;
        for (auto assign : chain) {
            tp.addRow({is_first ? std::to_string(i++) : "", weak_fields.print_assign(assign)});
            is_first = false;
        }
    }
    tp.print();
    return ss.str();
}

unsigned get_chain_id(const AssignChain &chain, const ordered_set<AssignChain> &all_chains) {
    unsigned id = 0;
    for (auto c : all_chains) {
        if (c == chain) return id;
        id++;
    }
    BUG("chain does not belong to group");
    return id;
}

std::string ComputeValuesAtDeparser::print_value_map(
    const FieldGroup &weak_field_group, const ordered_set<AssignChain> &all_chains_in_group) const {
    std::stringstream ss;

    TablePrinter tp(ss, {"Field", "Value at Deparser", "Chains"}, TablePrinter::Align::LEFT);

    for (auto f : weak_field_group) {
        if (weak_fields.read_only_weak_fields.count(f)) continue;

        tp.addSep();

        auto &value_map_for_field = value_to_chains.at(f);

        bool is_first = true;
        for (auto &kv : value_map_for_field) {
            auto &value = kv.first;
            auto &chains = kv.second;

            std::stringstream chain_ids;
            for (auto chain : chains) {
                auto id = get_chain_id(chain, all_chains_in_group);
                chain_ids << "#" << id << " ";
            }

            tp.addRow({is_first ? std::string(f->name) : "", value.print(), chain_ids.str()});
            is_first = false;
        }
    }
    tp.print();

    return ss.str();
}

Visitor::profile_t ComputeValuesAtDeparser::init_apply(const IR::Node *root) {
    auto rv = Inspector::init_apply(root);

    weak_field_groups.clear();
    value_to_chains.clear();

    group_weak_fields();

    std::vector<FieldGroup> groups_to_remove;

    for (auto &group : weak_field_groups) {
        bool ok = compute_all_reachable_values_at_deparser(group);

        if (!ok) groups_to_remove.push_back(group);
    }

    // erase groups we can't handle for now
    for (auto &group : groups_to_remove) {
        for (auto f : group) weak_fields.remove_weak_field(f);

        weak_field_groups.erase(
            std::remove(weak_field_groups.begin(), weak_field_groups.end(), group),
            weak_field_groups.end());
    }

    for (auto &a : weak_field_groups) {
        for (auto &b : weak_field_groups) {
            if (a == b) continue;

            for (auto f : a) {
                if (b.count(f)) BUG("%1% gets included in two groups?", f->name);
            }
        }
    }

    return rv;
}

ordered_set<const Value *> ComputeValuesAtDeparser::get_all_weak_srcs(
    const PHV::Field *field, assoc::set<const PHV::Field *> &visited) {
    ordered_set<const Value *> rv;

    if (visited.count(field)) return rv;

    visited.insert(field);

    if (weak_fields.field_to_weak_assigns.count(field)) {
        for (auto assign : weak_fields.field_to_weak_assigns.at(field)) {
            rv.insert(assign->src);

            if (auto src = assign->src->field) {
                auto src_srcs = get_all_weak_srcs(src, visited);
                for (auto s : src_srcs) rv.insert(s);
            }
        }
    }

    return rv;
}

ordered_set<const Value *> ComputeValuesAtDeparser::get_all_weak_srcs(const PHV::Field *field) {
    assoc::set<const PHV::Field *> visited;
    return get_all_weak_srcs(field, visited);
}

void ComputeValuesAtDeparser::group_weak_fields() {
    unsigned gid = 0;

    for (auto &kv : weak_fields.field_to_weak_assigns) {
        auto f = kv.first;

        bool found_union = false;

        for (auto &group : weak_field_groups) {
            if (found_union) break;

            if (group.count(f)) {
                found_union = true;
                break;
            }

            auto srcs = get_all_weak_srcs(f);

            for (auto src : srcs) {
                if (src->field) {
                    if (group.count(src->field)) {
                        found_union = true;
                        group.insert(f);
                        break;
                    }
                }
            }
        }

        if (!found_union) {
            FieldGroup new_group(gid++);

            new_group.insert(f);

            auto srcs = get_all_weak_srcs(f);

            for (auto src : srcs) {
                if (src->field) new_group.insert(src->field);
            }

            weak_field_groups.push_back(new_group);
        }
    }

    std::stable_sort(weak_field_groups.begin(), weak_field_groups.end(),
                     [=](const FieldGroup &a, const FieldGroup &b) { return a.size() < b.size(); });

    LOG1("\ntotal weak field groups: " << weak_field_groups.size());
    LOG1(print_field_groups(weak_field_groups));
}

std::vector<const IR::MAU::Table *> ComputeValuesAtDeparser::get_next_tables(
    const IR::MAU::Table *curr_table,
    const ordered_map<const IR::MAU::Table *, ordered_set<const Assign *>> &table_to_assigns) {
    // TODO assumes only 2 tables; in general need to get the next tables
    // from table graph.

    BUG_CHECK(table_to_assigns.size() <= 2, "more than 2 tables?");

    std::vector<const IR::MAU::Table *> next_tables;

    bool found_curr_table = false;
    for (auto &kv : table_to_assigns) {
        if (found_curr_table) {
            next_tables.push_back(kv.first);
            break;
        }

        if (curr_table == kv.first) found_curr_table = true;
    }

    BUG_CHECK(found_curr_table, "current table not found?");

    return next_tables;
}

ordered_set<AssignChain> ComputeValuesAtDeparser::enumerate_all_assign_chains(
    const IR::MAU::Table *curr_table,
    const ordered_map<const IR::MAU::Table *, ordered_set<const Assign *>> &table_to_assigns) {
    ordered_set<AssignChain> all_chains_at_curr_table;

    auto next_tables = get_next_tables(curr_table, table_to_assigns);

    for (auto next_table : next_tables) {
        auto all_chains_at_next_table = enumerate_all_assign_chains(next_table, table_to_assigns);

        for (auto chain : all_chains_at_next_table) all_chains_at_curr_table.insert(chain);

        for (auto assign : table_to_assigns.at(curr_table)) {
            for (auto chain : all_chains_at_next_table) {
                chain.push_front(assign);
                all_chains_at_curr_table.insert(chain);
            }
        }
    }

    for (auto assign : table_to_assigns.at(curr_table)) {
        AssignChain chain;
        chain.push_back(assign);
        all_chains_at_curr_table.insert(chain);
    }

    return all_chains_at_curr_table;
}

ordered_map<const PHV::Field *, Value> ComputeValuesAtDeparser::propagate_value_on_assign_chain(
    const AssignChain &chain) {
    ordered_map<const PHV::Field *, Value> rv;

    for (auto assign : chain) {
        if (assign->src->field) {
            if (rv.count(assign->src->field)) {
                Value val(rv.at(assign->src->field));
                rv[assign->dst] = val;
                continue;
            }
        }

        rv[assign->dst] = *(assign->src);
    }

    return rv;
}

// Perform copy/constant propagation for the group based on table dependency.
// For each field, what are the all possible values that are reachable at the
// deparser?
bool ComputeValuesAtDeparser::compute_all_reachable_values_at_deparser(
    const FieldGroup &weak_field_group) {
    LOG1("\ncompute all values for group " << weak_field_group.id << ":");

    ordered_map<const IR::MAU::Table *, ordered_set<const Assign *>> table_to_assigns;

    for (auto f : weak_field_group) {
        if (weak_fields.read_only_weak_fields.count(f)) continue;

        for (auto assign : weak_fields.field_to_weak_assigns.at(f)) {
            auto table = weak_fields.get_table(assign);
            table_to_assigns[table].insert(assign);
        }
    }

    // TODO For more than 2 tables, we need to propagate the values based on
    // the lattice order of table dependency, which is non-trivial. As first
    // cut, limit to 2 tables (sufficient for the tunnel encap/decap use case).
    // We also assume the tables are independently predicated which is an overly
    // conversative assumption. A table can be predicated by another table;

    if (table_to_assigns.size() > 2) {
        LOG2("\nmore than 2 tables involved for group " << weak_field_group.id << ", skip");

        if (LOGGING(2)) {
            for (auto &ta : table_to_assigns) std::clog << ta.first->name << std::endl;
        }

        return false;
    }

    auto first_table = (table_to_assigns.begin())->first;
    auto all_chains = enumerate_all_assign_chains(first_table, table_to_assigns);

    LOG2("\nTotal assign chains: " << all_chains.size());
    LOG2(print_assign_chains(all_chains));

    for (auto &chain : all_chains) {
        auto value_map_for_chain = propagate_value_on_assign_chain(chain);

        for (auto &kv : value_map_for_chain) {
            auto field = kv.first;
            auto &value = kv.second;

            auto &value_map_for_field = value_to_chains[field];
            value_map_for_field[value].insert(chain);
        }
    }

    LOG2("\ndeparser value map for group:");
    LOG2(print_value_map(weak_field_group, all_chains));

    return true;
}

static IR::TempVar *create_temp_var(cstring name, unsigned width, bool is_pov = false) {
    return new IR::TempVar(IR::Type::Bits::get(width), is_pov, name);
}

static IR::TempVar *create_bit(cstring name, bool is_pov = false) {
    return create_temp_var(name, 1, is_pov);
}

static IR::MAU::Instruction *create_set_bit_instr(const IR::TempVar *dst) {
    return new IR::MAU::Instruction("set"_cs, {dst, new IR::Constant(IR::Type::Bits::get(1), 1)});
}

void SynthesizePovEncoder::dbprint(std::ostream &out) const {
    out << "action_to_ctl_bit: " << action_to_ctl_bit.size() << std::endl;
    for (auto &kv : action_to_ctl_bit) out << kv.first->name << " : " << kv.second << std::endl;

    ordered_set<const IR::TempVar *> unique_value_pov_bits;
    for (auto &kv : value_to_pov_bit) {
        for (auto vb : kv.second) unique_value_pov_bits.insert(vb.second);
    }

    out << std::endl;

    out << "unique value pov bits: " << unique_value_pov_bits.size() << std::endl;
    for (auto &kv : value_to_pov_bit) {
        for (auto &vp : kv.second) out << kv.first->name << " : " << vp.second << std::endl;
    }

    out << std::endl;

    ordered_set<const IR::TempVar *> unique_default_pov_bits;
    for (auto vb : default_pov_bit) unique_default_pov_bits.insert(vb.second);

    out << "unique default pov bits: " << unique_default_pov_bits.size() << std::endl;
    for (auto &kv : default_pov_bit) out << kv.first->name << " : " << kv.second << std::endl;
}

std::vector<const IR::Expression *> SynthesizePovEncoder::get_valid_bits(const FieldGroup &group) {
    ordered_set<const IR::Expression *> valid_bits;

    for (auto f : group) {
        auto vld = pov_bits.get_valid_bit_expr(f);
        valid_bits.insert(vld);
    }

    return to_vector(valid_bits);
}

ordered_set<const IR::MAU::Action *> SynthesizePovEncoder::get_all_actions(
    const FieldGroup &group) {
    ordered_set<const IR::MAU::Action *> all_actions;

    for (auto f : group) {
        if (weak_fields.read_only_weak_fields.count(f)) continue;

        for (auto assign : weak_fields.field_to_weak_assigns.at(f)) {
            auto action = weak_fields.get_action(assign);
            all_actions.insert(action);
        }
    }

    LOG2("all actions for group " << group.id << ":");
    for (auto a : all_actions) LOG2("    " << a->name);

    return all_actions;
}

std::vector<const IR::TempVar *> SynthesizePovEncoder::create_action_ctl_bits(
    const ordered_set<const IR::MAU::Action *> &actions) {
    std::vector<const IR::TempVar *> rv;

    for (auto action : actions) {
        if (!action_to_ctl_bit.count(action))
            action_to_ctl_bit[action] = create_bit(action->name + ".$ctl");

        rv.insert(rv.begin(), action_to_ctl_bit.at(action));  // order is important
    }

    return rv;
}

bool SynthesizePovEncoder::have_same_vld_ctl_bits(const FieldGroup *a, const FieldGroup *b) {
    if (group_to_ctl_bits.count(a) && group_to_ctl_bits.count(b) && group_to_vld_bits.count(a) &&
        group_to_vld_bits.count(b)) {
        auto &a_ctl_bits = group_to_ctl_bits.at(a);
        auto &a_vld_bits = group_to_vld_bits.at(a);

        auto &b_ctl_bits = group_to_ctl_bits.at(b);
        auto &b_vld_bits = group_to_vld_bits.at(b);

        return a_ctl_bits == b_ctl_bits && a_vld_bits == b_vld_bits;
    }
    return false;
}

bool SynthesizePovEncoder::have_same_action_chain(const AssignChain &a, const AssignChain &b) {
    if (a.size() != b.size()) return false;

    for (unsigned i = 0; i < a.size(); i++) {
        auto p = a.at(i);
        auto q = b.at(i);

        auto pa = values_at_deparser.weak_fields.get_action(p);
        auto qa = values_at_deparser.weak_fields.get_action(q);

        if (pa != qa)  // shallow compare enough?
            return false;
    }

    return true;
}

bool SynthesizePovEncoder::have_same_assign_chains(const PHV::Field *p, const Value &pv,
                                                   const PHV::Field *q, const Value &qv) {
    auto &pc = values_at_deparser.value_to_chains.at(p).at(pv);
    auto &qc = values_at_deparser.value_to_chains.at(q).at(qv);

    LOG5("p chains\n" << values_at_deparser.print_assign_chains(pc));
    LOG5("q chains\n" << values_at_deparser.print_assign_chains(qc));

    if (pc.size() != qc.size()) return false;

    auto it1 = pc.begin();
    auto it2 = qc.begin();

    for (; it1 != pc.end() && it2 != qc.end(); ++it1, ++it2) {
        if (!have_same_action_chain(*it1, *it2)) {
            LOG5("two chain sets are not equivalent");
            return false;
        }
    }

    LOG5("two chain sets are equivalent");
    return true;
}

bool SynthesizePovEncoder::have_same_hdr_vld_bit(const PHV::Field *p, const PHV::Field *q) {
    auto pb = pov_bits.get_valid_bit_expr(p);
    auto qb = pov_bits.get_valid_bit_expr(q);

    return pb->equiv(*qb);
}

const IR::TempVar *SynthesizePovEncoder::find_equiv_version_bit_for_value(
    const PHV::Field *f, unsigned version, const Value &value, const VersionMap &version_map) {
    if (version == 0) {  // 0 for default
        // two fields' default pov bits are equiv, if all their version bits
        // are equiv TODO
        return nullptr;
    } else {
        for (auto &fv : version_map.value_to_version) {
            for (auto &vi : fv.second) {
                if (have_same_hdr_vld_bit(f, fv.first)) {  // quantifier
                    if (have_same_assign_chains(f, value, fv.first, vi.first)) return vi.second;
                }
            }
        }
    }

    return nullptr;
}

const IR::TempVar *SynthesizePovEncoder::find_equiv_version_bit_for_value(const PHV::Field *f,
                                                                          const FieldGroup &group,
                                                                          unsigned version,
                                                                          const Value &value) {
    for (auto &gi : group_to_version_map) {
        if (have_same_vld_ctl_bits(&group, gi.first)) {  // quantifier
            if (auto equiv = find_equiv_version_bit_for_value(f, version, value, gi.second))
                return equiv;
        }
    }

    return nullptr;
}

const IR::TempVar *SynthesizePovEncoder::create_version_bit_for_value(const PHV::Field *f,
                                                                      const FieldGroup &group,
                                                                      unsigned version,
                                                                      const Value &value) {
    auto equiv = find_equiv_version_bit_for_value(f, group, version, value);

    if (equiv) {
        LOG3(f->name << " v" << version << " has equivalent version bit with " << equiv);

        if (version == 0)  // 0 for default
            default_pov_bit[f] = equiv;
        else
            value_to_pov_bit[f][value] = equiv;

        return equiv;
    }

    std::string pov_bit_name = f->name + "_v" + std::to_string(version) + ".$valid";

    auto pov_bit = create_bit(pov_bit_name, true);

    if (version == 0) {  // 0 for default
        BUG_CHECK(!default_pov_bit.count(f), "default valid bit already exists?");
        default_pov_bit[f] = pov_bit;
    } else {
        BUG_CHECK(!value_to_pov_bit[f].count(value), "value valid bit already exists?");
        value_to_pov_bit[f][value] = pov_bit;
    }

    LOG5("created pov bit for " << f->name << " version " << version);

    return pov_bit;
}

const VersionMap &SynthesizePovEncoder::create_version_map(const FieldGroup &group) {
    for (auto f : group) {
        if (weak_fields.read_only_weak_fields.count(f)) continue;

        LOG4("\ncreate version map for " << f->name << ":");

        std::stringstream ss;
        TablePrinter tp(ss, {"Value at Deparser", "POV bit"}, TablePrinter::Align::LEFT);

        const IR::TempVar *dflt_version = nullptr;

        if (f->parsed()) {
            dflt_version = create_version_bit_for_value(f, group, 0x0, Value());
            group_to_version_map[&group].default_version[f] = dflt_version;

            std::stringstream tt;
            tt << dflt_version;
            tp.addRow({"(self)", tt.str()});
        }

        unsigned id = 1;

        for (auto &kv : values_at_deparser.value_to_chains.at(f)) {
            const IR::TempVar *version = nullptr;

            if (kv.first.field == f) {
                BUG_CHECK(dflt_version, "no default version bit of self reaching value for %1%?",
                          f->name);
                version = dflt_version;
            } else {
                version = create_version_bit_for_value(f, group, id++, kv.first);
            }

            group_to_version_map[&group].value_to_version[f][kv.first] = version;

            std::stringstream tt;
            tt << version;
            tp.addRow({kv.first.print(), tt.str()});
        }

        tp.print();
        LOG4(ss.str());
    }

    return group_to_version_map[&group];
}

template <typename T>
unsigned encode(const std::vector<T> &subset, const std::vector<T> &allset) {
    unsigned rv = 0;

    for (auto obj : subset) {
        auto it = std::find(allset.begin(), allset.end(), obj);

        BUG_CHECK(it != allset.end(), "valid bit not found in set?");

        unsigned offset = allset.size() - (it - allset.begin()) - 1;
        rv |= 1 << offset;
    }

    return rv;
}

unsigned SynthesizePovEncoder::encode_assign_chain(
    const AssignChain &chain, const ordered_set<const IR::MAU::Action *> &all_actions) {
    auto all_actions_vec = to_vector(all_actions);

    unsigned encoded = 0;

    for (auto assign : chain) {
        auto action = weak_fields.get_action(assign);

        auto it = std::find(all_actions_vec.begin(), all_actions_vec.end(), action);

        BUG_CHECK(it != all_actions_vec.end(), "action not found?");

        unsigned bit_offset = it - all_actions_vec.begin();

        encoded |= 1 << bit_offset;
    }

    return encoded;
}

std::string MatchAction::print() const {
    std::stringstream ss;

    ss << "keys:" << std::endl;
    for (auto k : keys) ss << k << std::endl;

    ss << std::endl;

    ss << "outputs:" << std::endl;
    for (auto o : outputs) ss << o << std::endl;

    ss << std::endl;

    for (auto ma : match_to_action_param) {
        std::stringstream as_binary;
        as_binary << std::bitset<32>(ma.first);
        ;
        ss << as_binary.str().substr(32 - keys.size()) << " -> ";

        as_binary.str("");
        as_binary << std::bitset<32>(ma.second);
        ss << as_binary.str().substr(32 - outputs.size()) << std::endl;
    }

    ss << std::endl;
    return ss.str();
}

bool SynthesizePovEncoder::is_valid(const PHV::Field *f,
                                    const std::vector<const IR::Expression *> &vld_bits_onset) {
    auto vld = pov_bits.get_valid_bit_expr(f);
    auto it = std::find(vld_bits_onset.begin(), vld_bits_onset.end(), vld);
    return it != vld_bits_onset.end();
}

MatchAction *SynthesizePovEncoder::create_match_action(const FieldGroup &group) {
    LOG1("\ncreate match action for group " << group.id << ":");

    auto all_actions = get_all_actions(group);

    auto &vld_bits = group_to_vld_bits[&group] = get_valid_bits(group);

    auto &ctl_bits = group_to_ctl_bits[&group] = create_action_ctl_bits(all_actions);

    auto &version_map = create_version_map(group);

    if (vld_bits.size() + ctl_bits.size() > 32) {
        P4C_UNIMPLEMENTED("decaf POV encoder can only accommodate match up to 32-bit currently");
        // TODO for match wider than 32-bit, we need a type wider than "unsigned"
    }

    auto all_version_bits = version_map.get_all_version_bits();

    if (all_version_bits.size() > 32) {
        P4C_UNIMPLEMENTED(
            "decaf POV encoder can only accommodate up to 32 unique version bits currently");
        // TODO for more than 32-bit, we need a type wider than "unsigned"
    }

    ordered_map<unsigned, ordered_set<const IR::TempVar *>> match_to_versions;

    // TODO prune possible vld sets
    auto vld_bits_all_onsets = enumerate_all_subsets(vld_bits);

    // TODO prune possible ctl sets
    auto ctl_bits_all_onsets = enumerate_all_subsets(ctl_bits);

    for (auto &vld_bits_onset : vld_bits_all_onsets) {
        if (vld_bits_onset.empty()) continue;

        unsigned vld = encode(vld_bits_onset, vld_bits);

        for (auto &ctl_bits_onset : ctl_bits_all_onsets) {
            if (ctl_bits_onset.empty()) continue;

            unsigned ctl = encode(ctl_bits_onset, ctl_bits);

            std::map<unsigned, assoc::set<const PHV::Field *>> on_set;

            for (auto f : group) {
                if (weak_fields.read_only_weak_fields.count(f)) continue;

                if (!is_valid(f, vld_bits_onset)) continue;

                auto &value_map_for_f = values_at_deparser.value_to_chains.at(f);

                for (auto &kv : value_map_for_f) {
                    auto &value = kv.first;

                    for (auto &chain : kv.second) {
                        unsigned key = encode_assign_chain(chain, all_actions);

                        if (key == ctl) {
                            unsigned vld_ctl = vld << ctl_bits.size() | ctl;

                            auto version = version_map.value_to_version.at(f).at(value);
                            match_to_versions[vld_ctl].insert(version);

                            on_set[vld_ctl].insert(f);
                        }
                    }
                }

                // miss action, default for all fields
                if (version_map.default_version.count(f)) {
                    auto dflt_version = version_map.default_version.at(f);
                    match_to_versions[vld << ctl_bits.size()].insert(dflt_version);
                }
            }

            for (auto &kv : on_set) {
                auto key = kv.first;
                auto &on_set_for_key = kv.second;

                for (auto f : group) {
                    if (weak_fields.read_only_weak_fields.count(f)) continue;

                    if (!is_valid(f, vld_bits_onset)) continue;

                    // field is default if not on
                    if (!on_set_for_key.count(f)) {
                        if (version_map.default_version.count(f)) {
                            auto instr = version_map.default_version.at(f);
                            match_to_versions[key].insert(instr);
                        }
                    }
                }
            }
        }  // for all ctl_bits_all_onsets
    }  // for all vld_bits_all_onsets

    std::vector<const IR::Expression *> keys;

    keys.insert(keys.begin(), vld_bits.begin(), vld_bits.end());
    keys.insert(keys.end(), ctl_bits.begin(), ctl_bits.end());

    ordered_map<unsigned, unsigned> match_to_action_param;

    for (auto &mv : match_to_versions) {
        unsigned action_param = 0;

        unsigned idx = 0;
        for (auto v : all_version_bits) {
            if (mv.second.count(v)) action_param |= (1 << idx);

            idx++;
        }

        match_to_action_param[mv.first] = action_param;
    }

    auto ma = new MatchAction(keys, all_version_bits, match_to_action_param);
    return ma;
}

IR::MAU::TableSeq *SynthesizePovEncoder::preorder(IR::MAU::TableSeq *seq) {
    prune();

    gress_t gress = VisitingThread(this);

    if (tables_to_insert.count(gress)) {
        LOG2("\n" << tables_to_insert.at(gress).size() << " tables to insert on " << gress);

        for (auto t : tables_to_insert.at(gress)) seq->tables.push_back(t);
    }

    return seq;
}

unsigned SynthesizePovEncoder::num_coalesced_match_bits(const FieldGroup &a, const FieldGroup &b) {
    auto av = get_valid_bits(a);
    auto aa = get_all_actions(a);
    auto bv = get_valid_bits(b);
    auto ba = get_all_actions(b);

    assoc::set<const IR::Expression *> vs;
    ordered_set<const IR::MAU::Action *> as;

    for (auto x : av) vs.insert(x);
    for (auto x : aa) as.insert(x);
    for (auto x : bv) vs.insert(x);
    for (auto x : ba) as.insert(x);

    return as.size() + vs.size();
}

FieldGroup join(const FieldGroup &a, const FieldGroup &b, int id) {
    FieldGroup joined(id);

    for (auto x : a) joined.insert(x);
    for (auto x : b) joined.insert(x);

    LOG3("joined group " << a.id << " and " << b.id << " as group " << id);
    LOG3(print_field_group(joined));

    return joined;
}

bool SynthesizePovEncoder::is_trivial_group(const FieldGroup &group) {
    if (group.size() != 1) return false;

    auto f = *(group.begin());
    auto &value_map_for_field = values_at_deparser.value_to_chains.at(f);

    for (auto &kv : value_map_for_field) {
        auto &chains = kv.second;
        for (auto &chain : chains) {
            if (chain.size() > 1) return false;

            auto assign = chain.at(0);
            auto action = weak_fields.get_action(assign);

            auto vld_bit = pov_bits.field_to_valid_bit.at(f);
            bool has_vld_bit_set = false;

            // check if f is validated in the same action as the value assignment
            if (pov_bits.validate_to_action.count(vld_bit)) {
                if (pov_bits.validate_to_action.at(vld_bit).count(action)) has_vld_bit_set = true;
            }

            if (!has_vld_bit_set) return false;

            // check f has no invalidation in the control flow
            // *technically* we only need to make sure there is invalidation after
            // all the value assignments to f, so this check is correct but too strict TODO
            if (pov_bits.invalidate_to_action.count(vld_bit)) return false;
        }
    }

    return true;
}

void SynthesizePovEncoder::resolve_trivial_group(const FieldGroup &group) {
    BUG_CHECK(group.size() == 1, "trivial group has more than 1 fields?");
    LOG1("\nresolve trivial group " << group.id << ":");

    auto &version_map = create_version_map(group);

    auto f = *(group.begin());
    auto &value_map_for_field = values_at_deparser.value_to_chains.at(f);

    for (auto &kv : value_map_for_field) {
        const IR::TempVar *version_bit = nullptr;

        if (kv.first.field && kv.first.field == f)
            version_bit = version_map.default_version.at(f);
        else
            version_bit = version_map.value_to_version.at(f).at(kv.first);

        auto &chains = kv.second;
        for (auto &chain : chains) {
            BUG_CHECK(chain.size() == 1, "trivial group cannot have chain depth greater than 1");

            auto assign = chain.at(0);
            assign_to_version_bit[assign->instr] = version_bit;
        }
    }
}

std::pair<std::map<gress_t, std::vector<FieldGroup>>, std::map<gress_t, std::vector<FieldGroup>>>
SynthesizePovEncoder::coalesce_weak_field_groups() {
    std::map<gress_t, std::vector<FieldGroup>> trivial, coalesced;

    int curr_id = values_at_deparser.weak_field_groups.size();

    for (auto &group : values_at_deparser.weak_field_groups) {
        auto gress = get_gress(group);

        if (is_trivial_group(group)) {
            trivial[gress].push_back(group);
            LOG3("group " << group.id << " is trivial (no encoder needed)");
        } else {
            bool joined = false;
            for (auto &coal : coalesced[gress]) {
                // greedily join groups till match bits exceed 10 bits (single SRAM)
                // could optimize by dynamic programming (knapsack problem) (TODO)

                if (num_coalesced_match_bits(coal, group) <= 10) {
                    coal = join(coal, group, curr_id++);
                    joined = true;
                    break;
                }
            }

            if (!joined) coalesced[gress].push_back(group);
        }
    }

    return {trivial, coalesced};
}

Visitor::profile_t SynthesizePovEncoder::init_apply(const IR::Node *root) {
    auto rv = MauTransform::init_apply(root);

    tables_to_insert.clear();
    value_to_pov_bit.clear();
    action_to_ctl_bit.clear();
    assign_to_version_bit.clear();

    // Also, within each table, we can compress the number of match entries
    // using don't care's (and use TCAM to implement table) -- this is classical
    // boolean optimization (TODO).

#if 1
    // coalesce weak groups to save logic IDs, and for better table placement
    // locality (i.e. save stages)

    std::map<gress_t, std::vector<FieldGroup>> trivial, coalesced;

    std::tie(trivial, coalesced) = coalesce_weak_field_groups();

    for (auto &gg : trivial) {
        for (auto &group : gg.second) resolve_trivial_group(group);
    }

    std::map<gress_t, std::vector<const MatchAction *>> match_actions;

    for (auto &gg : coalesced) {
        auto gress = gg.first;

        for (auto &group : gg.second) {
            auto match_action = create_match_action(group);
            match_actions[gress].push_back(match_action);
        }
    }
#else
    std::map<gress_t, std::vector<const MatchAction *>> match_actions;

    for (auto &group : values_at_deparser.weak_field_groups) {
        auto gress = get_gress(group);
        auto match_action = create_match_action(group);

        match_actions[gress].push_back(match_action);
    }
#endif

    for (auto &gm : match_actions) {
        for (auto &ma : gm.second) {
            cstring table_name = "_decaf_pov_encoder_"_cs;
            cstring action_name = "__soap__"_cs;
            auto table = create_pov_encoder(gm.first, table_name, action_name, *ma);

            tables_to_insert[table->gress].push_back(table);
        }
    }

    return rv;
}

void CreateConstants::create_temp_var_for_parser_constant_bytes(
    gress_t gress, const ordered_set<const IR::Constant *> &constants) {
    for (auto c : constants) {
        unsigned width = c->type->width_bits();

        // BUG_CHECK(width % 8 == 0, "constant not byte-aligned?");
        // XXX this check too strict or program incorrect?
        // p4_16/ptf/int_transit.p4

        auto cc = *c;

        while (width) {
            uint8_t l_s_byte = static_cast<uint8_t>(static_cast<big_int>(cc.value & 0xff));

            const_to_bytes[gress][c].insert(const_to_bytes[gress][c].begin(),
                                            l_s_byte);  // order is important

            if (deparser_bytes[gress].size() < Device::pardeSpec().numDeparserConstantBytes()) {
                deparser_bytes[gress].insert(l_s_byte);
            } else if (!byte_to_temp_var[gress].count(l_s_byte)) {
                std::string name = "$constant_" + std::to_string(cid++);

                auto temp_var = create_temp_var(cstring(name), 8);
                byte_to_temp_var[gress][l_s_byte] = temp_var;

                LOG4("created temp var " << name << " " << (void *)c << " 0x" << std::hex
                                         << (unsigned)l_s_byte << std::dec);
            }

            cc = cc >> 8;
            width -= 8;
        }
    }
}

void CreateConstants::dbprint(std::ostream &out) const {
    for (auto &kv : byte_to_temp_var) {
        auto gress = kv.first;

        if (byte_to_temp_var.count(gress)) {
            out << "\ntotal constant bytes: " << byte_to_temp_var.at(gress).size() << " at "
                << gress << std::endl;

            for (auto &kv : byte_to_temp_var.at(gress)) {
                out << "    0x" << std::hex << (unsigned)kv.first << std::dec << " ("
                    << (unsigned)kv.first << ")" << std::endl;
            }
        }
    }
}

Visitor::profile_t CreateConstants::init_apply(const IR::Node *root) {
    auto rv = PardeTransform::init_apply(root);

    cid = 0;
    const_to_bytes.clear();
    byte_to_temp_var.clear();
    deparser_bytes.clear();

    for (auto &kv : values_at_deparser.weak_fields.all_constants)
        create_temp_var_for_parser_constant_bytes(kv.first, kv.second);

    return rv;
}

void CreateConstants::insert_init_consts_state(IR::BFN::Parser *parser) {
    auto transition = new IR::BFN::Transition(match_t(), 0, parser->start);

    auto init_consts = new IR::BFN::ParserState(createThreadName(parser->gress, "$init_consts"_cs),
                                                parser->gress, {}, {}, {transition});

    for (auto &kv : const_to_bytes[parser->gress]) {
        for (auto byte : kv.second) {
            if (byte_to_temp_var[parser->gress].count(byte)) {
                auto temp_var = byte_to_temp_var.at(parser->gress).at(byte);

                auto rval = new IR::BFN::ConstantRVal(IR::Type::Bits::get(8), byte);
                auto extract = new IR::BFN::Extract(temp_var, rval);

                init_consts->statements.push_back(extract);
                LOG3("add " << extract << " to " << init_consts->name);
            }
        }
    }

    parser->start = init_consts;
}

IR::BFN::Parser *CreateConstants::preorder(IR::BFN::Parser *parser) {
    /*
      We choose to insert the constants at the beginning of the parser,
      but it doesn't have to be the case -- there are states throughout the
      parse graph with unused extractors where we can stash these constants
      thus potentially saving some parse bandwidth on the critical path.
      The problem is that all parse paths should parse the same set of constants
      while minimizing the states on the critical path. This is a global
      instruction scheduling program in the parser -- can be solved as a general
      parser optimizaiton pass (TODO).
    */

    if (const_to_bytes.count(parser->gress)) insert_init_consts_state(parser);

    return parser;
}

const IR::Node *RewriteWeakFieldWrites::preorder(IR::MAU::Action *action) {
    auto orig = getOriginal<IR::MAU::Action>();

    curr_to_orig[action] = orig;

    return action;
}

bool RewriteWeakFieldWrites::cache_instr(const IR::MAU::Action *orig_action,
                                         const IR::MAU::Instruction *new_instr) {
    if (orig_action_to_new_instr.count(orig_action)) {
        auto cached_instr = orig_action_to_new_instr.at(orig_action);

        BUG_CHECK(cached_instr->equiv(*new_instr), "set instr unequivalent to cached instr!");

        return false;
    }

    orig_action_to_new_instr[orig_action] = new_instr;

    return true;
}

const IR::Node *RewriteWeakFieldWrites::postorder(IR::MAU::Instruction *instr) {
    auto orig_instr = getOriginal<IR::MAU::Instruction>();

    if (synth_pov_encoder.assign_to_version_bit.count(orig_instr)) {
        // trivial case, directly set version bit

        auto version_bit = synth_pov_encoder.assign_to_version_bit.at(orig_instr);
        auto rv = create_set_bit_instr(version_bit);

        LOG3("rewrite " << instr << " as " << rv);
        return rv;
    } else {
        bool is_weak_assign = values_at_deparser.is_weak_assign(orig_instr);
        if (is_weak_assign) {
            // non-trivial case, set ctl bit

            auto action = findContext<IR::MAU::Action>();
            auto orig_action = curr_to_orig.at(action);

            auto ctl_bit = synth_pov_encoder.action_to_ctl_bit.at(orig_action);
            auto rv = create_set_bit_instr(ctl_bit);

            LOG3("rewrite " << instr << " as " << rv);

            bool cached = cache_instr(orig_action, rv);
            return cached ? rv : nullptr;
        }
    }

    return instr;
}

void RewriteDeparser::add_emit(IR::Vector<IR::BFN::Emit> &emits, const IR::Expression *source,
                               const IR::TempVar *pov_bit) {
    auto e = new IR::BFN::EmitField(source, pov_bit);
    emits.push_back(e);

    LOG3(e);
}

void RewriteDeparser::add_emit(IR::Vector<IR::BFN::Emit> &emits, uint8_t value,
                               const IR::TempVar *pov_bit) {
    auto c = new IR::Constant(IR::Type::Bits::get(8), value);
    auto e = new IR::BFN::EmitConstant(new IR::BFN::FieldLVal(pov_bit), c);
    emits.push_back(e);

    LOG3(e);
}

const IR::Expression *RewriteDeparser::find_emit_source(const PHV::Field *field,
                                                        const IR::Vector<IR::BFN::Emit> &emits) {
    for (auto prim : emits) {
        if (auto emit = prim->to<IR::BFN::EmitField>()) {
            auto f = phv.field(emit->source->field);
            if (f == field) return emit->source->field;
        }
    }

    return nullptr;
}

ordered_set<ordered_set<const IR::Expression *>> RewriteDeparser::get_all_disjoint_pov_bit_sets() {
    ordered_set<ordered_set<const IR::Expression *>> rv;

    auto &value_to_pov_bit = synth_pov_encoder.value_to_pov_bit;
    auto &default_pov_bit = synth_pov_encoder.default_pov_bit;

    for (auto &fb : default_pov_bit) {
        ordered_set<const IR::Expression *> disjoint_set;
        disjoint_set.insert(fb.second);

        for (auto vb : value_to_pov_bit.at(fb.first)) disjoint_set.insert(vb.second);

        rv.insert(disjoint_set);
    }

    if (LOGGING(4)) {
        LOG4("total disjoint pov bit sets: " << rv.size());
        for (auto &s : rv) {
            for (auto e : s) LOG4(e);
            LOG4("");
        }
    }

    return rv;
}

std::vector<const IR::BFN::Emit *> RewriteDeparser::coalesce_disjoint_emits(
    const std::vector<const IR::BFN::Emit *> &emits) {
    if (LOGGING(5)) {
        LOG5("coalesce group:");
        for (auto e : emits) LOG5(e);
    }

    ordered_set<const IR::Expression *> pov_bits;

    for (auto emit : emits) {
        auto pov_bit = emit->povBit->field;
        pov_bits.insert(pov_bit);
    }

    std::vector<const IR::BFN::Emit *> rv;

    for (auto pov_bit : pov_bits) {
        for (auto emit : emits) {
            if (pov_bit == emit->povBit->field) rv.push_back(emit);
        }
    }

    if (LOGGING(5)) {
        LOG5("result:");
        for (auto e : rv) LOG5(e);
    }

    return rv;
}

// check if emit belongs to a disjoint set
static const ordered_set<const IR::Expression *> *belongs_to(
    const IR::BFN::EmitField *emit,
    const ordered_set<ordered_set<const IR::Expression *>> &disjoint_pov_sets) {
    const ordered_set<const IR::Expression *> *disjoint_set = nullptr;
    auto pov_bit = emit->povBit->field;

    for (auto &ds : disjoint_pov_sets) {
        if (ds.count(pov_bit)) {
            disjoint_set = &ds;
            break;
        }
    }

    return disjoint_set;
}

void RewriteDeparser::coalesce_emits_for_packing(
    IR::BFN::Deparser *deparser,
    const ordered_set<ordered_set<const IR::Expression *>> &disjoint_pov_sets) {
    IR::Vector<IR::BFN::Emit> rv;

    for (auto it = deparser->emits.begin(); it != deparser->emits.end(); it++) {
        auto prim = *it;

        if (auto emit = prim->to<IR::BFN::EmitField>()) {
            auto disjoint_set = belongs_to(emit, disjoint_pov_sets);

            if (!disjoint_set) {
                rv.push_back(prim);
            } else {
                // collect all emits in disjoint set
                std::vector<const IR::BFN::Emit *> disjoint_emits;

                auto jt = it;
                for (; jt != deparser->emits.end(); jt++) {
                    if (!disjoint_set->count((*jt)->povBit->field)) break;

                    disjoint_emits.push_back(*jt);
                }

                // coalesce and add to result vector
                auto coalesced = coalesce_disjoint_emits(disjoint_emits);
                for (auto e : coalesced) rv.push_back(e);

                it = jt - 1;
            }
        } else {
            rv.push_back(prim);
        }
    }

    deparser->emits = rv;
}

void RewriteDeparser::infer_deparser_no_repeat_constraint(
    IR::BFN::Deparser *deparser,
    const ordered_set<ordered_set<const IR::Expression *>> &disjoint_pov_sets) {
    for (auto it = deparser->emits.begin(); it != deparser->emits.end(); it++) {
        auto ei = (*it)->to<IR::BFN::EmitField>();
        if (!ei) continue;

        if (auto temp_var = ei->source->field->to<IR::TempVar>()) {
            if (create_consts.is_inserted(temp_var)) continue;
        }

        for (auto jt = it + 1; jt != deparser->emits.end(); jt++) {
            auto ej = (*jt)->to<IR::BFN::EmitField>();
            if (!ej) continue;

            if (ei->source->field == ej->source->field) {
                auto dsi = belongs_to(ei, disjoint_pov_sets);
                auto dsj = belongs_to(ej, disjoint_pov_sets);

                if (dsi || dsj) {
                    bool can_repeat = true;

                    for (auto kt = it + 1; kt != jt; kt++) {
                        if (auto ek = (*kt)->to<IR::BFN::EmitField>()) {
                            if (ek->povBit->field == ej->povBit->field ||
                                ek->povBit->field == ei->povBit->field) {
                                can_repeat = false;
                                break;
                            }
                        } else {
                            can_repeat = false;
                            break;
                        }
                    }

                    if (can_repeat) {
                        auto field = phv.field(ei->source->field);

                        BUG_CHECK(field && field->size % 8 == 0,
                                  "non byte-aligned field gets decaf'd?");

                        if (field->size == 16 || field->size == 32)
                            must_split_fields.insert(field->name);
                    }
                }
            }
        }
    }

    if (LOGGING(5)) {
        LOG5("fields can repeat in the FD:");
        for (auto f : must_split_fields) LOG5(f);
    }
}

static void print_deparser(const IR::BFN::Deparser *deparser) {
    if (LOGGING(4)) {
        for (auto prim : deparser->emits) LOG4(prim);
    }
}

bool RewriteDeparser::preorder(IR::BFN::Deparser *deparser) {
    IR::Vector<IR::BFN::Emit> new_emits;

    auto &value_to_pov_bit = synth_pov_encoder.value_to_pov_bit;
    auto &default_pov_bit = synth_pov_encoder.default_pov_bit;

    ordered_map<const IR::Constant *, std::vector<uint8_t>> const_to_bytes;
    ordered_map<uint8_t, const IR::TempVar *> byte_to_temp_var;
    ordered_set<uint8_t> deparser_bytes;

    auto &c2b = create_consts.const_to_bytes;
    auto &b2t = create_consts.byte_to_temp_var;
    auto &db = create_consts.deparser_bytes;

    if (c2b.count(deparser->gress)) {
        const_to_bytes = c2b.at(deparser->gress);
        byte_to_temp_var = b2t.at(deparser->gress);
        deparser_bytes = db.at(deparser->gress);
    }

    for (auto prim : deparser->emits) {
        if (auto emit = prim->to<IR::BFN::EmitField>()) {
            auto f = phv.field(emit->source->field);

            if (value_to_pov_bit.count(f)) {
                LOG3("rewrite " << emit << " as:");

                for (auto &kv : value_to_pov_bit.at(f)) {
                    auto &value = kv.first;
                    auto pov_bit = kv.second;

                    if (value.field) {
                        auto emit_source = find_emit_source(value.field, deparser->emits);

                        BUG_CHECK(emit_source, "emit source not found?");

                        add_emit(new_emits, emit_source, pov_bit);
                    } else {  // constant
                        std::vector<uint8_t> bytes;

                        // Original constant IR node may have already been invalidated
                        // during IR rewrite so we do a deep comparison to find original
                        // constant reference.
                        for (auto &kv : const_to_bytes) {
                            if (value.constant->equiv(*kv.first)) bytes = kv.second;
                        }

                        BUG_CHECK(!bytes.empty(), "Constant %1% not extracted in parser?",
                                  value.constant);

                        for (auto byte : bytes) {
                            if (deparser_bytes.count(byte)) {
                                add_emit(new_emits, byte, pov_bit);
                            } else if (byte_to_temp_var.count(byte)) {
                                auto source = byte_to_temp_var.at(byte);
                                add_emit(new_emits, source, pov_bit);
                            } else {
                                BUG("decaf constant source not found?");
                            }
                        }
                    }
                }

                if (default_pov_bit.count(f)) {
                    auto dflt_pov_bit = default_pov_bit.at(f);
                    add_emit(new_emits, emit->source->field, dflt_pov_bit);
                }

                continue;
            }
        }

        new_emits.push_back(prim);
    }

    deparser->emits = new_emits;

    LOG4("deparser after rewrite:");
    print_deparser(deparser);

    auto disjoint_pov_sets = get_all_disjoint_pov_bit_sets();

    coalesce_emits_for_packing(deparser, disjoint_pov_sets);

    LOG4("deparser after coalescing:");
    print_deparser(deparser);

    infer_deparser_no_repeat_constraint(deparser, disjoint_pov_sets);

    return false;
}
