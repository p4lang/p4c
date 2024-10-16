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

#ifndef BF_P4C_PHV_UTILS_SLICE_ALLOC_H_
#define BF_P4C_PHV_UTILS_SLICE_ALLOC_H_

#include <optional>
#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/lib/small_set.h"
#include "bf-p4c/phv/phv.h"
#include "lib/bitvec.h"
#include "lib/symbitmatrix.h"
#include "ir/ir.h"

namespace PHV {

using namespace P4;

using ActionSet = SmallSet<const IR::MAU::Action*>;
using UnitSet = SmallSet<const IR::BFN::Unit*>;
using RefsMap = ordered_map<cstring, FieldUse>;

class DarkInitPrimitive;
class DarkInitEntry;
class Field;
class AllocContext;
class AllocSlice;
// Enum for selecting how to match AllocSlices
enum class SliceMatch {  DFLT = 0,            // Use min/max_stage_i
                         REF = 1,             // Use refs
                         REF_DG_LR = 2,       // Use d-graph stage of refs
                         REF_PHYS_LR = 4 };   // Use physical stage of refs

class DarkInitPrimitive {
 private:
     bool assignZeroToDestination;
     bool nop;
     std::unique_ptr<AllocSlice> sourceSlice;
     bool alwaysInitInLastMAUStage;
     bool alwaysRunActionPrim;
     ActionSet actions;
     UnitSet priorUnits;   // Hold units of prior overlay slice
     UnitSet postUnits;   // Hold units of post overlay slice
     std::vector<DarkInitEntry*> priorPrims;  // Hold prior ARA prims
     std::vector<DarkInitEntry*> postPrims;  // Hold post ARA prims

 public:
     DarkInitPrimitive(void)
         : assignZeroToDestination(false), nop(false), sourceSlice(),
         alwaysInitInLastMAUStage(false), alwaysRunActionPrim(false) { }

    explicit DarkInitPrimitive(const ActionSet& initPoints);
    explicit DarkInitPrimitive(AllocSlice& src);
    explicit DarkInitPrimitive(AllocSlice& src, const ActionSet& initPoints);
    explicit DarkInitPrimitive(const DarkInitPrimitive& other);
    DarkInitPrimitive& operator=(const DarkInitPrimitive& other);

    bool operator==(const DarkInitPrimitive& other) const;

     bool isEmpty() const {
         if (!nop && !sourceSlice && !assignZeroToDestination)
             return true;
         return false;
     }

     void addSource(const AllocSlice& sl);

     void setNop() {
         nop = true;
         sourceSlice.reset();
         assignZeroToDestination = false;
     }

     void addPriorUnits(const UnitSet& units, bool append = true) {
         if (!append) {
             priorUnits.clear();
         }
         priorUnits.insert(units.begin(), units.end());
     }

     void addPostUnits(const UnitSet& units, bool append = true) {
         if (!append) {
             postUnits.clear();
         }
         postUnits.insert(units.begin(), units.end());
     }

     void addPriorPrims(DarkInitEntry* prims, bool append = true) {
         if (!append) {
             priorPrims.clear();
         }
         priorPrims.push_back(prims);
     }
     void addPostPrims(DarkInitEntry* prims, bool append = true) {
         if (!append) {
             postPrims.clear();
         }
         postPrims.push_back(prims);
     }

