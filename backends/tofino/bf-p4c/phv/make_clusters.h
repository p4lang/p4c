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

#ifndef BF_P4C_PHV_MAKE_CLUSTERS_H_
#define BF_P4C_PHV_MAKE_CLUSTERS_H_

#include "bf-p4c/ir/bitrange.h"
#include "ir/ir.h"
#include "ir/visitor.h"
#include "lib/map.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"
#include "lib/range.h"
#include "bf-p4c/common/field_defuse.h"
#include "bf-p4c/ir/thread_visitor.h"
#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/lib/union_find.hpp"
#include "bf-p4c/phv/action_phv_constraints.h"
#include "bf-p4c/phv/alloc_setting.h"
#include "bf-p4c/phv/phv.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/analysis/pack_conflicts.h"
#include "bf-p4c/phv/pragma/pa_container_size.h"
#include "bf-p4c/phv/pragma/pa_byte_pack.h"
#include "bf-p4c/phv/utils/utils.h"
#include "bf-p4c/mau/gateway.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/mau/table_mutex.h"

namespace PHV {
class Field;
}  // namespace PHV

class PhvInfo;

/** @brief Builds "clusters" of field slices that must be placed in the same
 * group.
 *
 * Fields that are operands in the same MAU instruction must be placed at the
 * same alignment in PHV containers in the same MAU group.  An AlignedCluster
 * is formed using UnionFind to union slices in the same instruction.
 *
 * Additionally, some slices are required to be placed in the same container
 * (at different offsets).  Hence, their clusters must be placed in the same
 * MAU group.  A SuperCluster holds clusters that must be placed together,
 * along with a set of SliceLists, which are slices that must be placed (in
 * order) in the same container.
 *
 * @pre An up-to-date PhvInfo object.
 * @post A list of cluster groups, accessible via Clustering::cluster_groups().
 */
class Clustering : public PassManager {
    PhvInfo& phv_i;
    PhvUse& uses_i;
    const PackConflicts& conflicts_i;
    const PragmaContainerSize& pa_container_sizes_i;
    const PragmaBytePack& pa_byte_pack_i;
    const FieldDefUse& defuse_i;
    const DependencyGraph &deps_i;
    const TablesMutuallyExclusive& table_mutex_i;
    const PHV::AllocSetting& settings_i;

    /// Holds all aligned clusters.  Every slice is in exactly one cluster.
    std::list<PHV::AlignedCluster *> aligned_clusters_i;

    /// Holds all rotational clusters.  Every aligned cluster is in exactly
    /// one rotational cluster.
    std::list<PHV::RotationalCluster *> rotational_clusters_i;

    /// Groups of rotational clusters that must be placed in the same MAU
    /// group.  Every rotational cluster is in exactly one super cluster.
    std::list<PHV::SuperCluster *> super_clusters_i;

    /// field pairs that cannot be packed in a container because of inconsistent
    /// extractions from flexible header fields.
    assoc::hash_map<const PHV::Field*,
        assoc::hash_set<const PHV::Field*>> inconsistent_extract_no_packs_i;

    /// Maps fields to their slices.  Slice lists are ordered from LSB to MSB.
    ordered_map<const PHV::Field*, std::list<PHV::FieldSlice>> fields_to_slices_i;

    /// Collects validity bits involved in complex instructions, i.e.
    /// instructions that do anything other than assign a constant to the
    /// validity bit. Fields that are involved in the same assignment statement
    /// are part of the same UnionFind set.
    UnionFind<const PHV::Field*> complex_validity_bits_i;

    /// Utility method for querying \a fields_to_slices_i.
    /// @returns the slices of @p field in \a fields_to_slices_i overlapping with @p range.
    std::vector<PHV::FieldSlice> slices(const PHV::Field* field, le_bitrange range) const;

    /// This method will insert aligned cluster and rotational cluster for the field.
    /// @returns the created rotational cluster.
    PHV::RotationalCluster* insert_rotational_cluster(PHV::Field* f);

    /** For backtracking, clear all the pre-existing structs in the Clustering object.
      */
    class ClearClusteringStructs : public Inspector {
        Clustering& self;
        Visitor::profile_t init_apply(const IR::Node* node) override;

     public:
        explicit ClearClusteringStructs(Clustering& self) : self(self) { }
    };

