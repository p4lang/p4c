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

#include "metadata_initialization.h"

namespace {

/** A helper class that maps phv::Field to a IR::Expression.
 * It also provide a helper function to generate a IR::MAU::Instruction to initialize a
 * field to zero.
 */
class MapFieldToExpr : public Inspector {
 private:
    const PhvInfo& phv;
    ordered_map<int, const IR::Expression*>     field_expressions;

    /// For every IR::Expression object in the program, populate the field_expressions map.
    /// This might return a Field for slice and cast expressions on fields. It will
    /// work out ok solely becuase this is a preorder and you don't prune, so it
    /// will also visit the child, which is the field, and then replace the entry in the map.
    bool preorder(const IR::Expression* expr) override {
        auto* f = phv.field(expr);
        if (!f)
            return true;
        field_expressions[f->id] = expr;
        return true; }

    // Run the algorithm.
    profile_t init_apply(const IR::Node *root) override {
        field_expressions.clear();
        return Inspector::init_apply(root);
    }

 public:
    explicit MapFieldToExpr(const PhvInfo& p)
        : phv(p) { }

    const IR::Expression* get_expr(const PHV::Field* field) const {
        BUG_CHECK(field_expressions.count(field->id),
                  "Missing IR::Expression mapping of %1%", field->name);
        return field_expressions.at(field->id); }

    /// Returns a instruction that initialize this field.
    const IR::MAU::Instruction* generate_init_instruction(const PHV::Field* f) const {
        BUG_CHECK(f, "Field is nullptr in generate_init_instruction");
        const IR::Expression* zero_expr =
            new IR::Constant(IR::Type_Bits::get(f->size), 0);
        const IR::Expression* field_expr = get_expr(f);
        auto* prim =
            new IR::MAU::Instruction("set"_cs, { field_expr, zero_expr });
        return prim;
    }
};

/** Calculate tables that we can not insert initialization of field into, because they are after
 * a write to that field. Also, this pass build a map that maps a field to a set of actions
 * that wrote to this field.
 */
class WriteTableInfo : public BFN::ControlFlowVisitor, public MauInspector, TofinoWriteContext {
 private:
    /// Map a PHV::Field.id to a set a table that may show up after any write to the field.
    /// It is wrong to insert any initialization of this field to those tables.
    using AfterWriteTableList = std::map<int, std::set<const IR::MAU::Table*>>;

    const PhvInfo                                      &phv;
    AfterWriteTableList                                table_write_info;
    std::set<int>                                      been_written;
    std::map<int, ordered_set<const IR::MAU::Action*>>    written_in_actions;
    std::map<int, ordered_set<const IR::MAU::Table*>>     written_in_tables;

    bool preorder(const IR::Expression *e) override {
        auto *f = phv.field(e);
        // Prevent visiting HeaderRefs in Members when PHV lookup fails, eg. for
        // $valid fields before allocatePOV.
        if (!f && e->is<IR::Member>()) return false;
        if (!f) return true;

        // We add this field into been_written list after calculating the
        // table_write_info for f. The reason to do so is that we can insert
        // initialization on actions of this table as long as they do not set f.
        if (isWrite()) {
            been_written.insert(f->id);
            if (auto* action = findContext<IR::MAU::Action>()) {
                written_in_actions[f->id].insert(action);
            } else {
                BUG("Can not find action context for, f = %1%", f->name);
            }
            if (auto* table = findContext<IR::MAU::Table>()) {
                written_in_tables[f->id].insert(table);
            } else {
                BUG("Can not find action context for, f = %1%", f->name);
            }
        }
        return true;
    }

    bool preorder(const IR::MAU::Table* table) override {
        for (int id : been_written) {
            if (!phv.field(id) || phv.field(id)->gress != table->gress) {
                continue; }
            table_write_info[id].insert(table);
        }
        return true;
    }

    profile_t init_apply(const IR::Node* root) override {
        for (const auto& f : phv) {
            table_write_info[f.id] = {};
            written_in_actions[f.id] = {};
            written_in_tables[f.id] = {};
        }
        been_written.clear();
        return Inspector::init_apply(root);
    }

