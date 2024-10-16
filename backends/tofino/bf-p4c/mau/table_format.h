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

#ifndef BF_P4C_MAU_TABLE_FORMAT_H_
#define BF_P4C_MAU_TABLE_FORMAT_H_

#include <map>
#include <ostream>
#include "bf-p4c/mau/input_xbar.h"
#include "bf-p4c/mau/resource_estimate.h"
#include "ir/ir.h"
#include "lib/bitops.h"
#include "lib/bitvec.h"
#include "lib/safe_vector.h"

using namespace P4;

// Info for the allocation scheme for individual byte off of the input crossbar
struct ByteInfo : public IHasDbPrint {
 public:
    IXBar::Use::Byte byte;  // Obviously the byte
    bitvec bit_use;   // Relevant bits of the byte
    int byte_location = -1;

    explicit ByteInfo(const IXBar::Use::Byte b, bitvec bu)
        : byte(b), bit_use(bu) {}

    void dbprint(std::ostream &out) const;

    // Where the hole is in the byte, i.e. at the LSB or the MSB or in the middle
    enum HoleType_t { LSB, MIDDLE, MSB, INVALID };
    bool is_better_for_overhead(const ByteInfo &bi, int overhead_bits) const;
    int hole_size(HoleType_t hole_type, int *hole_start_pos = nullptr) const;
    bool better_hole_type(int hole, int comp_hole, int overhead_bits) const;
    void set_interleave_info(int overhead_bits);

    struct InterleaveInfo : public IHasDbPrint {
        bool interleaved = false;
        HoleType_t hole_type = INVALID;
        // How many bytes the combination of the match byte and overhead take
        int byte_cycle = 0;
        // The first bit position of the overhead in this byte cycle
        int overhead_start = 0;
        // The first bit position of the match byte in this byte cycle (always % 8 == 0)
        int match_byte_start = 0;
        void dbprint(std::ostream &out) const;
    };

    InterleaveInfo il_info;
};


struct TableFormat {
    static constexpr int OVERHEAD_BITS = 64;
    static constexpr int SINGLE_RAM_BITS = 128;
    static constexpr int SINGLE_RAM_BYTES = 16;
    static constexpr int MAX_GROUPS_PER_LAMB = 4;
    static constexpr int RAM_GHOST_BITS = IXBar::RAM_LINE_SELECT_BITS;
    static constexpr int GATEWAY_BYTES = 4;
    static constexpr int VERSION_BYTES = 14;
    static constexpr int VERSION_BITS = 4;
    static constexpr int VERSION_NIBBLES = 4;
    static constexpr int MID_BYTE_LO = 0;
    static constexpr int MID_BYTE_HI = 1;
    static constexpr int MID_BYTE_VERS = 3;
    static constexpr int MAX_SHARED_GROUPS = 2;
    static constexpr int MAX_GROUPS_PER_RAM = 5;
    static constexpr int FULL_IMEM_ADDRESS_BITS = 6;
    static constexpr int FULL_NEXT_TABLE_BITS = 8;
    static constexpr int NEXT_MAP_TABLE_ENTRIES = 8;
    static constexpr int IMEM_MAP_TABLE_ENTRIES = 8;
    // MSB bit of the selector length
    static constexpr int SELECTOR_LENGTH_MAX_BIT = 16;

    enum type_t { MATCH, NEXT, ACTION, IMMEDIATE, VERS, COUNTER, COUNTER_PFE, METER, METER_PFE,
                  METER_TYPE, INDIRECT_ACTION, SEL_LEN_MOD, SEL_LEN_SHIFT, VALID, ENTRY_TYPES,
                  INTERLEAVED_MATCH };

    struct Use {
        struct match_group_use {
            // int is the number of bits in allocation
            // bitvec is the mask, if bitvec.popcount() < Byte.hi - Byte.lo then it is
            // an 8 bit field that's partially ghosted,
            // if bitvec.popcount() > byte.hi - byte.lo then it is
            // a misaligned field where the mask was not known until PHV allocation

            /// The byte and byte_mask location in the format
            std::map<IXBar::Use::Byte, bitvec> match;
            bitvec mask[ENTRY_TYPES];
            bitvec match_byte_mask;  // The bytes that are allocated for matching
            bitvec allocated_bytes;

            void clear_match() {
                match.clear();
                mask[MATCH].clear();
                mask[VERS].clear();
                match_byte_mask.clear();
            }

            void clear() {
                clear_match();
                allocated_bytes.clear();
                for (int i = 0; i < ENTRY_TYPES; i++)
                    mask[i].clear();
            }

            bitvec match_bit_mask() const {
                return mask[MATCH] | mask[VERS];
            }

            bitvec entry_info_bit_mask() const {
                bitvec rv;
                for (int i = 0; i < ENTRY_TYPES; i++)
                    rv |= mask[i];
                return rv;
            }

            int entry_min_word() const {
                return entry_info_bit_mask().min().index() / SINGLE_RAM_BITS;
            }

            int entry_max_word() const {
                return entry_info_bit_mask().max().index() / SINGLE_RAM_BITS;
            }

