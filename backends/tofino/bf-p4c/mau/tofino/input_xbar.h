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

#ifndef BF_P4C_MAU_TOFINO_INPUT_XBAR_H_
#define BF_P4C_MAU_TOFINO_INPUT_XBAR_H_

#include "bf-p4c/mau/input_xbar.h"
#include "bf-p4c/common/alloc.h"

namespace Tofino {

using namespace P4;

struct IXBar : public ::IXBar {
    // FIXME -- make these per-device params
    static constexpr int EXACT_GROUPS = 8;
    static constexpr int EXACT_BYTES_PER_GROUP = 16;
    static constexpr int HASH_TABLES = 16;
    static constexpr int HASH_GROUPS = 8;
    static constexpr int HASH_INDEX_GROUPS = 4;  /* groups of 10 bits for indexing */
    static constexpr int HASH_SINGLE_BITS = 12;  /* top 12 bits of hash table individually */
    static constexpr int HASH_PARITY_BIT = 51;  /* If enabled reserved parity bit position */
    static constexpr int RAM_SELECT_BIT_START = 40;
    static constexpr int RAM_LINE_SELECT_BITS = 10;
    static constexpr int HASH_MATRIX_SIZE = RAM_SELECT_BIT_START + HASH_SINGLE_BITS;
    static constexpr int HASH_DIST_SLICES = 3;
    static constexpr int HASH_DIST_BITS = 16;
    static constexpr int HASH_DIST_EXPAND_BITS = 7;
    static constexpr int HASH_DIST_MAX_MASK_BITS = HASH_DIST_BITS + HASH_DIST_EXPAND_BITS;
    static constexpr int HASH_DIST_UNITS = 2;
    static constexpr int TOFINO_METER_ALU_BYTE_OFFSET = 8;
    static constexpr int LPF_INPUT_BYTES = 4;
    static constexpr int TERNARY_GROUPS = StageUse::MAX_TERNARY_GROUPS;
    static constexpr int BYTE_GROUPS = StageUse::MAX_TERNARY_GROUPS/2;
    static constexpr int TERNARY_BYTES_PER_GROUP = 5;
    static constexpr int TERNARY_BYTES_PER_BIG_GROUP = 11;
    static constexpr int GATEWAY_SEARCH_BYTES = 4;
    static constexpr int RESILIENT_MODE_HASH_BITS = 51;
    static constexpr int FAIR_MODE_HASH_BITS = 14;
    static constexpr int METER_ALU_HASH_BITS = 52;
    static constexpr int METER_ALU_HASH_PARITY_BYTE_START = 48;
    static constexpr int METER_PRECOLOR_SIZE = 2;
    static constexpr int REPEATING_CONSTRAINT_SECT = 4;
    static constexpr int MAX_HASH_BITS = 52;
    static constexpr le_bitrange SELECT_BIT_RANGE =
            le_bitrange(RAM_SELECT_BIT_START, METER_ALU_HASH_BITS-1);
    static constexpr le_bitrange INDEX_BIT_RANGE(int group) {
            return le_bitrange(group*RAM_LINE_SELECT_BITS, (group+1)*RAM_LINE_SELECT_BITS - 1); }
    static constexpr int INDEX_RANGE_SUBGROUP(le_bitrange r) { return r.lo / RAM_LINE_SELECT_BITS; }

    static int get_meter_alu_hash_bits() {
        // If hash parity is enabled reserve a bit for parity
        if (!BackendOptions().disable_gfm_parity)
            return METER_ALU_HASH_BITS - 1;
        return METER_ALU_HASH_BITS;
    }

    static int get_hash_single_bits() {
        static bool disable_gfm_parity = BackendOptions().disable_gfm_parity;
        // If hash parity is enabled reserve a bit for parity
        if (!disable_gfm_parity)
            return HASH_SINGLE_BITS - 1;
        return HASH_SINGLE_BITS;
    }

 private:
    static int get_hash_matrix_size() {
        return RAM_SELECT_BIT_START + get_hash_single_bits();
    }

    static int get_max_hash_bits() {
        // If hash parity is enabled reserve a bit for parity
        if (!BackendOptions().disable_gfm_parity)
            return MAX_HASH_BITS - 1;
        return MAX_HASH_BITS;
    }

    using Loc = ::IXBar::Loc;
    using byte_speciality_t = ::IXBar::byte_speciality_t;
    // FIXME -- should have a better way of importing all the byte_speciality_t values
    static constexpr auto NONE = ::IXBar::NONE;
    static constexpr auto ATCAM_DOUBLE = ::IXBar::ATCAM_DOUBLE;
    static constexpr auto ATCAM_INDEX = ::IXBar::ATCAM_INDEX;
    static constexpr auto RANGE_LO = ::IXBar::RANGE_LO;
    static constexpr auto RANGE_HI = ::IXBar::RANGE_HI;
    static constexpr auto BYTE_SPECIALITIES = ::IXBar::BYTE_SPECIALITIES;

