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

#ifndef BF_P4C_IR_TABLE_TREE_H_
#define BF_P4C_IR_TABLE_TREE_H_

#include "ir/ir.h"
#include "lib/hex.h"
#include "lib/indent.h"
#include "lib/n4.h"
#include "lib/ordered_map.h"
#include "lib/safe_vector.h"

class TableTree {
    mutable indent_t indent;
    mutable std::set<const IR::MAU::TableSeq *> done;
    safe_vector<cstring> name;
    const IR::MAU::TableSeq *seq;
    const IR::BFN::Pipe *pipe;

    void print(std::ostream &out,
               const safe_vector<cstring> &tag,
               const IR::MAU::TableSeq *s) const {
        const char *sep = "";
        out << indent++;
        for (auto n : tag) {
            out << sep << n;
            sep = ", "; }
        out << ": [" << s->seq_uid << "]";
        if (done.count(s)) {
            out << "..." << std::endl;
        } else {
            // for large seqs, output depmatrix as a LT matrix with lables
            bool big = (s->tables.size() > 5);
            if (big) out << std::endl << indent << "  +--" << s->tables[0]->name;
            for (unsigned i = 1; i < s->tables.size(); ++i) {
                if (big) out << std::endl << indent << " ";
                out << ' ';
                for (unsigned j = 0; j < i; ++j) out << (s->deps(i, j) ? '1' : '0');
                if (big) out << "+--" << s->tables[i]->name; }
            out << std::endl;
            done.insert(s);
            for (auto tbl : s->tables) print(out, tbl); }
        --indent; }
    void print(std::ostream &out, const IR::MAU::Table *tbl) const {
        ordered_map<const IR::MAU::TableSeq *, safe_vector<cstring>> next;
        out << indent++;
        if (tbl->global_id()) out << hex(*tbl->global_id()) << ": ";
        else if (tbl->stage() != -1) out << hex(tbl->stage() * 16) << ": ";
        out << tbl->name;
        const char *sep = "(";
        for (auto &row : tbl->gateway_rows) {
            out << sep;
            if (row.first)
                out << *row.first;
            else
                out << "1";
            if (row.second) out << " => " << row.second;
            sep = ", "; }
        out << (*sep == ',' ? ")" : "");
        if (tbl->layout.match_width_bits || tbl->layout.overhead_bits) {
            out << "{ " << (tbl->layout.gateway ? "G" : "")
                << (tbl->layout.ternary ? "T" : "E") << " " << tbl->layout.match_width_bits << "+"
                << tbl->layout.overhead_bits << ", " << tbl->layout.action_data_bytes;
            if (!tbl->ways.empty()) {
                out << " [" << tbl->ways[0].width << 'x' << tbl->ways[0].match_groups;
                for (auto &way : tbl->ways) out << " " << (way.entries/1024U) << "K";
                out << "]";
            } else {
                out << " " << (tbl->layout.entries / 1024U) << "K"; }
            out << " }"; }
        auto stage = tbl->get_provided_stage();
        if (stage >= 0) out << " @stage(" << stage << ")";
        out << std::endl;
        for (auto &at : tbl->attached) {
            if (at->attached->direct) continue;
            out << indent << at->attached->kind() << " " << at->attached->name
                << " " << n4(at->attached->size) << std::endl; }
        for (auto &n : tbl->next)
            next[n.second].push_back(n.first);
        for (auto &n : next)
            print(out, n.second, n.first);
        --indent; }

 public:
    TableTree(cstring name, const IR::MAU::TableSeq *seq) : name{name}, seq(seq), pipe(nullptr)  {}
    explicit TableTree(const IR::BFN::Pipe *pipe) : name{}, seq(nullptr), pipe(pipe)  {}
    friend std::ostream &operator<<(std::ostream &out, const TableTree &tt) {
        tt.indent = indent_t();
        tt.done.clear();
        if (tt.pipe) {
            if (tt.pipe->thread[INGRESS].mau)
                tt.print(out, {"ingress"_cs}, tt.pipe->thread[INGRESS].mau);
            if (tt.pipe->thread[EGRESS].mau)
                tt.print(out, {"egress"_cs}, tt.pipe->thread[EGRESS].mau);
            if (tt.pipe->ghost_thread.ghost_mau)
                tt.print(out, {"ghost mau"_cs}, tt.pipe->ghost_thread.ghost_mau);
        } else if (tt.seq) {
            tt.print(out, tt.name, tt.seq); }
        return out; }
};

#endif /* BF_P4C_IR_TABLE_TREE_H_ */