    void end_apply() {
        if (LOGGING(5)) {
            LOG5("After Write Table List:");
            for (const auto& f : phv) {
                LOG5(f.name << " has: ");
                for (const auto* tbl : table_write_info[f.id]) {
                    LOG5("After-write table: " << tbl->name); }
                for (const auto* act : written_in_actions[f.id]) {
                    LOG5("Written in action: " << act->name); }
                for (const auto* tab : written_in_tables[f.id]) {
                    LOG5("Written in table: " << tab->name); }
            }
        }
    }

    WriteTableInfo *clone() const override { return new WriteTableInfo(*this); }

    void flow_merge(Visitor& other) override {
        WriteTableInfo& v = dynamic_cast<WriteTableInfo&>(other);
        been_written.insert(v.been_written.begin(), v.been_written.end());
        for (const auto& f : phv) {
            table_write_info[f.id].insert(
                    v.table_write_info[f.id].begin(), v.table_write_info[f.id].end()); }
        for (const auto& kv : v.written_in_actions) {
            written_in_actions[kv.first].insert(kv.second.begin(), kv.second.end());
        }
        for (const auto& kv : v.written_in_tables) {
            written_in_tables[kv.first].insert(kv.second.begin(), kv.second.end());
        }
    }
    void flow_copy(::ControlFlowVisitor& other) override {
        WriteTableInfo& v = dynamic_cast<WriteTableInfo&>(other);
        been_written = v.been_written;
        table_write_info = v.table_write_info;
        written_in_actions = v.written_in_actions;
        written_in_tables = v.written_in_tables;
    }

 public:
    explicit WriteTableInfo(const PhvInfo &p)
        : phv(p) {
        joinFlows = true;
        visitDagOnce = false;
        BackwardsCompatibleBroken = true;
        // FIXME -- setting this false breaks tofino/switch_dc_basic with --alt-phv-alloc
        // possible just something due to an empty TableSeq being used in multiple places
        // (which could be fixed by filtering it out in filter_join_points), possibly
        // something more serious.
    }

    bool is_after_write(const PHV::Field* f, const IR::MAU::Table* table) const {
        return table_write_info.at(f->id).count(table);
    }

    bool is_written_in_action(const PHV::Field* f, const IR::MAU::Action* act) const {
        BUG_CHECK(written_in_actions.count(f->id), "Missing field written result %1%", f->name);
        return written_in_actions.at(f->id).count(act);
    }

    bool is_written_in_table(const PHV::Field* f, const IR::MAU::Table* tab) const {
        BUG_CHECK(written_in_tables.count(f->id), "Missing field written result %1%", f->name);
        return written_in_tables.at(f->id).count(tab);
    }

    const ordered_set<const IR::MAU::Table*> &get_written_in_tables(const PHV::Field* f) const {
        BUG_CHECK(written_in_tables.count(f->id), "Missing field written result %1%", f->name);
        return written_in_tables.at(f->id);
    }
};

// An adjacent list graph used for table flow graph
struct TableFlowGraph {
    using VertexId         =  int;
    using AdjacentVertices =  std::vector<VertexId>;
    using AdjacentList     =  std::vector<AdjacentVertices>;

    std::map<const IR::MAU::Table*, VertexId> table_id;
    std::vector<const IR::MAU::Table*> id_table;

    AdjacentList adjacent_list;
    VertexId n_nodes;
    std::set<VertexId> starts;  // Nodes that does not have any dependency.

    /// Generate a new node corresponding to a table.
    VertexId get_node(const IR::MAU::Table* table) {
        if (table_id.count(table)) {
            return table_id.at(table);
        } else {
            VertexId id = id_table.size();
            id_table.push_back(table);
            table_id[table] = id;
            adjacent_list.push_back({});
            starts.insert(id);
            return id;
        }
    }

    /// Add a dependency: @p from must be done first before @p to.
    void addEdge(const IR::MAU::Table* from, const IR::MAU::Table* to) {
        auto from_id = get_node(from);
        auto to_id   = get_node(to);
        adjacent_list[from_id].push_back(to_id);
        starts.erase(to_id);
    }

    const IR::MAU::Table* get_table(VertexId id) const {
        BUG_CHECK(id >= 0 && id < (int)id_table.size(), "Invalid Id: %1%", id);
        return id_table[id]; }

    void clear() {
        table_id.clear();
        id_table.clear();
        adjacent_list.clear();
        n_nodes = 0;
        starts.clear();
    }