     void setLastStageAlwaysInit() { alwaysInitInLastMAUStage = alwaysRunActionPrim = true; }
     void setAlwaysRunActionPrim() { alwaysRunActionPrim = true; }
     void setAssignZeroToDest();
     bool isNOP() const { return nop; }
     bool destAssignedToZero() const { return assignZeroToDestination; }
     bool mustInitInLastMAUStage() const { return alwaysInitInLastMAUStage; }
     bool isAlwaysRunActionPrim() const { return alwaysRunActionPrim; }
     AllocSlice* getSourceSlice() const {
         return sourceSlice.get();
     }
     bool setSourceLatestLiveness(StageAndAccess max);
     const ActionSet& getInitPoints() const { return actions; }
     const UnitSet& getARApriorUnits() const { return priorUnits; }
     const UnitSet& getARApostUnits() const { return postUnits; }
     const std::vector<DarkInitEntry*> getARApriorPrims() const { return priorPrims; }
     const std::vector<DarkInitEntry*> getARApostPrims() const { return postPrims; }
};

class AllocSlice {
    const Field* field_i;
    Container container_i;
    int field_bit_lo_i;
    int container_bit_lo_i;
    int width_i;
    ActionSet init_points_i;
    StageAndAccess min_stage_i;
    StageAndAccess max_stage_i;
    mutable RefsMap refs;
    DarkInitPrimitive init_i;

    // Is true if the alloc is copied from an alias destination alloc that requires an always run
    // in the final stage.
    bool shadow_always_run_i = false;

    // Is true if the alloc is copied from an alias destination alloc that is zero-initialized
    bool shadow_init_i = false;

    bool has_meta_init_i = false;

    // Is true if all stageAndAccess vars are generated from physical_live_range, i.e.,
    // min_stage_i and max_stage_i are based on physical stage info instead of min_stages.
    bool is_physical_stage_based_i = false;

    // After FinalizeStageAllocation set to true for assembly generation
    bool physical_deparser_stage_i = false;

    // Set to true to enable assembly generation for profile exceeding allowed stages
    bool physical_deparser_stage_exceeded_i = false;

 public:
    AllocSlice(const Field* f, Container c, int f_bit_lo, int container_bit_lo,
               int width);
    AllocSlice(const Field* f, Container c, int f_bit_lo, int container_bit_lo, int width,
               const ActionSet& action);
    AllocSlice(const Field* f, Container c, le_bitrange f_slice,
               le_bitrange container_slice);

    static const int NOTSET = -2;

    AllocSlice(const AllocSlice& a);
    AllocSlice& operator=(const AllocSlice& a);
    AllocSlice(AllocSlice&&) = default;
    AllocSlice& operator=(AllocSlice&&) = default;

    bool operator==(const AllocSlice& other) const;
    bool operator!=(const AllocSlice& other) const;
    bool operator<(const AllocSlice& other) const;
    bool isEarlierFieldslice(const AllocSlice& other) const;
    bool same_alloc_fieldslice(const AllocSlice& other) const;

    // returns a cloned AllocSlice split by field[start:start+ len - 1].
    // by_field indicates that @p start and @p len are applied on field.
    // e.g.
    // alloc_slice: container[5:28] <= f1[0:23]
    // ^^^.sub_alloc_by_field(3,6) returns
    // alloc_slice: container[8:13] <= f1[3:8]
    std::optional<AllocSlice> sub_alloc_by_field(int start, int len) const;

    const Field* field() const              { return field_i; }
    Container container() const             { return container_i; }
    le_bitrange field_slice() const         { return StartLen(field_bit_lo_i, width_i); }
    le_bitrange container_slice() const     { return StartLen(container_bit_lo_i, width_i); }
    le_bitrange container_bytes() const;
    bool is_initialized() const;
    int width() const                       { return width_i; }
    const DarkInitPrimitive& getInitPrimitive() const { return init_i; }
    DarkInitPrimitive& getInitPrimitive() { return init_i; }
    const StageAndAccess& getEarliestLiveness() const { return min_stage_i; }
    const StageAndAccess& getLatestLiveness() const { return max_stage_i; }
    std::pair<Container, int> container_byte() const {
        BUG_CHECK(container_bit_lo_i%8U + width_i <= 8U, "%s is not in one container byte", *this);
        return std::make_pair(container_i, container_bit_lo_i & ~7); }

    bool hasInitPrimitive() const;

    // Get the physical liverange of a slice based on its refs
    void get_ref_lr(StageAndAccess& minStg, StageAndAccess& maxStg) const;

    // @returns true is this alloc slice is live at @p stage for @p use.
    bool isLiveAt(int stage, const FieldUse& use) const;

