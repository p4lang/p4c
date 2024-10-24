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

#ifndef BF_P4C_MAU_TABLE_INJECTED_DEPS_H_
#define BF_P4C_MAU_TABLE_INJECTED_DEPS_H_

#include <map>
#include <stack>

#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/ir/control_flow_visitor.h"
#include "bf-p4c/mau/table_flow_graph.h"
#include "bf-p4c/mau/table_mutex.h"
#include "bf-p4c/mau/table_summary.h"
#include "bf-p4c/phv/phv_fields.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"
#include "mau_visitor.h"
#include "table_dependency_graph.h"

using namespace P4;

class InjectControlDependencies : public MauInspector {
    bool preorder(const IR::MAU::TableSeq *seq) override;

 private:
    DependencyGraph &dg;

 public:
    explicit InjectControlDependencies(DependencyGraph &out) : dg(out) { visitDagOnce = false; }
};

class PredicationBasedControlEdges : public MauInspector {
    DependencyGraph *dg;
    const ControlPathwaysToTable &ctrl_paths;
    ordered_map<const IR::MAU::Table *, ordered_set<const IR::MAU::Table *>> edges_to_add;

 public:
    std::map<cstring, const IR::MAU::Table *> name_to_table;

    bool edge(const IR::MAU::Table *a, const IR::MAU::Table *b) const {
        if (edges_to_add.count(a) == 0) return false;
        return edges_to_add.at(a).count(b);
    }

    PredicationBasedControlEdges(DependencyGraph *d, const ControlPathwaysToTable &cp)
        : dg(d), ctrl_paths(cp) {}

 private:
    profile_t init_apply(const IR::Node *node) override {
        auto rv = MauInspector::init_apply(node);
        edges_to_add.clear();
        return rv;
    }

    void postorder(const IR::MAU::Table *tbl) override;
    void end_apply() override;
};

/// Common functionality for injecting dependencies into a DependencyGraph.
class AbstractDependencyInjector : public MauInspector {
 protected:
    bool tables_placed = false;
    DependencyGraph &dg;
    const ControlPathwaysToTable &ctrl_paths;

    Visitor::profile_t init_apply(const IR::Node *node) override {
        auto rv = MauInspector::init_apply(node);
        tables_placed = false;
        return rv;
    }

    bool preorder(const IR::MAU::Table *table) override {
        tables_placed |= table->is_placed();
        return true;
    }

 public:
    AbstractDependencyInjector(DependencyGraph &dg, const ControlPathwaysToTable &cp)
        : dg(dg), ctrl_paths(cp) {}
};

/**
 * Currently it is not safe to run the flow graph after Table Placement in JBay, as the
 * DefaultNext pass won't be secure, we currently don't run it.  The flow graph really
 * just needs to be updated to run as a ControlFlowVisitor
 */
class InjectMetadataControlDependencies : public AbstractDependencyInjector {
    const PhvInfo &phv;
    const FlowGraph &fg;
    std::map<cstring, const IR::MAU::Table *> name_to_table;

    Visitor::profile_t init_apply(const IR::Node *node) override {
        auto rv = AbstractDependencyInjector::init_apply(node);
        name_to_table.clear();
        return rv;
    }

    bool preorder(const IR::MAU::Table *t) override {
        auto rv = AbstractDependencyInjector::preorder(t);
        name_to_table[t->name] = t;
        return rv;
    }

    void end_apply() override;

 public:
    InjectMetadataControlDependencies(const PhvInfo &p, DependencyGraph &g, const FlowGraph &f,
                                      const ControlPathwaysToTable &cp)
        : AbstractDependencyInjector(g, cp), phv(p), fg(f) {}
};

class InjectActionExitAntiDependencies : public AbstractDependencyInjector {
    const CalculateNextTableProp &cntp;

    void postorder(const IR::MAU::Table *table) override;

 public:
    InjectActionExitAntiDependencies(DependencyGraph &g, const CalculateNextTableProp &cntp,
                                     const ControlPathwaysToTable &cp)
        : AbstractDependencyInjector(g, cp), cntp(cntp) {}
};