    TableFlowGraph() : n_nodes(0) { }
};

std::ostream& operator<<(std::ostream& s, const TableFlowGraph& c) {
    int ident = 0;
    std::function<void(TableFlowGraph::VertexId)>print_table_recur =
    [&](TableFlowGraph::VertexId id) {
        auto table = c.get_table(id);
        for (int i = 0; i < ident; i++) {
            s << "\t";
        }
        s << "from: " <<
            table->name << (table->is_a_gateway_table_only() ? " gw" : "") << " to: {" << std::endl;

        ident++;
        for (auto adjacent : c.adjacent_list[id]) {
            print_table_recur(adjacent);
        }
        ident--;

        for (int i = 0; i < ident; i++) {
            s << "\t";
        }
        s << "}" << std::endl;
    };
    for (auto table_id : c.starts) {
        print_table_recur(table_id);
    }
    return s;
}

/** A base class for dfs-like searching, providing a helper function
 * to check whether a node is currently being on the visiting path.
 */
class TableFlowGraphSearchBase {
 public:
    using VertexId = TableFlowGraph::VertexId;
    const TableFlowGraph& graph;
    std::set<VertexId> vis;

    class Visiting {
        TableFlowGraphSearchBase& self;
        VertexId v;
     public:
        Visiting(TableFlowGraphSearchBase& self, VertexId id)
            : self(self), v(id) { self.vis.insert(v); }
        ~Visiting() { self.vis.erase(v); }
    };

    void init_vis()                        { vis.clear(); }
    bool visited(VertexId id)              { return vis.count(id); }

    explicit TableFlowGraphSearchBase(const TableFlowGraph& graph)
        : graph(graph) { }
};

/** Initialize @p field in actions of @p table */
struct MetaInitPlan {
    const PHV::Field* field;
    const IR::MAU::Table* table;
    MetaInitPlan(const PHV::Field* field,
                 const IR::MAU::Table* table)
        : field(field), table(table) { }
    bool operator<(const MetaInitPlan& other) const {
        if (field->id != other.field->id) {
            return field->id < other.field->id;
        } else if (table->name != other.table->name) {
            return table->name < other.table->name; }
        return false;
    }
};

std::ostream& operator<<(std::ostream& s, const MetaInitPlan& c) {
    s << "Initialize "<< c.field << " ...at... " << c.table->name;
    return s;
}

/** A greedy algorithm that insert initialization as long as,
 * 1. Needed, meaning that there is uninitialized read after this table.
 * 2. Valid, in that,
 *    + This table will not be applied after any write to that field.
 *    + Field is not used in this table. If it is used in this table
 *      then initializing it here won't be able to eliminate that possible
 *      uninitialized read.
 *    + The latest stage of the table to be inserted with metadata initialization should be
 *      ahead of the earliest stage of the table needed metadata initialization.
 *    + The table to be inserted with meta init should not reside on stages that tables writing to
 *      that field is placed on, unless table to be inserted is mutex with tables that writes to
 *      that field and are on the same stage, or table to be inserted is already writing to that
 *      field.
 * This greedy algorithm does not guarantee that all the metadata can be initialized.
 * Cases that it is known not able to handle,
 * 1. Table that has uninitialized reads is on stage 0;
 * 
 * It is possible that Table that has uninitialized reads does not have any data dependency before
 * it can has metadata initialization inserted. For example,
 * {
 *     t1.apply();
 *     t2.apply();
 * }
 * t2 has an uninitialized read of v1, but t1 and t2 has no data or control dependency. However,
 * the compiler has decided that t1 is placed stage 0 and t2 is placed stage 1. In this case,
 * v1 can be initialized on stage 0.
 *
 * This algorithm will introduce data dependency, but the algorithm will respect the table placement
 * result from the previous round. Therefore, the compiler should be able to find a feasible table
 * placement, because newly injected data dependencies should be redundant cross-stage edges, in
 * terms of the result from the previous round.
 */
class GreedyMetaInit : public TableFlowGraphSearchBase {
 private:
    using TableList = ordered_set<const IR::MAU::Table*>;
    struct InitTableResult {
        TableList init_list;
        bool need_init;
        std::optional<int> used_stage_number;
    };

    // using InitTableResult = std::pair<TableList, bool>;

    const WriteTableInfo& table_write_info;
    std::set<const IR::MAU::Table*> uses;
    std::set<const IR::MAU::Table*> writes;
    std::set<const IR::MAU::Table*> touched;
    const PHV::Field* field;
    const MauBacktracker &backtracker;
    const FieldDefUse& defuse;
    const TablesMutuallyExclusive& mutex;

