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

#ifndef BF_P4C_PHV_ACTION_PHV_CONSTRAINTS_H_
#define BF_P4C_PHV_ACTION_PHV_CONSTRAINTS_H_

#include <optional>
#include "ir/ir.h"
#include "lib/safe_vector.h"
#include "bf-p4c/common/map_tables_to_actions.h"
#include "bf-p4c/lib/union_find.hpp"
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/mau/action_analysis.h"
#include "bf-p4c/mau/table_dependency_graph.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/phv_parde_mau_use.h"
#include "bf-p4c/phv/analysis/pack_conflicts.h"
#include "bf-p4c/phv/utils/utils.h"

using ::P4::operator<<;

struct ActionPhvConstraintLogging {
    int error;
};

/**
 * This class is used to make action_phv_constraint logs easier to understand
 * during debugging. The 'can_pack' function returns
 */
struct ActionPhvConstraintCanPack {
    std::optional<ActionPhvConstraintLogging> logging;
    std::optional<PHV::Allocation::ConditionalConstraint> conditional_constraints;
};

enum class CanPackErrorCode : unsigned {
    NO_ERROR = 0,
    SLICE_EMPTY = 1,
    PACK_CONSTRAINT_PRESENT = 2,
    STATEFUL_DEST_CONSTRAINT = 3,
    BITMASK_CONSTRAINT = 4,
    SPECIALTY_DATA = 5,
    MIXED_OPERAND = 6,
    NONE_ADJACENT_FIELD = 7,
    COMPLEX_AD_PACKING = 8,
    BITWISE_MIXED_AD = 9,
    TF2_MORE_THAN_ONE_SOURCE = 10,
    TF2_ALL_WRITTEN_TOGETHER = 11,
    MORE_THAN_TWO_SOURCES = 12,
    TWO_SOURCES_AND_CONSTANT = 13,
    MOVE_AND_UNALLOCATED_SOURCE = 14,
    BITWISE_AND_UNALLOCATED_SOURCE = 15,
    SLICE_ALIGNMENT = 16,
    PACK_AND_ALIGNED = 17,
    INVALID_MASK = 18,
    SLICE_DIFF_OFFSET = 19,
    COPACK_UNSATISFIED = 20,
    MULTIPLE_ALIGNMENTS = 21,
    OVERLAPPING_SLICES = 22,
    CONSTRAINT_CHECKER_FAILED = 23,
};


std::ostream &operator<<(std::ostream &out, const CanPackErrorCode& info);

using CanPackReturnType = std::tuple<CanPackErrorCode,
      std::optional<PHV::Allocation::ConditionalConstraints>>;

/// CanPackErrorV2 has
/// (1) an error code for classification.
/// (2) detailed error messages for debugging.
/// (3) the destination container status when action synthesize failed.
struct CanPackErrorV2 {
    CanPackErrorCode code;
    cstring msg;
    bool remove_align_req = false;
    const std::vector<PHV::AllocSlice>* invalid_dest_packing = nullptr;
    CanPackErrorV2(CanPackErrorCode code) : code(code) {}
    CanPackErrorV2(CanPackErrorCode code, bool clear_align) : code(code),
                                                                remove_align_req(clear_align) {}
    CanPackErrorV2(CanPackErrorCode code, cstring msg) : code(code), msg(msg) {}
    CanPackErrorV2(CanPackErrorCode code, cstring msg,
                   const std::vector<PHV::AllocSlice>* dest_cont)
        : code(code), msg(msg), invalid_dest_packing(dest_cont) {}
    bool ok() const { return code == CanPackErrorCode::NO_ERROR; }
};

/** This class is meant to gather action information as well as provide information to PHV analysis
  * through function calls. Methods in AllocatePHV query the information contained in class members
  * here to determine constraints on packing induced by MAU actions.
  * @pre Must be run after InstructionSelection, as it is dependent on ActionAnalysis
  */
class ActionPhvConstraints : public Inspector {
 private:
    const PhvInfo &phv;
    const PhvUse  &uses;
    const PackConflicts &conflicts;
    const MapTablesToActions &tableActionsMap;
    const DependencyGraph& dg;

    ordered_map<const PHV::Field*, ordered_set<const PHV::Field*>> same_byte_fields;

    bool prelim_constraints_ok = true;

    struct ClassifiedSource {
        bool exist = false;
        PHV::Container container;
        ordered_set<PHV::AllocSlice> slices = {};
        bool aligned = true;
        int offset = 0;
    };
    friend std::ostream
    &operator<<(std::ostream &out, const ActionPhvConstraints::ClassifiedSource& src);

    using ClassifiedSources = std::array<ClassifiedSource, 2>;

    using PackingConstraints = ordered_map<const IR::MAU::Action*, UnionFind<PHV::FieldSlice>>;
    /// Defines a struct for returning number of containers.
    struct NumContainers {
        size_t num_allocated;
        size_t num_unallocated;
        bool double_unallocated = false;

        explicit NumContainers(const size_t numAlloc, const size_t numUnalloc) :
            num_allocated(numAlloc), num_unallocated(numUnalloc) {}

        NumContainers() : num_allocated(0), num_unallocated(0) {}
    };

    typedef enum action_data_uses_t { SOME_AD_CONSTANT = 0,
                                      ALL_AD_CONSTANT = 1,
                                      COMPLEX_AD_PACKING_REQ = 2,
                                      NO_AD_CONSTANT = 3 } ActionDataUses;

    /// Defines a struct for a particular field operation: either read or write.
    // TODO: Will we ever need a boolean indicating read or write in this
    // struct?
    struct OperandInfo {
        int unique_action_id = -1;
        enum field_read_flags_t {
            MOVE = 1,
            BITWISE = (1 << 1),
            WHOLE_CONTAINER = (1 << 2),
            ANOTHER_OPERAND = (1 << 3),
            MIXED = (1 << 4),
            WHOLE_CONTAINER_SAME_FIELD = (1 << 5),
            PART_OF_CONTAINER = (1 << 6),
        };
        uint8_t flags = 0;

        // An operand is either action data (ad), a constant (constant), or
        // drawn from a PHV container (phv_used).
        bool ad = false;
        unsigned special_ad = ActionAnalysis::ActionParam::NO_SPECIAL;
        bool constant = false;
        int64_t const_value = 0;
        std::optional<PHV::FieldSlice> phv_used = std::nullopt;
        cstring action_name;
        cstring operation;