    using FieldInfo = ::IXBar::FieldInfo;
    using parity_status_t = ::IXBar::parity_status_t;
    // FIXME -- should have a better waby of importing all the parity_status_t values
    static constexpr auto PARITY_NONE = ::IXBar::PARITY_NONE;
    static constexpr auto PARITY_ENABLED = ::IXBar::PARITY_ENABLED;
    static constexpr auto PARITY_DISABLED = ::IXBar::PARITY_DISABLED;

    /** IXBar tracks the use of all the input xbar bytes in a single stage.  Each byte use is set
     * to record the container it will be getting and the bit offset within the container.
     * NOTE: Changes here require changes to .gdbinit pretty printer */
    BFN::Alloc2D<std::pair<PHV::Container, int>, EXACT_GROUPS, EXACT_BYTES_PER_GROUP>
                                                                                    exact_use;
    BFN::Alloc2D<std::pair<PHV::Container, int>, TERNARY_GROUPS, TERNARY_BYTES_PER_GROUP>
                                                                                    ternary_use;
    BFN::Alloc1D<std::pair<PHV::Container, int>, BYTE_GROUPS>                       byte_group_use;
    BFN::Alloc2Dbase<std::pair<PHV::Container, int>> &use(bool ternary) {
        if (ternary) return ternary_use;
        return exact_use; }
    /* reverse maps of the above, mapping containers sets of group+byte */
    std::multimap<PHV::Container, Loc>         exact_fields;
    std::multimap<PHV::Container, Loc>         ternary_fields;
    std::multimap<PHV::Container, Loc> &fields(bool ternary) {
        return ternary ? ternary_fields : exact_fields; }

    // map from container to tables that use those fields (mostly for debugging)
    std::map<PHV::Container, std::set<cstring>>        field_users;

    /* Track the use of hashtables/groups too -- FIXME -- should it be a separate data structure?
     * strings here are table names
     * NOTE: Changes here require changes to .gdbinit pretty printer */
    unsigned                                    hash_index_inuse[HASH_INDEX_GROUPS] = { 0 };
    BFN::Alloc2D<cstring, HASH_TABLES, HASH_INDEX_GROUPS>    hash_index_use;
    BFN::Alloc2D<cstring, HASH_TABLES, HASH_SINGLE_BITS>     hash_single_bit_use;
    unsigned                                    hash_single_bit_inuse[HASH_SINGLE_BITS] = { 0 };
    BFN::Alloc1D<cstring, HASH_GROUPS>                      hash_group_print_use;
    unsigned                                    hash_group_use[HASH_GROUPS] = { 0 };
    BFN::Alloc2D<cstring, HASH_TABLES, HASH_DIST_SLICES>     hash_dist_use;
    bitvec                                      hash_dist_inuse[HASH_TABLES] = { bitvec() };
    BFN::Alloc2D<cstring, HASH_TABLES, HASH_DIST_SLICES * HASH_DIST_BITS>   hash_dist_bit_use;
    bitvec                                      hash_dist_bit_inuse[HASH_TABLES] = { bitvec() };
    parity_status_t  hash_group_parity_use[HASH_GROUPS] = { PARITY_NONE };

    // Added on update to be verified
    bitvec hash_used_per_function[HASH_GROUPS] = { bitvec() };

    int hash_dist_groups[HASH_DIST_UNITS] = {-1, -1};
    friend class IXBarRealign;

    /* API for unit tests */
 public:
    BFN::Alloc2Dbase<std::pair<PHV::Container, int>> &get_exact_use() { return exact_use; }
    BFN::Alloc2Dbase<std::pair<PHV::Container, int>> &get_ternary_use() { return ternary_use; }
    BFN::Alloc1D<std::pair<PHV::Container, int>, BYTE_GROUPS> &get_byte_group_use() {
        return byte_group_use; }

    // map (type, group, byte) coordinate to linear xbar output space
    unsigned toIXBarOutputByte(bool ternary, int group, int byte) {
        unsigned offset = 0;
        if (ternary) {
            unsigned mid_byte_count = (group / 2) + (group % 2);
            offset = (group / 2) * 10  + (group % 2) * 5 + byte + mid_byte_count;
        } else {
            offset = group * 16 + byte;
        }
        return offset;
    }

 private:
    std::set<cstring>                                       dleft_updates;