    bool usedIn(const IR::MAU::Table* tb)   { return uses.count(tb); }

    /** Allocate initialization for a field.
     * Algorithm:
     * DFS into the dependency graph. If a node is before some uninitialized read,
     * the field is not used in this table, and this table is not after any write to the
     * table, we initialized it in this table. This greedy strategy ensures that fields
     * are initialized to have the minimal live range.
     */
    InitTableResult greedy_allocate(VertexId v, const TableList& inited) {
        if (visited(v)) {
            LOG1("Found a loop in data dependency table");
            return { { }, false, std::nullopt }; }

        Visiting visiting(*this, v);
        auto* table = graph.get_table(v);
        bool on_use = usedIn(table);
        LOG5(field->name << " is used in table: " << table->name << (on_use ? " true" : " false"));
        bool need_init = false;
        auto stages = backtracker.stage(table, true);
        int earliest_stage = *(std::min_element(stages.begin(), stages.end()));
        int latest_stage = *(std::max_element(stages.begin(), stages.end()));
        BUG_CHECK(stages.size() > 0, "table %1% should be placed", table->name);
        LOG5(table->name << "'s earliest stage is: " << earliest_stage);
        TableList init_rst;

        // Already in initialization plans.
        if (inited.count(table)) {
            LOG5("already in initialization plan");
            return {{ }, false, std::nullopt };
        }


        int earliest_uninit_use_stage = Device::numStages();
        for (const auto& next : graph.adjacent_list[v]) {
            if (!visited(next)) {
                auto rst = greedy_allocate(next, inited);
                // Keep all the initialization that following nodes has done.
                for (const auto& tbl : rst.init_list) {
                    init_rst.insert(tbl); }
                if (!rst.need_init) continue;
                earliest_uninit_use_stage =
                    std::min(*rst.used_stage_number, earliest_uninit_use_stage);
                need_init = (need_init || rst.need_init);
            }
        }


        // calculate if this table can be placed without creating data dependencies that will
        // changes table placement result. To not change table placement result, this table must
        // meet any of following requirement for every table that writes to the field:
        // 1) is already writing to the field or,
        // 2) is mutex with a table that write to the field or
        // 3) is not placed on the same stage that a table writing the field is placed on.
        bool does_not_change_table_placement = true;

        auto& written_in_tables = table_write_info.get_written_in_tables(field);
        for (auto written_in_table : written_in_tables) {
            if (written_in_table == table) continue;
            if (mutex(table, written_in_table)) continue;
            auto written_in_table_stages = backtracker.stage(written_in_table, true);
            BUG_CHECK(written_in_table_stages.size() > 0, "table %1% should be placed",
                written_in_table->name);
            int written_in_table_earliest_stage =
                *(std::min_element(written_in_table_stages.begin(), written_in_table_stages.end()));
            int written_in_table_latest_stage =
                *(std::max_element(written_in_table_stages.begin(), written_in_table_stages.end()));
            le_bitrange written_in_table_live_range(
                written_in_table_earliest_stage, written_in_table_latest_stage);
            le_bitrange table_live_range(earliest_stage, latest_stage);
            if (table_live_range.overlaps(written_in_table_live_range)) {
                does_not_change_table_placement = false;
                break;
            }
        }

        // Insert Initialization if,
        // 1. Needed.
        // 2. This table does not uses of that field.
        // 3. Not a possible table after writting.
        // 4. latest stage of table to be inserted is ahead of ununinitialized read table's earliest
        // stage.
        // 5. This table is actually placed.
        // 6. Does not change table placement result.

        if (need_init && !on_use && does_not_change_table_placement &&
            !table_write_info.is_after_write(field, table) && !table->is_a_gateway_table_only() &&
            latest_stage < earliest_uninit_use_stage && stages.size() > 0) {
            init_rst.insert(table);
            need_init = false;
        }

        if (on_use) {
            LOG5("is on_use earliest_stage is " << earliest_stage);
            return {init_rst, true, earliest_stage};
        }
        if (need_init) {
            LOG5("is need_init earliest_uninit_use_stage is " << earliest_uninit_use_stage);
            return {init_rst, true, earliest_uninit_use_stage};
        }

        LOG5("return with need_init as false");
        return {init_rst, false, std::nullopt};
    }

