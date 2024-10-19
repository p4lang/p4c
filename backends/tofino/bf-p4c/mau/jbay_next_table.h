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

#ifndef BACKENDS_TOFINO_BF_P4C_MAU_JBAY_NEXT_TABLE_H_
#define BACKENDS_TOFINO_BF_P4C_MAU_JBAY_NEXT_TABLE_H_

#include "bf-p4c/lib/dyn_vector.h"
#include "bf-p4c/mau/memories.h"
#include "bf-p4c/mau/table_layout.h"
#include "mau_visitor.h"
#include "next_table.h"
#include "table_control_deps.h"
#include "table_mutex.h"

using namespace P4;

/* This pass determines next table propagation in tofino2. It minimizes the use of long branches
 * whenever possible by "pushing down" next table propagation to tables in the table sequence and
 * using global_exec/local_exec instead. This pass must be run after modifications to IR graph are
 * done, otherwise its data will be invalidated.
 */
class JbayNextTable : public PassRepeated, public NextTable, public IHasDbPrint {
 public:
    // Map from tables->condition (tseq names)->sets of tables that need to occur next
    using next_map_t = std::map<UniqueId, std::unordered_map<cstring, std::set<UniqueId>>>;
    next_map_t props;
    // Map from table to tag # to set of tables
    std::map<UniqueId, std::unordered_map<int, std::set<UniqueId>>> lbs;
    size_t get_num_lbs() { return size_t(max_tag + 1); }  // For metrics
    size_t get_num_dts() { return num_dts; }
    /**
     * @return pair consisting of the live range of a long branch that activates
     *         the indicated destination.  The pair is first stage, last stage.
     *         If no destination is found, returns -1, -1.
     */
    std::pair<ssize_t, ssize_t> get_live_range_for_lb_with_dest(UniqueId dest) const;
    explicit JbayNextTable(bool disableNextTableUse = false);
    friend std::ostream &operator<<(std::ostream &out, const JbayNextTable &nt) {
        out << "Long Branches:" << std::endl;
        for (auto id_tag : nt.lbs) {
            out << "  " << id_tag.first << " has long branch mapping:" << std::endl;
            for (auto tag_tbls : id_tag.second) {
                out << "    " << tag_tbls.first << ":" << std::endl;
                for (auto t : tag_tbls.second) {
                    out << "      " << t << std::endl;
                }
            }
        }
        if (nt.lbs.size() == 0) out << "  unused" << std::endl;
        out << "Liveness:" << std::endl;
        for (auto luse : nt.lbus) {
            out << "  " << luse << std::endl;
        }
        return out;
    }
    void dbprint(std::ostream &out) const { out << *this; }
    ordered_set<UniqueId> next_for(const IR::MAU::Table *tbl, cstring what) const;
    const std::unordered_map<int, std::set<UniqueId>> &long_branches(UniqueId tbl) const {
        if (!lbs.count(tbl)) return NextTable::long_branches(tbl);
        return lbs.at(tbl);
    }

 private:
    // Represents a long branch targeting a single destination (unique on destination)
    class LBUse {
     public:
        ssize_t fst, lst;                        // First and last stages this tag is used on
        const IR::MAU::Table *dest;              // The destination related with this use
        explicit LBUse(const IR::MAU::Table *d)  // Dummy constructor for lookup by destination
            : fst(-1), lst(-1), dest(d) {}
        LBUse(const IR::MAU::Table *d, size_t f, size_t l)  // Construct a LBUse for a dest
            : fst(f), lst(l), dest(d) {}
        // Always unique on dest, ensure that the < is deterministic
        bool operator<(const LBUse &r) const { return dest->unique_id() < r.dest->unique_id(); }
        bool operator&(const LBUse &a) const;  // Returns true if tags do have unmergeable overlap
        void extend(const IR::MAU::Table *);   // Extends this LB to a new source table
        // Returns true if on the same gress, false o.w.
        inline bool same_gress(const LBUse &r) const { return dest->thread() == r.dest->thread(); }
        // Returns true if this LB is targetable for DT, false o.w.
        inline gress_t thread() const { return dest->thread(); }
        friend std::ostream &operator<<(std::ostream &out, const LBUse &u) {
            out << "dest: " << u.dest->name << ", rng: " << u.fst << "->" << u.lst;
            return out;
        }
    };