class InjectControlExitDependencies : public AbstractDependencyInjector {
    std::map<gress_t, std::vector<const IR::MAU::Table *>> run_before_exit_tables;

    Visitor::profile_t init_apply(const IR::Node *node) override {
        LOG3("InjectControlExitDependencies begins");
        auto rv = AbstractDependencyInjector::init_apply(node);
        run_before_exit_tables.clear();
        return rv;
    }

    void end_apply(const IR::Node *node) override {
        Visitor::end_apply(node);
        link_run_before_exit_tables();
        LOG3("InjectControlExitDependencies ends");
    }

    bool preorder(const IR::MAU::Table *table) override;
    void postorder(const IR::MAU::Table *table) override;

    void collect_run_before_exit_table(const IR::MAU::Table *table);
    void inject_dependencies_from_gress_root_tables_to_first_rbe_table(
        const IR::MAU::Table *first_rbe_table);
    bool is_first_run_before_exit_table_in_gress(const IR::MAU::Table *rbe_table);
    const IR::MAU::TableSeq *get_gress_root_table_seq(const IR::MAU::Table *table);
    void link_run_before_exit_tables();
    void inject_control_exit_dependency(const IR::MAU::Table *source,
                                        const IR::MAU::Table *destination);

 public:
    InjectControlExitDependencies(DependencyGraph &dg, const ControlPathwaysToTable &cp)
        : AbstractDependencyInjector(dg, cp) {}
};

class InjectDarkAntiDependencies : public AbstractDependencyInjector {
    const PhvInfo &phv;
    bool placed = false;
    std::map<UniqueId, const IR::MAU::Table *> id_to_table;

    profile_t init_apply(const IR::Node *node) override {
        auto rv = AbstractDependencyInjector::init_apply(node);
        placed = false;
        id_to_table.clear();
        return rv;
    }

    bool preorder(const IR::MAU::Table *) override;
    void end_apply() override;
    // Get table corresponding to uid from id_to_table map
    // - handle case where @uid may not exists in id_to_table
    const IR::MAU::Table *getTable(UniqueId uid);

 public:
    InjectDarkAntiDependencies(const PhvInfo &p, DependencyGraph &g,
                               const ControlPathwaysToTable &cp)
        : AbstractDependencyInjector(g, cp), phv(p) {}
};

class TableFindInjectedDependencies : public PassManager {
    const PhvInfo &phv;
    DependencyGraph &dg;
    FlowGraph &fg;
    ControlPathwaysToTable ctrl_paths;
    CalculateNextTableProp cntp;
    const FieldDefUse *defuse;
    const TableSummary *summary;

    profile_t init_apply(const IR::Node *root) override {
        auto rv = PassManager::init_apply(root);
        fg.clear();
        return rv;
    }

 public:
    explicit TableFindInjectedDependencies(const PhvInfo &p, DependencyGraph &dg, FlowGraph &fg,
                                           const BFN_Options *options = nullptr,
                                           const TableSummary *summary = nullptr);

 private:
    // Duplicates to dominators
    ordered_map<const IR::MAU::TableSeq *, const IR::MAU::Table *> dominators;
};

// During alt-phv-alloc, the ALT_FINALIZE_TABLE round during table placement
// needs to have additional dependencies injected between overlayed fields which
// are disjoint. This ensures the tables are placed in the correct order to
// not violate read / write dependencies
class InjectDepForAltPhvAlloc : public MauInspector {
    const PhvInfo &phv;
    DependencyGraph &dg;
    const FieldDefUse &defuse;
    const TablesMutuallyExclusive &mutex;

    void end_apply() override;

 public:
    InjectDepForAltPhvAlloc(const PhvInfo &p, DependencyGraph &g, const FieldDefUse &du,
                            const TablesMutuallyExclusive &mu)
        : phv(p), dg(g), defuse(du), mutex(mu) {}
};

#endif /* BF_P4C_MAU_TABLE_INJECTED_DEPS_H_ */