        bool operator < (OperandInfo other) const {
            // TODO: What if ad == other.ad?
            if (ad && !other.ad)
                return true;
            else if (!ad && other.ad)
                return false;
            else if (constant && !other.constant)
                return true;
            else if (!constant && other.constant)
                return false;
            else if (constant && other.constant)
                return const_value < other.const_value;
            else if (phv_used && !other.phv_used)
                return true;
            else if (!phv_used && other.phv_used)
                return false;
            else if (!phv_used && !other.phv_used)
                return false;
            else if (phv_used->field()->id != other.phv_used->field()->id)
                return phv_used->field()->id < other.phv_used->field()->id;
            else if (phv_used->range().lo != other.phv_used->range().lo)
                return phv_used->range().lo < other.phv_used->range().lo;
            else
                return phv_used->range().hi < other.phv_used->range().hi;
        }

        bool operator == (OperandInfo other) const {
            return phv_used == other.phv_used
                && ad == other.ad
                && constant == other.constant
                && const_value == other.const_value;
        }

        bool operator == (const PHV::Field* field) const {
            return phv_used && phv_used->field()->id == field->id;
        }

        std::string toString() const {
            std::stringstream ss;
            ss << *this;
            return ss.str();
        }

        explicit OperandInfo(PHV::FieldSlice slice, int id = -1)
        : unique_action_id(id), phv_used(slice) { }

        OperandInfo() { }
    };

    friend std::ostream
    &operator<<(std::ostream &out, const ActionPhvConstraints::OperandInfo& info);

    friend std::ostream
    &operator<<(std::ostream &out, const safe_vector<ActionPhvConstraints::OperandInfo>& info);

    class ConstraintTracker {
        const PhvInfo& phv;
        ActionPhvConstraints& self;

        /// Maps fields to the actions that write them, keeping track of range
        /// of each write.
        ordered_map<const PHV::Field*,
            ordered_map<le_bitrange,
                ordered_set<const IR::MAU::Action *>>> field_writes_to_actions;

        /// Any OperandInfo (Field) within the set will have been written in the
        /// key action.
        ordered_map<const IR::MAU::Action *, ordered_set<OperandInfo>> action_to_writes;

        /// Any OperandInfo (Field) within the set will have been read in the
        /// key action.
        ordered_map<const IR::MAU::Action *, ordered_set<PHV::FieldSlice>> action_to_reads;

        /// Map from action to table
        ordered_map<const IR::MAU::Action *, const IR::MAU::Table *> action_to_table;

        /// Any OperandInfo in the std::vector will use the following as an operand in
        /// action where the key is the field written in that action.
        ordered_map<const PHV::Field*,
            ordered_map<const IR::MAU::Action *,
                ordered_map<le_bitrange,
                    std::vector<OperandInfo>>>> write_to_reads_per_action;

        ordered_map<const PHV::Field*,
            ordered_map<const IR::MAU::Action *,
                ordered_map<le_bitrange,
                    ordered_set<PHV::FieldSlice>>>> read_to_writes_per_action;

        /// Map from stateful destination field to a map of the destination slices to the set of
        /// ([write_lo, write_hi], [read_lo, read_hi]) slices used while setting those destinations.
        ordered_map<const PHV::Field*,
            ordered_map<le_bitrange,
                ordered_map<const IR::MAU::Action*,
                            ordered_set<std::pair<int, int>>>>> statefulWrites;

        /// Used to generate unique action IDs when creating `struct OperandInfo` objects
        static int current_action;

     public:
        ConstraintTracker(const PhvInfo& phv, ActionPhvConstraints& self)
        : phv(phv), self(self) { }

        /// Clear internal state.
        void clear();

        /// Add constraints induced by @p act.
        void add_action(
            const IR::MAU::Action *act,
            const ActionAnalysis::FieldActionsMap field_actions_map,
            const IR::MAU::Table *tbl);

        /// @returns true if @p field is written in @p act only using a PHV source.
        /// If an action data or constant source is used, then @returns false.
        /// @returns std::nullopt if the field is not written in this action.
        std::optional<bool>
        hasPHVSource(const PHV::Field* field, const IR::MAU::Action* act) const;

        /// @returns false if @p field is written in @p act using a PHV source.
        /// If an action data or constant source is used, then @returns true.
        /// @returns std::nullopt if the field is not written in this action.
        std::optional<bool>
        hasActionDataOrConstantSource(const PHV::Field* field, const IR::MAU::Action* act) const;

        /** If @p dst is a slice of the destination operand of an instruction
         * in @p act, then return one OperandInfo for each corresponding slice
         * of each source, if any.  Returns an empty vector if @p dst is not
         * written in @p act or if there are no sources.
         */
        std::vector<OperandInfo>
        sources(const PHV::FieldSlice& dst, const IR::MAU::Action* act) const;

        /// Convenience method that translates @p dst to a FieldSlice and passes
        /// it to `sources` above.
        std::vector<OperandInfo>
        sources(const PHV::AllocSlice& dst, const IR::MAU::Action* act) const {
            return sources(PHV::FieldSlice(dst.field(), dst.field_slice()), act);
        }

        /** If @p src is a slice of a source operand of any instructions
         * in @p act, then return one OperandInfo for each corresponding slice
         * of each destination, if any.  Returns an empty vector if @p src is not
         * read in @p act.
         */
        ordered_set<PHV::FieldSlice>
        destinations(PHV::FieldSlice src, const IR::MAU::Action* act) const;

        /// Convenience method that translates @p src to a FieldSlice and passes
        /// it to `destinations` above.
        ordered_set<PHV::FieldSlice>
        destinations(PHV::AllocSlice src, const IR::MAU::Action* act) const {
            return destinations(PHV::FieldSlice(src.field(), src.field_slice()), act);
        }

        /// @returns the actions that read @p src.
        ordered_set<const IR::MAU::Action*> read_in(PHV::FieldSlice src) const;

        /// Convenience method that translates @p src to a FieldSlice and passes
        /// it to `read_in` above.
        ordered_set<const IR::MAU::Action*> read_in(PHV::AllocSlice src) const {
            return read_in(PHV::FieldSlice(src.field(), src.field_slice()));
        }

        /// @returns the actions that write @p dst.
        ordered_set<const IR::MAU::Action*> written_in(PHV::FieldSlice dst) const;

        /// Convenience method that translates @p dst to a FieldSlice and passes
        /// it to `written_in` above.
        ordered_set<const IR::MAU::Action*> written_in(PHV::AllocSlice dst) const {
            return written_in(PHV::FieldSlice(dst.field(), dst.field_slice()));
        }

        /// @returns operation information for fields written in @p act.
        const ordered_set<OperandInfo>& writes(const IR::MAU::Action* act) const;