 public:
    GreedyMetaInit(const TableFlowGraph &graph,
                   const FieldDefUse& defuse,
                   const WriteTableInfo& af,
                   const PHV::Field* field,
                   const MauBacktracker &backtracker,
                   const TablesMutuallyExclusive& mutex)
        : TableFlowGraphSearchBase(graph), table_write_info(af), field(field),
        backtracker(backtracker), defuse(defuse), mutex(mutex) {
        // Populate uses.
        for (const auto& use : defuse.getAllUses(field->id)) {
            if (auto* table = use.first->to<IR::MAU::Table>()) {
                uses.insert(table);
                touched.insert(table); } }
        // Populate writes.
        for (const auto& write : defuse.getAllDefs(field->id)) {
            if (auto* table = write.first->to<IR::MAU::Table>()) {
                writes.insert(table);
                touched.insert(table); } }
    }

    /** An algorithm to generate metadata initialization.
     */
    std::set<MetaInitPlan> generate_metainit_plans() {
        LOG4("Generating InitPlan for " << field);
        init_vis();
        std::set<MetaInitPlan> result;
        TableList init_at_tables;
        for (const auto& start : graph.starts) {
            LOG4("Start from, start = " << graph.get_table(start)->name);
            BUG_CHECK(!visited(start), "Not a valid start: %1%", start);
            auto rst = greedy_allocate(start, init_at_tables);
            // if the any start table need init, then,
            // no need to do initialization on this path.
            if (rst.need_init) {
                continue; }

            init_at_tables.insert(rst.init_list.begin(), rst.init_list.end());
        }

        // Generate meta init plans.
        for (const auto& tb : init_at_tables) {
            result.insert(MetaInitPlan(field, tb)); }
        return result;
    }
};

class CollectPOVProtectedField : public Inspector {
 public:
    ordered_set<const PHV::Field*> pov_protected_fields;
    const PhvInfo& phv;
    bool preorder(const IR::BFN::DeparserParameter* param) {
        if (param->source) pov_protected_fields.insert(phv.field(param->source->field));
        return false;
    }

    bool preorder(const IR::BFN::Digest* digest) {
        pov_protected_fields.insert(phv.field(digest->selector->field));
        return false;
    }
    profile_t init_apply(const IR::Node *root) override {
        pov_protected_fields.clear();
        return Inspector::init_apply(root);
    }
    explicit CollectPOVProtectedField(const PhvInfo& phv) : phv(phv) {}
};

/** Generate metadata initialization using the greedy algorithm.
 * This pass will first build a table flow graph and then
 * for each field that can be initialized, generate initialization plan,
 * which specify `init field.x on all actions(excluding actions already
 * write to the field) in table.y.
 */
class ComputeMetadataInit : public Inspector {
    const PhvInfo&                 phv;
    const FieldDefUse&             defuse;
    const WriteTableInfo&          table_write_info;
    const TableFlowGraph&          graph;
    const MauBacktracker&          backtracker;
    const TablesMutuallyExclusive& mutex;
    const PragmaNoInit&            pa_no_init;
    const ordered_set<const PHV::Field*>& pov_protected_fields;

    std::vector<const PHV::Field*> to_be_inited;

    void calc_to_be_inited_fields() {
        to_be_inited.clear();
        int n_bits = 0;
        for (const auto& f : phv) {
            if (!Device::hasMetadataPOV()) {
                // Only for tofino, if a field is invalidated by the arch, then this field is pov
                // bit protected and will not overlay with other fields. So no need to do meta init.
                if (f.is_invalidate_from_arch()) continue;
            } else {
                // For all fields that are pov bit protected fields, no need to do meta init
                for (const auto &pov_protected_field : pov_protected_fields) {
                    if (pov_protected_field->id == f.id) continue;
                }
            }
            if (f.pov) continue;
            if (f.is_padding()) continue;
            if (f.is_deparser_zero_candidate()) continue;
            if (f.is_overlayable()) continue;
            if (f.deparsed_to_tm()) continue;
            if (f.bridged) continue;
            if (!defuse.hasUninitializedRead(f.id)) continue;
            if (pa_no_init.getFields().count(&f)) continue;

            auto& defs = defuse.getAllDefs(f.id);
            bool defined_in_parser =
                std::any_of(defs.begin(), defs.end(), [] (const FieldDefUse::locpair& loc) {
                    return loc.first->is<IR::BFN::ParserState>()
                    && !loc.second->is<ImplicitParserInit>(); });
            if (defined_in_parser) {
                continue; }

            LOG4("MetaInit, Can be initialized, f = " << f);
            n_bits += f.size;
            to_be_inited.push_back(&f);
        }
        LOG4("TOTAL uninitialized bits: " << n_bits);
    }