    /** Find validity bits involved in any MAU instruction other than
     *
     *   `*.$valid = n`,
     *
     * where `n` is a constant.
     */
    class FindComplexValidityBits : public Inspector {
        Clustering& self;
        PhvInfo& phv_i;

        bool preorder(const IR::MAU::Instruction*) override;
        void end_apply() override;

     public:
        explicit FindComplexValidityBits(Clustering& self) : self(self), phv_i(self.phv_i) { }
    };

    /** Break each field into slices based on the operations that use it.  For
     * example if a field f is used in two operations:
     *
     *    f1[7:4] = 0xFF;
     *    f1[5:2] = f2[4:0];
     *
     * then the field is split into four slices:
     *
     *    f1[7:6], f1[5:4], f1[3:2], f[1:0]
     *
     * However, if the field has the `no_split` constraint, then it is always
     * placed in a single slice the size of the whole field:
     *
     *    f1[7:0]
     *
     * This makes cluster formation more precise, by only placing field slices
     * in the same cluster for the bits of the fields actually used in the
     * operations.
     */
    class MakeSlices : public Inspector {
        Clustering& self;
        PhvInfo& phv_i;
        const PragmaContainerSize& pa_sizes_i;

        /// Sets of sets of slices, where each set of slices must share a
        /// fine-grained slicing.  (We use a vector because duplicates don't
        /// affect correctness.)
        std::vector<ordered_set<PHV::FieldSlice>> equivalences_i;

        /// Start by mapping each field to a single slice the size of the
        /// field.
        profile_t init_apply(const IR::Node *root) override;

        bool preorder(const IR::MAU::Table* tbl) override;

        /// For each occurrence of a field in a slicing operation, split its
        /// slices in fields_to_slice_i along the boundary of the new slice.
        bool preorder(const IR::Expression*) override;

        /// After each operand has been sliced, ensure for each instruction that
        /// the slice granularity of each operand matches.
        void postorder(const IR::MAU::Instruction*) override;

        // TODO: Add a check for shrinking casts that fails if any are
        // detected.

        /// Ensure fields in each UF group share the same fine-grained slicing.
        void end_apply() override;

     public:
        explicit MakeSlices(Clustering &self, const PragmaContainerSize& pa)
            : self(self), phv_i(self.phv_i), pa_sizes_i(pa) { }

        /// Utility method for updating fields_to_slices_i.
        /// Splits slices for @p field at @p range.lo and @p range.hi + 1.
        /// @returns true if any new slices were created.
        bool updateSlices(const PHV::Field* field, le_bitrange range);
    };

    MakeSlices          slice_i;

    class MakeAlignedClusters : public Inspector {
        Clustering& self;
        PhvInfo& phv_i;
        const PhvUse&  uses_i;

        /// The Union-Find data structure, which is used to build clusters.
        UnionFind<PHV::FieldSlice> union_find_i;

        /// Initialize the UnionFind data structure with all fields in phv_i.
        profile_t init_apply(const IR::Node *root) override;

        /// Union all operands of each primitive instruction.
        bool preorder(const IR::MAU::Instruction* inst) override;

        /** Union all operands in the gateway.
         * TODO: gateway operands can be in a same container,
         * and they are only required to be byte-aligned, not necessary in same MAU group.
         */
        bool preorder(const IR::MAU::Table *tbl) override;

        /// Build AlignedClusters from the UnionFind sets.
        void end_apply() override;

     public:
        explicit MakeAlignedClusters(Clustering &self)
        : self(self), phv_i(self.phv_i), uses_i(self.uses_i) { }
    };

    class MakeRotationalClusters : public Inspector {
        Clustering& self;
        PhvInfo& phv_i;

        /// The Union-Find data structure, which is used to build rotational
        /// clusters.
        UnionFind<PHV::AlignedCluster*> union_find_i;

        /// Map slices (i.e. set operands) to the aligned clusters that
        /// contain them.
        ordered_map<const PHV::FieldSlice, PHV::AlignedCluster*> slices_to_clusters_i;

        /// Populate union_find_i with all aligned clusters created in
        /// MakeAlignedClusters.
        Visitor::profile_t init_apply(const IR::Node *) override;

        /// Union AlignedClusters with slices that are operands of `set`
        /// instructions.
        bool preorder(const IR::MAU::Instruction*) override;

