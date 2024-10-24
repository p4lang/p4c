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

#ifndef BF_P4C_MAU_TOFINO_ACTION_DATA_BUS_H_
#define BF_P4C_MAU_TOFINO_ACTION_DATA_BUS_H_

/* clang-format off */
#include "bf-p4c/mau/action_data_bus.h"
#include "bf-p4c/mau/table_layout.h"
#include "bf-p4c/mau/action_format.h"
#include "lib/autoclone.h"
#include "lib/safe_vector.h"
#include "bf-p4c/common/alloc.h"
/* clang-format on */
namespace Tofino {

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
struct ActionDataBus : public ::ActionDataBus {
    static constexpr int OUTPUTS = 32;
    static constexpr int ADB_BYTES = 128;
    static constexpr int PAIRED_OFFSET = 16;
    static constexpr int BYTES_PER_RAM = 16;
    static constexpr int ADB_STARTS[ActionData::SLOT_TYPES] = {0, 32, 96};
    static constexpr int IMMED_DIVIDES[ActionData::SLOT_TYPES] = {16, 64, 128};
    static constexpr int IMMED_SECT = 4;
    static constexpr int CSR_RANGE = 16;
    static constexpr int CSR_SECTION[ActionData::SLOT_TYPES] = {4, 3, 2};
    static constexpr int RANDOM_NUMBER_GENERATORS = 2;

 private:
    BFN::Alloc2D<cstring, ActionData::SLOT_TYPES, OUTPUTS> cont_use;
    BFN::Alloc1D<cstring /* table name */, ADB_BYTES> total_use;
    BFN::Alloc2D<cstring, RANDOM_NUMBER_GENERATORS, IMMED_SECT> rng_use;

    bitvec cont_in_use[ActionData::SLOT_TYPES];
    bitvec total_in_use;
    bitvec rng_in_use[RANDOM_NUMBER_GENERATORS];

    safe_vector<std::pair<int, int>> immed_starts;
    std::set<cstring> atcam_updates;

    bool reserved_immed[3] = {false, false, false};

    struct ADB_CSR {
        safe_vector<bool> reserved;
        ActionData::SlotType_t type;

        explicit ADB_CSR(ActionData::SlotType_t t) : type(t) {
            reserved.resize(ActionDataBus::CSR_SECTION[type], false);
        }

        bool operator==(const ADB_CSR &adb_csr) const {
            if (reserved != adb_csr.reserved) return false;
            if (type != adb_csr.type) return false;
            return true;
        }
    };

    typedef safe_vector<ADB_CSR> ActionIXBar;
    safe_vector<ActionIXBar> action_ixbars;

 public:
    struct Use : public ::ActionDataBus::Use {
        void clear() override {
            ::ActionDataBus::Use::clear();
            rng_locs.clear();
        }
        Use *clone() const override { return new Use(*this); }
        bool emit_adb_asm(std::ostream &, const IR::MAU::Table *, bitvec source) const override;
        bool empty() const override { return ::ActionDataBus::Use::empty() && rng_locs.empty(); }
        int rng_unit() const override;

        struct RandomNumberGenerator {
            int unit;
            bitvec bytes;

            RandomNumberGenerator(int u, bitvec b) : unit(u), bytes(b) {}

            bool operator==(const RandomNumberGenerator &rng) const {
                if (unit != rng.unit) return false;
                if (bytes != rng.bytes) return false;
                return true;
            }
        };

        safe_vector<RandomNumberGenerator> rng_locs;
        bool operator==(const Use &use) const {
            if (rng_locs != use.rng_locs) return false;
            return true;
        }
    };

    void clear() override;

    bool operator==(const ActionDataBus &adb) const;
    bool operator!=(ActionDataBus &adb) { return !(*this == adb); }

 private:
    static Use &getUse(autoclone_ptr<::ActionDataBus::Use> &ac);
    int byte_to_output(int byte, ActionData::SlotType_t type);
    int output_to_byte(int output, ActionData::SlotType_t type);
    int find_byte_sz(ActionData::SlotType_t type);
    int csr_index(int start_byte, ActionData::SlotType_t type);
    bool is_csr_reserved(int start_byte, bitvec adjacent, int byte_offset,
                         ActionData::Location_t source);
    bool is_adf_csr_reserved(int start_byte, bitvec adjacent, int byte_offset);
    bool is_immed_csr_reserved(int start_byte);