        /// @returns operation information for fields written in @p act.
        const ordered_set<PHV::FieldSlice>& reads(const IR::MAU::Action* act) const;

        /// AVOID USING this function.
        /// @returns a OperandInfo structure if @p slice is written in @p act.
        /// TODO: The OperandInfo returned in this function is NOT COMPLETE!
        /// phv_used, ad, constant is uninitialized!
        std::optional<OperandInfo>
        is_written(PHV::FieldSlice slice, const IR::MAU::Action *act) const;

        /// AVOID USING this function.
        /// Convenience method that translates @p slice to a FieldSlice and passes
        /// it to `is_written` above.
        /// TODO: The OperandInfo returned in this function is NOT COMPLETE!
        std::optional<OperandInfo>
        is_written(PHV::AllocSlice slice, const IR::MAU::Action *act) const {
            return is_written(PHV::FieldSlice(slice.field(), slice.field_slice()), act);
        }

        /** If @p dst allocates field slice f[x:y] to container c[a:b], and @p src
         * is a field slice g[x':y'] involved in an MAU instruction with
         * f[x:y], then there exists a range of container bits [a':b'] at which
         * g[x':y'] will be aligned with the corresponding bits of f[x:y].
         *
         * For example, suppose we have:
         *
         *      bit<16> f
         *      bit<16> g
         *      f[7:0] = g[15:8]
         *
         * If c[31:24]<--f[7:0], then g[10:9] would need to be placed at
         * [26:25] (starting at bit 25) to be aligned with f.
         *
         * @returns the set of alignments of @p dst that cause it to align with
         * @p src, or an empty set if @p dst and @p src are not involved in an MAU
         * operation.  The set will contain multiple alignments if @p dst and
         * @p src are in different instructions (in different actions) at
         * different alignments.
         */
        ordered_set<int> source_alignment(PHV::AllocSlice dst, PHV::FieldSlice src) const;

        std::optional<int> can_be_both_sources(const std::vector<PHV::AllocSlice> &slices,
            ordered_set<PHV::FieldSlice> &packing_slices, PHV::FieldSlice src) const;

        /** Print the state of the maps */
        void printMapStates() const;

        /** Debug function to print the total number of reads and writes across all actions for the
         * fields in the vector of AllocSlices represented by @p slices
         */
        void print_field_ordering(const std::vector<PHV::AllocSlice>& slices) const;

        /** @returns the stateful destinations map.
          */
        const ordered_map<const PHV::Field*, ordered_map<le_bitrange,
              ordered_map<const IR::MAU::Action*, ordered_set<std::pair<int, int>>>>>&
                  getStatefulWrites() const {
            return statefulWrites;
        }

        /** Get the action for a table */
        const IR::MAU::Table *action_table(const IR::MAU::Action *) const;
    };

    ConstraintTracker constraint_tracker;

    /// a set of property of an action when phv is allocated.
    struct ActionContainerProperty {
        unsigned op_type;
        NumContainers sources;
        bool must_be_aligned;  // a phv source must be aligned
        int num_sources;       // the number of allocated container sources.
        int num_unallocated;   // the number of unallocated slices.
        bool use_ad;           // true if action data is used.

        /// For must_be_aligned actions only.
        /// will only be updated when must_be_aligned is updated by bitwise_or_move constraints.
        bool aligned_one_unallocated = false;
        bool aligned_two_unallocated = false;
        bool aligned_unallocated_requires_aligment = false;
    };

    using ActionPropertyMap =
        ordered_map<const IR::MAU::Action*, ActionPhvConstraints::ActionContainerProperty>;

    /// Clears any state accumulated in prior invocations of this pass.
    profile_t init_apply(const IR::Node *root) override;

    /** Builds the data structures to be used in the API function call
      * Also checks that the actions can be scheduled on Tofino in the same stage
      * without splitting and without considering PHV allocation (yet to happen)
      */
    bool preorder(const IR::MAU::Action *act) override;

    /// Print out all the maps created in ActionPhvConstraints.
    void end_apply() override;

    void determine_same_byte_fields();

    const ordered_set<PHV::FieldSlice> get_slices_in_same_byte(const PHV::FieldSlice& slice) const;

    /** When populating action related data for every action, also check if the action results in
      * unsatisfiable constraints, which would result in failure of PHV allocation. @returns false
      * if unsatisfiable constraints are introduced.
      */
    bool early_check_ok(const IR::MAU::Action* action);

    /** Determine the number of PHV sources required for an action that writes @p phvSlices
     * (all in the same container) and classifies them into different source numbers.
     */
    UnionFind<PHV::FieldSlice> classify_PHV_sources(
        const ordered_set<PHV::FieldSlice>& phvSlices,
        const ordered_map<PHV::FieldSlice, const PHV::SuperCluster::SliceList*>& lists_map) const;

    /** Craft an error message in @p ss, if the packing @p list in the same container violates
     * action constraints imposed by @p action. @p fieldAlignments are the alignments within
     * the container for different field slices in the supercluster to which list belongs.
     * @p lists_map is a map from each slice to the slice list it belongs to.
     */
    bool diagnoseInvalidPacking(
            const IR::MAU::Action* action,
            const PHV::SuperCluster::SliceList* list,
            const ordered_map<PHV::FieldSlice, unsigned>& fieldAlignments,
            const ordered_map<PHV::FieldSlice, const PHV::SuperCluster::SliceList*>& lists_map,
            std::stringstream& ss) const;

    /** Craft an error message in @p ss for @p action_name that requires too many sources (more
     * than 2 PHV sources when action data/constant are not present, or 2 or more PHV sources when
     * action data/constant is present). @p actionDataWrittenSlices are all the slices written by
     * action data/constant in @p action_name, @p notWrittenSlices are container slices not written
     * in the @p action_name, @p phvSources are the PHV sources classified according to the source
     * number, @p phvAlignedSlices represents the relative alignment required for the source slice
     * with respect to its destination.
     */
    void throw_too_many_sources_error(
            const ordered_set<PHV::FieldSlice>& actionDataWrittenSlices,
            const ordered_set<PHV::FieldSlice>& notWrittenSlices,
            const UnionFind<PHV::FieldSlice>& phvSources,
            const ordered_map<PHV::FieldSlice, std::pair<int, int>>& phvAlignedSlices,
            const IR::MAU::Action* action,
            std::stringstream& ss) const;