        /// Create rotational clusters from sets of aligned clusters in
        /// union_find_i.
        void end_apply() override;

     public:
        explicit MakeRotationalClusters(Clustering &self)
        : self(self), phv_i(self.phv_i) { }
    };

    /// CollectIncorrectFlexibleFieldExtract finds extraction that the offset
    /// of input buffer is not consistent with offset of the field in the header.
    /// For example,
    /// Assume we have a flexible header h1:
    /// header h1 {
    ///   \@flexible
    ///   bit<9> f1;
    ///   \@flexible
    ///   bit<9> f2;
    /// }, and a non-flexible header h2:
    /// header h2 {
    ///   bit<7> xx;
    ///   bit<9> f1;
    ///   bit<7> yy;
    ///   bit<9> f2;
    /// }.
    /// If flexible_packing pass decided to pack h1 to
    /// header h1 {
    ///   bit<7> pad1;
    ///   bit<9> f2;
    ///   bit<7> pad2;
    ///   bit<9> f1;
    /// }.
    /// and if user wrote a parser state that
    /// state example {
    ///     h1 fheader;
    ///     h2 normal;
    ///     parser.extract<h1>(fheader);
    ///     normal.xx = (bit<7>)7w0;
    ///     normal.f1 = fheader.f1;
    ///     normal.yy = (bit<7>)7w0;
    ///     Gotham.f2 = fheader.f2;
    ///     .....
    /// }
    /// then normal.f1 and normal.f2 cannot be packed into a W container because
    /// the offset on the buffer is different from the offset of the header.
    /// The output of this pass is a set of fieldslice that the header needs to be split after
    /// the fieldslice.
    class CollectInconsistentFlexibleFieldExtract : public Inspector {
        Clustering& self;
        const PhvInfo& phv_i;
        std::map<int, std::vector<const PHV::Field*>> headers_i;
        using ExtractVec = std::set<std::pair<nw_bitrange, const PHV::Field*>>;
        std::vector<ExtractVec> extracts_i;

        Visitor::profile_t init_apply(const IR::Node * root) override {
            auto rv = Inspector::init_apply(root);
            headers_i.clear();
            extracts_i.clear();
            return rv;
        }

        void save_header_layout(const IR::HeaderRef* hr);
        bool preorder(const IR::ConcreteHeaderRef*) override;
        bool preorder(const IR::HeaderStackItemRef*) override;
        bool preorder(const IR::BFN::ParserState* state) override;
        void end_apply() override;

     public:
        explicit CollectInconsistentFlexibleFieldExtract(Clustering& self, const PhvInfo& phv)
            : self(self), phv_i(phv) {}
    };

    /// CollectPlaceTogetherConstraints
    /// Place Together constraint is expressed as SliceList in backend.
    /// There are multiple sources of this constraint, Headers, Digest Field Lists.
    /// This pass collect all of them and produce a set of SliceLists that
    /// each fieldslice resides in only one of the list.
    class CollectPlaceTogetherConstraints : public Inspector {
        Clustering& self;
        PhvInfo& phv_i;
        const PackConflicts& conflicts_i;
        const ActionPhvConstraints& actions_i;
        const CollectInconsistentFlexibleFieldExtract& inconsistent_extract_i;

        // Reason of why fieldslices must be placed together.
        // NOTE: the lower the number, the higher the priority(more constraints).
        // This priority is used by the solver algorithm.
        enum class Reason : int {
            PaBytePack      = 0,  // pa_byte_pack pragma
            Header          = 1,  // parsed + deparsed
            Resubmit        = 2,  // 8-bytes limit + deparsed
            Mirror          = 3,  // deparsed
            Learning        = 4,  // no_pack in same byte, handled in phv_fields: aligment = 0
            Pktgen          = 5,  // unknown
            BridgedTogether = 6,  // egress metadata that are bridged together
            ConstrainedMeta = 7,  // metadata with constraints like solitary needs to in a list.
            PaContainerSize = 8,  // metadata with pa_container_size pragmas.
        };

        /// lists that must be placed together, may contains duplicated fieldslices.
        /// The solver algorithm will read from this variable as input.
        std::map<Reason, std::vector<PHV::SuperCluster::SliceList*>> place_together_i;

        /// Collection of slice lists, each of which contains slices that must
        /// be placed, in order, in the same container.
        ordered_set<PHV::SuperCluster::SliceList*> slice_lists_i;