    // Run the algorithm.
    profile_t init_apply(const IR::Node *root) override {
        calc_to_be_inited_fields();
        for (const auto* f : to_be_inited) {
            GreedyMetaInit init_strategy(graph, defuse, table_write_info, f, backtracker, mutex);
            std::set<MetaInitPlan> plans = init_strategy.generate_metainit_plans();
            for (const auto& v : plans) {
                LOG5(v);
                init_summay[v.table].push_back(v.field);
            }
        }
        return Inspector::init_apply(root);
    }

 public:
    ComputeMetadataInit(const TableFlowGraph &graph,
                        const PhvInfo& phv,
                        const FieldDefUse& defuse,
                        const WriteTableInfo& table_write_info,
                        const MauBacktracker &backtracker,
                        const TablesMutuallyExclusive& mutex,
                        const PragmaNoInit& pa_no_init,
                        const ordered_set<const PHV::Field*>& pov_protected_fields)
        :phv(phv), defuse(defuse), table_write_info(table_write_info), graph(graph),
        backtracker(backtracker), mutex(mutex), pa_no_init(pa_no_init),
        pov_protected_fields(pov_protected_fields) { }

    std::map<const IR::MAU::Table*, std::vector<const PHV::Field*>> init_summay;
};

class TableFlowGraphBuilder : public Inspector {
 public:
    TableFlowGraph graph;
    MauBacktracker &backtracker;
    int placement_stages = -1;

    explicit TableFlowGraphBuilder(MauBacktracker &backtracker) : backtracker(backtracker) {}

    bool preorder(const IR::MAU::TableSeq * table_seq) override {
        if (table_seq->tables.size() == 0) return true;
        // give tables in a TableSeq a partial order by stages, so that tables in the later stage
        // can insert metadata initialization on previous stages even if they don't have control
        // or data dependency.
        ordered_map<int, ordered_set<const IR::MAU::Table*>> earliest_stage_to_tables;
        ordered_map<int, ordered_set<const IR::MAU::Table*>> latest_stage_to_tables;
        ordered_map<const IR::MAU::Table*, int> table_to_earliest_stage;
        ordered_map<const IR::MAU::Table*, int> table_to_latest_stage;
        ordered_set<const IR::MAU::Table*> tables_not_connected;
        int placed_tables_in_table_seq = 0;

        for (auto table : table_seq->tables) {
            auto stages = backtracker.stage(table, true);
            if (stages.size() == 0) {
                continue;
            }
            int earliest_stage = *(std::min_element(stages.begin(), stages.end()));
            int latest_stage = *(std::max_element(stages.begin(), stages.end()));
            if (latest_stage >= placement_stages) placement_stages = (latest_stage + 1);
            tables_not_connected.insert(table);
            earliest_stage_to_tables[earliest_stage].insert(table);
            latest_stage_to_tables[latest_stage].insert(table);
            table_to_earliest_stage[table] = earliest_stage;
            table_to_latest_stage[table] = latest_stage;
            LOG5(table->name << " earliest stage: " << earliest_stage << " lateststage: " <<
                latest_stage);
            placed_tables_in_table_seq++;
        }
        if (placed_tables_in_table_seq == 0) return true;

        std::optional<const IR::MAU::Table*> leading_table = std::nullopt;
        std::optional<const IR::MAU::Table*> trailing_table = std::nullopt;

        for (int i = 0; i < placement_stages; i++) {
            for (auto table : earliest_stage_to_tables[i]) {
                if (!leading_table) {
                    leading_table = table;
                    trailing_table = table;
                    continue;
                }
                graph.addEdge(*trailing_table, table);
                LOG5(" in tableseq add edge from: " << (*trailing_table)->name << " to " <<
                    table->name);
                trailing_table = table;
            }
        }
        const Context * ctxt = nullptr;
        const IR::MAU::Table *parent_table = nullptr;
        do {
            parent_table = findContext<IR::MAU::Table>(ctxt);
            if (parent_table == nullptr) break;
            auto parent_stages = backtracker.stage(parent_table, true);
            LOG5("    ... found " << parent_table->name << " at stage " <<
                 *(parent_stages.begin()));
            if (parent_stages.size() > 0) break;
        } while (parent_table);

        if (parent_table) {
            LOG5("parent table " << parent_table->name);
            auto parent_stages = backtracker.stage(parent_table, true);
            BUG_CHECK(parent_stages.size() > 0, "table %1% should be placed1", parent_table->name);
            int parent_earliest_stage =
                *(std::min_element(parent_stages.begin(), parent_stages.end()));
            int parent_latest_stage =
                *(std::max_element(parent_stages.begin(), parent_stages.end()));
            le_bitrange parent_live_range(parent_earliest_stage, parent_latest_stage);
            le_bitrange leading_table_live_range(table_to_earliest_stage[*leading_table],
                table_to_latest_stage[*leading_table]);
            BUG_CHECK(parent_live_range.overlaps(leading_table_live_range) ||
                parent_live_range.hi < leading_table_live_range.lo,
                "leading table %1% is in front of parent %2%",
                (*leading_table)->name,
                parent_table->name);
            graph.addEdge(parent_table, *leading_table);
            LOG5(" cross tableseq add edge from: " << parent_table->name << " to " <<
                 (*leading_table)->name);
        }
        return true;
    }