    /** Detects if the destination to source mapping @p destToSource in @p action_name requires
     * a non-contiguous mask, that cannot be realized by deposit-field instructions. If not, then
     * craft and error message in @p ss.
     */
    void throw_non_contiguous_mask_error(
            const ordered_set<PHV::FieldSlice>& notWrittenSlices,
            const ordered_map<PHV::FieldSlice, ordered_set<PHV::FieldSlice>>& destToSource,
            const ordered_map<PHV::FieldSlice, unsigned>& fieldAlignments,
            cstring action_name,
            std::stringstream& ss) const;


    /// populates the UnionFind structure @p copacking_constraints, used to report new packing
    /// constraints induced by can_pack().


    /** Given the state of PHV allocation represented by @p alloc and @p container_state,
     * a vector of slices to be packed together and mutually live in a given container
     * @returns the number of container sources used in @p action
     */
    NumContainers num_container_sources(const PHV::Allocation& alloc,
                                        const PHV::Allocation::MutuallyLiveSlices& container_state,
                                        const IR::MAU::Action* action) const;

    /** Return the first allocated (in @p alloc) or proposed (in @p slices) source
     * of an instruction in @p action that writes to @p slice, if any.  There may
     * be up to two PHV sources per destination in any action, as each
     * destination may only be written to once.
     */
    std::optional<PHV::AllocSlice> getSourcePHVSlice(
        const PHV::Allocation& alloc,
        const std::vector<PHV::AllocSlice>& slices,
        const PHV::AllocSlice& dst,
        const IR::MAU::Action* action,
        const int stage) const;

    /** @returns true if @p slices packed in the same container read from action
     * data or from constant in action @p act
     */
    bool has_ad_or_constant_sources(
        const PHV::Allocation::MutuallyLiveSlices& slices,
        const IR::MAU::Action* action,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions) const;

    /** @returns true if all @p slices packed in the same container are all read from
     * action data or from constant in action @p act, or are never read from action data
     * or from constant in action @p act.
     */
    ActionDataUses all_or_none_constant_sources(
        const PHV::Allocation::MutuallyLiveSlices& slices,
        const IR::MAU::Action* act,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions) const;

    /** Check if two fields share the same container
      */
    bool fields_share_container(const PHV::Field *, const PHV::Field *);

    /** @returns the number of no unallocated bits in a set of mutually live field slices @p slice
      * in container @p c
      */
    int unallocated_bits(PHV::Allocation::MutuallyLiveSlices, const PHV::Container) const;

    /** @returns true if all AllocSlices in @p state are adjacent FieldSlices of the same field
      */
    bool are_adjacent_field_slices(const PHV::Allocation::MutuallyLiveSlices& state) const;

    /** @returns the number of holes that must be masked out by a given set of AllocSlices
      * @param state Candidate list of AllocSlices
      */
    unsigned count_container_holes(const PHV::Allocation::MutuallyLiveSlices& state) const;

    /** @returns false if the @p slices packed in the same byte result in violation of action
      * constraints for action @p act. Populates the error message accordingly in @p ss.
      * Used by early_constraints_ok.
      */
    bool valid_container_operation_type(
        const IR::MAU::Action* act,
        const ordered_set<PHV::FieldSlice>& slices,
        std::stringstream& ss) const;

    /** @returns the type of operation (OperandInfo::MOVE or OperandInfo::WHOLE_CONTAINER or
      * OperandInfo::BITWISE) if for @p action, the field slices in @p fields are all written
      * using either MOVE or BITWISE or WHOLE_CONTAINER operations (in case of WHOLE_CONTAINER,
      * no field in @p fields may be left unwritten by an action).
      * @returns OperandInfo::MIXED if there is a mix of WHOLE_CONTAINER, BITWISE, and MOVE
      * operations in the same action.
      */
    unsigned container_operation_type(
        const IR::MAU::Action*,
        const PHV::Allocation::MutuallyLiveSlices&,
        const PHV::Allocation::LiveRangeShrinkingMap&) const;

    /** Given a set of MutuallyLiveSlices @p container_state and the state of allocation @p alloc,
     * this function generates packing constraints induced by instructions in the action @p action.
     * These induced packing constraints are added to the UnionFind structure
     * @p packing_constraints.
     * Finally, when @p pack_unallocated_only is false, all field slices (both allocated and
     * unallocated sources) in @p container_state must be packed in the same container. When
     * @p pack_unallocated_only is true, only unallocated field slices in @p container_state
     * must be packed together.
     * @returns false if conditional constraints cannot be generated.
     */
    bool pack_slices_together(
        const PHV::Allocation& alloc,
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        PackingConstraints* packing_constraints,
        const IR::MAU::Action* action,
        bool pack_unallocated_only,
        bool has_ad_source = false) const;

    /** Check that at least one container in a two source PHV instruction is aligned with the
      * destination
      */
    std::optional<ClassifiedSources> verify_two_container_alignment(
            const PHV::Allocation& alloc,
            const PHV::Allocation::MutuallyLiveSlices& container_state,
            const IR::MAU::Action* action,
            const PHV::Container destination,
            ActionContainerProperty* action_prop) const;

    /** Returns offset (difference between lo bits) by which slice @p a differs from slice @p b
      * offset = (a.lo - b.lo) % b.container().size()
      * Invariant: Can only be called if @p a and @p b are assigned to same sized containers
      */
    inline int getOffset(le_bitrange a, le_bitrange b, PHV::Container c) const;

    /** Check that the bitmasks needed to realize a two source PHV instruction are valid
      */
    bool masks_valid(const ClassifiedSources& sources) const;

    /** @returns true if the @p container_state written to in action @p act is aligned with its
      * allocated sources.
      */
    bool is_aligned(
            const PHV::Allocation::MutuallyLiveSlices& container_state,
            const IR::MAU::Action* act,
            const PHV::Allocation& alloc) const;

    /** @returns true if a packing with one PHV source and one action data constant can be realized
      * such that the action data mask is contiguous. Generates and checks masks for only slices
      * with action data/constant sources, if actionDataOnly is true. Otherwise, generates and
      * checks masks for all slices.
      */
    bool masks_valid(
            const PHV::Allocation::MutuallyLiveSlices& container_state,
            const IR::MAU::Action* action,
            bool actionDataOnly,
            const PHV::Allocation::LiveRangeShrinkingMap& initActions) const;

    /** @returns true if the fields in @p container_state written in @p action can be synthesized
     * using a valid mask for a deposit-field instruction.
      */
    bool masks_valid(
            const PHV::Allocation::MutuallyLiveSlices& container_state,
            const IR::MAU::Action* action) const;