    class LocalizeSeqs : public PassManager {
        using TableToSeqSet =
            ordered_map<const IR::MAU::Table *, ordered_set<const IR::MAU::TableSeq *>>;
        using TableToTableSet =
            ordered_map<const IR::MAU::Table *, ordered_set<const IR::MAU::Table *>>;
        using SeqToThreads = ordered_map<const IR::MAU::TableSeq *,
                                         safe_vector<safe_vector<const IR::MAU::Table *>>>;

        TableToSeqSet table_to_seqs;
        TableToTableSet can_localize_prop_to;
        SeqToThreads seq_to_threads;

        profile_t init_apply(const IR::Node *node) override {
            auto rv = PassManager::init_apply(node);
            table_to_seqs.clear();
            can_localize_prop_to.clear();
            seq_to_threads.clear();
            return rv;
        }

        class BuildTableToSeqs : public MauTableInspector {
            LocalizeSeqs &self;
            profile_t init_apply(const IR::Node *node) override {
                auto rv = MauInspector::init_apply(node);
                self.table_to_seqs.clear();
                return rv;
            }
            bool preorder(const IR::MAU::Table *) override;

         public:
            explicit BuildTableToSeqs(LocalizeSeqs &s) : self(s) { visitDagOnce = false; }
        };

        class BuildCanLocalizeMaps : public MauTableInspector {
            LocalizeSeqs &self;
            profile_t init_apply(const IR::Node *node) override {
                auto rv = MauInspector::init_apply(node);
                self.can_localize_prop_to.clear();
                return rv;
            }
            bool preorder(const IR::MAU::Table *) override;

         public:
            explicit BuildCanLocalizeMaps(LocalizeSeqs &s) : self(s) {}
        };

     public:
        LocalizeSeqs() {
            addPasses({new BuildTableToSeqs(*this), new BuildCanLocalizeMaps(*this)});
        }

        bool can_propagate_to(const IR::MAU::Table *src, const IR::MAU::Table *dst) const {
            auto src_pos = can_localize_prop_to.find(src);
            if (src_pos == can_localize_prop_to.end()) return false;
            return src_pos->second.count(dst) > 0;
        }
    };

    LocalizeSeqs localize_seqs;

    // Represents a single wire with multiple LBUses in it
    class Tag {
        std::vector<LBUse> uses;  // Live ranges of this tag
     public:
        // Attempts to add a tag. Returns true if successful, false otherwise
        bool add_use(const LBUse &);
        std::vector<LBUse>::iterator begin() { return uses.begin(); }
        std::vector<LBUse>::iterator end() { return uses.end(); }
        size_t size() const { return uses.size(); }
    };

    // Gets the unique id for a table, depending on whether the table is placed or not
    static inline UniqueId get_uid(const IR::MAU::Table *t) {
        return t->is_placed() ? t->unique_id() : t->pp_unique_id();
    }

    /*===================================Data for next_table use ================================*/
    TablesMutuallyExclusive mutex;
    TableControlDeps control_dep;
    ordered_map<const IR::MAU::Table *, std::pair<int, int>> use_next_table;
    class FindNextTableUse;
    bool uses_next_table(const IR::MAU::Table *tbl) const { return use_next_table.count(tbl) > 0; }