            bitvec overhead_mask() const {
                return entry_info_bit_mask() - match_bit_mask();
            }


            bool overhead_in_RAM_word(int RAM_word) const {
                bitvec bv = overhead_mask();
                bv = bv & bitvec(RAM_word * SINGLE_RAM_BITS, SINGLE_RAM_BITS);
                return !bv.empty();
            }

            bool match_data_in_RAM_word(int RAM_word) const {
                bitvec bv = match_bit_mask();
                bv = bv & bitvec(RAM_word * SINGLE_RAM_BITS, SINGLE_RAM_BITS);
                return !bv.empty();
            }
        };

        struct TCAM_use {
            int group = -1;
            int byte_group = -1;
            int byte_config = -1;
            bitvec dirtcam;

            void set_group(int _group, bitvec _dirtcam);
            void set_midbyte(int _byte_group, int _byte_config);

            int range_index = -1;
            TCAM_use() {}
        };

        bool only_one_result_bus = false;
        safe_vector<match_group_use> match_groups;
        safe_vector<safe_vector<int>> match_group_map;
        safe_vector<TCAM_use> tcam_use;
        int split_midbyte = -1;

        std::map<int, safe_vector<int>> ixbar_group_per_width;
        safe_vector<bool> result_bus_needed;
        bitvec avail_sb_bytes;
        int proxy_hash_group = -1;
        bool identity_hash = false;

        /// The byte and individual bits to be ghosted. Ghost bits should be the
        /// same for all match entries.
        std::map<IXBar::Use::Byte, bitvec> ghost_bits;
        bitvec immed_mask;

        IR::MAU::PfeLocation stats_pfe_loc = IR::MAU::PfeLocation::NOT_SET;
        IR::MAU::PfeLocation meter_pfe_loc = IR::MAU::PfeLocation::NOT_SET;
        IR::MAU::TypeLocation meter_type_loc = IR::MAU::TypeLocation::NOT_SET;

        std::map<int, int> payload_map;

        void clear() {
            ghost_bits.clear();
            match_groups.clear();
            match_group_map.clear();
            tcam_use.clear();

            ixbar_group_per_width.clear();
            result_bus_needed.clear();
            avail_sb_bytes.clear();
            immed_mask.clear();
            stats_pfe_loc = IR::MAU::PfeLocation::NOT_SET;
            meter_pfe_loc = IR::MAU::PfeLocation::NOT_SET;
            meter_type_loc = IR::MAU::TypeLocation::NOT_SET;
            payload_map.clear();
        }

        bool has_overhead() const {
            if (match_groups.empty())
                return false;
            return !match_groups[0].overhead_mask().empty();
        }

        bitvec overhead() const {
            bitvec rv;
            for (auto &match_group : match_groups)
                rv |= match_group.overhead_mask();
            return rv;
        }

        bool instr_in_overhead() const {
            if (match_groups.empty())
                return false;
            return !match_groups[0].mask[ACTION].empty();
        }

        int next_table_bits() const {
            if (match_groups.empty())
                return false;
            return match_groups[0].mask[NEXT].popcount();
        }

        bitvec no_overhead_atcam_result_bus_words() const;
        bitvec result_bus_words() const;
    };

 protected:
    bitvec total_use;  // Total bitvec for all entries in table format
    bitvec interleaved_match_byte_use;
    bitvec match_byte_use;   // Bytes used by all match byte masks

    Use *use = nullptr;
    const LayoutOption &layout_option;
    const IXBar::Use *match_ixbar;
    // Size of the following vectors is the layout_option.way->width

    /// Which RAM sections contain the match groups
    // safe_vector<int> match_groups_per_RAM;
    /// Which RAM sections contain overhead info
    safe_vector<int> overhead_groups_per_RAM;

    safe_vector<int> shared_groups_per_RAM;
    safe_vector<int> full_match_groups_per_RAM;
    safe_vector<ByteInfo> match_bytes;
    safe_vector<ByteInfo> ghost_bytes;
    /// Specifically which search bus coordinates to which RAM
    safe_vector<int> search_bus_per_width;

    // Vector for a hash group, as large tables could potentially use multiple hash groups
    safe_vector<IXBar::Use::Byte> single_match;
    int ghost_bits_count = 0;

    const IR::MAU::Table *tbl;

 private:
    const IXBar::Use *proxy_hash_ixbar;

    std::set<int> fully_ghosted_search_buses;
    safe_vector<int> ghost_bit_buses;

    bitvec pre_match_total_use;

    bitvec interleaved_bit_use;

    enum packing_algorithm_t { SAVE_GW_SPACE, PACK_TIGHT, PACKING_ALGORITHMS };

    packing_algorithm_t pa = PACKING_ALGORITHMS;

    bitvec version_allocated;

    // Match group index in use coordinate to whenever they are found in the match_groups_per_RAM
    // i.e. if the match_groups_per_RAM looks like [2, 2], then use->match_groups[0] and
    // use->match_groups[1] are in the first RAM, etc.  Same thing applies for overhead groups