    /** For each set generated in @p copacking_constraints, populate map @p req_container
     * with the unallocated field slice and the container to which it should be allocated
     * (same as the container used by other allocated sources in that set).
     * @returns false if any mutually unsatisfiable container requirements are found.
     */
    bool assign_containers_to_unallocated_sources(
            const PHV::Allocation& alloc,
            const UnionFind<PHV::FieldSlice>& copacking_constraints,
            ordered_map<PHV::FieldSlice, PHV::Container>& req_container) const;

    //  (xxx)Deep [Specific Case Alert]: This will probably be subsumed by another function that
    //  handles the general case of producing conditional constraints for n unallocated sources
    //  packed into 2 containers.
    /** For cases where we have exactly two unallocated source PHV slices and no action
      * data/constant source (any number of allocated slices are okay), we need to enforce an
      * alignment for one of the source PHV slices. We choose to enforce the alignment constraint
      * for the source slice with the smaller width (which likely has more wiggle room for placement
      * within a container). This function returns the smaller of two unallocated slices found in
      * @p copacking_constraints. It also uses @p alloc to check that the two slices present are
      * indeed, unallocated.
      * @returns std::nullopt, if there are more than two unallocated source slices in
      * @p copacking_constraints.
      */
    std::optional<PHV::FieldSlice> get_smaller_source_slice(
            const PHV::Allocation& alloc,
            const UnionFind<PHV::FieldSlice>& copacking_constraints,
            const ordered_set<PHV::FieldSlice>& container_state) const;

    std::optional<PHV::FieldSlice> get_unallocated_slice(
            const PHV::Allocation& alloc,
            const UnionFind<PHV::FieldSlice>& copacking_constraints,
            const ordered_set<PHV::FieldSlice>& container_state) const;

    /// Track all the meter color destination to prioritize 8-bit PHV for such field.
    ordered_set<const PHV::Field*> meter_color_destinations;

    /// [Relaxed Artificial Constraint]: Right now action bus allocation requires any destination
    /// written by meter colors to be allocated to a 8-bit PHV if the result of the operation can't
    /// be rotated. This set keeps a track of all such destinations.
    ordered_set<const PHV::Field*> meter_color_destinations_8bit;

    /// (xxx)Deep [Artificial Constraint]: Right now, table placement requires that any field that
    /// gets its value from METER_ALU, HASH_DIST, RANDOM, or METER_COLOR, then it cannot be packed
    /// with other fields written in the same action
    ordered_map<const PHV::Field*, ordered_set<const IR::MAU::Action*>> special_no_pack;

    /// (xxx)Deep [Artificial Constraint]: @returns false if there is a field in
    /// @p container_state written by METER_ALU, HASH_DIST, RANDOM, or METER_COLOR,
    /// and another field in @p container_state is written by the same action
    bool check_speciality_packing(const PHV::Allocation::MutuallyLiveSlices& container_state) const;

    /// Generates copacking and alignment requirements due to @p action for the packing
    /// @p container_state in container @p c, where the number of PHV-backed sources are
    /// represented by @p sources and @p has_ad_constant_sources indicates if any slices
    /// in @p container_state is written using an action data or constant value. The copacking
    /// constraints are generated in @p copacking_constraints and the alignment requirements
    /// are generated and stored in @p phvMustBeAligned.
    /// @returns false if the packing cannot be realized and true if the packing
    /// is possible, with the conditional constraints generated.
    bool check_and_generate_constraints_for_move_with_unallocated_sources(
        const PHV::Allocation& alloc,
        const IR::MAU::Action* action,
        const PHV::Container& c,
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions,
        ActionContainerProperty* action_props,
        PackingConstraints* copacking_constraints) const;

    /// Generates conditional constraints for bitwise operations. Client for
    /// check_and_generate_constraints_for_bitwise_op_with_unallocated_sources.
    bool generate_conditional_constraints_for_bitwise_op(
            const PHV::Allocation::MutuallyLiveSlices& container_state,
            const IR::MAU::Action* action,
            const ordered_set<PHV::FieldSlice>& sources,
            PackingConstraints* copacking_constraints)
        const;

    /// Generates copacking and alignment requirements due to @p action for the packing
    /// @p container_state in container @p c, where the number of PHV-backed sources are
    /// represented by @p sources and @p has_ad_constant_sources indicates if any slices
    /// in @p container_state is written using an action data or constant value. The copacking
    /// constraints are generated in @p copacking_constraints and the alignment requirements
    /// are generated and stored in @p phvMustBeAligned.
    /// @returns false if the packing cannot be realized and true if the packing
    /// is possible, with the conditional constraints generated.
    bool check_and_generate_constraints_for_bitwise_op_with_unallocated_sources(
            const IR::MAU::Action* action,
            const PHV::Allocation::MutuallyLiveSlices& container_state,
            const NumContainers& sources,
            PackingConstraints* copacking_constraints)
        const;

    /// Perform analysis related to number of sources for every action.
    /// This method update copack_constraints and action_props.
    /// Brief description of how UnionFind (copack_constraints) is used here:
    /// - The UnionFind object contains a set of sets of field slices,
    ///   requiring that each field slice in the inner set be packed together.
    ///
    /// Example,
    ///   Metadata m {a, b, c, d, ...}  // Metadata header
    ///   Header vlan {                 // VLAN header
    ///       bit<3> priority;
    ///       bit<1> cfi;
    ///       bit<12> tag; }
    ///   Also, there are other headers m1 and m2 of type metadata m
    ///
    ///   Action {
    ///       m1.a = m2.a;
    ///       priority = m.c;
    ///       tag = m.d; }
    ///
    /// In this case, if container_state is {priority, cfi} and the candidate slice (slice) is tag,
    /// then the UnionFind structure will return {{m2.a}, {m.c, m.d}}.
    CanPackErrorCode check_and_generate_constraints_for_bitwise_or_move(
        const PHV::Allocation& alloc, const PHV::ActionSet& actions,
        const PHV::Allocation::MutuallyLiveSlices& container_state, const PHV::Container& c,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions, ActionPropertyMap* action_props,
        PackingConstraints* copack_constraints) const;

    /// Perform analysis on rotational aligment constraints.
    CanPackErrorCode check_and_generate_rotational_alignment_constraints(
        const PHV::Allocation& alloc, const std::vector<PHV::AllocSlice>& slices,
        const PHV::ActionSet& actions,
        const PHV::Allocation::MutuallyLiveSlices& container_state, const PHV::Container& c,
        ActionPropertyMap* action_props) const;

