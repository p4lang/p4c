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

#include "bf-p4c/phv/v2/greedy_allocator.h"

#include "bf-p4c/device.h"
#include "bf-p4c/parde/clot/clot.h"
#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/phv/v2/allocator_base.h"
#include "bf-p4c/phv/v2/greedy_tx_score.h"
#include "bf-p4c/phv/v2/kind_size_indexed_map.h"
#include "bf-p4c/phv/v2/phv_kit.h"
#include "bf-p4c/phv/v2/sort_macros.h"
#include "bf-p4c/phv/v2/trivial_allocator.h"
#include "bf-p4c/phv/v2/utils_v2.h"

namespace PHV {
namespace v2 {

namespace {

/// this function has to be defined in the file that invokes it.
Logging::FileLog *greedy_allocator_file_log(int pipeId, const cstring &prefix, int loglevel) {
    if (!LOGGING(loglevel)) return nullptr;
    auto filename = Logging::PassManager::getNewLogFileName(prefix);
    return new Logging::FileLog(pipeId, filename, Logging::Mode::AUTO);
}

// SuperClusterMetrics collects metrics for super clusters.
class SuperClusterMetrics {
 private:
    const PhvKit &kit_i;

 public:
    std::unordered_set<const SuperCluster *> has_pov;
    std::unordered_set<const SuperCluster *> non_sliceable;
    std::unordered_set<const SuperCluster *> has_no_split;
    std::unordered_set<const SuperCluster *> has_solitary;
    std::unordered_set<const SuperCluster *> has_exact_containers;
    std::unordered_set<const SuperCluster *> has_constraint_of_starting_index;
    std::unordered_map<const SuperCluster *, int> n_container_size_pragma;
    std::unordered_map<const SuperCluster *, int> n_container_type_pragma;
    std::unordered_map<const SuperCluster *, int> n_ixbar_read_bits;
    std::unordered_map<const SuperCluster *, int> n_max_sl_width_bits;
    std::unordered_map<const SuperCluster *, int> n_max_rot_cluster_size;

    SuperClusterMetrics(const PhvKit &kit, const std::list<SuperCluster *> &cluster_groups)
        : kit_i(kit) {
        for (const auto *sc : cluster_groups) {
            insert_if_any_of(sc, has_pov, [](const FieldSlice &fs) { return fs.field()->pov; });
            insert_if_any_of(sc, has_no_split, [](const FieldSlice &fs) {
                auto *f = fs.field();
                return f->no_split() || f->has_no_split_at_pos();
            });
            insert_if_any_of(sc, has_solitary,
                             [](const FieldSlice &fs) { return fs.field()->is_solitary(); });
            insert_if_any_of(sc, has_exact_containers,
                             [](const FieldSlice &fs) { return fs.field()->exact_containers(); });
            insert_if_any_of(sc, has_constraint_of_starting_index, [](const FieldSlice &fs) {
                return fs.alignment().has_value() || fs.field()->deparsed_bottom_bits() ||
                       fs.field()->deparsed_to_tm();
            });

            calc_max_sl_width_bits(sc);
            calc_non_sliceable(sc);
            calc_pa_container_size_pragma(sc);
            calc_container_type_pragma(sc);
            calc_n_max_rot_cluster_size(sc);
            calc_ixbar_read_bits(sc);
        }
    }

    bool wide_non_sliceable(const SuperCluster *sc) const {
        return n_max_sl_width_bits.at(sc) >= 8 && non_sliceable.count(sc);
    }

    int speculated_n_containers_required(const SuperCluster *sc) const {
        return std::max(n_max_rot_cluster_size.at(sc), int(sc->slice_lists().size()));
    }

 private:
    void insert_if_any_of(const SuperCluster *sc, std::unordered_set<const SuperCluster *> &set,
                          const std::function<bool(const FieldSlice &fs)> &pred) {
        if (sc->any_of_fieldslices(pred)) {
            set.insert(sc);
        }
    }