    // Size of outer vector is layout_option.way->match_groups.  Essentially which RAMs does
    // each match group use, for allocating version bits.
    // safe_vector<safe_vector<int>> match_group_info;

    const bitvec immediate_mask;
    bool gw_linked;
    FindPayloadCandidates &fpc;

    const PhvInfo &phv;

    bool skinny = false;
    void clear_match_state();
    void clear_pre_allocation_state();

    bitvec bitvec_necessary(type_t type) const;
    int overhead_bits_necessary() const;
    bool allocate_overhead_field(type_t type, int lsb_mem_word_offset, int bit_width, int entry,
        int RAM_word);
    bool allocate_overhead_entry(int entry, int RAM_word, int lsb_mem_word_offset);
    void setup_pfes_and_types();
    bool allocate_all_indirect_ptrs();
    bool allocate_all_immediate();
    bool allocate_all_instr_selection();
    bool allocate_match();
    bool allocate_match_with_algorithm();
    bool is_match_entry_wide() const;

    bool allocate_all_ternary_match();
    void initialize_dirtcam_value(bitvec &dirtcam, const IXBar::Use::Byte &byte);
    void ternary_midbyte(int midbyte, size_t &index, bool lo_midbyte);
    void ternary_version(size_t &index);

    bool analyze_skinny_layout_option(int per_RAM, safe_vector<IXBar::Use::GroupInfo> &sizes);
    bool analyze_wide_layout_option(safe_vector<IXBar::Use::GroupInfo> &sizes);
    void analyze_proxy_hash_option(int per_RAM);

    int hit_actions() const;
    bool allocate_next_table();
    bool allocate_selector_length();
    bool allocate_indirect_ptr(int total, type_t type, int group, int RAM);

    bool allocate_interleaved_byte(const ByteInfo &info, safe_vector<ByteInfo> &alloced,
        int width_sect, int entry, bitvec &byte_attempt, bitvec &bit_attempt);
    bool allocate_version(int width_sect, const safe_vector<ByteInfo> &alloced,
        bitvec &version_loc, bitvec &byte_attempt, bitvec &bit_attempt);

    int determine_group(int width_sect, int groups_allocated);
    void allocate_share(int width_sect, int group, safe_vector<ByteInfo> &unalloced_group,
        safe_vector<ByteInfo> &alloced, bitvec &version_loc, bitvec &byte_attempt,
        bitvec &bit_attempt, bool overhead_section);
    bool attempt_allocate_shares();
    bool allocate_shares();
    bool redistribute_entry_priority();
    void redistribute_next_table();
    bool build_match_group_map();
    bool build_payload_map();
    bool interleave_match_and_overhead();

    virtual void classify_match_bits();
    virtual bool allocate_sram_match();
    virtual bool allocate_match_byte(const ByteInfo &info, safe_vector<ByteInfo> &alloced,
            int width_sect, bitvec &byte_attempt, bitvec &bit_attempt);
    virtual bool requires_versioning() const { return layout_option.layout.requires_versioning; }
    virtual bool requires_valid_bit() const { return false; }
    virtual void find_bytes_to_allocate(int width_sect, safe_vector<ByteInfo> &unalloced);

 protected:
    virtual bool allocate_overhead(bool alloc_match = false);
    virtual void get_potential_ghost_byte(const IXBar::Use::Byte byte,
        const std::map<cstring, bitvec> &hash_masks,
        safe_vector<std::pair<IXBar::Use::Byte, bitvec>> &potential_ghost);
    virtual void choose_ghost_bits(
        safe_vector<std::pair<IXBar::Use::Byte, bitvec>> &potential_ghost);
    int bits_necessary(type_t type) const;
    bool initialize_byte(int byte_offset, int width_sect, const ByteInfo &info,
        safe_vector<ByteInfo> &alloced, bitvec &byte_attempt, bitvec &bit_attempted);
    virtual void allocate_full_fits(int width_sect, int group = -1);
    virtual bool analyze_layout_option();
    void fill_out_use(int group, const safe_vector<ByteInfo> &alloced, bitvec &version_loc);

 public:
    TableFormat(const LayoutOption &l, const IXBar::Use *mi, const IXBar::Use *phi,
                const IR::MAU::Table *t, const bitvec im, bool gl, FindPayloadCandidates &fpc,
                const PhvInfo &phv)
        : layout_option(l), match_ixbar(mi), tbl(t), proxy_hash_ixbar(phi), immediate_mask(im),
          gw_linked(gl), fpc(fpc), phv(phv) {}
    bool find_format(Use *u);
    void verify();
    static TableFormat* create(const LayoutOption &l, const IXBar::Use *mi, const IXBar::Use *phi,
                const IR::MAU::Table *t, const bitvec im, bool gl, FindPayloadCandidates &fpc,
                const PhvInfo &phv);
};

std::ostream& operator<<(std::ostream &out, const TableFormat::Use::match_group_use &m);
#endif /* BF_P4C_MAU_TABLE_FORMAT_H_ */