    /// update @p copacking_set and @p req_container.
    CanPackErrorCode check_and_generate_copack_set(
        const PHV::Allocation& alloc, const PHV::Allocation::MutuallyLiveSlices& container_state,
        const PackingConstraints& copack_constraints, const ActionPropertyMap& action_props,
        ordered_map<int, ordered_set<PHV::FieldSlice>>* copacking_set,
        ordered_map<PHV::FieldSlice, PHV::Container>* req_container) const;

    /// based on copacking_set and req_container generate conditional constraints.
    CanPackReturnType check_and_generate_conditional_constraints(
        const PHV::Allocation& alloc, const std::vector<PHV::AllocSlice>& slices,
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        const ordered_map<int, ordered_set<PHV::FieldSlice>>& copacking_set,
        const ordered_map<PHV::FieldSlice, PHV::Container>& req_container) const;

    /// @returns true if a packing violates the alignment constraints for stateful ALU destinations.
    bool stateful_destinations_constraints_violated(
            const PHV::Allocation::MutuallyLiveSlices& container_state) const;

    /// @returns true if any of the destinations require a speciality read and
    /// a bitmasked-set instruction for this packing, which violates MAU constraint.
    bool check_speciality_read_and_bitmask(
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions) const;

    /// Merge actions for all the candidate fields into a set, including initialization actions for
    /// any metadata fields overlaid due to live range shrinking.
    PHV::ActionSet make_writing_action_set(
        const PHV::Allocation& alloc, const PHV::Allocation::MutuallyLiveSlices& container_state,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions) const;

    /// generate action container properties for @p actions.
    ActionPropertyMap make_action_container_properties(
        const PHV::Allocation& alloc, const PHV::ActionSet& actions,
        const PHV::Allocation::MutuallyLiveSlices& container_state,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions, bool is_mocha_or_dark) const;

    /// generate pack constraints for all @p actions.
    PackingConstraints make_initial_copack_constraints(
        const PHV::ActionSet& actions,
        const PHV::Allocation::MutuallyLiveSlices& container_state) const;

    /// @returns the stages for the table associated with action @p action.
    /// The @p physical_stage is true, it will return physical live ranges, otherwise
    /// return min stage from table dependency graph.
    std::set<int> stages(const IR::MAU::Action* action, bool physical_stage) const;

 public:
    // Summary of sources for an action
    struct ActionSources {
        bool has_ad = false;
        bool has_const = false;

        // Allocated sources
        ordered_set<PHV::Container> phv;

        // Number of unallocated sources
        int unalloc = 0;

        // Destinations
        bitvec dest_bits;
    };

    explicit ActionPhvConstraints(
            const PhvInfo &p,
            const PhvUse& u,
            const PackConflicts& c,
            const MapTablesToActions& m,
            const DependencyGraph& d)
    : phv(p), uses(u), conflicts(c), tableActionsMap(m), dg(d),
      constraint_tracker(ConstraintTracker(p, *this)) { }

    /// Track all the meter color destination to prioritize 8-bit PHV for such field.
    bool is_meter_color_destination(const PHV::Field* f) const {
        return meter_color_destinations.count(f) > 0;
    }

    /// [Relaxed Artificial Constraint]: Right now action bus allocation requires any destination
    /// written by meter colors to be allocated to a 8-bit PHV if the result of the operation can't
    /// be rotated. This set keeps a track of all such destinations.
    bool is_meter_color_destination_8bit(const PHV::Field* f) const {
        return meter_color_destinations_8bit.count(f) > 0;
    }

    /// @returns true if the fieldslice @p fs1 and @p fs2 have a pack conflict.
    bool hasPackConflict(const PHV::FieldSlice fs1, const PHV::FieldSlice fs2) const {
        return conflicts.hasPackConflict(fs1, fs2);
    }

    /// @returns true if the field @p f is written using a speciality read (METER_ALU, HASH_DIST,
    /// RANDOM, or METER_COLOR).
    bool hasSpecialityReads(const PHV::Field* f) const {
        return special_no_pack.count(f) > 0;
    }

    /// @returns all the fields that are written using meter color fields.
    ordered_set<const PHV::Field*>& meter_color_dests() {
        return meter_color_destinations;
    }

    /// @returns all the fields that are written using meter color fields.
    const ordered_set<const PHV::Field*>& meter_color_dests() const {
        return meter_color_destinations;
    }

    /// @returns all the fields that are written using meter color fields that cannot be rotated.
    ordered_set<const PHV::Field*>& meter_color_dests_8bit() {
        return meter_color_destinations_8bit;
    }

    /// @returns all the fields that are written using meter color fields that cannot be rotated.
    const ordered_set<const PHV::Field*>& meter_color_dests_8bit() const {
        return meter_color_destinations_8bit;
    }

    /// @returns the set of all the actions which write field @p f.
    ordered_set<const IR::MAU::Action*> actions_writing_fields(const PHV::Field* f) const {
        return constraint_tracker.written_in(PHV::FieldSlice(f, StartLen(0, f->size)));
    }

    ordered_set<const IR::MAU::Action*> actions_writing_fields(const PHV::AllocSlice& slice) const {
        PHV::FieldSlice fieldSlice(slice.field(), slice.field_slice());
        return constraint_tracker.written_in(fieldSlice);
    }

    /// @returns the set of all actions which read field @p f.
    ordered_set<const IR::MAU::Action*> actions_reading_fields(const PHV::Field* f) const {
        return constraint_tracker.read_in(PHV::FieldSlice(f, StartLen(0, f->size)));
    }

    /// @returns the set of fields which are sources of @p slices of @p f across all actions.
    /// NOTE: sources are returned iff it completely writes some slice in @p slices.
    /// e.g.,
    /// assume we have f<32>[0:28] = a<28>;
    /// slices_sources(f, {f[0:31]}) will return {} instead of {a}, because the operation does
    /// not completely write any source slice.
    ordered_set<const PHV::Field*> slices_sources(
        const PHV::Field* dest, const std::vector<PHV::FieldSlice>& slices) const;

    /// @returns the set of fields which are destinations of @p slices of @p f across all actions.
    /// NOTE: destination are returned iff it completely read from some slice in @p slices.
    /// e.g., assume we have f<32>[0:28] = a<32>[0:28];
    /// slices_destinations(a, {a[0:31]}) will return {} instead of {f},
    /// because the operation does not completely write any source slice.
    ordered_set<const PHV::Field*> slices_destinations(
        const PHV::Field* src, const std::vector<PHV::FieldSlice>& slices) const;

    /// @returns the destination of field @p f in action @p action.
    /// @returns std::nullopt if @p f is not written in @p action.
    std::optional<const PHV::Field*> field_destination(
            const PHV::Field* f,
            const IR::MAU::Action* action) const;

