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

#ifndef BF_P4C_MAU_ACTION_DATA_BUS_H_
#define BF_P4C_MAU_ACTION_DATA_BUS_H_

#include "bf-p4c/mau/action_format.h"
#include "bf-p4c/mau/table_layout.h"
#include "lib/safe_vector.h"

using namespace P4;

/** Algorithm for the allocation of the action data bus.  The action data bus is broken into
 *  3 parts, a BYTE, HALF, and FULL region.  The byte and half regions are mutually exclusive,
 *  while the full region shares places with the byte and half region.  There are 32 outputs
 *  for each type of container region.  The bytes 0-31 are for the byte region, bytes 32-95
 *  are for the half region, and all 128 bytes are for the full regions.  Because actions
 *  within the same table are mutually exclusive, FULLs can potentially share locations with
 *  BYTES and HALFs on the action data bus.
 *
 *  The version of the algorithm updates the tables that have been previously allocated.  It
 *  then attempts to allocate the BYTES and HALVES.  Then the algorithm checks to see if the
 *  regions can be shared, and if not, allocates a separate byte.
 *
 *  The rules for allocation are set by the uArch on section 5.2.5.1/6.2.5.1 on the Action Data
 *  Bus.  The section is titled Action Output HV Xbar Programming, under the section Action
 *  Output HV Xbar(s) in the Section RAM Array.  The bytes are broken into 16 byte regions
 *  and bytes in particular offsets of the action data table have to be allocated contiguously
 *  within these 16 byte regions.  The constraints are better detailed over the alloc_bytes,
 *  alloc_halves, and alloc_fulls function calls.
 *
 *  For immediate actions, the constraints are detailed in the uArch in section 5.4.4.2/6.4.4.2.
 *  The section is titled Immediate Action Data, under Address Distribution, under Match Central.
 *  Unlike for action data tables, which are broken up into 8 16 byte muxes, there are only
 *  3 muxes for each immediate section.  There is a byte by byte mux between bytes 0-15, a
 *  half-word mux between bytes 16-63, and a full word mux between bytes 64-128.  These
 *  muxes can only have one input per immediate word.  Immediate allocation on the action bus
 *  occurs on a mod 4 post-shift in match central.
 *
 *  The constraints are similar to action data tables in that regions of the immediate data must
 *  be contiguously allocated within a 16 byte region of the action data bus.  With immediate
 *  action data, the input allocation must be 4 byte contiguous.
 */
struct ActionDataBus {
 public:
    struct Loc : public IHasDbPrint {
        int byte;
        ActionData::SlotType_t type;
        Loc(int b, ActionData::SlotType_t t) : byte(b), type(t) {}
        void dbprint(std::ostream &out) const {
            int slot_size = (8 << type);
            out << "ADB[" << byte << ":" << ((byte + slot_size / 8) - 1) << "]";
        }
        bool operator==(const Loc &loc) const {
            if (byte != loc.byte) return false;
            if (type != loc.type) return false;
            return true;
        };
        bool operator!=(const Loc &loc) const { return !(*this == loc); }
    };
    /** Information on the sharing of resources between the BYTE/HALF regions and a potential
     *  full setion sharing the same location in the action data table
     */
    struct FullShare {
        bool full_in_use = false;
        unsigned shared_status = 0;
        int shared_byte[2] = {-1, -1};
        bool full_bitmasked_set = false;
        int full_bitmasked_index = -1;
        void init_full_bitmask(int fbi) {
            full_bitmasked_set = true;
            full_bitmasked_index = fbi;
        }
    };

    /** The bytes used by an individual table, stored in the TableResourceAlloc */
    struct Use {
        /** An individual space within the Use.  Used to update the reserved space */
        struct ReservedSpace {
            Loc location;
            int byte_offset = -1;  ///> Bytes from the lsb of the Action Data Format
            bitvec bytes_used;     ///> Not all of the bytes in the slot may necessarily be used,
                                   ///> if they don't contain action data
            ActionData::Location_t source = ActionData::ALL_LOCATIONS;  ///> Is IMMED, ADT or METER
            ReservedSpace(Loc l, int bo, bitvec bu)
                : location(l), byte_offset(bo), bytes_used(bu) {}
            ReservedSpace(Loc l, int bo, bitvec bu, ActionData::Location_t src)
                : location(l), byte_offset(bo), bytes_used(bu), source(src) {}
            void dbprint(std::ostream &out) const {
                out << "Source:Offset " << source << ":" << byte_offset << " : " << location;
            }
            bool operator==(const ReservedSpace &rs) const {
                if (location != rs.location) return false;
                if (byte_offset != rs.byte_offset) return false;
                if (bytes_used != rs.bytes_used) return false;
                if (source != rs.source) return false;
                return true;
            }
            bool operator!=(const ReservedSpace &rs) const { return !(*this == rs); }
        };
        // Locations of action data where the ALUs will pull action data
        safe_vector<ReservedSpace> action_data_locs;
        // Slots reserved because unused action data was needed
        safe_vector<ReservedSpace> clobber_locs;
        virtual ~Use() {}
        virtual void clear() {
            action_data_locs.clear();
            clobber_locs.clear();
        }
        virtual Use *clone() const = 0;
        virtual bool emit_adb_asm(std::ostream &, const IR::MAU::Table *, bitvec source) const = 0;
        virtual bool empty() const { return action_data_locs.empty() && clobber_locs.empty(); }
        virtual int rng_unit() const = 0;
        bool operator==(const Use &use) const {
            if (action_data_locs != use.action_data_locs) return false;
            if (clobber_locs != use.clobber_locs) return false;
            return true;
        };
        bool operator!=(const Use &use) const { return !(*this == use); }
        friend std::ostream &operator<<(std::ostream &out, const Use &u);
    };

    /** Location Algorithm: For finding an allocation within the correct type region */
    enum loc_alg_t {
        FIND_NORMAL,
        FIND_LOWER,
        FIND_FULL,
        FIND_IMMED_UPPER,
        FIND_FULL_HALF,
        FIND_FULL_BYTE
    };

    virtual void clear();

 protected:
    // Holds information on already allocated ActionProfiles, Meters, and Stateful Alus
    ordered_map<const IR::MAU::AttachedMemory *, const Use &> allocated_attached;
    // Holds information on already allocated stateful ALUs in the same reduction or group
    ordered_map<cstring, const Use &> reduction_or_mapping;

 public:
    virtual ~ActionDataBus() {}

    virtual bool alloc_action_data_bus(const IR::MAU::Table *tbl,
                                       const ActionData::Format::Use *use,
                                       TableResourceAlloc &alloc) = 0;
    virtual bool alloc_action_data_bus(const IR::MAU::Table *tbl, const MeterALU::Format::Use *use,
                                       TableResourceAlloc &alloc) = 0;
    virtual void update(cstring name, const Use &alloc);
    virtual void update(cstring name, const Use::ReservedSpace &rs) = 0;
    virtual void update(const IR::MAU::Table *tbl);
    virtual void update(cstring name, const TableResourceAlloc *alloc, const IR::MAU::Table *tbl);
    virtual void update_action_data(cstring name, const TableResourceAlloc *alloc,
                                    const IR::MAU::Table *tbl);
    virtual void update_meter(cstring name, const TableResourceAlloc *alloc,
                              const IR::MAU::Table *tbl);

    static ActionDataBus *create();
    static int getAdbSize();
    friend std::ostream &operator<<(std::ostream &out, const ActionDataBus &adb);
};

#endif /* BF_P4C_MAU_ACTION_DATA_BUS_H_*/