    void reserve_csr(int start_byte, bitvec adjacent, int byte_offset,
                     ActionData::Location_t source);
    void reserve_adf_csr(int start_byte, bitvec adjacent, int byte_offset);
    void reserve_immed_csr(int start_byte);
    bitvec combined(const ActionData::BusInputs bv) const;

    void initialize_action_ixbar();

    bool find_location(bitvec combined_adjacent, int diff, ActionData::SlotType_t init_type,
                       loc_alg_t loc_alg, ActionData::Location_t source, int byte_offset,
                       int &start_byte);
    bool find_location(bitvec combined_adjacent, int diff, int initial_adb_byte, int final_adb_byte,
                       bool reset, ActionData::SlotType_t type, ActionData::Location_t source,
                       int byte_offset, int &start_byte);

    void analyze_full_share(Use &use, ActionData::BusInputs layouts, FullShare &full_share,
                            int init_byte_offset, int add_byte_offset,
                            ActionData::Location_t source);
    void analyze_full_shares(Use &use, ActionData::BusInputs layouts, bitvec full_bitmasked,
                             FullShare full_shares[4], int init_byte_offset,
                             ActionData::Location_t source);
    void reserve_space(Use &use, ActionData::SlotType_t type, bitvec adjacent,
                       bitvec combined_adjacent, int start_byte, int byte_offset,
                       ActionData::Location_t source, cstring name);
    bool fit_adf_section(Use &use, bitvec adjacent, bitvec combined_adjacent,
                         ActionData::SlotType_t type, loc_alg_t loc_alg, int init_byte_offset,
                         int sec_begin, int size, cstring name, ActionData::Location_t source);
    bool alloc_bytes(Use &use, bitvec layout, bitvec combined_layout, int init_byte_offset,
                     cstring name, ActionData::Location_t source);
    bool alloc_halves(Use &use, bitvec layout, bitvec combined_layout, int init_byte_offset,
                      cstring name, ActionData::Location_t source);
    bool alloc_fulls(Use &use, ActionData::BusInputs layouts, bitvec full_bitmasked,
                     int init_byte_offset, cstring name, ActionData::Location_t source);
    bool alloc_full_sect(Use &use, FullShare full_shares[4], bitvec combined_layout, int begin,
                         int init_byte_offset, cstring name, bitvec full_bitmasked,
                         ActionData::Location_t source);
    bool alloc_ad_table(const ActionData::BusInputs total_layouts,
                        const bitvec full_layout_bitmasked, Use &use, cstring name);
    bool alloc_meter_output(ActionData::BusInputs total_layouts, Use &use, cstring name);

    bitvec paired_immediate(bitvec layout, ActionData::SlotType_t type);
    bool fit_immed_sect(Use &use, bitvec layout, bitvec combined_layout,
                        ActionData::SlotType_t type, loc_alg_t loc_alg, cstring name);
    bool alloc_unshared_immed(Use &use, ActionData::SlotType_t, bitvec layout,
                              bitvec combined_layout, cstring name);
    bool alloc_shared_immed(Use &use, ActionData::BusInputs layouts, cstring name);
    bool alloc_immediate(const ActionData::BusInputs total_layouts, Use &use, cstring name);
    bool alloc_rng(Use &use, const ActionData::Format::Use *format, cstring name);

 public:
    bool alloc_action_data_bus(const IR::MAU::Table *tbl, const ActionData::Format::Use *use,
                               TableResourceAlloc &alloc) override;
    bool alloc_action_data_bus(const IR::MAU::Table *tbl, const MeterALU::Format::Use *use,
                               TableResourceAlloc &alloc) override;
    void update(cstring name, const ::ActionDataBus::Use &alloc) override;
    void update(cstring name, const Use::ReservedSpace &rs) override;
    void update(cstring name, const Use::RandomNumberGenerator &rng);
    void update(const IR::MAU::Table *tbl) override;
};

}  // end namespace Tofino

#endif /* BF_P4C_MAU_TOFINO_ACTION_DATA_BUS_H_*/