    /// @returns true if the field @p f is only ever written by move operations.
    bool move_only_operations(const PHV::Field* f) const;

    /// @returns true if: for every slice s in @p container_state, if an action a in
    /// @p set_of_actions writes that slice, then all other slices in @p container_state
    /// are also written by a; and if an action b in @p set_of_actions does not write
    /// slice s, then all other slices in @p container_state are also not written by action b.
    bool all_field_slices_written_together(
            const PHV::Allocation::MutuallyLiveSlices& container_state,
            const PHV::ActionSet& set_of_actions,
            const PHV::Allocation::LiveRangeShrinkingMap& initActions) const;

    /// @returns true if there is a no-pack conflict between the fields in @p container_state.
    bool pack_conflicts_present(
            const PHV::Allocation::MutuallyLiveSlices& container_state,
            const std::vector<PHV::AllocSlice>& slices) const;

    /// @returns true if parser constant extract constraints are satisfied for the candidate
    /// packing. This only applies to Tofino, where fields written to using constant extractors in
    /// the same parser state must be allocated within 4 consecutive bits of the same 16-bit
    /// container, and  within consecutive 3 bits of the same 32 bit container. Note that this
    /// method will always return true for Tofino2, because no fields are marked as parser constant
    /// extract candidates by the ParserConstantExtract pass.
    bool parser_constant_extract_satisfied(
            const PHV::Container& c,
            const PHV::Allocation::MutuallyLiveSlices& container_state) const;

    /** Checks whether packing @p slices into a container will violate MAU action
     * constraints. @p initActions is the map of fields to the actions where those
     * fields must be initialized to enable metadata overlay.
     *
     * @returns bit positions at which unallocated slices must be allocated for
     * the proposed packing to be valid, or std::nullopt if no packing is
     * possible.  An empty map indicates the proposed packing is
     * unconditionally valid.
     *
     * TODO: Right now, the packing constraints generated by can_pack()
     * have one limitation.  Suppose there are n sources related to moves for a
     * container in a particular action, and 2 of the sources (s1 and s2) are
     * already allocated. In that case, valid packing requires the remaining
     * n-2 sources to be packed with either s1 or s2. Right now, s1, s2, and
     * all the other sources are put in the same set in the UnionFind. The
     * allocator needs to be aware of this case.
     */
    CanPackReturnType can_pack(
            const PHV::Allocation& alloc,
            const std::vector<PHV::AllocSlice>& slices,
            const PHV::Allocation::MutuallyLiveSlices& original_container_state,
            const PHV::Allocation::LiveRangeShrinkingMap& initActions) const;

    /// returns an eroor when any action constraint of @p actions are violated if
    /// the destination container has slices in @p container_state.
    CanPackErrorV2 check_bitwise_and_basic_move_constraints(
            const PHV::ActionSet& actions,
            const PHV::Allocation::MutuallyLiveSlices& container_state,
            const ActionPropertyMap* action_props) const;

    /// can_pack_v2 checks whether allocating @p slices will violate any action constraints.
    CanPackErrorV2 can_pack_v2(
            const PHV::Allocation& alloc,
            const std::vector<PHV::AllocSlice>& slices) const;

    /** @returns true if this packing would create container conflicts because of metadata
      * initialization issues.
      */
    bool creates_container_conflicts(
            const PHV::Allocation::MutuallyLiveSlices& container_state,
            const PHV::Allocation::LiveRangeShrinkingMap& initActions,
            const MapTablesToActions& tableActionsMap) const;

    /// Counts the number of bitmasked-set instructions corresponding to @p slices in container
    /// @p c and allocation object @p alloc.
    int count_bitmasked_set_instructions(
            const std::vector<PHV::AllocSlice>& slices,
            const PHV::Allocation::LiveRangeShrinkingMap& initActions) const;

    /// @returns true if the allocation @p container_state in a particular container, where slices
    /// in @p fields_not_written_to are not written in the given action, would result in the
    /// synthesis of a bitmasked-set instruction.
    bool is_bitmasked_set(
            const std::vector<PHV::AllocSlice>& container_state,
            const ordered_set<PHV::AllocSlice>& fields_not_written_to) const;

    /** Approximates a topographical sorting of field lists such that all source-only slice lists
      * are considered for allocation before destination-only slice lists.
      * Sorts a given list of slice list such that the slice lists used the least times as sources
      * are first allocated containers. If the number of times fields in a slice list is used as
      * sources is the same as another slice list, the slice list which is written to in more
      * actions ranks earlier in the sorting.
      */
    void sort(std::list<const PHV::SuperCluster::SliceList*>& slice_list) const;

    /** Approximates a topographical sorting of FieldSlices such that all FieldSlices used only as
      * sources are considered for allocation before destination-only FieldSlices
      */
    void sort(std::vector<PHV::FieldSlice>& slice_list) const;

    /// sort @p slice_lists by
    /// 1. a topographical order of their data flow graph, where destinations will be sorted
    ///    to the front. Strongly-connected-components will be converted to one node. Every
    ///    slice lists will be represented by the highest priority field (deepest in source to
    ///    destination flow, i.e., like the `last destination`) of the list.
    /// 2. number of actions that write to the slice list.
    /// 3. lo part of the same-field list will be allcoated first.
    void dest_first_sort(std::vector<const PHV::SuperCluster::SliceList*>& slice_list) const;

    /** @returns the set of fields that are read in action @p act
      */
    ordered_set<const PHV::Field*> actionReads(const IR::MAU::Action* act) const;

    const ordered_set<PHV::FieldSlice>& actionReadsSlices(const IR::MAU::Action* act) const {
        return constraint_tracker.reads(act);
    }

    /** @returns the set of fields that are written in action @p act
      */
    ordered_set<const PHV::Field*> actionWrites(const IR::MAU::Action* act) const;

    const ordered_set<PHV::FieldSlice> actionWritesSlices(const IR::MAU::Action* act) const {
        ordered_set<PHV::FieldSlice> rs;
        auto& writtenSlices = constraint_tracker.writes(act);
        for (auto& info : writtenSlices)
            if (info.phv_used != std::nullopt)
                rs.insert(*(info.phv_used));
        return rs;
    }

    /** @returns true if the candidate bridged metadata packing @p packing satisfies action
      * constraints
      */
    bool checkBridgedPackingConstraints(const ordered_set<const PHV::Field*>& packing) const;

    /** @returns true if metadata initialization can be done for a field in container @p c
     * at action @p action.
     */
    bool cannot_initialize(
            const PHV::Container& c,
            const IR::MAU::Action* action,
            const PHV::Allocation& alloc) const;