    // @returns true if @p other and this AllocSlice have disjoint live ranges.
    bool isLiveRangeDisjoint(const AllocSlice& other, int gap = 0) const;

    bool representsSameFieldSlice(const AllocSlice& other) const {
        if (field_i != other.field()) return false;
        if (field_slice() != other.field_slice()) return false;
        if (width_i != other.width()) return false;
        return true;
    }

    bool extends_live_range(const AllocSlice& other) const {
        if ((min_stage_i.first < other.getEarliestLiveness().first) ||
            (max_stage_i.first > other.getLatestLiveness().first) ||
            ((min_stage_i.first == other.getEarliestLiveness().first) &&
             (min_stage_i.second < other.getEarliestLiveness().second)) ||
            ((max_stage_i.first == other.getLatestLiveness().first) &&
             (max_stage_i.second > other.getLatestLiveness().second)))
            return true;
        return false;
    }

    void setLiveness(const StageAndAccess& min, const StageAndAccess& max) {
        min_stage_i = std::make_pair(min.first, min.second);
        max_stage_i = std::make_pair(max.first, max.second);
    }

    void setLatestLiveness(const StageAndAccess& max) {
        max_stage_i = std::make_pair(max.first, max.second);
    }

    void setEarliestLiveness(const StageAndAccess& min) {
        min_stage_i = std::make_pair(min.first, min.second);
    }

    void setInitPrimitive(const DarkInitPrimitive* prim);

    bool hasMetaInit() const { return has_meta_init_i; }
    void setMetaInit() { has_meta_init_i = true; }
    const ActionSet& getInitPoints() const { return init_points_i; }
    void setInitPoints(const ActionSet& init_points) { init_points_i = init_points; }
    void setShadowAlwaysRun(bool val) { shadow_always_run_i = val; }
    bool getShadowAlwaysRun() const { return shadow_always_run_i; }
    void setShadowInit(bool val) { shadow_init_i = val; }
    bool getShadowInit() const { return shadow_init_i; }
    const RefsMap& getRefs() const { return refs; }
    void clearRefs() { refs.clear(); }
    void addRefs(const RefsMap& sl_refs, bool clear_refs = false) {
        if (clear_refs)
            refs.clear();
        for (auto ref_entry : sl_refs)
            addRef(ref_entry.first, ref_entry.second);
    }

    bool addRef(cstring, FieldUse f_use) const;

    bool isUsedDeparser() const;
    bool isUsedParser() const;

    bool isPhysicalStageBased() const { return is_physical_stage_based_i; }
    void setIsPhysicalStageBased(bool v) { is_physical_stage_based_i = v; }

    bool isPhysicalDeparserStage() const { return physical_deparser_stage_i; }
    void setPhysicalDeparserStage(bool v) { physical_deparser_stage_i = v; }

    bool isPhysicalDeparserStageExceeded() const { return physical_deparser_stage_exceeded_i; }
    void setPhysicalDeparserStageExceeded(bool v) { physical_deparser_stage_exceeded_i = v; }

    bool isUninitializedRead() const {
        return (getEarliestLiveness().second.isRead() &&
                getLatestLiveness().second.isRead());
    }


    // @returns true if this alloc slice is referenced within @p ctxt for @p use.
    bool isReferenced(const AllocContext* ctxt, const FieldUse* use,
                      SliceMatch match = SliceMatch::DFLT) const;
    std::string toString() const;

 private:
    // helpers to get id of parde units for StageAndAccess. Depending on is_physical_live_i,
    // stage indexes of parser and deparser are different.
    int parser_stage_idx() const;
    int deparser_stage_idx() const;
};

class DarkInitEntry {
 private:
     AllocSlice destinationSlice;
     DarkInitPrimitive initInfo;

