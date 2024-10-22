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

#ifndef BF_P4C_MAU_DEFAULT_NEXT_H_
#define BF_P4C_MAU_DEFAULT_NEXT_H_

#include "bf-p4c/ir/control_flow_visitor.h"
#include "bf-p4c/ir/table_tree.h"
#include "bf-p4c/phv/utils/utils.h"  // for operator<<(ordered_set)
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"
#include "mau_visitor.h"
#include "next_table.h"

using namespace P4;

class DefaultNext : public MauInspector, public NextTable, BFN::ControlFlowVisitor {
    int id = -1;
    static int id_counter;
    const bool &long_branch_disabled;
    std::set<cstring> *errors;
    ordered_map<const IR::MAU::Table *, ordered_set<const IR::MAU::Table *>> &possible_nexts;
    ordered_set<const IR::MAU::Table *> prev_tbls;

    bool preorder(const IR::Expression *) override { return false; }

    bool preorder(const IR::MAU::Table *tbl) override {
        LOG3(id << ": DefaultNext::preorder(" << tbl->name << ") prev=" << DBPrint::Brief
                << prev_tbls << DBPrint::Reset);
        if (tbl->is_detached_attached_tbl) return true;
        if (tbl->run_before_exit) collect_run_before_exit_table(tbl);
        for (auto prev : prev_tbls) {
            if (possible_nexts.count(prev)) {
                // Disabling for JBay, really only used for the characterize power, and necessary
                // to run until that pass is converted to a ControlFlowVisitor
                if (!possible_nexts.at(prev).count(tbl) && long_branch_disabled) {
                    error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                          "%1% is applied in multiple "
                          "places, and the next-table information cannot correctly propagate "
                          "through this multiple application",
                          prev);
                    if (errors) errors->insert(prev->externalName());
                }
            }
            possible_nexts[prev].insert(tbl);
        }
        prev_tbls.clear();
        return true;
    }

    void collect_run_before_exit_table(const IR::MAU::Table *rbe_table) {
        run_before_exit_tables[rbe_table->gress].push_back(rbe_table);
    }

    void postorder(const IR::MAU::Table *tbl) override {
        LOG3(id << ": DefaultNext::postorder(" << tbl->name << ")");
        prev_tbls.insert(tbl);
    }

    DefaultNext *clone() const override {
        auto *rv = new DefaultNext(*this);
        rv->id = ++id_counter;
        LOG3(id << ": DefaultNext::clone -> " << rv->id);
        return rv;
    }

    void flow_merge(Visitor &a_) override {
        auto &a = dynamic_cast<DefaultNext &>(a_);
        LOG3(id << ": DefaultNext::flow_merge <- " << a.id);
        prev_tbls.insert(a.prev_tbls.begin(), a.prev_tbls.end());
    }
    void flow_copy(::ControlFlowVisitor &a_) override {
        auto &a = dynamic_cast<DefaultNext &>(a_);
        prev_tbls = a.prev_tbls;
        run_before_exit_tables = a.run_before_exit_tables;
    }

    DefaultNext(const DefaultNext &a) = default;

    bool filter_join_point(const IR::Node *n) override { return !n->is<IR::MAU::TableSeq>(); }

    profile_t init_apply(const IR::Node *root) override {
        LOG3("DefaultNext starting");
        id = id_counter = 0;
        return MauInspector::init_apply(root);
    }

    bool preorder(const IR::BFN::Pipe *pipe) override {
        LOG5(TableTree("ingress"_cs, pipe->thread[INGRESS].mau)
             << TableTree("egress"_cs, pipe->thread[EGRESS].mau)
             << TableTree("ghost"_cs, pipe->ghost_thread.ghost_mau));
        possible_nexts.clear();
        prev_tbls.clear();
        return true;
    }

    void postorder(const IR::BFN::Pipe *) override {
        if (long_branch_disabled) {
            for (auto prev : prev_tbls) {
                if (possible_nexts.count(prev)) {
                    error(ErrorType::ERR_UNSUPPORTED_ON_TARGET,
                          "%1% is applied in multiple "
                          "places, and the next-table information cannot correctly propagate "
                          "through this multiple application",
                          prev);
                    if (errors) errors->insert(prev->externalName());
                }
            }
        }
    }

 public:
    std::map<gress_t, std::vector<const IR::MAU::Table *>> run_before_exit_tables;

    explicit DefaultNext(const bool &lbd, std::set<cstring> *errs = nullptr)
        : long_branch_disabled(lbd),
          errors(errs),
          possible_nexts(*new std::remove_reference<decltype(possible_nexts)>::type) {
        joinFlows = false;
        visitDagOnce = false;
        BackwardsCompatibleBroken = true;
    }

    const IR::MAU::Table *next(const IR::MAU::Table *t) const {
        if (possible_nexts.count(t)) {
            BUG_CHECK(!possible_nexts.at(t).empty(), "unexpected empty set");
            return possible_nexts.at(t).front();
        }
        return nullptr;
    }

    ordered_set<const IR::MAU::Table *> possible_next(const IR::MAU::Table *t) const {
        if (possible_nexts.count(t)) return possible_nexts.at(t);
        return {};
    }

    ordered_set<const IR::MAU::Table *> possible_next(cstring name) const {
        for (auto &pn : possible_nexts) {
            if (pn.first->name == name) return pn.second;
            if (pn.first->match_table && pn.first->externalName() == name) return pn.second;
        }
        return {};
    }

    const IR::MAU::Table *next_in_thread(const IR::MAU::Table *t) const {
        if (auto *n = next(t))
            if (n->gress == t->gress) return n;
        return nullptr;
    }

    UniqueId next_in_thread_uniq_id(const IR::MAU::Table *t) const {
        if (auto *n = next(t))
            if (n->gress == t->gress) return n->unique_id();
        return UniqueId("END"_cs);
    }

    std::set<UniqueId> possible_next_uniq_id(const IR::MAU::Table *t) const {
        std::set<UniqueId> rv;
        for (auto n : possible_next(t))
            if (n->gress == t->gress) rv.insert(n->unique_id());
        if (rv.empty()) rv.insert(UniqueId("END"_cs));
        return rv;
    }

    ordered_set<UniqueId> next_for(const IR::MAU::Table *tbl, cstring what) const override {
        if (what == "$miss" && tbl->next.count("$try_next_stage"_cs)) what = "$try_next_stage"_cs;
        if (tbl->actions.count(what) && tbl->actions.at(what)->exitAction) {
            if (run_before_exit_tables.count(tbl->gress) && !tbl->run_before_exit)
                return {(*run_before_exit_tables.at(tbl->gress).begin())->unique_id()};
            else
                return {};
        }
        if (!tbl->next.count(what)) what = "$default"_cs;
        if (tbl->next.count(what) && !tbl->next.at(what)->empty())
            return {tbl->next.at(what)->front()->unique_id()};
        auto *nt = next_in_thread(tbl);
        if (nt) return {nt->unique_id()};
        return {};
    }

    bool uses_next_table(const IR::MAU::Table *) const override { return true; }

    void dbprint(std::ostream &out) const override {
        out << "DefaultNext:" << IndentCtl::indent;
        for (auto &pn : possible_nexts) {
            out << Log::endl << pn.first << ": {";
            const char *sep = " ";
            for (auto el : pn.second) {
                out << sep << el;
                sep = ", ";
            }
            out << (sep + 1) << "}";
        }
        out << IndentCtl::unindent;
    }
};

#endif /* BF_P4C_MAU_DEFAULT_NEXT_H_ */