 public:
    using byte_type_t = ::IXBar::byte_type_t;
    // FIXME -- should have a better waby of importing all the byte_type_t values
    static constexpr auto NO_BYTE_TYPE = ::IXBar::NO_BYTE_TYPE;
    static constexpr auto ATCAM = ::IXBar::ATCAM;
    static constexpr auto PARTITION_INDEX = ::IXBar::PARTITION_INDEX;
    static constexpr auto RANGE = ::IXBar::RANGE;
    using HashDistDest_t = ::IXBar::HashDistDest_t;
    // FIXME -- should have a better waby of importing all the HashDistDest_t values
    static constexpr auto HD_IMMED_LO = ::IXBar::HD_IMMED_LO;
    static constexpr auto HD_IMMED_HI = ::IXBar::HD_IMMED_HI;
    static constexpr auto HD_STATS_ADR = ::IXBar::HD_STATS_ADR;
    static constexpr auto HD_METER_ADR = ::IXBar::HD_METER_ADR;
    static constexpr auto HD_ACTIONDATA_ADR = ::IXBar::HD_ACTIONDATA_ADR;
    static constexpr auto HD_PRECOLOR = ::IXBar::HD_PRECOLOR;
    static constexpr auto HD_HASHMOD = ::IXBar::HD_HASHMOD;
    static constexpr auto HD_DESTS = ::IXBar::HD_DESTS;

    struct Use : public ::IXBar::Use {
        bool            gw_search_bus = false;
        int             gw_search_bus_bytes = 0;
        bool            gw_hash_group = false;
        parity_status_t parity = PARITY_NONE;

        bool search_data() const { return gw_search_bus || gw_hash_group; }
        bool is_parity_enabled() const override { return parity == PARITY_ENABLED; }

        HashDistDest_t hash_dist_type = HD_DESTS;
        std::string hash_dist_used_for() const override {
            return IXBar::hash_dist_name(hash_dist_type); }

        /* which of the 16 hash tables we are using (bitvec) */
        unsigned        hash_table_inputs[HASH_GROUPS] = { 0 };
        /* hash seed for different hash groups */
        bitvec          hash_seed[HASH_GROUPS];

        /* tracking bits that are placed into the upper 12 bits of a hash group
         * (with an identity hash) for use by a gateway */
        struct Bits {
            cstring     field;
            int         group;
            int         lo, bit, width;
            bool        valid;
            Bits(cstring f, int g, int l, int b, int w)
            : field(f), group(g), lo(l), bit(b), width(w), valid(false) {}
            Bits(cstring f, int g, int l, int b, int w, bool v)
            : field(f), group(g), lo(l), bit(b), width(w), valid(v) {}
            int hi() const { return lo + width - 1; } };
        safe_vector<Bits>    bit_use;

        /* tracks hash use for Stateful and Selectors (and meter?) */
        struct MeterAluHash {
            bool allocated = false;
            int group = -1;
            bitvec bit_mask;
            IR::MAU::HashFunction algorithm;
            std::map<int, const IR::Expression *> computed_expressions;

            void clear() {
                allocated = false;
                group = -1;
                bit_mask.clear();
                // identity_positions.clear();
                computed_expressions.clear();
            }
        } meter_alu_hash;
        const std::map<int, const IR::Expression *> &hash_computed_expressions() const override {
            return meter_alu_hash.computed_expressions; }

        /* tracks hash dist use (and hashes */
        struct HashDistHash {
            bool allocated = false;
            int group = -1;
            bitvec galois_matrix_bits;
            IR::MAU::HashFunction algorithm;
            std::map<int, le_bitrange> galois_start_bit_to_p4_hash;
            const IR::Expression *hash_gen_expr = nullptr;   // expression from HashGenExpr
            cstring name;    // Original name in case of hash distribution unit sharing

            void clear() {
                allocated = false;
                group = -1;
                galois_matrix_bits.clear();
                galois_start_bit_to_p4_hash.clear();
            }
        } hash_dist_hash;
        int hash_dist_hash_group() const override { return hash_dist_hash.group; }

        struct ProxyHashKey {
            bool allocated = false;
            int group = -1;
            bitvec hash_bits;
            IR::MAU::HashFunction algorithm;

            void clear() {
                allocated = false;
                group = -1;
                hash_bits.clear();
            }
        } proxy_hash_key_use;

        struct SaluInputSource {
            unsigned    data_bytemask = 0;
            unsigned    hash_bytemask = 0;  // redundant with meter_alu_hash.bit_mask?

            void clear() { data_bytemask = hash_bytemask = 0; }
            bool empty() const { return !data_bytemask && !hash_bytemask; }
        } salu_input_source;

        // The order in the P4 program that the fields appear in the list
        safe_vector<const IR::Expression *> field_list_order;
        LTBitMatrix symmetric_keys;