 public:
     explicit DarkInitEntry(AllocSlice& dest) : destinationSlice(dest) { }
     explicit DarkInitEntry(AllocSlice& dest, const ActionSet& initPoints)
         : destinationSlice(dest), initInfo(initPoints) { }
     explicit DarkInitEntry(AllocSlice& dest, AllocSlice& src)
         : destinationSlice(dest), initInfo(src) { }
     explicit DarkInitEntry(AllocSlice& dest, AllocSlice& src, const ActionSet& init)
         : destinationSlice(dest), initInfo(src, init) { }
     explicit DarkInitEntry(const AllocSlice& dest, const DarkInitPrimitive &src)
         : destinationSlice(dest), initInfo(src) { }

     void addSource(AllocSlice sl) { initInfo.addSource(sl); }
     void setNop() { initInfo.setNop(); }
     void setLastStageAlwaysInit() { initInfo.setLastStageAlwaysInit(); }
     void setAlwaysRunInit() { initInfo.setAlwaysRunActionPrim(); }
     bool isNOP() const { return initInfo.isNOP(); }
     bool destAssignedToZero() const { return initInfo.destAssignedToZero(); }
     bool mustInitInLastMAUStage() const { return initInfo.mustInitInLastMAUStage(); }
     AllocSlice* getSourceSlice() const { return initInfo.getSourceSlice(); }

     void addPriorUnits(const UnitSet& units, bool append = true) {
         initInfo.addPriorUnits(units, append);
     }
     void addPostUnits(const UnitSet& units, bool append = true) {
         initInfo.addPostUnits(units, append);
     }

     void addPriorPrims(DarkInitEntry* prims, bool append = true) {
         initInfo.addPriorPrims(prims, append);
     }
     void addPostPrims(DarkInitEntry* prims, bool append = true) {
         initInfo.addPostPrims(prims, append);
     }

     const ActionSet& getInitPoints() const { return initInfo.getInitPoints(); }
    const AllocSlice& getDestinationSlice() const { return destinationSlice; }
    AllocSlice getDestinationSlice() { return destinationSlice; }
    const DarkInitPrimitive& getInitPrimitive() const { return initInfo; }
    DarkInitPrimitive& getInitPrimitive() { return initInfo; }
    void setDestinationLatestLiveness(const StageAndAccess& max) {
        destinationSlice.setLatestLiveness(max);
    }
    void setDestinationEarliestLiveness(const StageAndAccess& min) {
        destinationSlice.setEarliestLiveness(min);
    }

    void addRefs(const RefsMap& sl_refs, bool clear_refs = false) {
        destinationSlice.addRefs(sl_refs, clear_refs);
    }

    void addDestinationUnit(cstring tName, FieldUse tRef) {
        destinationSlice.addRef(tName, tRef);
    }

    bool operator<(const DarkInitEntry& other) const {
        if (destinationSlice != other.getDestinationSlice())
            return destinationSlice < other.getDestinationSlice();
        if (getSourceSlice() && other.getSourceSlice() &&
            *(getSourceSlice()) != *(other.getSourceSlice()))
            return *(getSourceSlice()) < *(other.getSourceSlice());
        if (getSourceSlice() && !other.getSourceSlice())
            return true;
        return false;
    }

    bool operator==(const DarkInitEntry& other) const {
        LOG4("DarkInitEntry == : " << &destinationSlice << " <-> ");

        return
            (destinationSlice == other.getDestinationSlice()) &&
            (initInfo == other.getInitPrimitive());
    }

    bool operator!=(const DarkInitEntry& other) const {
        return !this->operator==(other);
    }
};

std::ostream &operator<<(std::ostream &out, const AllocSlice&);
std::ostream &operator<<(std::ostream &out, const AllocSlice*);
std::ostream &operator<<(std::ostream &out, const std::vector<AllocSlice>&);
std::ostream &operator<<(std::ostream &out, const DarkInitEntry&);
std::ostream &operator<<(std::ostream &out, const DarkInitPrimitive&);

}   // namespace PHV

std::ostream &operator<<(std::ostream &, const safe_vector<PHV::AllocSlice> &);

#endif  /* BF_P4C_PHV_UTILS_SLICE_ALLOC_H_ */