    void calc_ixbar_read_bits(const SuperCluster *sc) {
        int n_bits = 0;
        sc->forall_fieldslices([&](const FieldSlice &fs) {
            if (!kit_i.uses.ixbar_read(fs.field(), fs.range()).empty()) {
                n_bits += fs.size();
            }
        });
        n_ixbar_read_bits[sc] = n_bits;
    }

    void calc_container_type_pragma(const SuperCluster *sc) {
        const auto &pa_ct = kit_i.pragmas.pa_container_type();
        ordered_set<const Field *> fields;
        sc->forall_fieldslices([&](const FieldSlice &fs) {
            if (pa_ct.required_kind(fs.field())) fields.insert(fs.field());
        });
        n_container_type_pragma[sc] = fields.size();
    }

    void calc_pa_container_size_pragma(const SuperCluster *sc) {
        const auto &container_sizes = kit_i.pragmas.pa_container_sizes();
        ordered_set<const Field *> fields;
        sc->forall_fieldslices([&](const FieldSlice &fs) {
            if (container_sizes.is_specified(fs.field())) fields.insert(fs.field());
        });
        n_container_size_pragma[sc] = fields.size();
    }

    void calc_non_sliceable(const SuperCluster *sc) {
        auto itr_ctx = kit_i.make_slicing_ctx(sc);
        itr_ctx->set_config(
            Slicing::IteratorConfig{true, false, true, true, false, (1 << 25), (1 << 16)});
        int n = 0;
        bool sliceable = false;
        itr_ctx->iterate([&](std::list<SuperCluster *> clusters) {
            n++;
            if (clusters.size() > 1 || n > 1) {
                sliceable = true;
                return false;
            }
            return true;
        });
        if (!sliceable) {
            non_sliceable.insert(sc);
        }
    }

    void calc_max_sl_width_bits(const SuperCluster *sc) {
        n_max_sl_width_bits[sc] = sc->max_width();
        for (const auto *sl : sc->slice_lists()) {
            int length = SuperCluster::slice_list_total_bits(*sl);
            if (sl->front().alignment()) {
                length += sl->front().alignment()->align;
            }
            n_max_sl_width_bits[sc] = std::max(n_max_sl_width_bits[sc], length);
        }
    }

    void calc_n_max_rot_cluster_size(const SuperCluster *sc) {
        n_max_rot_cluster_size[sc] = 0;
        for (const auto *rot : sc->clusters()) {
            n_max_rot_cluster_size[sc] =
                std::max(n_max_rot_cluster_size[sc], int(rot->slices().size()));
        }
    }
};

struct ScAllocTracker {
    const PhvKit &kit_i;
    explicit ScAllocTracker(const PhvKit &kit) : kit_i(kit) {}

    struct AllocSummary {
        const PHV::SuperCluster *sc;
        gress_t gress;
        ordered_set<AllocSlice> slices;
        ordered_set<Container> containers;
        ordered_set<Container> must_alloc_to_8_bit_container;
        bool has_exact_containers = false;
        bool has_pa_container_size = false;
        bool has_pa_container_type = false;
        bool has_pov = false;
        bool has_learning_bits = false;

        // This function returns the number of container of different catagories that will be freed
        // for phv allocation after this super cluster is deallocated. If this super cluster has
        // pa_container_size or pa_container_type pragma, then return 0 container to supply. We do
        // not swap super cluster with pragmas for now. And we do not count some 8-bit containers
        // that are occupied by a slicelist that is <= 8 bits and has exact_container constraint,
        // since this slicelist has to be placed in 8-bit container and these containers cannot be
        // effectively freed and used for allocating other fieldslices.
        KindSizeIndexedMap containers_can_supply_by_sc(const ConcreteAllocation &curr_alloc) const {
            KindSizeIndexedMap rv;
            if (has_pa_container_size) return rv;
            if (has_pa_container_type) return rv;
            for (const auto &c : containers) {
                const auto &status = curr_alloc.getStatus(c);
                bool no_other_cluster = true;
                for (const auto &sl : status->slices) {
                    no_other_cluster &= slices.count(sl);
                    if (!no_other_cluster) break;
                }
                if (must_alloc_to_8_bit_container.count(c)) continue;
                if (no_other_cluster) {
                    rv[{c.type().kind(), c.type().size()}]++;
                }
            }
            return rv;
        }