        /// Track headers already visited, by tracking the IDs of the first
        /// fields.
        ordered_set<int> headers_i;

        /// Collection of candidate slice lists that introduced by bridged_extracted_together_i.
        /// They will be added to slice_lists_i is there is no conflict.
        ordered_set<PHV::SuperCluster::SliceList*> bridged_extracted_together_i;

        /// clear above states
        Visitor::profile_t init_apply(const IR::Node * root) override {
            auto rv = Inspector::init_apply(root);
            place_together_i.clear();
            slice_lists_i.clear();
            headers_i.clear();
            bridged_extracted_together_i.clear();
            return rv;
        };

        /// compute slice_list_i result.
        void end_apply() override;

        /// generate slicelists into slice_lists_i from place_together_constraints.
        /// Problem to solve:
        ///   Fields can be used in header, digest list, or others, bridge_pack guarantees
        ///   that there exists a set of slicelists, that each fieldlist showed up only
        ///   once in a slicelist, and all alignment constraints are sat.
        /// The algorithm is simple, it creates slice list by following the priority
        /// of the reason, and skips duplicated fieldslices (i.e. slices that are already
        /// a slice list). For example, being header is the most prioritized reason,
        /// because those fields are parsed and deparsed. See comments of Reason for more.
        void solve_place_together_constraints();

        /// pack metadata fieldslices specified in pa_byte_pack and update their alignments
        /// in PhvInfo. If there were any invalid packing layout, it will call \:\:error();
        void pack_pa_byte_pack_and_update_alignment();

        /// pack metadata fieldslices with pa_container_size pramgas into a slicelist.
        void pack_pa_container_sized_metadata();

        /// pack metadata fieldslices with constraints together.
        /// e.g.
        /// It will pack f1<12>[0:4] solitary, f1<12>[5:11] solitary into
        /// [f1<12>[0:4] solitary, f1<12>[5:11] solitary].
        void pack_constrained_metadata();

        /// add valid bridged_extracted_together_i to slice_lists_i
        /// must happen after all other slice list makings are done.
        void pack_bridged_extracted_together();

        /// Pack pov bits into slice lists.
        void pack_pov_bits();

        /// Helper function for visiting HeaderRefs, write place_together.
        void visit_header_ref(const IR::HeaderRef* hr);

        /// Helper function for visiting DigestFieldLists, write place_together.
        void visit_digest_fieldlist(const IR::BFN::DigestFieldList* fl, int skip, Reason reason);

        /// call visitHeaderRef
        bool preorder(const IR::ConcreteHeaderRef*) override;

        /// call visitHeaderRef
        bool preorder(const IR::HeaderStackItemRef*) override;

        /// call visitDigestFieldList
        bool preorder(const IR::BFN::Digest*) override;

        /// collect fields adjacent fields are egress bridged extracted together
        /// and save them in bridged_extracted_together_i.
        bool preorder(const IR::BFN::ParserState* state) override;

     public:
        explicit CollectPlaceTogetherConstraints(
            Clustering& self, const ActionPhvConstraints& actions,
            const CollectInconsistentFlexibleFieldExtract& inconsistent)
            : self(self),
              phv_i(self.phv_i),
              conflicts_i(self.conflicts_i),
              actions_i(actions),
              inconsistent_extract_i(inconsistent) {}

        ordered_set<PHV::SuperCluster::SliceList*> get_slice_lists() const { return slice_lists_i; }
    };

    class MakeSuperClusters : public Inspector {
        Clustering& self;
        PhvInfo& phv_i;
        const CollectPlaceTogetherConstraints& place_togethers_i;

        /// Clear state to enable backtracking
        Visitor::profile_t init_apply(const IR::Node *) override;

        /// Create cluster groups by taking the union of clusters of slices
        /// that appear in the same list.
        void end_apply() override;

        /// Add padding into slice list for fields that will be marshaled, because they
        /// have exact_container requirement but might be non-byte-aligned.
        /// Singleton padding cluster will be inserted into @p cluster_set, if it is added
        /// into @p slice_lists.
        void addPaddingForMarshaledFields(
                ordered_set<const PHV::RotationalCluster*>& cluster_set,
                ordered_set<PHV::SuperCluster::SliceList*>& slice_lists);