        void clear() override {
            ::IXBar::Use::clear();
            gw_search_bus = false;
            gw_search_bus_bytes = 0;
            gw_hash_group = false;
            parity = PARITY_NONE;
            hash_dist_type = HD_DESTS;
            for (auto &ht : hash_table_inputs) ht = 0;
            for (auto &hs : hash_seed) hs.clear();
            bit_use.clear();
            meter_alu_hash.clear();
            hash_dist_hash.clear();
            proxy_hash_key_use.clear();
            salu_input_source.clear();
            field_list_order.clear();
        }
        Use *clone() const override { return new Use(*this); }
        bool empty() const override {
            return ::IXBar::Use::empty() && bit_use.empty() &&
                !meter_alu_hash.allocated && !hash_dist_hash.allocated &&
                !proxy_hash_key_use.allocated && salu_input_source.empty(); }
        void dbprint(std::ostream &) const override;

        void add(const Use &alloc);
        safe_vector<Byte> atcam_partition(int *hash_group = nullptr) const override;
        safe_vector<TotalInfo> bits_per_search_bus() const;
        unsigned compute_hash_tables();
        bool emit_gateway_asm(const MauAsmOutput &, std::ostream &, indent_t,
                              const IR::MAU::Table *) const override { return false; }
        void emit_ixbar_asm(const PhvInfo &phv, std::ostream& out, indent_t indent,
                            const TableMatch *fmt, const IR::MAU::Table *) const override;
        void emit_salu_bytemasks(std::ostream &out, indent_t indent) const override;
        void emit_ixbar_hash_table(int hash_table, safe_vector<Slice> &match_data,
                safe_vector<Slice> &ghost, const TableMatch *fmt,
                std::map<int, std::map<int, Slice>> &sort) const override;
        bitvec galois_matrix_bits() const override { return hash_dist_hash.galois_matrix_bits; }
        int hash_groups() const override;
        TotalBytes match_hash(safe_vector<int> *hash_groups = nullptr) const override;
        bitvec meter_bit_mask() const override { return meter_alu_hash.bit_mask; }
        int ternary_align(const Loc &) const override;
        int total_input_bits() const override {
            int rv = 0;
            for (auto fl : field_list_order) {
                rv += fl->type->width_bits(); }
            return rv; }
        void update_resources(int, BFN::Resources::StageResources &) const override;
        const char *way_source_kind() const override { return "group"; }
    };
    static Use &getUse(autoclone_ptr<::IXBar::Use> &ac);
    static const Use &getUse(const autoclone_ptr<::IXBar::Use> &ac);

    /**
     * The Hash Distribution Unit is captured in uArch section 6.4.3.5.3 Hash Distribution.
     * This is sourcing calculations from the Galois matrix and sends them to various locations
     * in the MAU, as discussed in the comments above allocHashDist.
     * FIXME -- this is tofino-specific, so should be in tofino/input_xbar.h
     *
     * This captures the data that will pass to a single possible destination after the expand
     * but before the Mask/Shift block
     */
    struct HashDistAllocPostExpand {
        P4HashFunction *func;
        le_bitrange bits_in_use;
        HashDistDest_t dest;
        int shift;
        // Only currently used for dynamic hash.  Goal is to remove
        const IR::MAU::HashDist *created_hd;
        // Workaround for multi-stage fifo tests.  Goal is to remove this as well and have
        // the hash/compiler to generate this individually and determine it, but that's not
        // very optimal in the given structure
        bool chained_addr = false;
        bitvec possible_shifts() const;

     public:
        HashDistAllocPostExpand(P4HashFunction *f, le_bitrange b, HashDistDest_t d, int s)
            : func(f), bits_in_use(b), dest(d), shift(s) {}
        bool operator<(const HashDistAllocPostExpand& hd) const {
            return std::tie(dest, shift, bits_in_use, chained_addr) <
                std::tie(hd.dest, hd.shift, hd.bits_in_use, hd.chained_addr);
        }
    };

    struct HashDistIRUse {
        autoclone_ptr<::IXBar::Use> use;
        le_bitrange p4_hash_range;
        HashDistDest_t dest;
        // Only currently used for dynamic hash.  Goal is to remove
        const IR::MAU::HashDist *created_hd = nullptr;
        cstring dyn_hash_name;
        bool is_dynamic() const { return !dyn_hash_name.isNull(); }
    };

    struct HashDistUse {
        // Source of this translated HashDistUse.
        safe_vector<HashDistAllocPostExpand> src_reqs;

        safe_vector<HashDistIRUse> ir_allocations;
        int expand = -1;
        int unit = -1;
        int shift = -1;
        bitvec mask;

        std::set<cstring> outputs;

        int hash_group() const;
        bitvec destinations() const;
        bitvec galois_matrix_bits() const;