        // is_sc_swap_feasible checks the feasibility of swapping this super cluster out. It means
        // that excluding all containers with exact_container constraints, does this super cluster
        // supply enough containers needed by @p required.
        bool is_sc_swap_feasible(const ConcreteAllocation &curr_alloc,
                                 const KindSizeIndexedMap &required) const {
            auto supply = containers_can_supply_by_sc(curr_alloc);
            for (const auto &[kind_sz, n_required] : required.m) {
                const int can_supply_this_ks = supply.get_or(kind_sz.first, kind_sz.second, 0);
                if (can_supply_this_ks < n_required) {
                    LOG5("not a candidate because it cannot supply containers needed");
                    return false;
                }
            }
            LOG5("this is a possible candidate");
            return true;
        }

        bool is_better_than(const AllocSummary *other) const {
            IF_NEQ_RETURN_IS_LESS(has_exact_containers, other->has_exact_containers);
            IF_NEQ_RETURN_IS_LESS(has_learning_bits, other->has_learning_bits);
            IF_NEQ_RETURN_IS_LESS(has_pov, other->has_pov);
            // We want to make changes to fewest containers possible
            IF_NEQ_RETURN_IS_LESS(containers.size(), other->containers.size());
            return false;
        }
    };
    ordered_map<const SuperCluster *, const AllocSummary *> sc_alloc;

    /// will overwrite if recorded before.
    void record(const ordered_map<const PHV::SuperCluster *, TxContStatus> &sc_alloc_diffs) {
        for (const auto &kv : sc_alloc_diffs) {
            const auto *sc = kv.first;
            const auto &diff = kv.second;
            auto *summary = new AllocSummary();
            summary->gress = sc->slices().front().field()->gress;
            summary->sc = sc;
            for (const auto &container_status : diff) {
                auto must_alloc_to_8_bit_container = false;
                for (const auto &allocslice : container_status.second.slices) {
                    auto slicelists =
                        sc->slice_list(FieldSlice(allocslice.field(), allocslice.field_slice()));
                    // try to find if this container is occupied by a slice list <= 8 bits and has
                    // exact_container constraint.
                    for (auto slicelist : slicelists) {
                        int bits = PHV::SuperCluster::slice_list_total_bits(*slicelist);
                        if (bits <= 8 && allocslice.field()->exact_containers()) {
                            BUG_CHECK(
                                container_status.first.size() == 8,
                                "a slice list smaller"
                                "than or equal to 8 bits is not allocated to 8-bit container");
                            must_alloc_to_8_bit_container = true;
                        }
                    }

                    summary->slices.insert(allocslice);
                    summary->has_exact_containers |= allocslice.field()->exact_containers();
                    summary->has_pov |= allocslice.field()->pov;
                    summary->has_pa_container_size |=
                        kit_i.pragmas.pa_container_sizes().is_specified(allocslice.field());
                    summary->has_pa_container_type |= !!kit_i.pragmas.pa_container_type()
                                                            .required_kind(allocslice.field())
                                                            .has_value();
                    summary->has_learning_bits |= kit_i.uses.is_learning(allocslice.field());
                }
                summary->containers.insert(container_status.first);
                if (must_alloc_to_8_bit_container)
                    summary->must_alloc_to_8_bit_container.insert(container_status.first);
            }
            sc_alloc[sc] = summary;
        }
    }

    void remove_record(const PHV::SuperCluster *sc) {
        BUG_CHECK(sc_alloc.count(sc), "removing sc of no record: %1%", sc);
        sc_alloc.erase(sc);
    }