    void end_apply() override {
        LOG4("Table Flow Graph:");
        LOG4(graph);
    }
    Visitor::profile_t init_apply(const IR::Node *root) {
        profile_t rv = Inspector::init_apply(root);
        graph.clear();
        placement_stages = Device::numStages();
        return rv;
    }
};

class ApplyMetadataInitialization : public MauTransform {
    const ComputeMetadataInit        &rst;
    const MapFieldToExpr             &phv_to_expr;
    const WriteTableInfo           &table_write_info;

 public:
    ApplyMetadataInitialization(const ComputeMetadataInit& rst,
                                const MapFieldToExpr& phv_to_expr,
                                const WriteTableInfo& table_write_info)
        : rst(rst), phv_to_expr(phv_to_expr), table_write_info(table_write_info) { }

    const IR::MAU::Action *postorder(IR::MAU::Action * act) override {
        auto* act_orig = getOriginal<IR::MAU::Action>();
        auto* table_orig = findOrigCtxt<IR::MAU::Table>();
        if (!table_orig || !rst.init_summay.count(table_orig)) {
            return act; }
        auto& fields = rst.init_summay.at(table_orig);
        for (const auto& f : fields) {
            if (table_write_info.is_written_in_action(f, act_orig)) {
                LOG4("Field " << f->name << " has been written in " << act_orig->name.name
                     << " .. skipped");
                continue; }
            auto* prim = phv_to_expr.generate_init_instruction(f);
            act->action.push_back(prim);
            LOG4("A initialization instruction for field: " << f->name << " has been inserted into "
                << table_orig->name);
        }
        return act;
    }
};


}  // unnamed namespace


PHV::v2::MetadataInitialization::MetadataInitialization(MauBacktracker& backtracker,
    const PhvInfo &phv, FieldDefUse& defuse):
    backtracker(backtracker) {
    auto* collect_pov_protected_field = new CollectPOVProtectedField(phv);
    auto* pa_no_init = new PragmaNoInit(phv);
    auto* mutex = new TablesMutuallyExclusive();
    auto* tfg_builder = new TableFlowGraphBuilder(backtracker);
    auto* field_to_expr = new MapFieldToExpr(phv);
    auto* table_write_info = new WriteTableInfo(phv);
    auto* gen_init_plans = new ComputeMetadataInit(tfg_builder->graph, phv, defuse,
        *table_write_info, backtracker, *mutex, *pa_no_init,
        collect_pov_protected_field->pov_protected_fields);
    auto* apply_init_insert =
        new ApplyMetadataInitialization(*gen_init_plans, *field_to_expr, *table_write_info);
    addPasses({
        new DumpPipe("before v2 metadata initialization"),
        collect_pov_protected_field,
        pa_no_init,
        mutex,
        tfg_builder,
        field_to_expr,
        table_write_info,
        gen_init_plans,
        apply_init_insert,
        new DumpPipe("after v2 metadata initialization")
    });
}