        cstring used_by;
        std::string used_for() const;

        void clear() {
            src_reqs.clear();
            ir_allocations.clear();
            expand = -1;
            unit = -1;
            shift = 0;
            mask.clear();
            outputs.clear();
        }
    };

 private:
    ordered_map<const IR::MAU::Table *, const safe_vector<HashDistUse> *> tbl_hash_dists;
    static unsigned hash_table_inputs(const HashDistUse &hdu);

    class XBarHashDist : public MauInspector {
        safe_vector<HashDistAllocPostExpand> alloc_reqs;
        IXBar &self;
        const PhvInfo &phv;
        const IR::MAU::Table *tbl;
        // const ActionFormat::Use *af;
        const ActionData::Format::Use *af;
        const LayoutOption *lo;
        TableResourceAlloc *resources;
        const attached_entries_t &attached_entries;

        void build_action_data_req(const IR::MAU::HashDist *hd);
        void build_req(const IR::MAU::HashDist *hd, const IR::Node *rel_node,
            bool created_hd = false);

        void build_function(const IR::MAU::HashDist *hd, P4HashFunction **func,
            le_bitrange *bits = nullptr);
        bool preorder(const IR::MAU::HashDist *) override;
        bool preorder(const IR::MAU::TableSeq *) override { return false; }
        bool preorder(const IR::MAU::AttachedOutput *) override { return false; }
        bool preorder(const IR::MAU::StatefulCall *) override { return false; }

     public:
        void immediate_inputs();
        void hash_action();
        bool allocate_hash_dist();
        XBarHashDist(IXBar &s, const PhvInfo &p, const IR::MAU::Table *t,
                const ActionData::Format::Use *a, const LayoutOption *l, TableResourceAlloc *r,
                const attached_entries_t &ae)
            : self(s), phv(p), tbl(t), af(a), lo(l), resources(r), attached_entries(ae) {}
    };

 private:
    enum AvailBytesPerRepeatingSect_t { AV_FULL = 1, AV_HALF = 2, AV_BYTE = 4 };

/* An individual SRAM group or half of a TCAM group */
    struct grp_use : public IHasDbPrint {
        enum type_t { MATCH, HASH_DIST, FREE };
        int group;
        /**
         * The byte positions in the grp_use that are equivalent some input xbar bytes, i.e.
         * if 2 tables are matching on the same field, after the first one is allocated, the
         * second would see this on the grp_use as possible to share
         */
        bitvec found;

        /**
         * The byte positions in the grp_use that are not used by any table, and are open.
         * Due to the nature of the constraints mentioned on the comments on is_better_group,
         * the available bytes are stored in an 4 wide array.  At each index i, the bitvec
         * is the available bytes for a 32 bit container byte at byte offset i.
         *
         * The free() function passes a parameter which of these four indices can a particularly
         * constrained byte can allocate to.  For instance, a 32 bit byte 1 will have an
         * can_use of 0x2, a 16 bit byte 0 will have an can_use of 0x5, and an 8 bit byte will
         * have a can_use of 0xf.
         *
         * @sa is_better_group in input_xbar.cpp
         */
        std::array<bitvec, REPEATING_CONSTRAINT_SECT> _free = { { bitvec() } };
        bitvec free(bitvec can_use) const {
            bitvec rv;
            for (auto bit : can_use)
                rv |= _free.at(bit);
            return rv;
        }

        bitvec max_free() const { return free(bitvec(0, REPEATING_CONSTRAINT_SECT)); }
        bool attempted = false;
        bool hash_open[2] = { true, true };
        type_t hash_table_type[2] = { FREE, FREE };
        int range_index = -1;

        bool hash_dist_avail(int ht) const {
            return hash_table_type[ht] == HASH_DIST || hash_table_type[ht] == FREE;
        }

        bool hash_dist_only(int ht) const {
            return hash_table_type[ht] == HASH_DIST;
        }

        void dbprint(std::ostream &out) const {
            out << group << " found: 0x" << found << " free: 0x" << max_free();
        }

        int total_avail() const { return found.popcount() + max_free().popcount(); }
        bool range_set() const { return range_index != -1; }

        bool is_better_group(const grp_use &b, bool prefer_found,
            int required_allocation_bytes, std::map<int, int> &constraints_to_reqs) const;
        explicit grp_use(int g) : group(g) {}
    };

    using mid_byte_use = grp_use;

 public:  // for gtest only
    struct hash_matrix_reqs {
        // The max number of groups that can be used by this table.  Required by gateways,
        // and stateful/meter tables using the search bus
        int max_search_buses = -1;
        int index_groups = 0;
        int select_bits = 0;
        bool hash_dist = false;
        bool requires_versioning = false;