     public:
        explicit MakeSuperClusters(
                Clustering &self,
                const CollectPlaceTogetherConstraints& place_togethers)
            : self(self), phv_i(self.phv_i), place_togethers_i(place_togethers) { }
    };

    class ValidateClusters : public Inspector {
     private:
        Clustering& self;
        const MauBacktracker &mau_bt;

        profile_t init_apply(const IR::Node* root) override;

        // basic checks: slicelist is not empty, no duplicated fieldslices.
        static void validate_basics(const std::list<PHV::SuperCluster*>& clusters);

        /** For the deparser zero optimization, we need to make sure that every deparser zero
         * optimizable field is never in a supercluster that also contains non deparser zero fields.
         * This class performs that validation after superclusters are generated.
         */
        static void validate_deparsed_zero_clusters(const std::list<PHV::SuperCluster*>& clusters);

        // size % 8 = 0 if exact_containers
        static std::optional<cstring> validate_exact_container_lists(
            const std::list<PHV::SuperCluster*>& clusters, const PhvUse& uses);

        // alignments of fieldslices in list must be all sat.
        static void validate_alignments(const std::list<PHV::SuperCluster*>& clusters,
                                        const PhvUse& uses);

        // inconsistent extractions cannot share fields in a same byte of a header.
        static void validate_extract_from_flexible(
            const std::list<PHV::SuperCluster*>& clusters,
            std::function<bool(const PHV::Field* a, const PHV::Field* b)> no_pack);

        // if field->same_container_group, then all the fieldslices of the field must
        // be in the same super cluster.
        static void validate_same_container_group_fields(
            const std::list<PHV::SuperCluster*>& clusters,
            const ordered_map<const PHV::Field*, std::list<PHV::FieldSlice>>& field_to_slices);

     public:
        explicit ValidateClusters(Clustering& c, const MauBacktracker &mau_bt) :
            self(c), mau_bt(mau_bt) { }
    };

    class UpdateSameContainerAllocConstraint : public Inspector {
     private:
        Clustering& self;

        profile_t init_apply(const IR::Node* root) override;

     public:
        explicit UpdateSameContainerAllocConstraint(Clustering& c) : self(c) { }
    };

 public:
    Clustering(PhvInfo& p, PhvUse& u, const PackConflicts& c, const PragmaContainerSize& pa_sz,
               const PragmaBytePack& pa_byte_pack, const ActionPhvConstraints& a,
               const FieldDefUse& defuse, const DependencyGraph &deps,
               const TablesMutuallyExclusive& table_mutex, const PHV::AllocSetting& settings,
               const MauBacktracker& mau_bt)
        : phv_i(p), uses_i(u), conflicts_i(c), pa_container_sizes_i(pa_sz),
          pa_byte_pack_i(pa_byte_pack), defuse_i(defuse), deps_i(deps),
          table_mutex_i(table_mutex), settings_i(settings), slice_i(*this, pa_sz) {
        auto* inconsistent_extracts =
            new CollectInconsistentFlexibleFieldExtract(*this, this->phv_i);
        auto* place_togethers =
            new CollectPlaceTogetherConstraints(*this, a, *inconsistent_extracts);
        addPasses({
            new ClearClusteringStructs(*this),   // clears pre-existing maps
            new FindComplexValidityBits(*this),  // populates complex_validity_bits_i
            &slice_i,
            new MakeAlignedClusters(*this),     // populates aligned_clusters_i
            new MakeRotationalClusters(*this),  // populates rotational_clusters_i
            inconsistent_extracts, place_togethers,
            new MakeSuperClusters(*this, *place_togethers),  // populates super_clusters_i
            new ValidateClusters(*this, mau_bt),             // validate clustering is correct.
            new UpdateSameContainerAllocConstraint(*this)    // update SameContainerAllocConstraint
        });
    }

    /// @returns all clusters, where every slice is in exactly one cluster.
    const std::list<PHV::SuperCluster*>& cluster_groups() const { return super_clusters_i; }

    /// return true if two fields cannot be packed into one container because there
    /// are inconsistent extraction to them from \@flexible headers.
    bool no_pack(const PHV::Field* a, const PHV::Field* b) const {
        return inconsistent_extract_no_packs_i.count(a) &&
               inconsistent_extract_no_packs_i.at(a).count(b);
    }
};

#endif /* BF_P4C_PHV_MAKE_CLUSTERS_H_ */