    /** @returns true if the POV bit @p f can be packed in the same container and the next
     * position in @p slice_list.
     */
    bool can_pack_pov(
            const PHV::SuperCluster::SliceList* slice_list,
            const PHV::Field* f) const;

    /** Diagnose PHV allocation failure for supercluster @p sc containing field slices
     * in slice lists with alignment @p sliceAlignments.
     * @returns true if a concrete error violation is detected. Populates @p error_msg with
     * the appropriate error message.
     */
    bool diagnoseSuperCluster(
            const ordered_set<const PHV::SuperCluster::SliceList*>& sc,
            const ordered_map<PHV::FieldSlice, unsigned>& fieldAlignments,
            std::stringstream& error_msg) const;

    /** @returns true if field @p f is written in action @p act.
      */
    bool written_in(const PHV::Field* f, const IR::MAU::Action* act) const;

    /** @returns true if field slice @p slice is written in action @p act.
      */
    bool written_in(const PHV::AllocSlice& slice, const IR::MAU::Action* act) const {
        PHV::FieldSlice field_slice(slice.field(), slice.field_slice());
        return constraint_tracker.written_in(field_slice).count(act);
    }

    /** @returns true if field @p f is written in action @p act by action data/constant source.
      */
    bool written_by_ad_constant(const PHV::Field* f, const IR::MAU::Action* act) const;

    /** For GTest function.
      * Checks if the field_writes_to_actions ordered_map entry is valid or not
      */
    bool is_in_field_writes_to_actions(cstring, const IR::MAU::Action *) const;
    /** For GTest function.
      * Checks if the action_to_writes ordered_map entry is valid or not
      */
    bool is_in_action_to_writes(const IR::MAU::Action *, cstring) const;
    /** For GTest function.
      * Checks if the write_to_reads_per_action ordered_map entry is valid or not
      */
    bool is_in_write_to_reads(cstring, const IR::MAU::Action *, cstring) const;

    /** compute_sources_first_order returns a mapping from field to an integer which
     *  represents the priority (lower number higher priority) if we need to allocate
     *  source fields first. Because fields can form a strongly connected component, i.e. a loop,
     *  in terms of writes, e.g. two fields write to each other, we will run a Tarjan algorithm
     *  first to replace those SCCs with a single node and then we run a topological sort.
     *  For example, Assume a <- b represents b writes to a, and we have
     *
     *  a <- b <- c <-> e
     *       ^
     *       |
     *       d
     *
     *  will be converted to,
     *  a <- b <- {c, e}
     *       ^
     *       |
     *       d
     *
     *  and the output will be,
     *  d, c, e: 1
     *  b: 2
     *  a: 3
     *
     *  i.e. we encounter a loop, convert that loop into one node.
     */
    ordered_map<const PHV::Field*, int> compute_sources_first_order(
        const ordered_map<const PHV::Field*, std::vector<PHV::FieldSlice>>& fields) const;

    /** Find all sources of an action that write to a given container
     *
     * @param act action for which to find the sources
     * @param c destination container of interest for the action
     * @param new_slices slices to be added to the allocation
     * @param alloc current Allocation object containing allocated slices
     *
     * @returns an ActionSources struct with the source information
     *
     * FIXME: This function can be made non-public once live range shrinking/dark overlay
     * functionality is merged and all initialization points are correctly exposed.
     */
    ActionSources getActionSources(
            const IR::MAU::Action* act, const PHV::Container& c,
            ordered_set<PHV::AllocSlice>& new_slices,
            const PHV::Allocation& alloc) const;

    /** Checks whether packing @p slices into a container will violate MAU action constraints for
     *  all move-based instructions, using an action move constraint solver for normal container
     *  destination. For mocha and dark container, check basic constraints that:
     *  (1) mocha container can only be written in whole by ad/constant/container.
     *  (2) dark container can only be written in while by container.
     */
    CanPackErrorV2 check_move_constraints(
        const PHV::Allocation& alloc, const IR::MAU::Action* action,
        const std::vector<PHV::AllocSlice>& slices,
        const PHV::Allocation::MutuallyLiveSlices& container_state, const PHV::Container& c,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions) const;

    /** Besides actual actions, we need to check always run actions:
    * (1) always run action does not overwrite other fields. For example, if we
    *     have two sources to one destination in one ARA, we need verify there's
    *     no other field in destination that are not mutex.
    * (2) TODO: always run action can be synthesized: we cannot do it now
    *     because we don't have a way to get all ARA instructions for a specific point, and
    *     because we do table placement after PHV, it's always possible
    *     to have any ARA, so the check here is trying to avoid introducing new dependencies.
    *     table placement will add new stages when multiple instruction cannot
    *     be synthesized in one always run action.
    */
    CanPackErrorCode check_ara_move_constraints(
        const PHV::Allocation& alloc,
        const PHV::Allocation::MutuallyLiveSlices& container_state, const PHV::Container& c,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions) const;

    /// Check action move constraints when a source slice is allocated in @p slices.
    /// It will iterate over all actions that has read any in @p slices and run move constraint
    /// checker on its destination container, with fields during the action.
    CanPackErrorV2 check_move_constraints_from_read(
        const PHV::Allocation& alloc,
        const std::vector<PHV::AllocSlice>& candidates,
        const PHV::Allocation::LiveRangeShrinkingMap& initActions) const;

    /// Use action constraint solver to check move-based instruction constraints from
    /// reader-side, i.e., @p candidates are sources of instructions.
    CanPackErrorV2 check_read_action_move_constraints(
    const PHV::Allocation& alloc,
    const std::vector<PHV::AllocSlice>& candidates,
    const IR::MAU::Action* action,
    const PHV::Allocation::LiveRangeShrinkingMap& initActions) const;

    /// @returns error if the number of sources is more than allowed for in @p action, when we
    /// allocate sources @p candidates to @p alloc.
    CanPackErrorV2 check_read_action_num_source_constraints(
        const PHV::Allocation& alloc,
        const std::vector<PHV::AllocSlice>& candidates,
        const IR::MAU::Action* action) const;
};

std::ostream &
operator<<(std::ostream &out, const ActionPhvConstraints::OperandInfo& info);
std::ostream &
operator<<(std::ostream &out, const safe_vector<ActionPhvConstraints::OperandInfo>& info);
std::ostream &
operator<<(std::ostream &out, const ActionPhvConstraints::ClassifiedSource& src);

#endif  /* BF_P4C_PHV_ACTION_PHV_CONSTRAINTS_H_ */