        static hash_matrix_reqs max(bool hd, bool ternary = false) {
            hash_matrix_reqs rv;
            if (ternary)
                return rv;
            rv.hash_dist = hd;
            if (rv.hash_dist) {
                rv.index_groups = HASH_DIST_SLICES;
            } else {
                rv.index_groups = HASH_INDEX_GROUPS;
                rv.select_bits = IXBar::get_hash_single_bits();
            }
            return rv;
        }

        bool fit_requirements(bitvec hash_matrix_in_use) const;

        hash_matrix_reqs() {}
        hash_matrix_reqs(int ig, int sb, bool hd)
            : index_groups(ig), select_bits(sb), hash_dist(hd) {}
    };

 private:
    // Used to determine what phv fields need to be allocated on the input xbar for the
    // stateful ALU to work.  Private internal to allocStateful
    class FindSaluSources : public MauInspector {
        const PhvInfo              &phv;
        ContByteConversion  &map_alloc;
        safe_vector<const IR::Expression *> &field_list_order;
        // Holds which bitranges of fields have been requested, and will not allocate
        // if a bitrange has been requested multiple times
        std::map<cstring, bitvec>  fields_needed;
        const IR::MAU::Table       *tbl;

        profile_t init_apply(const IR::Node *root) override;
        bool preorder(const IR::MAU::StatefulAlu *) override;
        bool preorder(const IR::MAU::SaluAction *) override;
        bool preorder(const IR::Expression *e) override;
        bool preorder(const IR::MAU::HashDist *) override;
        bool preorder(const IR::MAU::IXBarExpression *) override;
        ///> In order to prevent any annotations, i.e. chain_vpn, and determining this as a source
        bool preorder(const IR::Annotation *) override { return false; }
        bool preorder(const IR::Declaration_Instance *) override { return false; }
        bool preorder(const IR::MAU::Selector *) override { return false; }

        static void collapse_contained(std::map<le_bitrange, const IR::Expression *> &m);

     public:
        FindSaluSources(IXBar &, const PhvInfo &phv, ContByteConversion &ma,
                        safe_vector<const IR::Expression *> &flo, const IR::MAU::Table *t)
        : phv(phv), map_alloc(ma), field_list_order(flo), tbl(t) {}

        ordered_map<const PHV::Field *, std::map<le_bitrange, const IR::Expression *>>
                                                        phv_sources;
        std::vector<const IR::MAU::IXBarExpression *>   hash_sources;
        bool                                            dleft = false;
    };

    void clear();
    bool allocMatch(bool ternary, const IR::MAU::Table *tbl, const PhvInfo &phv, Use &alloc,
                    safe_vector<IXBar::Use::Byte *> &alloced, hash_matrix_reqs &hm_reqs);
    bool allocPartition(const IR::MAU::Table *tbl, const PhvInfo &phv, Use &alloc,
                        bool second_try);
    int getHashGroup(unsigned hash_table_input, const hash_matrix_reqs *hm_reqs = nullptr);
    void getHashDistGroups(unsigned hash_table_input, int hash_group_opt[HASH_DIST_UNITS]);
    bool allocProxyHash(const IR::MAU::Table *tbl, const PhvInfo &phv,
        const LayoutOption *lo, Use &alloc, Use &proxy_hash_alloc);
    bool allocProxyHashKey(const IR::MAU::Table *tbl, const PhvInfo &phv, const LayoutOption *lo,
        Use &proxy_hash_alloc, hash_matrix_reqs &hm_reqs);

    bool allocAllHashWays(bool ternary, const IR::MAU::Table *tbl, Use &alloc,
        const LayoutOption *layout_option, size_t start, size_t last,
        const hash_matrix_reqs &hm_reqs);
    bool allocHashWay(const IR::MAU::Table *tbl, const LayoutOption *layout_option,
        size_t index, std::map<int, bitvec> &slice_to_select_bits, Use &alloc,
        unsigned local_hash_table_input, unsigned hf_hash_table_input, int hash_group);
    bool allocGateway(const IR::MAU::Table *, const PhvInfo &phv, Use &alloc,
        const LayoutOption *lo, bool second_try);
    bool allocSelector(const IR::MAU::Selector *, const IR::MAU::Table *, const PhvInfo &phv,
                       Use &alloc, cstring name);
    bool allocStateful(const IR::MAU::StatefulAlu *, const IR::MAU::Table *, const PhvInfo &phv,
                       Use &alloc, bool on_search_bus);
    bool allocMeter(const IR::MAU::Meter *, const IR::MAU::Table *, const PhvInfo &phv,
                    Use &alloc, bool on_search_bus);