    /*===================================Data gathered by Prop===================================*/
    std::map<int, std::unique_ptr<Memories>> mems;          // Map from stage to tables
    std::map<int, int> stage_id;                            // Map from stage to next open LID
    ordered_set<LBUse> lbus;                                // Long branches that are needed
    ordered_map<UniqueId, ordered_set<UniqueId>> dest_src;  // Map from dest. to set of srcs
    // Map from dest. to containing seqs
    std::map<UniqueId, ordered_set<const IR::MAU::TableSeq *>> dest_ts;
    std::set<UniqueId> al_runs;  // Set of tables that always run
    int max_stage;               // The last stage seen

    // Computes a minimal scheme for how to propagate next tables and records long branches for
    // allocation. Also gathers information for dumb table allocation/addition.
    class Prop : public MauInspector {
        JbayNextTable &self;

        // Holds information for propagating a single table sequence
        struct NTInfo;
        // Adds a table and corresponding table sequence to props map
        void add_table_seq(const IR::MAU::Table *, std::pair<cstring, const IR::MAU::TableSeq *>);
        void local_prop(const NTInfo &nti, std::map<int, bitvec> &executed_paths);
        void cross_prop(const NTInfo &nti, std::map<int, bitvec> &executed_paths);
        void next_table_prop(const NTInfo &nti, std::map<int, bitvec> &executed_paths);
        bool preorder(const IR::MAU::Table *) override;

        profile_t init_apply(const IR::Node *root) override;
        void end_apply() override;

     public:
        explicit Prop(JbayNextTable &ntp) : self(ntp) {}
    };

    /*==================================Data gathered by LBAlloc==================================*/
    dyn_vector<Tag> stage_tags;     // Track usage of tags x stages so far
    size_t max_tag;                 // Largest tag allocated (number of lbs)
    std::map<LBUse, int> use_tags;  // Map from uses to tags
    // Allocates long branches into tags
    class LBAlloc : public MauInspector {
        JbayNextTable &self;
        profile_t init_apply(const IR::Node *root) override;  // Allocates long branches to tags
        bool preorder(const IR::BFN::Pipe *) override;        // Early abort
        int alloc_lb(const LBUse &);  // Finds a tag in self.stage_tags to fit the LBUse

        std::stringstream log;      // Log that captures pretty printing
        void pretty_tags();         // Print tags nice
        void pretty_srcs();         // Print src info nice
        void end_apply() override;  // Pretty print

     public:
        explicit LBAlloc(JbayNextTable &ntp) : self(ntp) {}
    };

    size_t num_dts = 0;
    // Attempts to reduce the number of tags in use to the max supported by the device
    class TagReduce : public MauTransform {
        JbayNextTable &self;

        template <class T>
        class sym_matrix;                         // Symmetric matrix for merging
        struct merge_t;                           // Represents an "in-progress" merge of two tags
        sym_matrix<merge_t> find_merges() const;  // Finds all possible merges given current tags
        merge_t merge(Tag l, Tag r) const;        // Merges two tags, creating list of dummy tables
        bool merge_tags();  // Merge tags by adding DTs to fit. Returns false if failed
        std::map<int, std::vector<IR::MAU::Table *>> stage_dts;  // Map from stage # to DTs
        void alloc_dt_mems();  // Allocates memories for DTs in stage_dts

        // Map from table sequences to dumb tables to add to sequence
        std::map<const IR::MAU::TableSeq *, std::vector<IR::MAU::Table *>> dumb_tbls;

        profile_t init_apply(const IR::Node *root) override;
        // Early abort if push-down is sufficient. Adds DTs and allocates them memories
        IR::Node *preorder(IR::BFN::Pipe *) override;
        IR::Node *preorder(IR::MAU::Table *) override;  // Set always run tags
        IR::Node *preorder(
            IR::MAU::TableSeq *) override;  // Add specified tables to table sequences
     public:
        explicit TagReduce(JbayNextTable &ntp) : self(ntp) { visitDagOnce = false; }
    };
};

struct LongBranchAllocFailed : public Backtrack::trigger {
    LongBranchAllocFailed() : trigger(OK) {}
};

#endif /* BACKENDS_TOFINO_BF_P4C_MAU_JBAY_NEXT_TABLE_H_ */