    /// Returns a super cluster (summary) that is potentially the best candidate that
    /// if we redo its allocation, we can find available contaienr for @p sc.
    const AllocSummary *best_swap(const ConcreteAllocation &curr_alloc, const SuperCluster *sc,
                                  const KindSizeIndexedMap &required) {
        LOG5("required:" << required);
        const AllocSummary *best = nullptr;
        auto gress = sc->slices().front().field()->gress;
        for (const auto *s : Values(sc_alloc)) {
            LOG5("searching best swap: " << s->sc);
            if (s->gress != gress) {
                LOG5("not a candidate because they are not in the same gress");
                continue;
            }

            bool can_supply = s->is_sc_swap_feasible(curr_alloc, required);
            if (!can_supply) continue;

            if (!best || s->is_better_than(best)) {
                best = s;
            }
        }
        return best;
    }
};

}  // namespace

std::deque<std::pair<int, const PHV::SuperCluster *>>
GreedyAllocator::RefinedSuperClusterSet::normal_sc_que() const {
    std::deque<std::pair<int, const SuperCluster *>> sc_queue;
    int i = 0;
    for (const auto *sc : normal) {
        sc_queue.push_back({i, sc});
        i++;
    }
    return sc_queue;
}

ContainerGroupsBySize GreedyAllocator::make_container_groups_by_size() const {
    ContainerGroupsBySize rv;
    auto container_groups = PhvKit::make_device_container_groups();
    for (auto *group : container_groups) {
        rv[group->width()].push_back(*group);
    }
    return rv;
}

GreedyAllocator::PreSliceResult GreedyAllocator::pre_slice_all(
    const Allocation &empty_alloc, const std::list<SuperCluster *> &clusters,
    ordered_set<const SuperCluster *> &invalid_clusters, AllocatorMetrics &alloc_metrics) const {
    // use trivial allocator to pre-slice and validate super clusters.
    const auto trivial_allocator = new v2::TrivialAllocator(kit_i, phv_i, pipe_id_i);
    PreSliceResult rst;
    for (const auto sc : clusters) {
        auto pre_sliced =
            trivial_allocator->pre_slice(empty_alloc, sc, alloc_metrics, max_slicing_tries_i);
        LOG3("Pre-slice: " << sc);
        if (pre_sliced.invalid) {
            LOG3("Found unallocatable sliced cluster:" << pre_sliced.invalid);
            invalid_clusters.insert(pre_sliced.invalid);
        } else {
            LOG3("pre-slice succeeded, result:");
            for (auto *sliced : pre_sliced.sliced) {
                LOG3(sliced);
                rst.clusters.push_back(sliced);
                rst.baseline_cont_req[sliced] = pre_sliced.baseline_cont_req.at(sliced);
            }
        }
    }
    // after per-slicing, there might be clusters that are fully clotted, that they
    // were sliced out from the header, or simply unreferenced (unlikely,
    // but not harmful to remove them again). We remove them here because they do not need
    // any allocation.
    rst.clusters = PhvKit::remove_unref_clusters(kit_i.uses, rst.clusters);
    rst.clusters = PhvKit::remove_clot_allocated_clusters(kit_i.clot, rst.clusters);
    return rst;
}

GreedyAllocator::RefinedSuperClusterSet GreedyAllocator::prepare_refined_set(
    const std::list<SuperCluster *> &clusters) const {
    RefinedSuperClusterSet rst;
    for (const auto &sc : clusters) {
        if (sc->is_deparser_zero_candidate()) {
            rst.deparser_zero.push_back(sc);
        } else if (sc->needsStridedAlloc()) {
            rst.strided.push_back(sc);
        } else {
            rst.normal.push_back(sc);
        }
    }
    return rst;
}

void GreedyAllocator::sort_normal_clusters(std::list<PHV::SuperCluster *> &clusters) const {
    auto m = SuperClusterMetrics(kit_i, clusters);
    clusters.sort([&m](const SuperCluster *a, const SuperCluster *b) {
        IF_NEQ_RETURN_IS_GREATER(m.has_pov.count(a), m.has_pov.count(b));
        IF_NEQ_RETURN_IS_GREATER(m.speculated_n_containers_required(a),
                                 m.speculated_n_containers_required(b));
        IF_NEQ_RETURN_IS_GREATER(m.wide_non_sliceable(a), m.wide_non_sliceable(b));
        IF_NEQ_RETURN_IS_GREATER(m.has_exact_containers.count(a), m.has_exact_containers.count(b));
        IF_NEQ_RETURN_IS_GREATER(m.has_no_split.count(a), m.has_no_split.count(b));
        IF_NEQ_RETURN_IS_GREATER(m.has_solitary.count(a), m.has_solitary.count(b));
        IF_NEQ_RETURN_IS_GREATER(m.n_container_size_pragma.at(a), m.n_container_size_pragma.at(b));
        IF_NEQ_RETURN_IS_GREATER(m.n_container_type_pragma.at(a), m.n_container_type_pragma.at(b));
        IF_NEQ_RETURN_IS_GREATER(m.has_constraint_of_starting_index.count(a),
                                 m.has_constraint_of_starting_index.count(b));
        IF_NEQ_RETURN_IS_GREATER(m.n_ixbar_read_bits.at(a), m.n_ixbar_read_bits.at(b));
        IF_NEQ_RETURN_IS_GREATER(m.n_max_sl_width_bits.at(a), m.n_max_sl_width_bits.at(b));
        IF_NEQ_RETURN_IS_GREATER(m.n_max_rot_cluster_size.at(a), m.n_max_rot_cluster_size.at(b));
        return false;
    });
}

GreedyAllocator::AllocResultWithSlicingDetails GreedyAllocator::slice_and_allocate_sc(
    const ScoreContext &ctx, const Allocation &alloc, const PHV::SuperCluster *sc,
    const ContainerGroupsBySize &container_groups, AllocatorMetrics &alloc_metrics,
    const int max_slicings) const {
    auto slicing_ctx = kit_i.make_slicing_ctx(sc);
    // max packing and strict mode.
    slicing_ctx->set_config(
        Slicing::IteratorConfig{false, false, true, true, false, (1 << 25), (1 << 16)});
    int n_tried = 0;
    std::optional<Transaction> best_tx;
    std::optional<TxScore *> best_score;
    std::optional<std::list<PHV::SuperCluster *>> best_slicing;
    ordered_map<const PHV::SuperCluster *, TxContStatus> *best_sliced_tx = nullptr;
    int best_slicing_idx = 0;
    AllocError *last_err = new AllocError(ErrorCode::NO_SLICING_FOUND);
    *last_err << "found unsatisfiable constraints.";
    slicing_ctx->iterate([&](std::list<PHV::SuperCluster *> sliced) {
        n_tried++;
        if (LOGGING(3)) {
            LOG3("Slicing-attempt-" << n_tried << ":");
            for (auto *sc : sliced) LOG3(sc);
        }
        auto sliced_tx = new ordered_map<const PHV::SuperCluster *, TxContStatus>();
        auto this_slicing_tx = alloc.makeTransaction();
        for (const auto *sc : sliced) {
            if (kit_i.is_clot_allocated(kit_i.clot, *sc)) {
                LOG3("skip clot allocated cluster: " << sc->uid);
                continue;
            }
            if (sc->is_deparser_zero_candidate()) {
                LOG3("Found another deparser-zero cluster: " << sc);
                auto tx = alloc_deparser_zero_cluster(ctx, this_slicing_tx, sc, phv_i);
                this_slicing_tx.commit(tx);
                continue;
            }
            auto rst =
                try_sliced_super_cluster(ctx, this_slicing_tx, sc, container_groups, alloc_metrics);
            if (rst.ok()) {
                sliced_tx->emplace(sc, rst.tx->get_actual_diff());  // copy before commit.
                this_slicing_tx.commit(*rst.tx);
            } else {
                last_err = new AllocError(rst.err->code);
                LOG3("Slicing-attempt-" << n_tried << ": failed, while allocating: " << sc);
                *last_err << ". Failed when allocating this sliced " << sc;
                // found a slice list that cannot be allocated because packing issue.
                if (rst.err->code == ErrorCode::ACTION_CANNOT_BE_SYNTHESIZED &&
                    rst.err->reslice_required) {
                    for (const auto *sl : *rst.err->reslice_required) {
                        LOG3("Found invalid packing slice list: " << sl);
                        slicing_ctx->invalidate(sl);
                    }
                } else {
                    *last_err << rst.err_str();
                }
                return n_tried < max_slicings;
            }
        }
        LOG3("Slicing-attempt-" << n_tried << ": succeeded.");
        auto *this_slicing_score = ctx.score()->make(this_slicing_tx);
        LOG3("Slicing-attempt-" << n_tried << " score: " << this_slicing_score->str());
        if (!best_score || this_slicing_score->better_than(*best_score)) {
            if (best_score) {
                LOG3("Slicing-attempt-" << n_tried << " yields a *BETTER* allocation.");
                LOG3("Previous score: " << (*best_score)->str());
            } else {
                LOG3("Slicing-attempt-" << n_tried << " is the first succeeded slicing.");
            }
            best_tx = this_slicing_tx;
            best_score = this_slicing_score;
            best_slicing = sliced;
            best_sliced_tx = sliced_tx;
            best_slicing_idx = n_tried;
        } else {
            LOG3("Slicing-attempt-" << n_tried << " did *NOT* yield a better allocation.");
        }
        return n_tried < max_slicings;
    });

    /// allocation failed
    if (!best_score) {
        return AllocResultWithSlicingDetails(last_err);
    }
    AllocResultWithSlicingDetails rst(*best_tx);
    rst.best_score = *best_score;
    rst.best_slicing = *best_slicing;
    rst.best_slicing_idx = best_slicing_idx;
    rst.sliced_tx = best_sliced_tx;
    return rst;
}

bool GreedyAllocator::allocate(std::list<SuperCluster *> clusters_input,
                               AllocatorMetrics &alloc_metrics) {
    LOG1("Run GreedyAllocator.");
    alloc_metrics.start_clock();

    // print table ixbar usage
    LOG3(kit_i.mau.get_table_summary()->ixbarUsagesStr(&phv_i));
    bool nonfatal = false;
    if (kit_i.mau.get_table_summary()->getActualState() ==
        State::ALT_FINALIZE_TABLE_SAME_ORDER_TABLE_FIXED) {
        // treat failure as nonfatal if it is on ALT_FINALIZE_TABLE_SAME_ORDER_TABLE_FIXED and
        // stop table replay fitting.
        nonfatal = true;
    }
    // pre-slicing
    LOG1("GreedyAllocator: start pre-slicing");
    ordered_set<const SuperCluster *> invalid_clusters;
    auto pre_sliced = pre_slice_all(ConcreteAllocation(phv_i, kit_i.uses), clusters_input,
                                    invalid_clusters, alloc_metrics);
    if (!invalid_clusters.empty()) {
        if (nonfatal) {
            kit_i.mau.get_table_summary()->stop_table_replay_fitting();
        } else {
            error("GreedyAllocation failed because these clusters have unsatisfiable constraints.");
            for (const auto &sc : invalid_clusters) {
                error("unsat cluster: %1%", cstring::to_cstring(sc));
            }
            return false;
        }
    }

    // create strided clusters.
    pre_sliced.clusters =
        PhvKit::create_strided_clusters(kit_i.strided_headers, pre_sliced.clusters);

    // categorize super clusters.
    auto refined_cluster_set = prepare_refined_set(pre_sliced.clusters);

    // sorting.
    sort_normal_clusters(refined_cluster_set.normal);

    // logging all clusters
    if (LOGGING(3)) {
        LOG3(">>> Ready to allocate clusters:");
        LOG3("Deparser zero clusters:");
        for (const auto &sc : refined_cluster_set.deparser_zero) {
            LOG3(sc);
        }
        LOG3("Strided clusters:");
        for (const auto &sc : refined_cluster_set.strided) {
            LOG3(sc);
        }
        LOG3("Sorted normal clusters:");
        int n = 0;
        for (const auto &sc : refined_cluster_set.normal) {
            LOG3(n << "th: " << sc);
            n++;
        }
    }

    // actual allocation starts here.
    const auto container_groups = make_container_groups_by_size();
    ConcreteAllocation alloc(phv_i, kit_i.uses);
    auto *score_maker =
        new GreedyTxScoreMaker(kit_i, PhvKit::make_device_container_groups(),
                               refined_cluster_set.normal, pre_sliced.baseline_cont_req);
    auto *search_config = new SearchConfig();
    search_config->n_dfs_steps_sc_alloc = 1024;
    search_config->n_best_of_sc_alloc = 32;
    search_config->try_byte_aligned_starts_first_for_table_keys = true;

    // setup allocator context.
    auto ctx =
        PHV::v2::ScoreContext().with_score(score_maker).with_search_config(search_config).with_t(0);

    // allocate deparser zero clusters
    std::stringstream history;
    for (const auto *sc : refined_cluster_set.deparser_zero) {
        auto tx = alloc_deparser_zero_cluster(v2::ScoreContext().with_t(1), alloc, sc, phv_i);
        score_maker->record_commit(tx, sc);
        history << "Allocating deparser-zero cluster " << sc;
        history << "Allocation Decisions: \n";
        history << AllocResult::pretty_print_tx(tx, cstring::empty) << "\n";
        history << "\n";
        alloc.commit(tx);
    }

    // allocate strided clusters
    for (const auto *sc : refined_cluster_set.strided) {
        auto rst = alloc_strided_super_clusters(ctx, alloc, sc, container_groups, alloc_metrics,
                                                max_slicing_tries_i);
        if (!rst.ok()) {
            if (nonfatal) {
                kit_i.mau.get_table_summary()->stop_table_replay_fitting();
            } else {
                error("Failed to allocate stride cluster: %1%, because %2%",
                      cstring::to_cstring(sc), rst.err_str());
            }
        } else {
            history << "Allocating strided cluster " << sc;
            history << "Allocation Decisions: \n";
            history << rst.tx_str(cstring::empty) << "\n";
            history << "\n";
            score_maker->record_commit(*rst.tx, sc);
            alloc.commit(*rst.tx);
        }
    }

    // allocate normal clusters.
    LOG3("GreedyAllocator starts to allocate normal clusters");
    ordered_map<const SuperCluster *, const AllocError *> unallocated;
    ScAllocTracker sc_tracker(kit_i);
    // the max number of times to try swapping out super clusters for the unallocated.
    int n_iterations_left = 10;  // + (refined_cluster_set.normal.size() / 2);
    int swapped_out_sc_idx = refined_cluster_set.normal.size();
    auto sc_q = refined_cluster_set.normal_sc_que();
    while (!sc_q.empty()) {
        int idx = 0;
        const SuperCluster *sc = nullptr;
        std::tie(idx, sc) = sc_q.front();
        sc_q.pop_front();
        history << score_maker->status() << "\n";
        LOG3(score_maker->status());
        LOG3("Allocating a normal cluster of index " << idx << ":\n " << sc);
        history << "Allocating a normal cluster of index " << idx << ":\n " << sc;
        auto detailed_rst = slice_and_allocate_sc(ctx, alloc, sc, container_groups, alloc_metrics,
                                                  max_slicing_tries_i);
        if (!detailed_rst.rst.ok()) {
            LOG3("Failed to allocate: SC-" << sc->uid);
            history << detailed_rst;
            const ScAllocTracker::AllocSummary *to_swap = nullptr;
            if (n_iterations_left > 0) {
                n_iterations_left--;
                to_swap = sc_tracker.best_swap(alloc, sc, pre_sliced.baseline_cont_req.at(sc));
            }
            if (!to_swap) {
                LOG5("cannot find a super cluster to swap");
                unallocated[sc] = detailed_rst.rst.err;
                continue;
            } else {
                LOG3("iterative-search: will try to fit this cluster by removing allocation of "
                     << to_swap->sc);
                history
                    << "iterative-search: will try to fit this cluster by removing allocation of "
                    << to_swap->sc << "\n";
                // Iterative search:
                // (0) update score_context by recording this deallocation.
                // (1) set a baseline (the sliced cluster does not have it) for to_swap.
                // (2) remove allocation of to_swap
                // (3) push {sc, to_swap} to the front of queue.
                auto new_baseline_for_sc =
                    score_maker->record_deallocation(to_swap->sc, alloc, to_swap->slices);
                pre_sliced.baseline_cont_req[to_swap->sc] = new_baseline_for_sc;
                alloc.deallocate(to_swap->slices);
                sc_tracker.remove_record(to_swap->sc);
                sc_q.push_front({swapped_out_sc_idx++, to_swap->sc});
                sc_q.push_front({idx, sc});
            }
        } else {
            history << detailed_rst;
            sc_tracker.record(*detailed_rst.sliced_tx);
            score_maker->record_commit(*detailed_rst.rst.tx, sc);
            alloc.commit(*detailed_rst.rst.tx);
            LOG3("Successfully allocated: SC-" << sc->uid);
        }
    }
    history << "Post-allocation status:\n" << score_maker->status() << "\n";
    LOG3(score_maker->status());

    if (unallocated.empty()) {
        LOG3("Greedy allocation succeeded.");
        history << "Greedy allocation succeeded.\n";
        PhvKit::bind_slices(alloc, phv_i);
        PhvKit::sort_and_merge_alloc_slices(phv_i);
        phv_i.set_done(false);
    } else {
        if (nonfatal) {
            kit_i.mau.get_table_summary()->stop_table_replay_fitting();
        } else {
            error("PHV fitting failed, %1% clusters cannot be allocated.", unallocated.size());
            for (const auto &kv : unallocated) {
                error("Cannot allocated %1%, because %2%", cstring::to_cstring(kv.first),
                      kv.second->str());
            }
        }
    }
    alloc_metrics.stop_clock();

    // log allocation history.
    auto logfile = greedy_allocator_file_log(pipe_id_i, "phv_greedy_allocation_history_"_cs, 1);
    LOG1("Greedy Allocation history");
    LOG1(history.str());
    Logging::FileLog::close(logfile);

    // log allocation summary
    auto summary_logfile = greedy_allocator_file_log(pipe_id_i, "phv_allocation_summary_"_cs, 1);
    LOG1(alloc);
    Logging::FileLog::close(summary_logfile);

    // Log Allocation Metrics
    LOG1(alloc_metrics);
    return unallocated.empty();
}

std::ostream &operator<<(std::ostream &out,
                         const GreedyAllocator::AllocResultWithSlicingDetails &rst) {
    if (!rst.rst.ok()) {
        out << "Failed because: " << rst.rst.err_str() << "\n\n";
        return out;
    }
    out << "Successfully Allocated.\n";
    out << "By slicing into the following superclusters:";
    for (auto *sc : rst.best_slicing) {
        out << "\n" << sc;
    }
    out << "It was the " << rst.best_slicing_idx << "th slicing we tried.\n";
    out << "Allocation Decisions: \n";
    out << rst.rst.tx_str() << "\n";
    out << rst.best_score->str() << "\n";
    out << "\n";
    return out;
}

}  // namespace v2
}  // namespace PHV