    bool allocTable(const IR::MAU::Table *tbl, const PhvInfo &phv, TableResourceAlloc &alloc,
                    const LayoutOption *lo, const ActionData::Format::Use *af,
                    const attached_entries_t &ae);
    bool allocTable(const IR::MAU::Table *tbl, const IR::MAU::Table *gw, const PhvInfo &phv,
                    TableResourceAlloc &alloc, const LayoutOption *lo,
                    const ActionData::Format::Use *af, const attached_entries_t &ae) override;

    void update(cstring name, const ::IXBar::Use &alloc) override;
    void update(cstring name, const HashDistUse &hash_dist_alloc);
    void update(const IR::MAU::Table *tbl, const TableResourceAlloc *rsrc) override;
    void update(const IR::MAU::Table *tbl) override;
    void add_collisions() override;
    void verify_hash_matrix() const override;
    void dbprint(std::ostream &) const override;

    const Loc *findExactByte(PHV::Container c, int byte) const {
        for (auto &p : Values(exact_fields.equal_range(c)))
            if (exact_use.at(p.group, p.byte).second/8 == byte)
                return &p;
        /* FIXME -- what if it's in more than one place? */
        return nullptr; }
    unsigned find_balanced_group(Use &alloc, int way_size);

 public:  // for gtest
    bool find_alloc(safe_vector<IXBar::Use::Byte> &alloc_use, bool ternary,
        safe_vector<IXBar::Use::Byte *> &alloced, hash_matrix_reqs &hm_reqs,
        unsigned byte_mask = ~0U);

 private:
    bitvec can_use_from_flags(int flags) const;
    int groups(bool ternary) const;
    int mid_bytes(bool ternary) const;
    int bytes_per_group(bool ternary) const;

    void increase_ternary_ixbar_space(int &groups_needed, int &nibbles_needed,
        bool requires_versioning);
    bool calculate_sizes(safe_vector<Use::Byte> &alloc_use, bool ternary, int &total_bytes_needed,
        int &groups_needed, int &mid_bytes_needed, bool requires_versioning);
    void initialize_orders(safe_vector<grp_use> &order, safe_vector<mid_byte_use> &mid_byte_order,
        bool ternary);
    void setup_byte_vectors(safe_vector<Use::Byte> &alloc_use, bool ternary,
        safe_vector<Use::Byte *> &unalloced, safe_vector<Use::Byte *> &alloced,
        safe_vector<grp_use> &order, safe_vector<mid_byte_use> &mid_byte_order,
        hash_matrix_reqs &hm_reqs, unsigned byte_mask);

    void print_groups(safe_vector<grp_use> &order, safe_vector<mid_byte_use> &mid_byte_order,
        bool ternary);
    void fill_out_use(safe_vector<IXBar::Use::Byte *> &alloced, bool ternary);
    void calculate_available_groups(safe_vector<grp_use> &order, hash_matrix_reqs &hm_reqs);
    void calculate_available_hash_dist_groups(safe_vector<grp_use> &order,
        hash_matrix_reqs &hm_reqs);
    grp_use::type_t is_group_for_hash_dist(int hash_table);
    bool violates_hash_constraints(safe_vector <grp_use> &order, bool hash_dist, int group,
        int byte);
    void reset_orders(safe_vector<grp_use> &order, safe_vector<mid_byte_use> &mid_byte_order);
    void calculate_found(safe_vector<Use::Byte *> &unalloced, safe_vector<grp_use> &order,
        safe_vector<mid_byte_use> &mid_byte_order, bool ternary, bool hash_dist,
        unsigned byte_mask);
    void calculate_free(safe_vector<grp_use> &order, safe_vector<mid_byte_use> &mid_byte_order,
        bool ternary, bool hash_dist, unsigned byte_mask);
    void found_bytes(grp_use *grp, safe_vector<IXBar::Use::Byte *> &unalloced, bool ternary,
        int &match_bytes_placed, int search_bus);
    void found_mid_bytes(mid_byte_use *mb_grp, safe_vector<Use::Byte *> &unalloced,
        bool ternary, int &match_bytes_placed, int search_bus, bool &prefer_half_bytes,
        bool only_alloc_nibble);
    void free_bytes(grp_use *grp, safe_vector<Use::Byte *> &unalloced,
        safe_vector<Use::Byte *> &alloced, bool ternary, bool hash_dist, int &match_bytes_placed,
        int search_bus);
    void free_mid_bytes(mid_byte_use *mb_grp, safe_vector<Use::Byte *> &unalloced,
        safe_vector<Use::Byte *> &alloced, int &match_bytes_placed, int search_bus,
        bool &prefer_half_bytes, bool only_alloc_nibble);
    void allocate_byte(bitvec *bv, safe_vector<IXBar::Use::Byte *> &unalloced,
        safe_vector<IXBar::Use::Byte *> *alloced, IXBar::Use::Byte &need, int group,
        int byte, size_t &index, int &free_bytes, int &ixbar_bytes_placed,
        int &match_bytes_placed, int search_bus, int *range_index);
    void allocate_mid_bytes(safe_vector<Use::Byte *> &unalloced,
        safe_vector<Use::Byte *> &alloced, bool ternary, bool prefer_found,
        safe_vector<grp_use> &order, safe_vector<mid_byte_use> &mid_byte_order,
        int nibbles_needed, int &bytes_to_allocate);

    int determine_best_group(safe_vector<Use::Byte *> &unalloced, safe_vector<grp_use> &order,
        bool ternary, bool prefer_found, int required_allocation_bytes);

    void allocate_groups(safe_vector<Use::Byte *> &unalloced, safe_vector<Use::Byte *> &alloced,
                         bool ternary, bool prefer_found, safe_vector<grp_use> &order,
                         safe_vector<mid_byte_use> &mid_byte_order, int &bytes_to_allocate,
                         int groups_needed, bool hash_dist, unsigned byte_mask);
    bool handle_pragma_ixbar_group_num(safe_vector<Use::Byte *> &unalloced,
                                       safe_vector<Use::Byte *> &alloced, bool ternary,
                                       safe_vector<grp_use> &order,
                                       safe_vector<mid_byte_use> &mid_byte_order,
                                       int &bytes_to_allocate, bool hash_dist,
                                       unsigned byte_mask, int search_bus);

    hash_matrix_reqs match_hash_reqs(const LayoutOption *lo, size_t start, size_t end,
        bool ternary);
    void layout_option_calculation(const LayoutOption *layout_option,
                                   size_t &start, size_t &last);

    int max_bit_to_byte(bitvec bit_mask);
    int max_index_group(int max_bit);
    int max_index_single_bit(int max_bit);
    unsigned index_groups_used(bitvec bv) const;
    unsigned select_bits_used(bitvec bv) const;
    bool hash_use_free(int max_group, int max_single_bit, unsigned hash_table_input);
    void write_hash_use(int max_group, int max_single_bit, unsigned hash_table_input,
        cstring name);
    bool can_allocate_on_search_bus(Use &alloc, const PHV::Field *field, le_bitrange range,
        int ixbar_position);
    bool setup_stateful_search_bus(const IR::MAU::StatefulAlu *salu, Use &alloc,
                                   const FindSaluSources &sources, unsigned &byte_mask);
    bool setup_stateful_hash_bus(const PhvInfo &, const IR::MAU::StatefulAlu *salu, Use &alloc,
                                 const FindSaluSources &sources);

    bitvec determine_final_xor(const IR::MAU::HashFunction *hf, const PhvInfo &phv,
        std::map<int, le_bitrange> &bit_starts,
        safe_vector<const IR::Expression *> field_list, int total_input_bits);
    void determine_proxy_hash_alg(const PhvInfo &, const IR::MAU::Table *tbl, Use &use, int group);

    bool isHashDistAddress(HashDistDest_t dest) const;

    void determineHashDistInUse(int hash_group, bitvec &units_in_use, bitvec &hash_bits_in_use);
    void buildHashDistIRUse(HashDistAllocPostExpand &alloc_req, HashDistUse &use,
        IXBar::Use &all_reqs, const PhvInfo &phv, int hash_group, bitvec hash_bits_used,
        bitvec total_post_expand_bits, const IR::MAU::Table* tbl, cstring name);
    void lockInHashDistArrays(safe_vector<Use::Byte *> *alloced, int hash_group,
        unsigned hash_table_input, int asm_unit, bitvec hash_bits_used, HashDistDest_t dest,
        cstring name);


    bool allocHashDistSection(bitvec post_expand_bits, bitvec possible_shifts, int hash_group,
        int &unit_allocated, bitvec &hash_bits_allocated);
    bool allocHashDistWideAddress(bitvec post_expand_bits, bitvec possible_shifts, int hash_group,
        bool chained_addr, int &unit_allocated, bitvec &hash_bits_allocated);
    bool allocHashDist(safe_vector<HashDistAllocPostExpand> &alloc_reqs, HashDistUse &use,
        const PhvInfo &phv, cstring name, const IR::MAU::Table* tbl, bool second_try);
    void createChainedHashDist(const HashDistUse &hd_alloc, HashDistUse &chained_hd_alloc,
        cstring name);
    void update_hash_parity(IXBar::Use &use, int hash_group);
    parity_status_t update_hash_parity(int hash_group);
};

}  // namespace Tofino

#endif /* BF_P4C_MAU_TOFINO_INPUT_XBAR_H_ */
