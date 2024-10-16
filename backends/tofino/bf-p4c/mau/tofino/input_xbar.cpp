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

#include "input_xbar.h"
#include "bf-p4c/common/slice.h"
#include "bf-p4c/device.h"
#include "bf-p4c/logging/resources.h"
#include "bf-p4c/mau/asm_hash_output.h"
#include "bf-p4c/mau/gateway.h"
#include "bf-p4c/mau/ixbar_expr.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/resource_estimate.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-utils/include/dynamic_hash/dynamic_hash.h"
#include "lib/algorithm.h"
#include "lib/bitvec.h"
#include "lib/bitops.h"
#include "lib/bitrange.h"
#include "lib/hex.h"
#include "lib/range.h"
#include "lib/log.h"
#include "lib/safe_vector.h"

constexpr le_bitrange Tofino::IXBar::SELECT_BIT_RANGE;
// = le_bitrange(RAM_SELECT_BIT_START, METER_ALU_HASH_BITS-1);

// helper functions for logging -- these need to not be in namespace Tofino or they
// mess up template instantiations of operator<<
std::ostream &operator<<(std::ostream &out,
    const ordered_map<const PHV::Field *,
                      std::map<le_bitrange, const IR::Expression *>> &phv_sources) {
    const char *sep = "phv_sources: ";
    for (auto &el : phv_sources) {
        for (auto &sl : el.second) {
            out << sep << el.first->name << "(" << sl.first.lo << ".." << sl.first.hi << "): "
                << *sl.second;
            sep = ", "; } }
    return out;
}
std::ostream &operator<<(std::ostream &out,
    const std::vector<const IR::MAU::IXBarExpression *> &hash_sources) {
    const char *sep = "  hash_sources: ";
    for (auto &el : hash_sources) {
        out << sep << *el;
        sep = ", "; }
    return out;
}

std::ostream &operator<<(std::ostream &out, Tofino::IXBar::HashDistAllocPostExpand &hda) {
    if (hda.func)
        hda.func->dbprint(out);
    else
        out << "(null)";
    out << " " << IXBar::hash_dist_name(hda.dest);
    out << "[" << hda.bits_in_use.hi << ":" << hda.bits_in_use.lo << "]";
    if (hda.shift) out << " shft=" << hda.shift;
    if (hda.chained_addr) out << " chain";
    return out;
}

void dump(Tofino::IXBar::HashDistAllocPostExpand &hda) { std::cout << hda << std::endl; }
void dump(std::vector<Tofino::IXBar::HashDistAllocPostExpand> &hdav) {
    for (auto& hda : hdav)
        std::cout << hda << std::endl;
}

namespace Tofino {

IXBar::Use &IXBar::getUse(autoclone_ptr<::IXBar::Use> &ac) {
    Use *rv;
    if (ac) {
        rv = dynamic_cast<Use *>(ac.get());
        BUG_CHECK(rv, "Wrong kind of IXBar::Use present");
    } else {
        ac.reset((rv = new Use)); }
    return *rv;
}

const IXBar::Use &IXBar::getUse(const autoclone_ptr<::IXBar::Use> &ac) {
    BUG_CHECK(ac, "Null autoclone");
    const Use *rv = dynamic_cast<const Use *>(ac.get());
    BUG_CHECK(rv, "Wrong kind of IXBar::Use present");
    return *rv;
}

void IXBar::Use::add(const IXBar::Use &alloc) {
    ::IXBar::Use::add(alloc);
    gw_search_bus = alloc.gw_search_bus;
    gw_search_bus_bytes = alloc.gw_search_bus_bytes;
    gw_hash_group = alloc.gw_hash_group;
    parity = alloc.parity;

    for (auto &input : alloc.hash_table_inputs) {
        int i = &input - &alloc.hash_table_inputs[0];
        if (hash_table_inputs[i] != 0 && input != 0)
            BUG("When adding allocs of ways, somehow ended up on the same hash group");
        hash_table_inputs[i] |= input;
        BUG_CHECK(hash_seed[i].popcount() == 0 || alloc.hash_seed[i].popcount() == 0,
                  "Hash seed already present for group %1%", i);
        hash_seed[i] |= alloc.hash_seed[i];
    }
    for (auto old_bits : bit_use) {
        for (auto new_bits : alloc.bit_use) {
            if (old_bits.group == new_bits.group)
                BUG("Bit uses from separate hash groups are within same slice");
        }
    }
    bit_use.insert(bit_use.end(), alloc.bit_use.begin(), alloc.bit_use.end());
}

/** Provides the bytes and hash group location of the partition index of an atcam table
 */
safe_vector<IXBar::Use::Byte> IXBar::Use::atcam_partition(int *hash_group) const {
    safe_vector<IXBar::Use::Byte> partition = ::IXBar::Use::atcam_partition(hash_group);
    if (hash_group) {
        for (auto &input : hash_table_inputs) {
            if (input) {
                *hash_group = &input - &hash_table_inputs[0];
                break; }
        }
    }
    return partition;
}

/** Provides information per search bus of how many bytes/bits a particular section of
 *  table uses in order to determine what section is the best candidate to ghost off
 */
safe_vector<IXBar::Use::TotalInfo> IXBar::Use::bits_per_search_bus() const {
    safe_vector<TotalInfo> rv;
    safe_vector<int> hash_groups;
    auto match_bytes = match_hash(&hash_groups);
    int hash_index = 0;

    for (auto &single_match : match_bytes) {
        int bits_per[IXBar::EXACT_GROUPS] = { 0 };
        int bytes_per[IXBar::EXACT_GROUPS] = { 0 };
        int group_per[IXBar::EXACT_GROUPS];
        std::fill(group_per, group_per + IXBar::EXACT_GROUPS, -1);

        for (auto &b : *single_match) {
            assert(b.loc.group >= 0 && b.loc.group < 8);
            assert(b.search_bus >= 0 && b.search_bus < 8);
            bits_per[b.search_bus] += b.bit_use.popcount();
            bytes_per[b.search_bus]++;
            if (group_per[b.search_bus] != -1)
                BUG_CHECK(group_per[b.search_bus] == b.loc.group, "Bytes on same search bus are "
                          "not contained within the same ixbar group");
            group_per[b.search_bus] = b.loc.group;
        }

        safe_vector<GroupInfo> sizes;
        int search_bus_index = 0;
        for (int i = 0; i < IXBar::EXACT_GROUPS; i++) {
             if (bits_per[i] == 0) continue;
             sizes.emplace_back(search_bus_index, group_per[i], bytes_per[i], bits_per[i]);
             search_bus_index++;
        }

        std::sort(sizes.begin(), sizes.end(),
            [=](const GroupInfo &a, const GroupInfo &b) {
            return a.search_bus < b.search_bus;
        });
        rv.emplace_back(hash_groups[hash_index], sizes);
        hash_index++;
    }
    return rv;
}

unsigned IXBar::Use::compute_hash_tables() {
    unsigned hash_table_input = 0;
    for (auto &b : use) {
        assert(b.loc.group >= 0 && b.loc.group < HASH_TABLES/2);
        unsigned grp = 1U << (b.loc.group * 2);
        if (b.loc.byte >= 8) grp <<= 1;
        hash_table_input |= grp; }
    return hash_table_input;
}

void IXBar::Use::dbprint(std::ostream &out) const {
    ::IXBar::Use::dbprint(out);
}

void IXBar::Use::emit_salu_bytemasks(std::ostream &out, indent_t indent) const {
    if (salu_input_source.data_bytemask)
        out << indent << "data_bytemask: " << salu_input_source.data_bytemask << std::endl;
    if (salu_input_source.hash_bytemask)
        out << indent << "hash_bytemask: " << salu_input_source.hash_bytemask << std::endl;
}

/* Determine which bytes of a table's input xbar belong to an individual hash table,
   so that we can output the hash of this individual table. */
void IXBar::Use::emit_ixbar_hash_table(int hash_table, safe_vector<Slice> &match_data,
        safe_vector<Slice> &ghost, const TableMatch *fmt,
        std::map<int, std::map<int, Slice>> &sort) const {
    LOG5("Emitting ixbar hash table");
    if (sort.empty()) return;

    auto update_match_data = [&](Slice &reg){
        if (fmt != nullptr) {
            safe_vector<Slice> reg_ghost;
            safe_vector<Slice> reg_hash = reg.split(fmt->ghost_bits, reg_ghost);
            ghost.insert(ghost.end(), reg_ghost.begin(), reg_ghost.end());
            // if dynamic_table_key_masks pragma is applied to the
            // table, ghost bits are disabled, as a result, match key must be
            // emitted as match data to generated the correct hash section in
            // bfa and context.json
            if (!fmt->identity_hash || fmt->dynamic_key_masks)
                match_data.insert(match_data.end(), reg_hash.begin(), reg_hash.end());
        } else {
            match_data.emplace_back(reg);
        }
    };

    unsigned half = hash_table & 1;
    if (sort.count(hash_table/2) == 0) return;
    for (auto &match : sort.at(hash_table/2)) {
        Slice reg = match.second;
        if (match.first/64U != half) {
            if ((match.first + reg.width() - 1)/64U != half)
                continue;
            assert(half);
            reg = reg(64 - match.first, 64);
        } else if ((match.first + reg.width() - 1)/64U != half) {
            assert(!half);
            reg = reg(0, 63 - match.first); }
        if (!reg) continue;
        update_match_data(reg);
    }
}

int IXBar::Use::hash_groups() const {
    int rv = 0;
    unsigned counted = 0;
    for (auto way : way_use) {
        if (((1U << way.source) & counted) == 0) {
            rv++;
            counted |= 1U << way.source;
        }
    }
    return rv;
}

IXBar::Use::TotalBytes IXBar::Use::match_hash(safe_vector<int> *hash_groups) const {
    TotalBytes rv = ::IXBar::Use::match_hash(hash_groups);
    if (!rv.empty()) return rv;

    for (auto &input : hash_table_inputs) {
        if (input == 0) continue;
        auto rv_index = new safe_vector<Byte>();
        for (auto byte : use) {
            int hash_group = byte.loc.group * 2 + byte.loc.byte / 8;
            if ((1 << hash_group) & input) {
                rv_index->push_back(byte);
            }
        }

        rv.push_back(rv_index);
        if (hash_groups)
            hash_groups->push_back(&input - &hash_table_inputs[0]);
    }
    return rv;
}

int IXBar::Use::ternary_align(const Loc &loc) const {
    size_t byte_offset = loc.group * IXBar::TERNARY_BYTES_PER_GROUP;
    byte_offset += (loc.group + 1) / 2;   // adjust for mid-byte
    byte_offset += loc.byte;
    return byte_offset % 4;
}

void IXBar::Use::update_resources(int stage, BFN::Resources::StageResources &stageResource) const {
    using UsageType = BFN::Resources::HashBitResource::UsageType;
    ::IXBar::Use::update_resources(stage, stageResource);

    // Used for the upper 12 bits of gateways
    for (auto &bits : bit_use) {
        for (int b = 0; b < bits.width; b++) {
            int bit = bits.bit + b + IXBar::HASH_INDEX_GROUPS * TableFormat::RAM_GHOST_BITS;
            auto key = std::make_pair(bit, bits.group);

            LOG_FEATURE("resources", 3, "\tadding resource hash_bits from bit_use(" << bit <<
                                        ", " << bits.group << "): {" << used_by << " --> " <<
                                        used_for() << ": " <<
                                        (bits.field + std::to_string(bits.lo + b)) << "}");
            BUG_CHECK(used_for() == "gateway", "Not gateway use of upper hash bit");
            stageResource.hashBits[key].append(
                used_by,
                used_for(),
                UsageType::Gateway,
                bits.lo + b,
                bits.field.c_str());
        }
    }

    // Used for the bits provided to the selector
    if (meter_alu_hash.allocated) {
        auto &mah = meter_alu_hash;
        for (auto bit : mah.bit_mask) {
            auto key = std::make_pair(bit, mah.group);

            LOG_FEATURE("resources", 3, "\tadding resource hash_bits from select_use(" << bit <<
                                        ", " << mah.group << "): {" << used_by << " --> " <<
                                        used_for() << ": " << "Selection Hash Bit " << bit << "}");
            stageResource.hashBits[key].append(
                used_by,
                used_for(),
                UsageType::SelectionBit,
                bit);
        }
    }
    // Used for the bits for hash distribution
    auto &hdh = hash_dist_hash;
    if (hdh.allocated) {
        for (auto bit : hdh.galois_matrix_bits) {
            int position = -1;
            for (auto bit_start : hdh.galois_start_bit_to_p4_hash) {
                int init_hb = bit_start.first;
                auto br = bit_start.second;
                if (bit >= init_hb && bit < init_hb + br.size())
                    position = br.lo + (bit - init_hb);
            }
            auto key = std::make_pair(bit, hdh.group);

            LOG_FEATURE("resources", 3, "\tadding resource hash_bits from hash_dist(" << bit <<
                                        ", " << hdh.group << "): {" << used_by << " --> " <<
                                        used_for() << ": " << "Hash Dist Bit " << position << "}");
            stageResource.hashBits[key].append(
                used_by,
                used_for(),
                UsageType::DistBit,
                position);
        }
    }
}

constexpr int IXBar::HASH_INDEX_GROUPS;
constexpr IXBar::HashDistDest_t IXBar::HD_STATS_ADR;

unsigned IXBar::hash_table_inputs(const HashDistUse &hdu) {
    unsigned rv = 0;
    for (auto &ir_alloc : hdu.ir_allocations) {
        if (auto use = dynamic_cast<const IXBar::Use *>(ir_alloc.use.get())) {
            rv |= use->hash_table_inputs[hdu.hash_group()];
        }
    }
    return rv;
}

int IXBar::HashDistUse::hash_group() const {
    int hash_group = -1;
    for (auto &ir_alloc : ir_allocations) {
        if (hash_group == -1)
            hash_group = ir_alloc.use->hash_dist_hash_group();
        else
            BUG_CHECK(hash_group == ir_alloc.use->hash_dist_hash_group(), "Hash Groups "
                 "are different across units");
    }
    return hash_group;
}

bitvec IXBar::HashDistUse::destinations() const {
    bitvec rv;
    for (auto &ir_alloc : ir_allocations) {
        rv.setbit(static_cast<int>(ir_alloc.dest));
    }
    return rv;
}

bitvec IXBar::HashDistUse::galois_matrix_bits() const {
    bitvec rv;
    for (auto &ir_alloc : ir_allocations) {
        rv |= ir_alloc.use->galois_matrix_bits();
    }
    return rv;
}

std::string IXBar::HashDistUse::used_for() const {
    auto dests = destinations();
    std::string rv = "";
    std::string sep = "";
    for (auto bit : dests) {
        std::string type = IXBar::hash_dist_name(static_cast<HashDistDest_t>(bit));
        rv += sep + type;
        sep = ", ";
    }
    return rv;
}

void IXBar::clear() {
    exact_use.clear();
    ternary_use.clear();
    byte_group_use.clear();
    exact_fields.clear();
    ternary_fields.clear();
    hash_index_use.clear();
    hash_single_bit_use.clear();
    hash_group_print_use.clear();
    hash_dist_use.clear();
    hash_dist_bit_use.clear();
    memset(hash_index_inuse, 0, sizeof(hash_index_inuse));
    memset(hash_single_bit_inuse, 0, sizeof(hash_single_bit_inuse));
    memset(hash_group_use, 0, sizeof(hash_group_use));
    memset(hash_group_parity_use, 0, sizeof(hash_group_parity_use));
    memset(hash_dist_inuse, 0, sizeof(hash_dist_inuse));
    memset(hash_dist_bit_inuse, 0, sizeof(hash_dist_bit_inuse));
    memset(hash_dist_groups, -1, sizeof(hash_dist_groups));
}

IXBar::parity_status_t IXBar::update_hash_parity(int hash_group) {
    if (hash_group < 0) return PARITY_NONE;

    auto parity_status = hash_group_parity_use[hash_group];
    parity_status_t parity_update = PARITY_DISABLED;
    if (parity_status == IXBar::PARITY_NONE) {
        auto enable_parity = (!BackendOptions().disable_gfm_parity);
        parity_update = enable_parity ? IXBar::PARITY_ENABLED : IXBar::PARITY_DISABLED;
    } else {
        parity_update = parity_status;
    }
    hash_group_parity_use[hash_group] = parity_update;
    return parity_update;
}

void IXBar::update_hash_parity(IXBar::Use &use, int hash_group) {
    if (hash_group < 0) return;
    const auto u = use;
    use.parity = update_hash_parity(hash_group);
}

bool IXBar::hash_matrix_reqs::fit_requirements(bitvec hash_matrix_in_use) const {
    if (!hash_dist) {
        int free_indexes = 0;
        for (int i = 0; i < HASH_INDEX_GROUPS; i++) {
            bitvec idx_use = hash_matrix_in_use.getslice(i * RAM_LINE_SELECT_BITS,
                                                         RAM_LINE_SELECT_BITS);
            if (idx_use.empty())
                free_indexes++;
        }
        if (free_indexes < index_groups)
            return false;

        bitvec select_use = hash_matrix_in_use.getslice(RAM_SELECT_BIT_START,
                                                        IXBar::get_hash_single_bits());
        if (IXBar::get_hash_single_bits() - select_use.popcount() < select_bits)
            return false;
        return true;
    }
    return false;
}

static int align_flags[IXBar::REPEATING_CONSTRAINT_SECT] = {
    /* these flags are the alignment restrictions that FAIL for each byte in 4 */
    IXBar::Use::Align16hi | IXBar::Use::Align32hi,
    IXBar::Use::Align16lo | IXBar::Use::Align32hi,
    IXBar::Use::Align16hi | IXBar::Use::Align32lo,
    IXBar::Use::Align16lo | IXBar::Use::Align32lo,
};


bitvec IXBar::can_use_from_flags(int flags) const {
    bitvec rv(0, REPEATING_CONSTRAINT_SECT);
    for (int i = 0; i < REPEATING_CONSTRAINT_SECT; i++) {
        if ((flags & align_flags[i]) != 0)
            rv.clrbit(i);
    }
    return rv;
}

int inline IXBar::groups(bool ternary) const {
    return ternary ? TERNARY_GROUPS : EXACT_GROUPS;
}

int inline IXBar::mid_bytes(bool ternary) const {
    return ternary ? BYTE_GROUPS : 0;
}

int inline IXBar::bytes_per_group(bool ternary) const {
    return ternary ? TERNARY_BYTES_PER_GROUP : EXACT_BYTES_PER_GROUP;
}

void IXBar::increase_ternary_ixbar_space(int &groups_needed, int &nibbles_needed,
        bool /* requires_versioning */) {
    // (TODO): Try to optimize it in the future.
    if (groups_needed > nibbles_needed)
        nibbles_needed++;
    else
        groups_needed++;
}


bool IXBar::calculate_sizes(safe_vector<Use::Byte> &alloc_use, bool ternary,
        int &total_bytes_needed, int &groups_needed, int &nibbles_needed,
        bool requires_versioning) {
    if (groups_needed == 0) {
        for (auto byte : alloc_use) {
            if (byte.is_spec(ATCAM_DOUBLE))
                total_bytes_needed += 2;
            else
                total_bytes_needed++;
        }


        if (ternary) {
            groups_needed = 1;
            nibbles_needed = 0;
            while (groups_needed * TERNARY_BYTES_PER_GROUP + (nibbles_needed + 1) / 2
                   < total_bytes_needed) {
                increase_ternary_ixbar_space(groups_needed, nibbles_needed, requires_versioning);
            }
        } else {
            groups_needed = (total_bytes_needed + bytes_per_group(false) - 1)
                            / bytes_per_group(false);
            nibbles_needed = 0;
        }
    } else {
        if (ternary) {
            increase_ternary_ixbar_space(groups_needed, nibbles_needed, requires_versioning);
        } else {
            groups_needed++;
        }
    }

    if (groups_needed > groups(ternary))
        return false;
    return true;
}

/**
 * The current algorithm, due to the odd constraints of the hash distribution section
 * separate available hashing for hash distribution vs. available hashing for everywhere
 * else.  The purpose of this is to determine what groups are completely unavailable
 * within the hash distribution section, due to the number of bits needed.
 *
 * TODO: Potentially in the hash_matrix_reqs, instead of looking at hash distribution
 * as a total, look at an individual group, and allocate on a group by group basis.
 */
void IXBar::calculate_available_hash_dist_groups(safe_vector<grp_use> &order,
        hash_matrix_reqs &hm_reqs) {
    for (int i = 0; i < HASH_DIST_UNITS; i++) {
        if (hash_dist_groups[i] == -1) continue;
        auto hash_tables = hash_group_use[hash_dist_groups[i]];
        bitvec slices_used;
        for (auto &grp : order) {
            for (int i = 0; i < 2; i++) {
                if ((hash_tables & (1U << (2 * grp.group + i))) == 0U)
                    continue;
                for (int hash_slice = 0; hash_slice < HASH_DIST_SLICES; hash_slice++) {
                    if (hash_dist_inuse[2 * grp.group + i].getbit(hash_slice))
                        slices_used.setbit(hash_slice);
                }
            }
        }
        if (HASH_DIST_SLICES - slices_used.popcount() >= hm_reqs.index_groups)
            continue;
        for (auto bit : bitvec(hash_tables)) {
            order[bit / 2].hash_open[bit % 2] = false;
        }
    }
}

/** The hash matrix, described in section 6.1.9 Hash Generation of the uArch, is a further
 *  constraint on where bytes can be placed within the input xbar.  The hash matrix is tied
 *  specifically to the exact match, and is not used for ternary.  The hash matrix is used
 *  for the following reasons:
 *     - Generating a lookup address for an exact table
 *     - Generating a hash for a selector to use
 *     - Generating any kind of hash to use through hash distribution.
 *
 *  The hash matrix itself is an 1024b * 52b matrix, broken into 16 64b by 52b matrix.  In order
 *  to generate a hash, the input xbar itself is broken into 16 64 bit sections.  Each 64 bit
 *  section performs a matrix multiply with its corresponding 64bx52b matrix to come up with
 *  a 16 separate 52b words.  Then any of the 52b words can be XORed together.  This is useful,
 *  for example, if a tables match data is larger than 64 bits, and thus requires the combination
 *  of multiple 64 bit sections.
 *
 *  Thus in order to successfully allocate a byte within a 64 bit section, that byte cannot use
 *  hash bits used by any other byte not part of the hash generation in this section.  For
 *  example, say a P4 program had two exact match tables with 32 bit keys, and each key needed
 *  30 bits of hash data.  Now because the hash matrix is only 52 bits, it is impossible for
 *  each of these tables data to be within the same 64 section of the input xbar, and have
 *  an independent hash calculation.
 *
 *  Futhermore, the algorithm currently restricts hash distribution and everything else to
 *  never be within the same 64 bit hash table.  This is just because it would be fairly tedious
 *  to check, and may limit hash distribution much later, especially for a greedy algorithm.
 */
void IXBar::calculate_available_groups(safe_vector<grp_use> &order,
                                       hash_matrix_reqs &hm_reqs) {
    for (auto &grp : order) {
        for (int i = 0; i < 2; i++) {
            grp.hash_open[i] = true;
        }
    }

    if (hm_reqs.hash_dist) {
        calculate_available_hash_dist_groups(order, hm_reqs);
    } else {
        for (auto &grp : order) {
            for (int i = 0; i < 2; i++) {
                int ways_available = 0;
                int select_bits_available = 0;
                for (int hg = 0; hg < HASH_INDEX_GROUPS; hg++) {
                     if (!(hash_index_inuse[hg] & (1 << (2 * grp.group + i))))
                         ways_available++;
                }
                for (int sb = 0; sb < IXBar::get_hash_single_bits(); sb++) {
                    if (!(hash_single_bit_inuse[sb] & (1 << (2 * grp.group + i))))
                        select_bits_available++;
                }

                if (ways_available < hm_reqs.index_groups ||
                    select_bits_available < hm_reqs.select_bits) {
                    grp.hash_open[i] = false;
                }
            }
        }
    }

    for (auto &grp : order) {
        for (int i = 0; i < 2; i++) {
            grp.hash_table_type[i] = is_group_for_hash_dist(2 * grp.group + i);
        }
    }
}

/** Determine if a group is either for hash distribution or match only */
IXBar::grp_use::type_t IXBar::is_group_for_hash_dist(int hash_table) {
    for (int i = 0; i < HASH_GROUPS; i++) {
        bool hash_dist = false;
        if (i == hash_dist_groups[0] || i == hash_dist_groups[1]) {
            hash_dist = true;
        }
        if (((1 << hash_table) | hash_group_use[i]) != hash_group_use[i]) continue;
        if (hash_dist)
            return grp_use::HASH_DIST;
        else
            return grp_use::MATCH;
    }
    return grp_use::FREE;
}

/** Due to the earlier calculations on what 64 bit hash matrices are available, determine
 *  whether a byte within the input xbar is available.
 */
bool IXBar::violates_hash_constraints(safe_vector<grp_use> &order, bool hash_dist, int group,
        int byte) {
    if (!order[group].hash_open[byte / 8]) {
        return true;
    }
    if (hash_dist) {
        if (!order[group].hash_dist_avail(byte / 8)) {
            return true;
        }
    } else {
        if (order[group].hash_dist_only(byte / 8)) {
            return true;
        }
    }
    return false;
}

/** Put the grp_use and mid_byte_use vectors in group order */
void IXBar::reset_orders(safe_vector<grp_use> &order, safe_vector<mid_byte_use> &mid_byte_order) {
    std::sort(order.begin(), order.end(), [=](const grp_use &a, const grp_use &b) {
        return a.group < b.group;
    });

    std::sort(mid_byte_order.begin(), mid_byte_order.end(),
              [=](const mid_byte_use &a, const mid_byte_use &b) {
        return a.group < b.group;
    });
}

/** Calculates the bytes per each group/midbyte that have previously been allocated by a table,
 *  and can also be used by this table.
 */
void IXBar::calculate_found(safe_vector<IXBar::Use::Byte *> &unalloced,
                            safe_vector<grp_use> &order,
                            safe_vector<mid_byte_use> &mid_byte_order,
                            bool ternary, bool hash_dist, unsigned byte_mask) {
    if (byte_mask != ~0U)
        return;

    auto &use = this->use(ternary);
    auto &fields = this->fields(ternary);
    reset_orders(order, mid_byte_order);
    for (int i = 0; i < groups(ternary); i++) {
        order[i].found.clear();
    }

    for (int i = 0; i < mid_bytes(ternary); i++) {
        mid_byte_order[i].found[0] = false;
    }

    for (auto &need : unalloced) {
        for (auto &p : Values(fields.equal_range(need->container))) {
            if (ternary && p.byte == TERNARY_BYTES_PER_GROUP) {
                if (need->is_range())
                    continue;
                if (byte_group_use[p.group/2].second == need->lo) {
                    mid_byte_order[p.group/2].found[0] = true;
                }
                continue;
            }

            if (use[p.group][p.byte].second == need->lo) {
                if (!ternary && violates_hash_constraints(order, hash_dist, p.group, p.byte)) {
                    continue;
                }
                if (need->is_range() && order[p.group].range_set()
                    && order[p.group].range_index != need->range_index)
                    continue;

                if (!(byte_mask & (1U << p.byte)))
                    continue;
                order[p.group].found[p.byte] = true;
            }
        }
    }
}

/** Calculate the empty locations within the groups and midbytes
 */
void IXBar::calculate_free(safe_vector<grp_use> &order,
        safe_vector<mid_byte_use> &mid_byte_order, bool ternary, bool hash_dist,
        unsigned byte_mask)  {
    auto &use = this->use(ternary);
    reset_orders(order, mid_byte_order);
    // FIXME: This is a way too tight constraint in order to get stful.p4 to correctly compile
    // There needs to be some coordination with what is actually needed vs. what is actually
    // available.  One doesn't need the whole byte_mask to be free, in order to allocate it.
    if (byte_mask != ~0U) {
        for (int grp = 0; grp < groups(ternary); grp++) {
            bool whole_section_free = true;
            for (int byte = 0; byte < bytes_per_group(ternary); byte++) {
                if (!(byte_mask & (1U << byte)))
                    continue;
                if (use[grp][byte].first)
                    whole_section_free = false;
            }
            for (int byte = 0; byte < bytes_per_group(ternary) && whole_section_free; byte++) {
                if (!(byte_mask & (1U << byte)))
                    continue;
                Loc loc(grp, byte);
                order[grp]._free[loc.getOrd(ternary) % REPEATING_CONSTRAINT_SECT][byte] = true;
            }
        }
        return;
    }

    for (int grp = 0; grp < groups(ternary); grp++) {
        for (int byte = 0; byte < bytes_per_group(ternary); byte++) {
            Loc loc(grp, byte);
            if (!ternary && violates_hash_constraints(order, hash_dist, grp, byte))
                continue;
            if (!use[grp][byte].first)
                order[grp]._free[loc.getOrd(ternary) % REPEATING_CONSTRAINT_SECT][byte] = true;
        }
    }

    for (int mid_byte = 0; mid_byte < mid_bytes(ternary); mid_byte++) {
        Loc loc(mid_byte * 2, TERNARY_BYTES_PER_GROUP);
        if (!byte_group_use[mid_byte].first) {
            int index = loc.getOrd(ternary) % REPEATING_CONSTRAINT_SECT;
            mid_byte_order[mid_byte]._free[index][0] = true;
        }
    }
}

/** Find the unalloced bytes in the current table that are already contained within the xbar
 *  group and share those locations.
 */
void IXBar::found_bytes(grp_use *grp, safe_vector<IXBar::Use::Byte *> &unalloced, bool ternary,
                       int &match_bytes_placed, int search_bus) {
    if (!grp) return;
    auto &use = this->use(ternary);
    auto &fields = this->fields(ternary);
    int found_bytes = grp->found.popcount();
    int ixbar_bytes_placed = 0;
    int total_match_bytes = ternary ? TERNARY_BYTES_PER_GROUP : EXACT_BYTES_PER_GROUP;
    for (size_t i = 0; i < unalloced.size(); i++) {
        auto &need = *(unalloced[i]);
        if (found_bytes == 0)
            break;
        if (match_bytes_placed >= total_match_bytes)
            break;
        int match_bytes_added = need.is_spec(ATCAM_DOUBLE) ? 2 : 1;
        if (match_bytes_placed + match_bytes_added > total_match_bytes)
            continue;
        if (need.is_range() && grp->range_set() && need.range_index != grp->range_index)
            continue;


        for (auto &p : Values(fields.equal_range(need.container))) {
            if (ternary && p.byte == TERNARY_BYTES_PER_GROUP)
                continue;


            if ((grp->group == p.group) && (use[p.group][p.byte].second == need.lo)) {
                if (!grp->found.getbit(p.byte))
                    continue;
                if (!ternary && !grp->hash_open[p.byte / 8])
                    continue;

                allocate_byte(&grp->found, unalloced, nullptr, need, p.group, p.byte, i,
                              found_bytes, ixbar_bytes_placed, match_bytes_placed, search_bus,
                              &grp->range_index);
                break;
            }
        }
    }
    LOG4("        Total found bytes placed was " << ixbar_bytes_placed << " "
          << match_bytes_placed);
}

/** Find the unalloced bytes in the current table that are already contained within the xbar
 *  midbyte for ternary and share those locations.
 */
void IXBar::found_mid_bytes(mid_byte_use *mb_grp, safe_vector<IXBar::Use::Byte *> &unalloced,
    bool ternary, int &match_bytes_placed, int search_bus, bool &prefer_nibble,
    bool only_alloc_nibble) {
    auto &fields = this->fields(ternary);
    if (!mb_grp) return;
    if (!mb_grp->found[0]) return;
    int found_bytes = 1;
    int ixbar_bytes_placed = 0;
    int total_match_bytes = 1;

    for (size_t i = 0; i < unalloced.size(); i++) {
        auto &need = *(unalloced[i]);

        if (found_bytes == 0)
            break;
        if (match_bytes_placed >= total_match_bytes)
            break;
        for (auto &p : Values(fields.equal_range(need.container))) {
            if (!(ternary && p.byte == TERNARY_BYTES_PER_GROUP))
                continue;

            if (only_alloc_nibble && !need.only_one_nibble_in_use())
                continue;

            if ((mb_grp->group == p.group/2) && (byte_group_use[p.group/2].second == need.lo)) {
                allocate_byte(nullptr, unalloced, nullptr, need, p.group, p.byte, i,
                              found_bytes, ixbar_bytes_placed, match_bytes_placed, search_bus,
                              nullptr);
                mb_grp->found[0] = false;
                if (need.only_one_nibble_in_use())
                    prefer_nibble = false;
                break;
            }
        }
    }
    LOG5("        Total found mid bytes placed was " << ixbar_bytes_placed << " "
          << match_bytes_placed);
}

/** Fills out all currently unoccupied xbar bytes within a group with bytes from the current table
 *  following alignment constraints.
 */
void IXBar::free_bytes(grp_use *grp, safe_vector<IXBar::Use::Byte *> &unalloced,
                      safe_vector<IXBar::Use::Byte *> &alloced,
                      bool ternary, bool hash_dist, int &match_bytes_placed, int search_bus) {
    if (!grp) return;
    int ixbar_bytes_placed = 0;
    int free_bytes = grp->max_free().popcount();
    int total_match_bytes = ternary ? TERNARY_BYTES_PER_GROUP : EXACT_BYTES_PER_GROUP;
    int ternary_offset = (grp->group / 2) * TERNARY_BYTES_PER_BIG_GROUP;
    ternary_offset += (grp->group % 2) * (TERNARY_BYTES_PER_GROUP + 1);
    int align_offset = ternary ? ternary_offset : 0;
    int align = align_offset & 3;

    for (size_t i = 0; i < unalloced.size(); i++) {
        if (free_bytes == 0)
            break;
        if (match_bytes_placed >= total_match_bytes)
            break;
        auto &need = *(unalloced[i]);
        int match_bytes_added = need.is_spec(ATCAM_DOUBLE) ? 2 : 1;
        if (match_bytes_placed + match_bytes_added > total_match_bytes)
            continue;
        if (need.is_range() && grp->range_set() && need.range_index != grp->range_index)
            continue;

        int chosen_byte = -1;
        bool found = false;
        for (auto byte : grp->max_free()) {
            // Alignment check to verify that the byte is a legal container position
            if (align_flags[(byte+align) & 3] & need.flags) {
                 continue;
            }

            allocate_byte(nullptr, unalloced, &alloced, need, grp->group, byte, i, free_bytes,
                          ixbar_bytes_placed, match_bytes_placed, search_bus, &grp->range_index);

            grp->_free[need.loc.getOrd(ternary) % 4][byte] = false;
            chosen_byte = byte;
            found = true;
            break;
        }

        if (hash_dist && !ternary && found) {
            grp->hash_table_type[chosen_byte / 8] = grp_use::HASH_DIST;
        }
    }
    LOG5("        Total free bytes placed was " << ixbar_bytes_placed << " "
          << match_bytes_placed << " " << grp->max_free());
}

/** Fills out a currently unoccupied midbyte from the current table, following PHV alignment
 *  constraints
 */
void IXBar::free_mid_bytes(mid_byte_use *mb_grp, safe_vector<IXBar::Use::Byte *> &unalloced,
        safe_vector<Use::Byte *> &alloced, int &match_bytes_placed, int search_bus,
        bool &prefer_nibble, bool only_alloc_nibble) {
    if (!mb_grp) return;
    if (!mb_grp->max_free().getbit(0))
        return;

    int ixbar_bytes_placed = 0;
    int free_bytes = 1;
    int align_offset = (mb_grp->group * TERNARY_BYTES_PER_BIG_GROUP)
                        + TERNARY_BYTES_PER_GROUP;
    int align = align_offset & 3;
    int total_match_bytes = 1;

    // Before trying the most constrained bytes (sourced from 32 bit PHVs), instead attempt to
    // allocate only bytes that use one nibble
    if (prefer_nibble) {
        for (size_t i = 0; i < unalloced.size(); i++) {
            if (free_bytes == 0)
                break;
            if (match_bytes_placed >= total_match_bytes)
                break;
            auto &need = *(unalloced[i]);
            if (need.is_range())
                continue;
            if (!need.only_one_nibble_in_use())
                continue;
            if (align_flags[align & 3] & need.flags) continue;
            allocate_byte(nullptr, unalloced, &alloced, need, mb_grp->group * 2, 5, i,
                          free_bytes, ixbar_bytes_placed, match_bytes_placed, search_bus,
                          nullptr);
            mb_grp->_free[need.loc.getOrd(true) % 4][0] = false;
            prefer_nibble = false;
        }
    }

    if (only_alloc_nibble)
        return;

    for (size_t i = 0; i < unalloced.size(); i++) {
        if (free_bytes == 0)
            break;
        if (match_bytes_placed >= total_match_bytes)
            break;
        auto &need = *(unalloced[i]);
        if (need.is_range())
            continue;
        if (align_flags[align & 3] & need.flags) continue;
        allocate_byte(nullptr, unalloced, &alloced, need, mb_grp->group * 2,
                      bytes_per_group(true), i, free_bytes, ixbar_bytes_placed,
                      match_bytes_placed, search_bus, nullptr);
        mb_grp->_free[need.loc.getOrd(true) % 4][0] = false;
        if (need.only_one_nibble_in_use())
            prefer_nibble = false;
    }
    LOG5("        Total free mid bytes placed was " << ixbar_bytes_placed << " "
          << match_bytes_placed);
}


/** Allocate all fields within a IXBar::Byte object given its selection throughout the
 *  algorithm.  If it is not shared, move the byte to the alloced list to be filled in
 *  to the IXBar local objects later
 */
void IXBar::allocate_byte(bitvec *bv, safe_vector<Use::Byte *> &unalloced,
                          safe_vector<Use::Byte *> *alloced, Use::Byte &need, int group, int byte,
                          size_t &index, int &avail_bytes, int &ixbar_bytes_placed,
                          int &match_bytes_placed, int search_bus, int *range_index) {
    need.loc.group = group;
    need.loc.byte = byte;
    need.search_bus = search_bus;
    if (bv != nullptr)
        (*bv)[byte] = false;
    if (alloced)
        alloced->push_back(unalloced[index]);

    unalloced.erase(unalloced.begin() + index);
    index--;
    avail_bytes--;
    ixbar_bytes_placed++;
    if (need.is_spec(ATCAM_DOUBLE))
        match_bytes_placed += 2;
    else
        match_bytes_placed++;

    if (need.is_range() && range_index) {
        *range_index = need.range_index;
    }
}

/* When all bytes of the current table have been given a placement, this function fills out
   the xbars use for later record keeping and checks */
void IXBar::fill_out_use(safe_vector<IXBar::Use::Byte *> &alloced, bool ternary) {
    auto &use = this->use(ternary);
    auto &fields = this->fields(ternary);
    for (auto &need : alloced) {
        fields.emplace(need->container, need->loc);
        if (ternary && need->loc.byte == 5) {
            byte_group_use[need->loc.group/2] = *(need);
        } else {
            use[need->loc] = *(need);
        }
    }
}

/** Given whether the input xbar is either for the TCAM or the SRAM, create and size the order
 *  and midbyte order accordingly
 */
void IXBar::initialize_orders(safe_vector<grp_use> &order,
        safe_vector<mid_byte_use> &mid_byte_order, bool ternary) {
    order.clear();
    mid_byte_order.clear();
    for (int group = 0; group < groups(ternary); group++) {
        order.emplace_back(group);
    }

    for (int mid_byte = 0; mid_byte < mid_bytes(ternary); mid_byte++) {
        mid_byte_order.emplace_back(mid_byte);
    }
}

/** Clear the alloced vector, reinitialize all bytes to unallocated, and reset unalloced
 */
void IXBar::setup_byte_vectors(safe_vector<IXBar::Use::Byte> &alloc_use, bool ternary,
        safe_vector<IXBar::Use::Byte *> &unalloced, safe_vector<IXBar::Use::Byte *> &alloced,
        safe_vector<grp_use> &order, safe_vector<mid_byte_use> &mid_byte_order,
        hash_matrix_reqs &hm_reqs, unsigned byte_mask) {
    unalloced.clear();
    alloced.clear();
    for (auto byte : alloc_use) {
        byte.unallocate();
    }

    if (byte_mask == ~0U) {
        std::sort(alloc_use.begin(), alloc_use.end(),
                  [=](const Use::Byte &a, const Use::Byte &b) {
            int t;
            if (a.is_range() && !b.is_range())
                return true;
            if (!a.is_range() && b.is_range())
                return false;
            if ((t = a.range_index - b.range_index) != 0)
                return t < 0;
            if ((t = static_cast<size_t>(a.flags) - static_cast<size_t>(b.flags)) != 0)
                return t > 0;
            return a < b;
        });
    }

    for (auto &need : alloc_use) {
        unalloced.push_back(&need);
    }
    if (!ternary) {
        calculate_available_groups(order, hm_reqs);
    }

    /* Initial found and free calculations */
    calculate_found(unalloced, order, mid_byte_order, ternary, hm_reqs.hash_dist, byte_mask);
    calculate_free(order, mid_byte_order, ternary, hm_reqs.hash_dist, byte_mask);
}

void IXBar::print_groups(safe_vector<grp_use> &order, safe_vector<mid_byte_use> &mid_byte_order,
        bool ternary) {
    for (int grp = 0; grp < groups(ternary); grp++) {
        LOG6("\t  Group " << order[grp]);
        if (ternary && (grp % 2) == 0)
            LOG6("\t  Mid byte " << mid_byte_order[grp / 2]);
    }
}

/**
 * Given a certain number of nibbles allowed for a single ternary match table, determine which
 * byte_groups (out of the possible 6 bytes between each even-odd pair of ternary xbar groups)
 * are the best to allocate into.
 *
 * If the nibbles allowed are odd, then if it is possible to allocate a byte that only uses one
 * of the nibbles in that particular byte, so that only one TCAM is required, the allocation
 * scheme will try to only allocate that one nibble
 */
void IXBar::allocate_mid_bytes(safe_vector<IXBar::Use::Byte *> &unalloced,
        safe_vector<IXBar::Use::Byte *> &alloced, bool ternary, bool prefer_found,
        safe_vector<grp_use> &order, safe_vector<mid_byte_use> &mid_byte_order,
        int nibbles_needed, int &bytes_to_allocate) {
    int bytes_needed = nibbles_needed / 2 + nibbles_needed % 2;
    bool prefer_nibble = nibbles_needed % 2;
    for (int search_bus = 0; search_bus < bytes_needed; search_bus++) {
        if (unalloced.size() == 0)
            return;
        // Only one nibble is allowed to be allocated, i.e. the byte that is allocated has to be
        // a nibble
        bool only_alloc_nibble = prefer_nibble && (search_bus == bytes_needed - 1);
        std::map<int, int> constraints_to_reqs;
        for (auto byte : unalloced) {
            if (byte->is_range())
                continue;
            // If only nibble bytes are allowed, then only look at bytes that use a single nibble
            // for gathering information about constraints
            if (only_alloc_nibble && !byte->only_one_nibble_in_use())
                continue;
            bitvec key_bv = can_use_from_flags(byte->flags);
            int key = key_bv.getrange(0, REPEATING_CONSTRAINT_SECT);
            if (constraints_to_reqs.count(key) == 0)
                constraints_to_reqs[key] = 1;
            else
                constraints_to_reqs[key]++;
        }

        mid_byte_use *selected_grp = nullptr;
        for (auto &mb_grp : mid_byte_order) {
            if (selected_grp == nullptr) {
                selected_grp = &mb_grp;
            } else if (mb_grp.is_better_group(*selected_grp, prefer_found, 1,
                                              constraints_to_reqs)) {
                selected_grp = &mb_grp;
            }
        }

        if (!selected_grp) continue;
        LOG5("       Mid byte selected was " << selected_grp->group << " bytes left "
             << unalloced.size());
        int match_bytes_placed = 0;
        found_mid_bytes(selected_grp, unalloced, ternary, match_bytes_placed, search_bus,
                        prefer_nibble, only_alloc_nibble);
        free_mid_bytes(selected_grp, unalloced, alloced, match_bytes_placed, search_bus,
                       prefer_nibble, only_alloc_nibble);
        selected_grp->attempted = true;
        bytes_to_allocate -= match_bytes_placed;
        calculate_found(unalloced, order, mid_byte_order, ternary, false, ~0U);
    }
}

/**
 * The purpose of this is to determine if the grp_use b is a better choice for allocation
 * than the current grp_use, translated to a.
 *
 * The constraints from XBar are that each Byte, indexed by a number, can be allocated only
 * by certain PHV Container Bytes.  The rule is the following:
 *
 *     Size = Container Size in Bytes
 *     Container Byte # can go to XBar Byte # if (Container Byte # % Size) == (XBar Byte # % Size)
 *
 * This rule means each 8 bit container can go anywhere on the xbar, each byte of a 16 bit
 * container can go to half of the xbar positions, and each byte of a 32 bit container
 * can go to a quarter of the xbar positions.
 *
 * With TCAMs specifically, due to the strange alignments of 5 x 1 x 5 byte structure, the
 * restrictions on each grp_use are different given which grp_use is used.  Thus picking
 * a grp_use that best corresponds with the constraints in the XBar requirements is
 * generally helpful.
 *
 * This for TCAMs does not take into account the byte swizzler, which could loosen up
 * some of the constraints
 *
 * constraints_to_reqs is a map with:
 *     - key: an unsigned representing the constraints of a byte within a 4 byte region of
 *            xbar e.g. (32 bit lo = 0x1, 16 bit lo = 0x5, 16 bit hi = 0xa, 8 bit = 0xf)
 *     - value: The number of bytes required that have that constraint
 */
bool IXBar::grp_use::is_better_group(const grp_use &b, bool prefer_found,
        int required_allocation_bytes, std::map<int, int> &constraints_to_reqs) const {
    auto &a = *this;
    if (!a.attempted && b.attempted) return true;
    if (a.attempted && !b.attempted) return false;
    bool a_candidate = a.total_avail() > required_allocation_bytes;
    bool b_candidate = b.total_avail() > required_allocation_bytes;

    if (a_candidate && !b_candidate)
        return true;
    if (b_candidate && !a_candidate)
        return false;


    int t;
    if (prefer_found) {
       if ((t = a.found.popcount() - b.found.popcount()) != 0)
           return t > 0;
       if ((t = a.total_avail() - b.total_avail()) != 0)
           return t < 0;
    }

    int a_can_alloc_32 = 0;
    int b_can_alloc_32 = 0;

    // How many of the 32 bit PHV container bytes can this grp_use support
    for (auto entry : constraints_to_reqs) {
        bitvec bv_key(entry.first);
        if (bv_key.popcount() != AV_FULL)
            continue;
        a_can_alloc_32 += std::min(a.free(bv_key).popcount(), entry.second);
        b_can_alloc_32 += std::min(b.free(bv_key).popcount(), entry.second);
    }

    if ((t = a_can_alloc_32 - b_can_alloc_32) != 0)
        return t > 0;

    grp_use a_lock_32(a);
    grp_use b_lock_32(b);
    // removes the 32 bit allocated (as those will be preferred always, except in the case
    // of a multi range match key)
    for (auto entry : constraints_to_reqs) {
        bitvec bv_key(entry.first);
        if (bv_key.popcount() != AV_FULL)
            continue;

        for (int i = 0; i < a.free(bv_key).popcount(); i++) {
            int free_index = bv_key.min().index();
            bitvec &constrained_bv = a_lock_32._free[free_index];
            int first_bit = constrained_bv.ffs();
            if (first_bit < 0)
                break;
            if (i == entry.second)
                break;
            a_lock_32._free[free_index].clrbit(first_bit);
        }

        for (int i = 0; i < b.free(bv_key).popcount(); i++) {
            int free_index = bv_key.min().index();
            bitvec &constrained_bv = b_lock_32._free[free_index];
            int first_bit = constrained_bv.ffs();
            if (first_bit < 0)
                break;
            if (i == entry.second)
                break;
            b_lock_32._free[free_index].clrbit(first_bit);
        }
    }

    BUG_CHECK(a.max_free().popcount() - a_lock_32.max_free().popcount() == a_can_alloc_32 &&
              b.max_free().popcount() - b_lock_32.max_free().popcount() == b_can_alloc_32,
              "Error in TCAM Xbar allocation determination");

    int a_can_alloc_16 = 0;
    int b_can_alloc_16 = 0;

    int a_can_alloc_16_lock_32 = 0;
    int b_can_alloc_16_lock_32 = 0;

    for (auto entry : constraints_to_reqs) {
        bitvec bv_key(entry.first);
        if (bv_key.popcount() != AV_HALF)
            continue;
        a_can_alloc_16 += std::min(a.free(bv_key).popcount(), entry.second);
        b_can_alloc_16 += std::min(b.free(bv_key).popcount(), entry.second);

        a_can_alloc_16_lock_32 += std::min(a_lock_32.free(bv_key).popcount(), entry.second);
        b_can_alloc_16_lock_32 += std::min(b_lock_32.free(bv_key).popcount(), entry.second);
    }

    // Favor the groups that have to place the largest number of 32 bit bytes
    int max_32_bit_value = 0;
    int a_can_alloc_32_max_reqs = 0;
    int b_can_alloc_32_max_reqs = 0;
    for (auto entry : constraints_to_reqs) {
        bitvec bv_key(entry.first);
        if (bv_key.popcount() != AV_FULL)
            continue;
        max_32_bit_value = std::max(max_32_bit_value, entry.second);
    }

    for (auto entry : constraints_to_reqs) {
        bitvec bv_key(entry.first);
        if (bv_key.popcount() != AV_FULL)
            continue;
        if (entry.second != max_32_bit_value)
            continue;
        a_can_alloc_32_max_reqs += std::min(a.free(bv_key).popcount(), entry.second);
        b_can_alloc_32_max_reqs += std::min(b.free(bv_key).popcount(), entry.second);
    }

    // How many 16 bit PHV container bytes can be allocated, once the 32 bit ones are locked
    if ((t = a_can_alloc_16_lock_32 - b_can_alloc_16_lock_32) != 0)
        return t > 0;
    if ((t = a_can_alloc_32_max_reqs - b_can_alloc_32_max_reqs) != 0)
        return t > 0;
    if ((t = a_can_alloc_16 - b_can_alloc_16) != 0)
        return t > 0;

    if ((t = a.total_avail() - b.total_avail()) != 0)
        return t > 0;
    return a.group < b.group;
}

bool IXBar::handle_pragma_ixbar_group_num(safe_vector<IXBar::Use::Byte *> &unalloced,
        safe_vector<IXBar::Use::Byte *> &alloced, bool ternary,
        safe_vector<grp_use> &order, safe_vector<mid_byte_use> &mid_byte_order,
        int &bytes_to_allocate, bool hash_dist, unsigned byte_mask, int search_bus) {
    safe_vector<IXBar::Use::Byte *> unalloced_with_pragma;
    int ixbar_group_num = -1;
    for (auto byte : unalloced) {
        if (byte->ixbar_group_num != -1) {
            ixbar_group_num = byte->ixbar_group_num;
            unalloced_with_pragma.push_back(byte); }
    }
    if (ixbar_group_num == -1)
        return false;

    int match_bytes_placed = 0;
    grp_use *selected_grp = &order[ixbar_group_num];
    safe_vector<IXBar::Use::Byte *> alloced_by_pragma;
    LOG5("       Group selected was " << selected_grp->group << " bytes left " << unalloced.size());
    found_bytes(selected_grp, unalloced_with_pragma, ternary, match_bytes_placed, search_bus);
    free_bytes(selected_grp, unalloced_with_pragma, alloced_by_pragma, ternary, hash_dist,
               match_bytes_placed, search_bus);
    selected_grp->attempted = true;
    bytes_to_allocate -= match_bytes_placed;
    calculate_found(unalloced_with_pragma, order, mid_byte_order, ternary, hash_dist, byte_mask);

    for (auto byte : alloced_by_pragma) {
        unalloced.erase(std::remove(unalloced.begin(), unalloced.end(), byte), unalloced.end());
    }

    for (auto byte : alloced_by_pragma) {
        alloced.push_back(byte);
    }
    return true;
}

void IXBar::allocate_groups(safe_vector<IXBar::Use::Byte *> &unalloced,
        safe_vector<IXBar::Use::Byte *> &alloced, bool ternary, bool prefer_found,
        safe_vector<grp_use> &order, safe_vector<mid_byte_use> &mid_byte_order,
        int &bytes_to_allocate, int groups_needed, bool hash_dist, unsigned byte_mask) {
    for (int search_bus = 0; search_bus < groups_needed; search_bus++) {
        if (unalloced.size() == 0)
            return;

        if (handle_pragma_ixbar_group_num(unalloced, alloced, ternary, order, mid_byte_order,
                                          bytes_to_allocate, hash_dist, byte_mask, search_bus)) {
            continue;
        }

        int match_bytes_placed = 0;
        int max_possible_bytes = (groups_needed - search_bus) * bytes_per_group(ternary);
        int leeway = std::max(max_possible_bytes - bytes_to_allocate, 0);
        int required_allocation_bytes = std::max(bytes_per_group(ternary) - leeway, 0);
        std::map<int, int> constraints_to_reqs;

        for (auto byte : unalloced) {
            bitvec key_bv = can_use_from_flags(byte->flags);
            int key = key_bv.getrange(0, REPEATING_CONSTRAINT_SECT);
            if (constraints_to_reqs.count(key) == 0)
                constraints_to_reqs[key] = 1;
            else
                constraints_to_reqs[key]++;
        }

        grp_use *selected_grp = nullptr;
        for (auto &grp : order) {
            if (selected_grp == nullptr) {
                selected_grp = &grp;
            } else if (grp.is_better_group(*selected_grp, prefer_found, required_allocation_bytes,
                                           constraints_to_reqs)) {
                selected_grp = &grp;
            }
        }

        if (!selected_grp) continue;
        LOG5("       Group selected was " << selected_grp->group << " bytes left "
             << unalloced.size());
        found_bytes(selected_grp, unalloced, ternary, match_bytes_placed, search_bus);
        free_bytes(selected_grp, unalloced, alloced, ternary, hash_dist, match_bytes_placed,
                   search_bus);
        selected_grp->attempted = true;
        bytes_to_allocate -= match_bytes_placed;
        calculate_found(unalloced, order, mid_byte_order, ternary, hash_dist, byte_mask);
    }
}


/** The algorithm for the allocation of bytes from the table to the input xbar, both for the
 *  TCAM and SRAM xbar.  The input xbar is defined in section 6.1.8 Match Input Xbar of the
 *  uArch.
 *
 *  The input xbar for SRAM is 128 bytes, divided into 8 sections of 16 bytes (128 bits).  As
 *  defined in section 6.2.3 Exact Match Row Vertical/Horizontal (VH) Xbars, any of the 8
 *  sections can go to any search bus within the match array.  However, a search bus can
 *  only input one of input xbar groups.  Thus to minimize the width of the match, the algorithm
 *  must minimize the number of input xbar groups needed.  Keep in mind that a search bus is
 *  128 bits as well, so that only one search bus is need for an entire input xbar group.
 *
 *  The input xbar for TCAM is 66 bytes, divided into 6 groups of 11 bytes.  These groups of 11
 *  bytes can be thought of as a group of 5 bytes, a mid byte and a final group of 5 bytes.
 *  The reason for this odd structure is described in section 6.3.5 TCAM Matching Setup.  Each
 *  TCAM is 44 bits.  The lower 40 bits of the TCAM can pull from any 5 byte group, and the
 *  upper 4 bits can pull from any nibble of a midbyte.
 *
 *  An input xbar groups has the capability to be shared between multiple tables.  Say for
 *  instance two separate ternary tables include a 40 bit field, and were to be placed in the
 *  same stage.  If that 40 bit field occupied a single TCAM input xbar group, then two separate
 *  TCAMs, one for each table can pull from the same input xbar groups.
 *
 *  The algorithm works as follows:
 *    - Calculate the minimum number of midbytes/groups needed for a particular table
 *    - Run potentially two versions of a fitting algorithm.
 *      * On the first iteration, prefer groups that have capabilities to share input xbar bytes.
 *      * If, when found groups were preferred, the bytes did not fit within the alloted
 *        amount of space, prefer input xbar groups with the most open space and just try to
 *        pack.
 *      * If neither fit, then increase the minimum amount groups/midbytes
 *
 * ARGUMENTS:
 *   alloc_use  vector of Byte objects that need to be allocated on the ixbar
 *   ternary    true for ternary ixbar, false for exact
 *   alloced    output -- the Byte objects that were successfully allocated, as we want to fill
 *              out the values after we know the bytes don't break any hash constraints
 *   hm_reqs    how much space, if any is required to be reserved on the hash matrix
 *   byte_mask  which bytes in ixbar groups to use -- default mask of ~0 means use any bytes
 */
bool IXBar::find_alloc(safe_vector<IXBar::Use::Byte> &alloc_use, bool ternary,
                      safe_vector<IXBar::Use::Byte *> &alloced, hash_matrix_reqs &hm_reqs,
                      unsigned byte_mask) {
    int total_bytes_needed = 0;
    int groups_needed = 0;
    int nibbles_needed = 0;

    safe_vector<grp_use> small_order;
    bool allocated = false;
    safe_vector<grp_use> order;
    safe_vector<mid_byte_use> mid_byte_order;
    safe_vector<IXBar::Use::Byte *> unalloced;
    bool first_time = true;

    do {
        bool possible = calculate_sizes(alloc_use, ternary, total_bytes_needed, groups_needed,
                                        nibbles_needed, hm_reqs.requires_versioning);
        LOG6("total byte needed " << total_bytes_needed << " group needed " << groups_needed
             << " nibbles_needed " << nibbles_needed << " hm_req " << hm_reqs.requires_versioning);
        if (!possible)
            break;
        if (hm_reqs.max_search_buses > 0 && groups_needed > hm_reqs.max_search_buses)
            break;
        for (int i = 0; i < 2 && !allocated; i++) {
            bool prefer_found = i == 0;
            initialize_orders(order, mid_byte_order, ternary);
            setup_byte_vectors(alloc_use, ternary, unalloced, alloced, order, mid_byte_order,
                               hm_reqs, byte_mask);

            LOG4("      Algorithm iteration groups " << groups_needed << " nibbles "
                 << nibbles_needed << " prefer_overlay " << prefer_found);
            if (first_time) {
                LOG6("\tPre allocation groups:");
                print_groups(order, mid_byte_order, ternary);
                first_time = false;
            }
            int bytes_to_allocate = total_bytes_needed;
            if (ternary) {
                allocate_mid_bytes(unalloced, alloced, ternary, prefer_found, order,
                                   mid_byte_order, nibbles_needed, bytes_to_allocate);
            }
            allocate_groups(unalloced, alloced, ternary, prefer_found, order, mid_byte_order,
                            bytes_to_allocate, groups_needed, hm_reqs.hash_dist, byte_mask);
            allocated = unalloced.size() == 0;
        }
    } while (allocated == false);

    if (allocated) {
        LOG6("\tPost allocation groups:");
        print_groups(order, mid_byte_order, ternary);
    }
    return allocated;
}

/* Simple first step that aligns with the possible options for layout option way sizes.
   For example, the max a way size can currently be will be 16, and at most 3 16 deep ways
   can be within a single column.  Thus it may need a second hash group */
void IXBar::layout_option_calculation(const LayoutOption *layout_option,
                                      size_t &start, size_t &last) {
    if (!layout_option) return;
    if (layout_option->layout.ternary) {
        start = 0; last = 0; return;
    }
    start = last;
    if (start == 0 && layout_option && layout_option->select_bus_split != -1) {
        last = layout_option->select_bus_split;
    } else {
        last = layout_option->way_sizes.size();
    }
}

unsigned IXBar::index_groups_used(bitvec bv) const {
    unsigned rv = 0;
    for (int i = 0; i < HASH_INDEX_GROUPS; i++) {
        if (bv.getrange(i * RAM_LINE_SELECT_BITS, RAM_LINE_SELECT_BITS))
            rv |= (1 << i);
    }
    return rv;
}

unsigned IXBar::select_bits_used(bitvec bv) const {
    unsigned rv = 0;
    for (int i = RAM_SELECT_BIT_START; i < get_hash_matrix_size(); i++) {
        if (bv.getbit(i))
            rv |= (1 << (i - RAM_SELECT_BIT_START));
    }
    return rv;
}

IXBar::hash_matrix_reqs IXBar::match_hash_reqs(const LayoutOption *lo,
        size_t start, size_t last, bool ternary) {
    if (ternary) {
        auto rv = hash_matrix_reqs();
        rv.requires_versioning = lo->layout.requires_versioning;
        return rv;
    }

    int bits_required = 0;
    for (size_t index = start; index < last; index++) {
        bits_required += ceil_log2(lo->way_sizes[index]);
    }
    bits_required = std::min(bits_required, IXBar::get_hash_single_bits());
    int groups_required = std::min(last - start, static_cast<size_t>(IXBar::HASH_INDEX_GROUPS));
    return hash_matrix_reqs(groups_required, bits_required, false);
}

Visitor::profile_t IXBar::FindSaluSources::init_apply(const IR::Node *root) {
    profile_t rv = MauInspector::init_apply(root);
    if (!tbl->for_dleft() || tbl->match_key.empty())
        return rv;
    dleft = true;
    KeyInfo ki;
    for (auto read : tbl->match_key) {
        if (!read->for_dleft())
            continue;
        FieldManagement(&map_alloc, field_list_order, read->expr, &fields_needed,
                        phv, ki, tbl);
    }
    return rv;
}

bool IXBar::FindSaluSources::preorder(const IR::MAU::StatefulAlu *) {
    return !dleft;
}

bool IXBar::FindSaluSources::preorder(const IR::MAU::SaluAction *a) {
    visit(a->action, "action");  // just visit the action instructions
    return false;
}

bool IXBar::FindSaluSources::preorder(const IR::Expression *e) {
    le_bitrange bits;
    KeyInfo ki;
    if (auto *finfo = phv.field(e, &bits)) {
        FieldManagement(&map_alloc, field_list_order, e, &fields_needed, phv, ki, tbl);
        if (!findContext<IR::MAU::IXBarExpression>()) {
            phv_sources[finfo][bits] = e;
            collapse_contained(phv_sources[finfo]);
        }
        return false;
    }
    return true;
}

bool IXBar::FindSaluSources::preorder(const IR::MAU::HashDist *) {
    // Handled in a different section
    return false;
}

bool IXBar::FindSaluSources::preorder(const IR::MAU::IXBarExpression *e) {
    for (auto ex : hash_sources)
        if (ex->equiv(*e))
            return true;
    hash_sources.push_back(e);
    return true;
}

/* remove ranges from the map iff they are contained in some other range in the map */
void IXBar::FindSaluSources::collapse_contained(std::map<le_bitrange, const IR::Expression *> &m) {
    for (auto it = m.begin(); it != m.end();) {
        bool remove = false;
        for (auto &el : Keys(m)) {
            if (el == it->first) continue;
            if (el.contains(it->first)) {
                remove = true;
                break; }
            if (el.lo > it->first.lo) break; }
        if (remove)
            it = m.erase(it);
        else
            ++it; }
}

bool IXBar::allocMatch(bool ternary, const IR::MAU::Table *tbl,
                       const PhvInfo &phv, Use &alloc,
                       safe_vector<IXBar::Use::Byte *> &alloced,
                       hash_matrix_reqs &hm_reqs) {
    if (tbl->match_key.empty()) return true;
    ContByteConversion map_alloc;
    std::map<cstring, bitvec> fields_needed;
    KeyInfo ki;
    ki.is_atcam = tbl->layout.atcam;

    // For overlapping keys of different types where one type is "range", the
    // range key takes precedence to correctly set up dirtcam bits
    std::map<std::pair<cstring, le_bitrange>, const IR::MAU::TableKey*> validKeys;
    for (auto ixbar_read : tbl->match_key) {
        if (!ixbar_read->for_match())
            continue;
        le_bitrange bits = {};
        auto *f = phv.field(ixbar_read->expr, &bits);
        if (!f) continue;
        auto idx = std::make_pair(f->name, bits);
        if (validKeys.count(idx)) {
            if (!(ixbar_read->for_range() && !validKeys[idx]->for_range()))
                continue;
        }
        validKeys[idx] = ixbar_read;
    }

    for (auto vkey : validKeys) {
        FieldManagement(&map_alloc, alloc.field_list_order, vkey.second, &fields_needed,
                        phv, ki, tbl);
    }

    create_alloc(map_alloc, alloc);
    LOG3("need " << alloc.use.size() << " bytes for table " << tbl->name);

    bool rv = find_alloc(alloc.use, ternary, alloced, hm_reqs);
    if (!ternary && !tbl->layout.atcam && rv)
        alloc.compute_hash_tables();
    if (!rv) alloc.clear();

    alloc.type = ternary ? Use::TERNARY_MATCH : Use::EXACT_MATCH;
    alloc.used_by = tbl->match_table->externalName();
    return rv;
}

unsigned IXBar::find_balanced_group(Use &alloc, int way_size) {
    std::map<unsigned, unsigned> sliceDepths;
    for (unsigned i = 0; i < alloc.way_use.size(); ++i) {
        int slice_group = INDEX_RANGE_SUBGROUP(alloc.way_use[i].index);
        int slice_ways  = (1U << bitvec(alloc.way_use[i].select_mask).popcount());
        sliceDepths[slice_group] += slice_ways;
    }
    unsigned minWayDepth = -1;
    int sel_group = -1;
    int way_bits = ceil_log2(way_size);
    for (unsigned i = 0; i < alloc.way_use.size(); ++i) {
        bitvec slice_way_mask(alloc.way_use[i].select_mask);
        int slice_way_bits = slice_way_mask.popcount();
        if (way_bits > slice_way_bits) continue;
        int slice_group = INDEX_RANGE_SUBGROUP(alloc.way_use[i].index);
        unsigned sliceDepth = way_size + sliceDepths[slice_group];
        if (sliceDepth < minWayDepth) {
            minWayDepth = sliceDepth;
            sel_group = slice_group;
        }
    }
    BUG_CHECK(sel_group > -1, "Cannot find a 'way' to reuse hash bits");
    return sel_group;
}
/**
 * This is to determine which hash group, of any of the 8 hash functions, are available.
 * A hash function is a set any of the 16 hash tables that will be XORed together.  The hash
 * functions themselves are 52 bits.
 *
 * So let's say we have the following example.
 *
 * A table has allocated 16 bytes of exact match in group 2, thus using hash tables 5-6
 * for the hash calculation.  Let's say it needs 3 ways.
 *
 * The hash function then becomes some bits between 0-29 bits.
 *
 * Another table, let's say an ATCAM table, only has 10 bits to consider.  In this case
 * the hash calculation would only require 1 way.  If these 2 bytes go to IXBAR group 7, bytes 0
 * and 1, then the hash function also would include hash table 14, from bits 30-39.
 *
 * Thus, overlaps between different tables are possible as long as hash functions do not collide.
 * This is opened up by the second check.
 */
int IXBar::getHashGroup(unsigned hash_table_input, const hash_matrix_reqs *hm_reqs) {
    // One can assume that the find legal bytes will have legal space within the hash function
    // If not the higher functions will find space
    for (int i = 0; i < HASH_GROUPS; i++) {
        if (hash_group_use[i] == hash_table_input) {
            return i;
        }
    }

    // Determining if the usage of hash bits can possibly overlap.  This is only necessary for
    // non-hash dist, this is handled with getHashDistGroups
    if (hm_reqs && !hm_reqs->hash_dist) {
        bitvec ht_usage;
        // Gather all the hash matrix currently in use, to verify that there is space for the
        // other bits to possibly share.
        for (auto idx : Range(0, HASH_INDEX_GROUPS - 1)) {
            if ((hash_index_inuse[idx] & hash_table_input) != 0)
                ht_usage.setrange(idx * RAM_LINE_SELECT_BITS, RAM_LINE_SELECT_BITS);
        }

        for (auto single_bit : Range(0, IXBar::get_hash_single_bits() - 1)) {
             if ((hash_single_bit_inuse[single_bit] & hash_table_input) != 0)
                 ht_usage.setbit(single_bit + RAM_SELECT_BIT_START);
        }

        for (auto ht : bitvec(hash_table_input))
            ht_usage |= hash_dist_bit_inuse[ht];
        LOG6("\t\tht_usage 0x" << ht_usage << " 0x" << hex(hash_table_input));

        bitvec hash_table_input_bv(hash_table_input);
        bitvec hash_matrix_usage_hf[HASH_GROUPS];
        bitvec hash_matrix_usage_collision[HASH_GROUPS];

        for (int i = 0; i < HASH_GROUPS; i++) {
            bool is_hash_dist_group = hash_dist_groups[0] == i || hash_dist_groups[1] == i;
            if (is_hash_dist_group) continue;
            bitvec max_usage(0, get_hash_matrix_size());
            bitvec curr_usage_hf;
            bitvec curr_usage_collision;

            // Gather up potential requirements from all other hash tables in use
            unsigned hf_ht = hash_group_use[i];
            unsigned collision_ht = hash_table_input & ~hash_group_use[i];
            LOG6("\t\thash group " << i << " 0x" << hex(hash_group_use[i]));
            LOG6("\t\twith_ht 0x" << hex(hf_ht) << " without_ht 0x" << hex(collision_ht));
            for (auto idx : Range(0, HASH_INDEX_GROUPS - 1)) {
                if ((hash_index_inuse[idx] & hf_ht) != 0)
                    curr_usage_hf.setrange(idx * RAM_LINE_SELECT_BITS,
                                                       RAM_LINE_SELECT_BITS);
                if ((hash_index_inuse[idx] & collision_ht) != 0)
                    curr_usage_collision.setrange(idx * RAM_LINE_SELECT_BITS, RAM_LINE_SELECT_BITS);
            }
            for (auto single_bit : Range(0, IXBar::get_hash_single_bits() - 1)) {
                if ((hash_single_bit_inuse[single_bit] & hf_ht) != 0)
                    curr_usage_hf.setbit(single_bit + RAM_SELECT_BIT_START);
                if ((hash_single_bit_inuse[single_bit] & collision_ht) != 0)
                    curr_usage_collision.setbit(single_bit + RAM_SELECT_BIT_START);
            }
            hash_matrix_usage_hf[i] = curr_usage_hf;
            hash_matrix_usage_collision[i] = curr_usage_collision;
            LOG6("\t\thash_matrix_usage " << i << " 0x" << hash_matrix_usage_hf[i]
                 << " 0x" << hash_matrix_usage_collision[i] << " 0x" << hex(hash_group_use[i]));
        }

        // Only pick a hash function that has the legal amount of space, as well as a hash
        // function that is already in use.
        for (int i = 0; i < HASH_GROUPS; i++) {
            if (hash_group_use[i] == 0) continue;
            bool is_hash_dist_group = hash_dist_groups[0] == i || hash_dist_groups[1] == i;
            if (is_hash_dist_group != hm_reqs->hash_dist)
                continue;
            if (!(hash_matrix_usage_hf[i] & hash_matrix_usage_collision[i]).empty())
                continue;
            if (hm_reqs->fit_requirements(hash_matrix_usage_hf[i] | hash_matrix_usage_collision[i]))
                return i;
        }

        // This is not a requirement, but in order to keep hash functions more available,
        // if an allocation that shares space with another hash function cannot share that
        // hash function, it's better if the allocation is not on those ixbar bytes.  The
        // second try will guarantee that the sharing is minimized
        for (int i = 0; i < HASH_GROUPS; i++) {
            if (hash_group_use[i] == 0) continue;
            if ((hash_group_use[i] & hash_table_input) != 0U) return -1;
        }
    }

    for (int i = 0; i < HASH_GROUPS; i++) {
        if (hash_group_use[i] == 0) {
            return i;
        }
    }

    LOG2("failed to allocate hash group");
    return -1;
}

/**
 * The purpose of this function is, given the choices of hash_tables (16 64 bit hashes)
 * used, what hash function (8x52 bit hashes) are available.  At most two options are
 * possible for hash distribution.
 */
void IXBar::getHashDistGroups(unsigned hash_table_input, int hash_group_opt[HASH_DIST_UNITS]) {
    std::set<int> groups_with_overlap;
    // If both groups overlap with the given requirement, then this will lead to hash collisions
    // and will not work
    for (int i = 0; i < HASH_DIST_UNITS; i++) {
        if (hash_dist_groups[i] >= 0 &&
            (hash_table_input & hash_group_use[hash_dist_groups[i]]) != 0U)
            groups_with_overlap.insert(i);
    }

    // If one group's hash table usealready overlaps, then use that particular group, and
    // don't use the other group
    if (groups_with_overlap.size() > 1) {
        return;
    } else if (groups_with_overlap.size() == 1) {
        for (int i = 0; i < HASH_DIST_UNITS; i++) {
            if (groups_with_overlap.count(i) > 0)
                hash_group_opt[i] = hash_dist_groups[i];
            else
                hash_group_opt[i] = -1;
        }
        return;
    }

    // A new group might be required, so create at most one new group
    bool first_unused = false;
    for (int i = 0; i < HASH_DIST_UNITS; i++) {
        if (hash_dist_groups[i] == -1) {
            if (!first_unused) {
                first_unused = true;
                hash_group_opt[i] = getHashGroup(hash_table_input);
            }
        } else {
            hash_group_opt[i] = hash_dist_groups[i];
        }
    }
}

/**
 * Really should be replaced by an extern function call
 */
void IXBar::determine_proxy_hash_alg(const PhvInfo &phv, const IR::MAU::Table *tbl,
                                     Use &alloc, int group) {
    bool hash_function_found = false;
    auto annot = tbl->match_table->getAnnotations();
    if (auto s = annot->getSingle("proxy_hash_algorithm"_cs)) {
        auto pragma_val = s->expr.at(0)->to<IR::StringLiteral>();
        if (pragma_val == nullptr) {
            error(ErrorType::ERR_INVALID,
                    "proxy_hash_algorithm pragma on table %1% must be a string", tbl);
        } else if (alloc.proxy_hash_key_use.algorithm.setup(pragma_val)) {
            hash_function_found = true;
        }
    } else if (tbl->layout.proxy_hash_algorithm.size != 0) {
        hash_function_found = true;
        alloc.proxy_hash_key_use.algorithm = tbl->layout.proxy_hash_algorithm;
    }
    if (!hash_function_found) {
        alloc.proxy_hash_key_use.algorithm = IR::MAU::HashFunction::random();
    }

    if (alloc.proxy_hash_key_use.algorithm.type == IR::MAU::HashFunction::IDENTITY)
        error(ErrorType::ERR_UNSUPPORTED,
                "A proxy hash table with an identity algorithm is not supported, as specified "
                "on table %1%.  Using a normal exact match table.", tbl);

    int start_bit = alloc.proxy_hash_key_use.hash_bits.ffs();
    std::map<int, le_bitrange> bit_starts;
    int bits_seen = 0;
    do {
        int end_bit = alloc.proxy_hash_key_use.hash_bits.ffz(start_bit);
        int p4_lo = bits_seen;
        int p4_hi = p4_lo + end_bit - start_bit - 1;
        bit_starts[start_bit] = { p4_lo, p4_hi };
        bits_seen += end_bit - start_bit;
        start_bit = alloc.proxy_hash_key_use.hash_bits.ffs(end_bit);
    } while (start_bit >= 0);
    alloc.hash_seed[group]
        |= determine_final_xor(&alloc.proxy_hash_key_use.algorithm, phv, bit_starts,
                               alloc.field_list_order, alloc.total_input_bits());
}

/**
 * This function is to determine the galois matrix/ixbar requirements for the key to be
 * used in the hash matrix function.  This is responsible for programming the
 * proxy_hash_key_use portion of a Use object.
 *
 * This specifically coordinates with the proxy_hash_function JSON node in the compiler
 */
bool IXBar::allocProxyHashKey(const IR::MAU::Table *tbl, const PhvInfo &phv,
        const LayoutOption *lo, Use &alloc, hash_matrix_reqs &hm_reqs) {
    if (tbl->match_key.empty()) return true;
    ContByteConversion map_alloc;
    std::map<cstring, bitvec> fields_needed;
    safe_vector<Use::Byte *> alloced;
    KeyInfo ki;
    for (auto ixbar_read : tbl->match_key) {
        if (!ixbar_read->for_match())
            continue;
        FieldManagement(&map_alloc, alloc.field_list_order, ixbar_read, &fields_needed,
                        phv, ki, tbl);
    }

    create_alloc(map_alloc, alloc);
    LOG3("need " << alloc.use.size() << " bytes for proxy hash key of table " << tbl->name);

    bool rv = find_alloc(alloc.use, false, alloced, hm_reqs);
    if (!rv) {
        alloc.clear();
        return false;
    }

    unsigned hash_table_input = alloc.compute_hash_tables();
    int hash_group = getHashGroup(hash_table_input);
    if (hash_group < 0) {
        alloc.clear();
        return false;
    }

    // Determining what bits are available are within the hash matrix
    bitvec unavailable_bits;
    for (int index = 0; index < HASH_INDEX_GROUPS; index++) {
        for (auto ht : bitvec(hash_table_input)) {
            if ((1 << ht) & hash_index_inuse[index]) {
                unavailable_bits.setrange(index * RAM_LINE_SELECT_BITS, RAM_LINE_SELECT_BITS);
            }
        }
    }
    for (int bit = 0; bit < IXBar::get_hash_single_bits(); bit++) {
        for (auto ht : bitvec(hash_table_input)) {
            if ((1 << ht) & hash_single_bit_inuse[bit]) {
                unavailable_bits.setbit(bit + RAM_SELECT_BIT_START);
            }
        }
    }

    bitvec available_bits = bitvec(0, get_hash_matrix_size()) - unavailable_bits;
    if (available_bits.popcount() < lo->layout.match_width_bits) {
        alloc.clear();
        return false;
    }

    // Breaking up the possible available bits into bytes, which will be the bytes
    // compared in the match format
    safe_vector<std::pair<int, bitvec>> hash_bytes_used;
    for (int i = 0; i < get_hash_matrix_size(); i+= 8) {
        bitvec value = available_bits.getslice(i, 8);
        if (value.empty())
            continue;
        hash_bytes_used.emplace_back(i, value);
    }

    // Pick the bytes that have the most available bits to perform a comparison
    std::sort(hash_bytes_used.begin(), hash_bytes_used.end(),
              [](const std::pair<int, bitvec> &a, const std::pair<int, bitvec> &b) {
        int t;
        if ((t = a.second.popcount() - b.second.popcount()) != 0)
            return t > 0;
        return a.first < b.first;
    });

    bitvec used_bits;
    for (auto pair : hash_bytes_used) {
        int bits_required = lo->layout.match_width_bits - used_bits.popcount();
        if (bits_required == 0)
            break;
        else if (bits_required < 0)
            BUG("Proxy hash calculation error in ixbar for table %s", tbl->name);
        bitvec possible_byte_bits = pair.second;
        bitvec unused_byte_bits;
        int extra_bits = possible_byte_bits.popcount() - bits_required;
        if (extra_bits > 0) {
            int bits_removed = 0;
            for (auto bit : possible_byte_bits) {
                if (bits_removed == extra_bits)
                    break;
                unused_byte_bits.setbit(bit);
                bits_removed++;
            }
        }
        bitvec used_byte_bits = possible_byte_bits - unused_byte_bits;
        used_bits |= (used_byte_bits << pair.first);
    }

    // Write all of the data structures
    unsigned indexes = index_groups_used(used_bits);
    for (auto idx : bitvec(indexes)) {
        for (auto ht : bitvec(hash_table_input))
            hash_index_use[ht][idx] = tbl->name + "$proxy_hash";
        hash_index_inuse[idx] = hash_table_input;
    }
    unsigned select_bits = select_bits_used(used_bits);
    for (auto bit : bitvec(select_bits)) {
        for (auto ht : bitvec(hash_table_input))
            hash_single_bit_use[ht][bit] = tbl->name + "$proxy_hash";
        hash_single_bit_inuse[bit] = hash_table_input;
    }
    hash_group_use[hash_group] |= hash_table_input;
    update_hash_parity(alloc, hash_group);

    fill_out_use(alloced, false);
    alloc.hash_table_inputs[hash_group] = hash_table_input;
    alloc.proxy_hash_key_use.allocated = true;
    alloc.proxy_hash_key_use.group = hash_group;
    alloc.proxy_hash_key_use.hash_bits = used_bits;
    determine_proxy_hash_alg(phv, tbl, alloc, hash_group);
    return true;
}

/**
 * A proxy hash table is an implementation of a match table where instead of matching directly
 * on a key, the match is on a calculated hash of that key.
 *
 * In the diagram in the uArch section 6.2.3 Exact Match Row Vertical/Horizontal (VH) bars,
 * a search bus can be sourced from either any of the 8 input xbar groups, or any of the 8
 * hash groups.  A standard match table would match on the search data, while a proxy hash
 * matches on the hash data.
 *
 * The proxy hash table is a choice for the P4 programmer.  A proxy hash can pack more
 * tightly than a standard match table, as the hash bits can be significantly smaller than
 * match bits would be.  This of course comes at the risks of collisions on entries.
 *
 * A proxy hash table requires two hash functions, one to determine the RAM position of the
 * key (similar to any SRam based match table), and a second hash for the comparison of the
 * hash key.
 */
bool IXBar::allocProxyHash(const IR::MAU::Table *tbl, const PhvInfo &phv, const LayoutOption *lo,
        Use &alloc, Use &proxy_hash_alloc) {
    if (!lo) return false;
    safe_vector<IXBar::Use::Byte *> alloced;
    safe_vector<Use> all_tbl_allocs;
    bool finished = false;
    size_t start = 0; size_t last = 0;
    while (!finished) {
        Use next_alloc;
        layout_option_calculation(lo, start, last);
        /* Essentially a calculation of how much space is potentially available */
        auto hm_reqs = match_hash_reqs(lo, start, last, false);
        auto max_hm_reqs = hash_matrix_reqs::max(false, false);

        if (!(allocMatch(false, tbl, phv, next_alloc, alloced, hm_reqs)
            && allocAllHashWays(false, tbl, next_alloc, lo, start, last, hm_reqs))
            && !(allocMatch(false, tbl, phv, next_alloc, alloced, max_hm_reqs)
            && allocAllHashWays(false, tbl, next_alloc, lo, start, last, max_hm_reqs))) {
            next_alloc.clear();
            alloc.clear();
            return false;
        } else {
           fill_out_use(alloced, false);
        }
        alloced.clear();
        all_tbl_allocs.push_back(next_alloc);
        if (last == lo->way_sizes.size())
            finished = true;
    }
    for (auto a : all_tbl_allocs) {
        alloc.add(a);
    }

    this->add_collisions();  //  added 8/19/2022 ~10pm NY time
                             //  based on a pair-debugging session with Chris

    hash_matrix_reqs ph_hm_reqs;
    ph_hm_reqs.index_groups = max_index_group(lo->layout.match_width_bits);
    ph_hm_reqs.select_bits = max_index_single_bit(lo->layout.match_width_bits);
    auto max_ph_hm_reqs = hash_matrix_reqs::max(false, false);

    if (!allocProxyHashKey(tbl, phv, lo, proxy_hash_alloc, ph_hm_reqs)
        && !allocProxyHashKey(tbl, phv, lo, proxy_hash_alloc, max_ph_hm_reqs)) {
        alloc.clear();
        proxy_hash_alloc.clear();
        return false;
    }
    return true;
}


/* Allocate all hashes used within a hash group of a table. The number of hashes in the
   hash group are determined by the layout option */
bool IXBar::allocAllHashWays(bool ternary, const IR::MAU::Table *tbl, Use &alloc,
        const LayoutOption *layout_option, size_t start, size_t last,
        const hash_matrix_reqs &hm_reqs) {
    if (!layout_option) return false;
    if (ternary)
        return true;
    unsigned local_hash_table_input = alloc.compute_hash_tables();
    int hash_group = getHashGroup(local_hash_table_input, &hm_reqs);
    if (hash_group < 0) {
        alloc.clear();
        return false;
    }
    unsigned hf_hash_table_input = local_hash_table_input | hash_group_use[hash_group];
    LOG3("\thash_group " << hash_group << " 0x" << hex(local_hash_table_input) <<  " 0x"
         << hex(hf_hash_table_input));
    int free_groups = 0;
    int group;
    for (group = 0; group < HASH_INDEX_GROUPS; group++) {
        if (!(hash_index_inuse[group] & hf_hash_table_input)) {
            free_groups++;
        }
    }

    if (free_groups == 0) {
        alloc.clear();
        return false;
    }

    int way_bits_needed = 0;
    for (auto &way : tbl->ways) {
        way_bits_needed += ceil_log2(way.entries/1024U/way.match_groups);
    }
    int way_bits = 0;
    for (int bit = 0; bit < IXBar::get_hash_single_bits(); bit++) {
        if (!(hash_single_bit_inuse[bit] & hf_hash_table_input)) {
            way_bits++;
        }
    }

    if (way_bits == 0 && way_bits_needed > 0) {
        alloc.clear();
        return false;
    }

    std::map<int, bitvec> slice_to_select_bits;
    // Currently should  never return false
    for (size_t index = start; index < last; index++) {
        if (!allocHashWay(tbl, layout_option, index, slice_to_select_bits, alloc,
                          local_hash_table_input, hf_hash_table_input, hash_group)) {
            alloc.clear();
            return false;
        }
    }
    /* No longer does a logical table have one hash_table_input, but a couple if the
       table requires multiple hash groups */
    alloc.hash_table_inputs[hash_group] = local_hash_table_input;
    hash_group_use[hash_group] |= local_hash_table_input;
    update_hash_parity(alloc, hash_group);

    // If a random_seed is specified via pragma, do not generate the random seed here.
    if (tbl->random_seed >= 0)
        return true;

    LOG3("IXBarRandom seed: " << IXBarRandom::seed);
    if (layout_option->identity) {
        LOG3(" Used as identity function.");
        return true;
    }

    std::map<unsigned, std::set<std::pair<unsigned, unsigned>>> requiredSeedCombinations;
    for (auto way : alloc.way_use)
        requiredSeedCombinations[way.source]
            .emplace(INDEX_RANGE_SUBGROUP(way.index), way.select_mask);

    for (auto kv : requiredSeedCombinations) {
        unsigned group = kv.first;
        for (auto v : kv.second) {
            LOG3("  Way details: " << kv.first << ", " << v.first << ", " << v.second);
            unsigned slice = v.first;
            unsigned mask = v.second;
            unsigned random_number = IXBarRandom::nextRandomNumber(RAM_LINE_SELECT_BITS);
            bitvec random_seed = bitvec(random_number) << (RAM_LINE_SELECT_BITS * slice);
            bitvec mask_bits(mask);
            bitvec mask_seed;
            for (auto b : mask_bits) {
                unsigned random_bit = IXBarRandom::nextRandomNumber();
                bitvec random_bit_seed = bitvec(random_bit) << b;
                mask_seed |= random_bit_seed;
            }
            LOG3("    Random number: " << random_number << ", Random seed: " << random_seed <<
                 ", Mask bits: " << mask_bits << ", Mask seed: " << (mask_seed << 40));
            random_seed |= (mask_seed << 40);
            LOG3("  Random seed: " << random_seed);
            alloc.hash_seed[group] |= random_seed;
        }
    }
    return true;
}

/* Individual Hash way allocated, called from allocAllHashWays.  Sets up the select bit
   mask provided by the layout option */
bool IXBar::allocHashWay(const IR::MAU::Table *tbl, const LayoutOption *layout_option,
        size_t index, std::map<int, bitvec> &slice_to_select_bits, Use &alloc,
        unsigned local_hash_table_input, unsigned hf_hash_table_input, int hash_group) {
    int way_size = layout_option->way_sizes[index];
    int way_bits = ceil_log2(way_size);
    int group;
    unsigned way_mask = 0;
    bool shared = false;
    LOG3("\tNeed " << way_bits << " mask bits for way " << alloc.way_use.size() <<
         " in table " << tbl->name);
    for (group = 0; group < HASH_INDEX_GROUPS; group++) {
        if (!(hash_index_inuse[group] & hf_hash_table_input)) {
            break; } }
    if (group >= HASH_INDEX_GROUPS) {
        if (alloc.way_use.empty()) {
            group = 0;  // share with another table?
            BUG("Group was allocated with no available space to push hash ways");
        } else {
            group = find_balanced_group(alloc, way_size);
            shared = true;
        }
        LOG3("all hash slices in use, reusing " << group); }
    // Calculation of the separate select bits among many stages
    unsigned free_bits = 0; unsigned used_bits = 0;

    for (int bit = 0; bit < IXBar::get_hash_single_bits(); bit++) {
        if (!(hash_single_bit_inuse[bit] & hf_hash_table_input)) {
            free_bits |= 1U << bit;
        }
    }
    for (auto &way_use : alloc.way_use) {
        BUG_CHECK(way_use.select.lo == RAM_SELECT_BIT_START, "invalid select range for tofino");
        used_bits |= way_use.select_mask;
    }

    if (way_bits == 0) {
        way_mask = 0;
    } else if (shared) {
        BUG_CHECK(slice_to_select_bits.count(group) > 0,
                "Slice must be allocated before to be shared");
        bitvec prev_mask = slice_to_select_bits.at(group);
        if (prev_mask.popcount() < way_bits) {
            BUG("Slice to be shared is bigger than previous allocation");
        }
        way_mask = bitvec(prev_mask.min().index(),
                          way_bits).getrange(0, IXBar::get_hash_single_bits());
        BUG_CHECK(bitvec(way_mask).is_contiguous(), "Previous slice allocation "
                 "was not contiguous");
    } else if (static_cast<int>(bitcount(free_bits)) < way_bits) {
        LOG3("\tFree bits available is too small");
        return false;
    } else {
        // Select bits are not required to be contiguous in hardware, but in the driver entry
        // it appears that they have to be
        bool found = false;
        bitvec free_bits_bv = bitvec(free_bits);
        for (auto br : bitranges(free_bits_bv)) {
            if (br.second - br.first + 1 >= way_bits) {
                way_mask = ((1U << way_bits) - 1) << br.first;
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }

    slice_to_select_bits[group] |= bitvec(way_mask);

    alloc.way_use.emplace_back(hash_group, INDEX_BIT_RANGE(group), SELECT_BIT_RANGE, way_mask);
    LOG3("\tAllocation of way " << hash_group << " " << group << " 0x" << hex(way_mask));
    hash_index_inuse[group] |= hf_hash_table_input;
    for (auto bit : bitvec(way_mask)) {
        hash_single_bit_inuse[bit] |= hf_hash_table_input;
    }
    for (auto ht : bitvec(local_hash_table_input)) {
        hash_index_use[ht][group] = tbl->name;
        for (auto bit : bitvec(way_mask)) {
            hash_single_bit_use[ht][bit] = tbl->name;
        }
    }
    for (auto ht : bitvec(hf_hash_table_input & ~local_hash_table_input)) {
        hash_index_use[ht][group] = "$collision"_cs;
        for (auto bit : bitvec(way_mask)) {
            hash_single_bit_use[ht][bit] = "$collision"_cs;
        }
    }

    return true;
}

/** Allocate the partition index of an algorithmic TCAM table.  Unlike an exact match table
 *  in which every single bit needs to be in the hash matrix, only the partition index
 *  appears within the hash matrix.  Thus the algorithm only calculates the hash information
 *  for the partition, and once it is completed, only then will it append it to the rest
 *  of the match alloc.
 */
bool IXBar::allocPartition(const IR::MAU::Table *tbl, const PhvInfo &phv, Use &match_alloc,
                           bool second_try) {
    if (tbl->match_key.empty()) return true;
    ContByteConversion map_alloc;
    Use alloc;
    std::map<cstring, bitvec> fields_needed;
    safe_vector<Use::Byte *> alloced;
    hash_matrix_reqs hm_reqs;

    if (second_try) {
        hm_reqs = hash_matrix_reqs::max(false);
    } else {
        hm_reqs.index_groups = 1;
        hm_reqs.select_bits = std::max(tbl->layout.partition_bits - TableFormat::RAM_GHOST_BITS, 0);
    }

    KeyInfo ki;
    ki.is_atcam = true;
    ki.partition = true;
    ki.partition_bits = tbl->layout.partition_bits;
    for (auto ixbar_read : tbl->match_key) {
        if (!ixbar_read->for_match())
            continue;
        FieldManagement(&map_alloc, alloc.field_list_order, ixbar_read, &fields_needed,
                        phv, ki, tbl);
    }
    LOG3("partition_bits " << tbl->layout.partition_bits);
    create_alloc(map_alloc, alloc);
    BUG_CHECK(alloc.use.size() > 0, "No partition index found for %1%", tbl);

    bool rv = find_alloc(alloc.use, false, alloced, hm_reqs);
    alloc.type = Use::ATCAM_MATCH;
    alloc.used_by = tbl->match_table->externalName();

    unsigned local_hash_table_input = 0;
    if (!rv) {
        alloc.clear();
        return rv;
    } else {
        local_hash_table_input = alloc.compute_hash_tables();
    }

    int hash_group = getHashGroup(local_hash_table_input, &hm_reqs);
    if (hash_group < 0) {
        alloc.clear();
        return false;
    }
    unsigned hf_hash_table_input = local_hash_table_input | hash_group_use[hash_group];

    int group = -1;
    for (int i = 0; i < HASH_INDEX_GROUPS; i++) {
        if ((hash_index_inuse[i] & hf_hash_table_input) == 0) {
            group = i;
            break;
        }
    }

    if (group == -1) {
        alloc.clear();
        return false;
    }

    int bits_needed = std::max(tbl->layout.partition_bits - 10, 0);
    int bits_found = 0;
    unsigned way_mask = 0;
    for (int i = 0; i < IXBar::get_hash_single_bits(); i++) {
        if (bits_found >= bits_needed)
            break;
        if ((hash_single_bit_inuse[i] & hf_hash_table_input) == 0) {
            way_mask |= (1 << i);
            bits_found++;
        }
    }

    if (bits_found < bits_needed) {
        alloc.clear();
        return false;
    }

    /** The partition fits.  Just update all of the values */
    match_alloc.use.insert(match_alloc.use.end(), alloc.use.begin(), alloc.use.end());
    match_alloc.type = alloc.type;
    match_alloc.hash_table_inputs[hash_group] = local_hash_table_input;
    hash_group_use[hash_group] |= local_hash_table_input;
    update_hash_parity(match_alloc, hash_group);
    match_alloc.way_use.emplace_back(hash_group, INDEX_BIT_RANGE(group),
                                     SELECT_BIT_RANGE, way_mask);
    hash_index_inuse[group] |= hf_hash_table_input;
    for (auto bit : bitvec(way_mask)) {
        hash_single_bit_inuse[bit] |= hf_hash_table_input;
    }
    for (auto ht : bitvec(local_hash_table_input)) {
        hash_index_use[ht][group] = tbl->name;
        for (auto bit : bitvec(way_mask)) {
            hash_single_bit_use[ht][bit] = tbl->name;
        }
    }
    for (auto ht : bitvec(hf_hash_table_input & ~local_hash_table_input)) {
        hash_index_use[ht][group] = tbl->name;
        for (auto bit : bitvec(way_mask)) {
            hash_single_bit_use[ht][bit] = tbl->name;
        }
    }
    fill_out_use(alloced, false);
    return true;
}

bool IXBar::allocGateway(const IR::MAU::Table *tbl, const PhvInfo &phv, Use &alloc,
                         const LayoutOption *lo, bool second_try) {
    alloc.gw_search_bus = false; alloc.gw_hash_group = false;
    alloc.gw_search_bus_bytes = 0;
    int hash_bus_bits = 0;
    bool xor_required = false;
    CollectGatewayFields *collect;
    alloc.type = Use::GATEWAY;
    alloc.used_by = tbl->build_gateway_name();

    if (lo && lo->layout.gateway_match) {
        collect = new CollectMatchFieldsAsGateway(phv, &alloc);
    } else {
        collect = new CollectGatewayFields(phv, &alloc);
    }

    tbl->apply(*collect);
    if (collect->info.empty()) return true;

    ContByteConversion map_alloc;

    for (auto &info : collect->info) {
        int flags = 0;

        if (!info.second.xor_with.empty()) {
            flags |= IXBar::Use::NeedXor;
            // FIXME: This need to be coordinated with the actual PHV!!!
            xor_required = true;
        } else if (info.second.need_range) {
            flags |= IXBar::Use::NeedRange;
            hash_bus_bits += info.first.size();
        } else if (info.second.valid_bit) {
            hash_bus_bits += 1;
        }
        cstring aliasSourceName;
        if (collect->info_to_uses.count(&info.second)) {
            LOG5("Found gateway alias source name");
            aliasSourceName = collect->info_to_uses[&info.second];
        }
        if (aliasSourceName)
            add_use(map_alloc, info.first.field(), phv, tbl, aliasSourceName,
                    &info.first.range(), flags, NO_BYTE_TYPE);
        else
            add_use(map_alloc, info.first.field(), phv, tbl, std::nullopt,
                    &info.first.range(), flags, NO_BYTE_TYPE);
    }
    safe_vector<IXBar::Use::Byte *> xbar_alloced;

    create_alloc(map_alloc, alloc);

    hash_matrix_reqs hm_reqs;
    if (second_try) {
        hm_reqs.select_bits = IXBar::get_hash_single_bits();
    } else {
        hm_reqs.select_bits = hash_bus_bits;
    }
    // More constraied than necessary, if the gateway only requires hash bits, but the
    // search bus requirements must be on one and only one search bus
    hm_reqs.max_search_buses = 1;

    if (!find_alloc(alloc.use, false, xbar_alloced, hm_reqs)) {
        alloc.clear();
        return false; }
    if (!collect->compute_offsets()) {
        alloc.clear();
        LOG3("collect.compute_offsets failed?");
        return false; }

    if (collect->bits) {
        alloc.gw_hash_group = true;
    }
    if (collect->bytes > 0) {
        alloc.gw_search_bus = true;
        alloc.gw_search_bus_bytes = collect->bytes;
        if (xor_required)
            alloc.gw_search_bus_bytes += GATEWAY_SEARCH_BYTES;
    }

    if (collect->bits > 0) {
        // Per use hash table information vs. potential shared across the hash function
        // information.  Local_hash_table_input is per use, hash_table_input is for all
        // hashtables in the group
        int local_hash_table_input = alloc.compute_hash_tables();
        int hash_group = getHashGroup(local_hash_table_input, &hm_reqs);

        if (hash_group < 0) {
            alloc.clear();
            return false; }
        int hf_hash_table_input = local_hash_table_input | hash_group_use[hash_group];
        /* FIXME -- don't need use all hash tables that we're using the ixbar for -- just those
         * tables for bytes we want to put through the hash table to get into the upper gw bits */
        unsigned avail = 0;
        unsigned need = (1U << collect->bits) - 1;
        for (auto j : Range(0, IXBar::get_hash_single_bits() - 1)) {
            if ((hash_single_bit_inuse[j] & hf_hash_table_input) == 0) {
                avail |= (1U << j);
            }
        }
        int shift = 0;
        while (((avail >> shift) & need) != need && shift < 12) {
            shift += 4;
        }
        if (((avail >> shift) & need) != need) {
            alloc.clear();
            LOG3("failed to find " << collect->bits << " continuous nibble aligned bits in 0x" <<
                 hex(avail));
            return false; }
        for (auto &info : collect->info) {
            for (auto &offset : info.second.offsets) {
                if (offset.first < 32) continue;
                offset.first += shift;
                if (info.second.valid_bit) {
                    alloc.bit_use.emplace_back(info.first.field()->name, hash_group,
                                               offset.second.lo, offset.first - 32, 1,
                                               info.second.valid_bit);
                } else {
                    alloc.bit_use.emplace_back(info.first.field()->name, hash_group,
                                               offset.second.lo, offset.first - 32,
                                               offset.second.size()); } } }
        for (auto ht : bitvec(local_hash_table_input))
            for (int i = 0; i < collect->bits; ++i)
                hash_single_bit_use[ht][shift + i] = tbl->name + "$gw";
        for (auto ht : bitvec(hf_hash_table_input & ~local_hash_table_input))
            for (int i = 0; i < collect->bits; ++i)
                hash_single_bit_use[ht][shift + i] = "$collision"_cs;
        for (int i = 0; i < collect->bits; ++i)
            hash_single_bit_inuse[shift + i] |= hf_hash_table_input;
        alloc.hash_table_inputs[hash_group] = local_hash_table_input;
        hash_group_use[hash_group] |= local_hash_table_input;
        update_hash_parity(alloc, hash_group);
    }
    fill_out_use(xbar_alloced, false);
    for (int bit = 0; bit < IXBar::get_hash_single_bits(); bit++) {
        LOG3("\tHash bit at bit " << bit << " is " << hex(hash_single_bit_inuse[bit]));
    }
    return true;
}

/**
 * For a meter alu input through hash, the hash output goes through a byte mask.  Thus
 * the hash matrix must reserve up to the byte position of the upper most bit
 */
int IXBar::max_bit_to_byte(bitvec bit_mask) {
    int max_bit = bit_mask.max().index();
    int rv = ((max_bit + 8) / 8) * 8 - 1;
    rv = std::min(rv, get_meter_alu_hash_bits());
    return rv;
}

int IXBar::max_index_group(int max_bit) {
    int total_max = (max_bit + TableFormat::RAM_GHOST_BITS - 1) / TableFormat::RAM_GHOST_BITS;
    return std::min(total_max, HASH_INDEX_GROUPS);
}

int IXBar::max_index_single_bit(int max_bit) {
    int max_single_bit = max_bit - TableFormat::RAM_GHOST_BITS * HASH_INDEX_GROUPS;
    max_single_bit = std::max(max_single_bit, 0);
    BUG_CHECK(max_single_bit <= IXBar::get_hash_single_bits(),
              "Requesting a bit beyond the size of the Galois matrix");
    return max_single_bit;
}

bool IXBar::hash_use_free(int max_group, int max_single_bit, unsigned hash_table_input) {
    for (int i = 0; i < HASH_TABLES; i++) {
        if (((1 << i) & hash_table_input) == 0)
            continue;
        for (int j = 0; j < max_group; j++) {
            if ((hash_index_inuse[j] & (1 << i)) != 0)
                return false;
        }
        for (int j = 0; j < max_single_bit; j++) {
            if ((hash_single_bit_inuse[j] & (1 << i)) != 0)
                return false;
        }
    }
    return true;
}

void IXBar::write_hash_use(int max_group, int max_single_bit, unsigned hash_table_input,
        cstring name) {
    for (int i = 0; i < HASH_TABLES; i++) {
        if (((1 << i) & hash_table_input) == 0)
            continue;
        for (int j = 0; j < max_group; j++) {
            hash_index_use[i][j] = name;
            hash_index_inuse[j] |= (1 << i);
        }
        for (int j = 0; j < max_single_bit; j++) {
            hash_single_bit_use[i][j] = name;
            hash_single_bit_inuse[j] |= (1 << i);
        }
    }
}

/** The Meter ALU, responsible for any meters, selectors, or stateful ALU operations
 *  potentially needs information from PHV in order to perform the associated
 *  calculations.  The allocSelector, allocMeter, and allocStateful are to determine
 *  the requirements and the allocation of these values to the ALU.
 *
 *  Data can reach the ALU through two pathways, either through a search bus or after
 *  a hash calculation.
 *
 *  The pathway for the search bus is described in section 6.2.3 Exact Match Row
 *  Veritical/Horizontal (VH) Xbars.  In the diagram, on each row, a bus headed directly
 *  to the meter ALU block can come from any 8 of the input xbar groups.  In Tofino,
 *  the upper 8 bytes go through, while on JBay, the entirety of the input xbar group
 *  can go through.  However, unlike the normal search buses, there is no byte swizzler.
 *  The group is ANDed with a byte_mask
 *
 *  The hash pathway is described in section 6.2.4.8.1 Exact Match RAM Addressing.  The Galois
 *  matrix can be used to calculate any kind of hash, or if the alignment on the search bus
 *  is not possible, then the hash matrix can be used to position any of the search bus
 *  results into the correct position.  However, the limitation is that this can only
 *  process up to 51 bits, rather than the full range occasionally needed by wide stateful
 *  ALU tables
 */

/** Selector allocation algorithm.  Currently reserves an entire section of hash matrix,
 *  even if the selector is only on fair mode, rather than resilient
 */
bool IXBar::allocSelector(const IR::MAU::Selector *as, const IR::MAU::Table *tbl,
                          const PhvInfo &phv, Use &alloc, cstring name) {
    auto pos = allocated_attached.find(as);
    if (pos != allocated_attached.end()) {
        alloc = dynamic_cast<const Use &>(pos->second);
        return true;
    }

    safe_vector<IXBar::Use::Byte *>  alloced;
    ContByteConversion map_alloc;
    std::map<cstring, bitvec>        fields_needed;
    KeyInfo ki;
    for (auto ixbar_read : tbl->match_key) {
        if (!ixbar_read->for_selection()) continue;
        FieldManagement(&map_alloc, alloc.field_list_order, ixbar_read->expr, &fields_needed,
                        phv, ki, tbl);
    }
    create_alloc(map_alloc, alloc);

    LOG3("need " << alloc.use.size() << " bytes for table " << tbl->name);
    hash_matrix_reqs hm_reqs = hash_matrix_reqs::max(false);
    bool rv = find_alloc(alloc.use, false, alloced, hm_reqs);
    unsigned local_hash_table_input = 0;
    if (rv)
         local_hash_table_input = alloc.compute_hash_tables();
    if (!rv) alloc.clear();
    if (!rv) return false;

    alloc.type = Use::SELECTOR;
    alloc.used_by = as->name + "";
    alloc.symmetric_keys = tbl->sel_symmetric_keys;

    int hash_group = getHashGroup(local_hash_table_input);
    if (hash_group < 0) {
        alloc.clear();
        return false;
    }
    unsigned hf_hash_table_input = local_hash_table_input | hash_group_use[hash_group];

    auto &mah = alloc.meter_alu_hash;

    mah.allocated = true;
    mah.group = hash_group;
    mah.algorithm = as->algorithm;
    int mode_width_bits = as->mode == IR::MAU::SelectorMode::RESILIENT ?
                                      RESILIENT_MODE_HASH_BITS : FAIR_MODE_HASH_BITS;
    if (mah.algorithm.size_from_algorithm()) {
        if (mode_width_bits > mah.algorithm.size) {
            // FIXME: Debatably be moved to an error, but have to wait on a p4-tests update
            warning(ErrorType::WARN_OVERFLOW,
                      "%1%: The algorithm for selector %2% has a size of %3% bits, when the mode "
                      "selected requires %4% bits. Padding with zeros.",
                      as, as->name, mah.algorithm.size, mode_width_bits);
            // alloc.clear();
            // return false;
        }
    }

    // Mark the positions of each of the fields on the identity hash, so that the
    // asm_output can easily locate the positions of all of the the associated fields
    if (mah.algorithm.type == IR::MAU::HashFunction::IDENTITY) {
        int bits_seen = 0;
        for (auto it = alloc.field_list_order.rbegin(); it != alloc.field_list_order.rend(); it++) {
            auto *expr = *it;
            int diff = bits_seen + expr->type->width_bits() - mode_width_bits;
            if (diff > 0)
                expr = MakeSlice(expr, 0, expr->type->width_bits() - diff - 1);
            mah.computed_expressions.emplace(bits_seen, expr);
            bits_seen += expr->type->width_bits();
            if (bits_seen >= mode_width_bits)
                break;
        }

        if (bits_seen < mode_width_bits) {
            // FIXME: See above at previous FIXME
            warning(ErrorType::WARN_OVERFLOW,
                      "%1%: The identity hash for selector %2% has a size of %3% bits, "
                      "when the mode selected required %4% bits. Padding with zeros.",
                      as, as->name, bits_seen, mode_width_bits);
            // alloc.clear();
            // return false;
        }
    }
    mah.bit_mask.setrange(0, mode_width_bits);
    alloc.hash_table_inputs[hash_group] = local_hash_table_input;
    int max_bit = max_bit_to_byte(mah.bit_mask);
    int max_group = max_index_group(max_bit);
    int max_single_bit = max_index_single_bit(max_bit);
    BUG_CHECK(hash_use_free(max_group, max_single_bit, hf_hash_table_input), "The calculation for "
              "the hash matrix should be completely free at this point");
    write_hash_use(max_group, max_single_bit, local_hash_table_input, alloc.used_by);
    write_hash_use(max_group, max_single_bit, hf_hash_table_input & ~local_hash_table_input,
                   "$collision"_cs);
    fill_out_use(alloced, false);
    hash_group_print_use[hash_group] = name;
    hash_group_use[hash_group] |= local_hash_table_input;
    update_hash_parity(alloc, hash_group);
    allocated_attached.emplace(as, alloc);
    return rv;
}

/**
 *  Given a field range, and a byte position to start the allocation on the ixbar, can that
 *  field be allocated starting from lo to hi within that search bus position.
 */
bool IXBar::can_allocate_on_search_bus(IXBar::Use &alloc, const PHV::Field *field,
        le_bitrange range, int ixbar_position) {
    bitvec seen_bits;

    for (auto &byte : alloc.use) {
        // FIXME -- should be iterating over field_bytes here rather than just looking at [0]
        auto &fi = byte.field_bytes[0];
        if (fi.field != field->name)
            continue;
        if (!fi.range().overlaps(range))
            continue;
        // Because the mask is per byte rather than per bit, the rest of the bit needs to be empty
        if (byte.bit_use != byte.non_zero_bits) {
            LOG4("  other data packed in byte: " << byte);
            return false; }
        if (byte.field_bytes.size() > 1)
            return false;
        if (fi.width() != 8 && fi.hi != range.hi) {
            LOG4("  not byte aligned: " << fi);
            return false; }
        if (fi.cont_loc().min().index() != 0) {
            LOG4("  not in bit 0 of byte");
            return false; }
        int byte_position = ((fi.lo - range.lo) + 7)/ 8;
        // Validate that the byte, given the PHV allocation for that particular byte
        if (align_flags[(ixbar_position + byte_position) & 3] & byte.flags) {
            LOG4("  not aligned within container for ixbar");
            return false; }
        seen_bits.setrange(fi.lo, fi.width());
    }
    if (!seen_bits.is_contiguous() || !(seen_bits.min().index() == range.lo)
        || !(seen_bits.max().index() == range.hi)) {
        LOG4("  not the right bits for the range");
        return false; }
    return true;
}

/** Allocation of the meter input xbar if it is an LPF or WRED.  On Tofino, specifically have
 *  to reserve bytes 8-11 of a specific input xbar group currently.
 *
 *  Similar to stateful ALU allocation, except in the LPF/WRED case it is only one field,
 *  and thus one input xbar source
 */
bool IXBar::allocMeter(const IR::MAU::Meter *mtr, const IR::MAU::Table *tbl, const PhvInfo &phv,
                       Use &alloc, bool on_search_bus) {
    auto pos = allocated_attached.find(mtr);
    if (pos != allocated_attached.end()) {
        alloc = dynamic_cast<const Use &>(pos->second);
        return true;
    }

    if (!mtr->alu_output())
        return true;
    if (mtr->input == nullptr)
        return true;
    ContByteConversion map_alloc;
    safe_vector<IXBar::Use::Byte *> alloced;
    std::set<cstring> fields_needed;
    std::optional<cstring> aliasSourceName = phv.get_alias_name(mtr->input);
    le_bitrange bits;

    auto *finfo = phv.field(mtr->input, &bits);
    BUG_CHECK(finfo, "%s: The input for %s does not have a PHV allocation", mtr->srcInfo,
              mtr->name);
    if (!fields_needed.count(finfo->name)) {
        fields_needed.insert(finfo->name);
        add_use(map_alloc, finfo, phv, tbl, aliasSourceName, &bits, 0, NO_BYTE_TYPE);
    }
    create_alloc(map_alloc, alloc);

    unsigned byte_mask = ~0U;
    hash_matrix_reqs hm_reqs;
    if (on_search_bus) {
        if (!can_allocate_on_search_bus(alloc, finfo, bits, 0)) {
            alloc.clear();
            return false;
        }
        // Because the names of the Bytes are based on containers, the bytes need to be sorted in
        // field order for the allocation to work.
        std::sort(alloc.use.begin(), alloc.use.end(),
            [&](const IXBar::Use::Byte &a, const IXBar::Use::Byte &b) {
            auto a_fi = a.field_bytes[0];
            auto b_fi = b.field_bytes[0];
            return a_fi < b_fi;
        });
        // Byte mask for a meter is 4 bytes
        byte_mask = ((1U << LPF_INPUT_BYTES) - 1);
        if (Device::currentDevice() == Device::TOFINO)
            byte_mask <<= TOFINO_METER_ALU_BYTE_OFFSET;
        hm_reqs.max_search_buses = 1;
    } else {
        hm_reqs.index_groups = HASH_INDEX_GROUPS;
    }

    LOG3("Alloc meter " << mtr->name << " on_search_bus " << on_search_bus << " with "
          << alloc.use.size() << " bytes");

    if (!find_alloc(alloc.use, false, alloced, hm_reqs, byte_mask)) {
        alloc.clear();
        return false;
    }

    alloc.type = Use::METER;
    alloc.used_by = mtr->name.toString();

    if (!on_search_bus) {
        auto &mah = alloc.meter_alu_hash;
        unsigned local_hash_table_input = alloc.compute_hash_tables();
        int hash_group = getHashGroup(local_hash_table_input);
        if (hash_group < 0) {
            alloc.clear();
            return false;
        }
        unsigned hf_hash_table_input = local_hash_table_input | hash_group_use[hash_group];

        mah.allocated = true;
        mah.group = hash_group;
        mah.bit_mask.setrange(0, bits.size());
        mah.algorithm = IR::MAU::HashFunction::identity();
        mah.computed_expressions[0] = mtr->input;
        int max_bit = max_bit_to_byte(mah.bit_mask);
        int max_group = max_index_group(max_bit);
        int max_single_bit = max_index_single_bit(max_bit);
        BUG_CHECK(hash_use_free(max_group, max_single_bit, hf_hash_table_input), "The calculation "
                  "for the hash matrix should be completely free at this point");
        write_hash_use(max_group, max_single_bit, local_hash_table_input, alloc.used_by);
        write_hash_use(max_group, max_single_bit, hf_hash_table_input & ~local_hash_table_input,
                       "$collision"_cs);
        alloc.hash_table_inputs[mah.group] = local_hash_table_input;
        hash_group_print_use[hash_group] = alloc.used_by;
        hash_group_use[hash_group] |= local_hash_table_input;
        update_hash_parity(alloc, hash_group);
    }

    fill_out_use(alloced, false);
    allocated_attached.emplace(mtr, alloc);
    return true;
}

/**
 *  Given a list of PHV fields that have are sources for this Stateful Alu, see if their
 *  given PHV allocation can fit through the search bus.  This will also reset the order
 *  of the bytes in the allocation if the order is to be reversed.
 *
 */
bool IXBar::setup_stateful_search_bus(const IR::MAU::StatefulAlu *salu, Use &alloc,
                                      const FindSaluSources &sources, unsigned &byte_mask) {
    int width = salu->source_width()/8U;

    bool phv_src_reserved[2] = { false, false };
    bool reversed = false;
    int total_sources = 2;
    if (sources.phv_sources.size() > 2)
        failure_reason = salu->name.toString() + " requires more than 2 PHV inputs";

    int source_index = 0;
    // Check to see which input xbar positions the sources can be allocated to
    for (auto &source : sources.phv_sources) {
        auto field = source.first;
        for (auto &range : Keys(source.second)) {
            bool can_fit[2] = { false, false };
            bool can_fit_anywhere = false;
            for (int i = 0; i < total_sources; i++) {
                if (phv_src_reserved[i])
                    continue;
                can_fit[i] = can_allocate_on_search_bus(alloc, field, range, i * width);
                can_fit_anywhere |= can_fit[i];
            }
            if (!can_fit_anywhere) {
                LOG3("  - can't fit anywhere on search bus");
                return false; }
            // If a source can only fit in one of the two input xbar positions, reserve
            // that position for that source
            if (can_fit[0] && !can_fit[1])
                phv_src_reserved[0] = true;
            if (can_fit[1] && !can_fit[0])
                phv_src_reserved[1] = true;

            // If the first source is in the second position on the input xbar, then
            // reverse the vector of bytes for the allocation
            if (source_index == 0 && phv_src_reserved[1])
                reversed = true;
        }
    }

    safe_vector<PHV::FieldSlice> ixbar_source_order;

    // Up to two sources on the stateful ALU.  The sources are ordered from the FindSaluSources.
    // An order from low bit to high byte is required on the input xbar.  If reversed is true,
    // then the order of phv_sources is the opposite of the ixbar_source_order
    for (auto &source : sources.phv_sources) {
        auto field = source.first;
        for (auto &range : Keys(source.second)) {
            if (reversed)
                ixbar_source_order.emplace(ixbar_source_order.begin(), field, range);
            else
                ixbar_source_order.emplace_back(field, range);
        }
    }

    // Because the names of the Bytes are based on containers, the bytes need to be sorted in
    // field order for the allocation to work.
    auto first_field = ixbar_source_order[0].field();
    std::sort(alloc.use.begin(), alloc.use.end(),
        [&](const IXBar::Use::Byte &a, const IXBar::Use::Byte &b) {
        auto a_fi = a.field_bytes[0];
        auto b_fi = b.field_bytes[0];
        if (a_fi.field == first_field->name && b_fi.field != first_field->name)
            return true;
        if (a_fi.field != first_field->name && b_fi.field == first_field->name)
            return false;
        return a_fi.lo < b_fi.lo;
    });

    byte_mask = 0;
    int byte_offset = 0;
    // This handles the corner case of a single source that can only go into the phv_hi slot
    // of the stateful alu
    if (reversed && sources.phv_sources.size() == 1 &&
        sources.phv_sources.begin()->second.size() == 1) {
        byte_offset += width;
    }

    const int ixbar_initial_position =
        Device::currentDevice() == Device::TOFINO ? TOFINO_METER_ALU_BYTE_OFFSET : 0;
    for (auto source : sources.phv_sources) {
        for (auto &range : Keys(source.second)) {
            unsigned source_byte_mask = (1 << ((range.size() + 7) / 8)) - 1;
            alloc.salu_input_source.data_bytemask |= source_byte_mask << byte_offset;
            byte_mask |= source_byte_mask << (byte_offset + ixbar_initial_position);
            byte_offset += width;
        }
    }
    return true;
}

/**
 *  Determines whether a stateful ALU can fit within a hash bus.  If it can, determine
 *  the positions of these individual sources within the hash matrix, as the Galois
 *  matrix has to set up an identity matrix for this
 */
bool IXBar::setup_stateful_hash_bus(const PhvInfo &, const IR::MAU::StatefulAlu *salu,
                                    Use &alloc, const FindSaluSources &sources) {
    auto &mah = alloc.meter_alu_hash;
    unsigned local_hash_table_input = alloc.compute_hash_tables();
    int hash_group = getHashGroup(local_hash_table_input);
    if (hash_group < 0) {
        alloc.clear();
        return false;
    }

    unsigned hf_hash_table_input = local_hash_table_input | hash_group_use[hash_group];

    mah.allocated = true;
    mah.group = hash_group;
    if (sources.dleft) {
        mah.algorithm = IR::MAU::HashFunction::random();
        mah.bit_mask.setrange(1, get_meter_alu_hash_bits() - 1);
        // For dleft digest, we need a fixed 1 bit to ensure the digest is nonzero
        // as the valid bit is not contiguous with the digest in the sram field, so
        // trying to match it would waste more hash bits.
        alloc.hash_seed[hash_group] = bitvec(1);
        alloc.salu_input_source.hash_bytemask = 0x7f;  // 7 bytes to cover all 52 bits
    } else {
        mah.algorithm = IR::MAU::HashFunction::identity();
        bitvec phv_src_inuse;
        // Collate all slices from phv sources into a slice list and sort it
        // based on slice width
        std::vector<const IR::Expression*> slices;
        for (auto &field : Values(sources.phv_sources)) {
            for (auto &slice : Values(field)) {
                slices.push_back(slice);
            }
        }
        std::sort(slices.begin(), slices.end(),
                [](const IR::Expression* a, const IR::Expression*b) {
                return a->type->width_bits() > b->type->width_bits(); });

        // Since the sorting is done by width, while looping the greater value
        // always gets selected first and is placed in the lowermost slot which
        // is 32 bits. This ensures we always attempt a best case fitting
        // scenario.
        //
        // In the unsorted case it is possible we end up putting a lower width
        // into the lower slot and then not having enough bits to fit in the
        // upper slot
        //
        // Case A :
        // Slice 1 : 19 bits, Slice 2 : 32 bits
        // Unsorted Case :
        //  Slot A (32 bit) : 19 bits, Slot B (19 bits) : 32 bit (cannot fit)
        // Sorted Case :
        //  Slot A (32 bit) : 32 bits, Slot B (19 bits) : 19 bit (fits)
        //
        // Case B :
        // Slice 1 : 16 bits, Slice 2 : 32 bits, Hash Bit : 51
        // Unsorted Case :
        //  Slot A (32 bit) : 16 bits, Slot B (16 bits) : 32 bit (cannot fit)
        // Sorted Case :
        //  Slot A (32 bit) : 32 bits, Slot B (16 bits) : 16 bit (fits)
        //
        //  NOTE: Since 51st bit is reserved for parity, we cannot use the
        //  entire byte with bit 51 i.e. bits[53..48]. However, we only have 52
        //  bits in hash so the unused bits are bits[51..48]
        //
        // Case C :
        // Slice 1 : 32 bits, Slice 2 : 16 bits, Hash Bit : 51
        // Unsorted Case :
        //  Slot A (32 bit) : 32 bits, Slot B (16 bits) : 16 bit (fit)
        // Sorted Case :
        //  Slot A (32 bit) : 32 bits, Slot B (16 bits) : 16 bit (fits)
        //
        //  NOTE: In case the slices are always ordered we will always fit, but
        //  this ordering is not guaranteed
        //
        // Case D :
        // Slice 1 : 32 bits, Slice 2 : 19 bits, Hash Bit : 51
        // Unsorted Case :
        //  Slot A (32 bit) : 32 bits, Slot B (16 bits) : 19 bit (cannot fit)
        // Sorted Case :
        //  Slot A (32 bit) : 32 bits, Slot B (16 bits) : 19 bit (cannot fits)
        //
        //  NOTE: In this scenario the slices can never fit in either sorted or unsorted cases
        for (auto &slice : slices) {
            int alu_slot_index = phv_src_inuse.ffz();
            if (alu_slot_index > 1) {
                failure_reason = salu->name.toString() + " requires more than 2 PHV inputs";
                return false; }
            int start_bit = alu_slot_index * salu->source_width();
            int end_bit = start_bit + slice->type->width_bits();
            // If parity is enabled on the hash group stateful hash cannot use
            // any bit in the same byte as the parity bit
            // Parity None/Disabled  - Bits Avail [51..0]
            // Parity Enabled        - Bits Avail [47..0]
            auto parity_status = hash_group_parity_use[hash_group];
            if (parity_status == PARITY_NONE) {
                if (end_bit >= METER_ALU_HASH_BITS) return false;
                if (end_bit >= METER_ALU_HASH_PARITY_BYTE_START) {
                    // Explicitly disable parity on the hash group as we do not
                    // want any other shared resource to enable it
                    hash_group_parity_use[hash_group] = PARITY_DISABLED;
                }
            } else if (parity_status == PARITY_ENABLED) {
                if (end_bit >= METER_ALU_HASH_PARITY_BYTE_START) {
                    return false;
                }
            } else if (parity_status == PARITY_DISABLED) {
                if (end_bit >= METER_ALU_HASH_BITS) return false;
            }
            phv_src_inuse[alu_slot_index] = true;
            mah.computed_expressions[start_bit] = slice;
            mah.bit_mask.setrange(start_bit, slice->type->width_bits());
        }
        for (auto source : sources.hash_sources) {
            int alu_slot_index = phv_src_inuse.ffz();
            if (salu->source_width() >= 32 && alu_slot_index == 0 &&
                source->expr->type->width_bits() <= get_meter_alu_hash_bits()
                                                    - salu->source_width())
                alu_slot_index++;
            int start_bit = alu_slot_index * salu->source_width();
            if (start_bit + source->expr->type->width_bits() > get_meter_alu_hash_bits())
                return false;
            phv_src_inuse[alu_slot_index] = true;
            mah.computed_expressions[start_bit] = source->expr;
            mah.bit_mask.setrange(start_bit, source->expr->type->width_bits());
        }
    }
    alloc.salu_input_source.hash_bytemask = bitmask2bytemask(mah.bit_mask);

    // Because of a byte mask, must reserve the full byte on the hash bus
    int max_bit = max_bit_to_byte(mah.bit_mask);
    int max_group = max_index_group(max_bit);
    int max_single_bit = max_index_single_bit(max_bit);
    BUG_CHECK(hash_use_free(max_group, max_single_bit, hf_hash_table_input), "The calculation "
              "for the hash matrix should be completely free at this point");
    write_hash_use(max_group, max_single_bit, local_hash_table_input, alloc.used_by);
    write_hash_use(max_group, max_single_bit, hf_hash_table_input & ~local_hash_table_input,
                   "$collision"_cs);
    alloc.hash_table_inputs[mah.group] = local_hash_table_input;
    hash_group_print_use[hash_group] = alloc.used_by;
    hash_group_use[hash_group] |= local_hash_table_input;
    update_hash_parity(alloc, hash_group);
    return true;
}

/**  If a stateful ALU is in dual mode, then the stateful ALU has two possible inputs, which
 *  can go to either the lo or hi ALU.  The sizes of these sources can either be
 *  8, 16, or 32 bits, and the stateful ALU can have either one or two sources.  The inputs
 *  come in from an initial offset on the input xbar.  For Tofino, the start byte is 8,
 *  in JBay, the start byte is 0.
 *
 *  Thus, in dual mode:
 *       the bytes for 8 bit inputs are start_byte and start_byte + 1
 *       the bytes for 16 bit inputs are start_byte and start_byte + 2
 *       the bytes for 32 bit inputs are start_byte and start_byte + 4
 *
 *  Either phv source can go to the alu_lo or alu_hi, so the order in which the stateful
 *  ALU fields are allocated do not matter.
 *
 *  Currently the algorithm only allocates fully on a hash bus or a search bus.  In theory
 *  these could be combined, if the allocation was too constrained for either but not
 *  for both.  In general, it might be easier just to influence PHV allocation in that
 *  case.
 */
bool IXBar::allocStateful(const IR::MAU::StatefulAlu *salu, const IR::MAU::Table *tbl,
                          const PhvInfo &phv, Use &alloc, bool on_search_bus) {
    auto pos = allocated_attached.find(salu);
    if (pos != allocated_attached.end()) {
        alloc = dynamic_cast<const Use &>(pos->second);
        return true;
    }

    ContByteConversion map_alloc;
    LOG3("IXBar::allocStateful(" << salu->name << ", " << tbl->name << ", " <<
         (on_search_bus ? "true" : "false") << ")");
    FindSaluSources sources(*this, phv, map_alloc, alloc.field_list_order, tbl);
    salu->apply(sources);
    LOG5("  " << sources.phv_sources << sources.hash_sources);
    create_alloc(map_alloc, alloc);
    if (alloc.use.size() == 0)
        return true;

    unsigned byte_mask = ~0U;
    hash_matrix_reqs hm_reqs;

    if (on_search_bus) {
        if (sources.dleft || !sources.hash_sources.empty() ||
            !setup_stateful_search_bus(salu, alloc, sources, byte_mask)) {
            if (sources.dleft)
                LOG4("  - has dleft source");
            else if (!sources.hash_sources.empty())
                LOG4("  - has hash source");
            alloc.clear();
            return false;
        }
        hm_reqs.max_search_buses = 1;
    } else {
        int total_bits = 0;
        for (auto &source : Values(sources.phv_sources))
            for (auto &range : Keys(source))
                total_bits += range.size();
        for (auto source : sources.hash_sources)
            total_bits += source->expr->type->width_bits();
        if (total_bits > get_meter_alu_hash_bits()) {
            LOG4("  total_bits(" << total_bits << ") > METER_ALU_HASH_BITS(" <<
                 get_meter_alu_hash_bits() << ")");
            return false; }
        hm_reqs = hash_matrix_reqs::max(false);
    }


    LOG3("Alloc stateful alu " << salu->name << " " << (on_search_bus ? "" : "!") <<
         "on_search_bus  with " << alloc.use.size() << " bytes");
    safe_vector<IXBar::Use::Byte *> xbar_alloced;
    if (!find_alloc(alloc.use, false, xbar_alloced, hm_reqs, byte_mask)) {
        alloc.clear();
        return false;
    }

    alloc.type = Use::STATEFUL_ALU;
    alloc.used_by = salu->name.toString();

    if (!on_search_bus) {
        if (!setup_stateful_hash_bus(phv, salu, alloc, sources)) {
            alloc.clear();
            return false;
        }
    }

    LOG3("allocStateful succeeded: " << alloc);
    fill_out_use(xbar_alloced, false);
    allocated_attached.emplace(salu, alloc);
    return true;
}

/** Allocate space for a pre-color.  A pre-color has to be in a pair of bits starting at an even
 *  numbered bit, but can be anywhere within all of hash distribution, as the OXBar can pick
 *  any 2 of all 6 of the hash distribution units to use as an input xbar.
 *
 *  FIXME: Due to mask calculation, currently an entire meter pre-color group has to
 *  reserve an entire slice of a hash distribution unit, when it could fit anywhere.  This
 *  is a larger issue within the allocation of hash distribution in general, and needs
 *  to be fixed eventually.  However, this is not a large issue, as pre-color currently
 *  is rarely realistically used in switch profiles.
 *
 *  This also is an issue with the current output of hash distribution within the assembler,
 *  as the current syntax is unclear.
 */

/**
 * This is the portion of the hash distribution section that is dedicated to a hash mod for
 * a selector.  The hash mod is at most 15 bits.  Bits[9:0] are an input to the
 * mod calculation to determine the mod % sel_length_pool_size.  Bits[14:10] are possibly
 * needed, depending on the maximum size of the pool, and are the bits ORed in after the
 * selector length shift.
 *
 * These requirements are from the Selector, and are already provided here.  The other
 * requirement is where the hash starts, as the hash calculation needs to begin at the bit
 * where the hash input to the selector ended.
 */


/** Allocation for an individual hash distribution requirement.  Hash distribution is a piece of
 *  match central that can take PHV information through the hash matrix/input xbar, where this
 *  information can be used within the performatnce of the action.  The following uses are:
 *      - Using a PHV field as an address for address distribution for a counter, meter, or
 *        action data table
 *      - Calculating a hash to be used as immediate data for the table.
 *      - Providing a meter with a pre-color
 *      - Providing a hash to a selector table to calculate a RAM line hash.  This can be used for
 *        selector pools that require pool sizes > 120, as 120 is the max size for an individual
 *        RAM line in Tofino
 *      - Pulling in any PHV that can be used as immediate, in case the PHV field cannot fit
 *        in a cluster with other PHV fields
 *
 *   Specifically, the hash distribution is described in MicroArch section 6.4.3.5.3.  I'll
 *   outline the major constraints.  The hash distribution is broken into 2 units, of which
 *   each unit has access to an individual hash group output from the hash matrix.  These 2 units
 *   are 48 bits wide, and are broken into 3 16 bit slices, totalling to 96 bits of hash
 *   distribution per stage.  These 16 bit slices can be reformed into addresses, immediate, etc,
 *   through the hash distribution unit, but essentially it uses the same shift and mask procedures
 *   used by match central.
 *
 *   In the case where an address is larger than 16 bits, i.e. for a large stateful table, one
 *   has to use the expand block within hash distribution, which puts a weird restriction on the
 *   bits on the hash bus the address needs.  This constraint is specified in the following Tofino
 *   register: pipes.mau.rams.match.merge.mau_hash_group_expand.  Otherwise, the data can use
 *   the hash bits as one would expect
 *
 *   The last thing is that hash distribution must always be enabled on, as per_flow_enable cannot
 *   be brought through hash distribution.  The only place this doesn't apply is on a miss, where
 *   the miss can actually override the results from hash distribution.
 *
 *   Like every other allocation, it first finds the locations on the input xbar, and then
 *   tries to fit this into the hash matrix
 */

bool IXBar::isHashDistAddress(HashDistDest_t dest) const {
    return dest == HD_STATS_ADR || dest == HD_METER_ADR || dest == HD_ACTIONDATA_ADR;
}

/**
 * View which bits and units are already in use in this hash group (one of the two hash group
 * inputs to hash distribution)
 */
void IXBar::determineHashDistInUse(int hash_group, bitvec &units_in_use, bitvec &hash_bits_in_use) {
    for (int i = 0; i < HASH_TABLES; i++) {
        if (((1 << i) & hash_group_use[hash_group]) == 0) continue;
        units_in_use |= hash_dist_inuse[i];
        hash_bits_in_use |= hash_dist_bit_inuse[i];
    }
    // Because for some hash, no matrix bits are used by the seed is used.  Really this
    // could potentially done with the immediate_default register, but this isn't happening
    // with the current assembly
    hash_bits_in_use |= hash_used_per_function[hash_group];
}

/**
 * Finds an available hash distribution unit and hash bits available.  Just needs a single unit
 * and at most 16 bits within this region.  Possible shifts is the possible right shifts of the
 * data that would still result in the correct allocation after allocation.
 */
bool IXBar::allocHashDistSection(bitvec post_expand_bits, bitvec possible_shifts, int hash_group,
        int &unit_allocated, bitvec &hash_bits_allocated) {
    bitvec units_in_use;
    bitvec hash_bits_in_use;
    determineHashDistInUse(hash_group, units_in_use, hash_bits_in_use);

    bool found = false;
    for (int i = 0; i < HASH_DIST_SLICES; i++) {
        if (units_in_use.getbit(i))
            continue;
        unit_allocated = i;
        bitvec hash_bits_in_unit = hash_bits_in_use.getslice(i * HASH_DIST_BITS, HASH_DIST_BITS);
        for (auto possible_shift : possible_shifts) {
            bitvec hash_bits_to_be_used = post_expand_bits << possible_shift;
            BUG_CHECK(hash_bits_to_be_used.max().index() < HASH_DIST_BITS, "Data is not "
                "contained within a single hash dist unit");
            if ((hash_bits_to_be_used & hash_bits_in_unit).empty()) {
                found = true;
                hash_bits_allocated = hash_bits_to_be_used << (i * HASH_DIST_BITS);
                break;
            }
        }
        if (found)
            break;
    }
    return found;
}

/**
 * @sa As mentioned in allocHashDist, a wide address is any address larger than 16 bits,
 * as this is larger than the general input for a hash distribution unit.  The expand block is
 * then used.  The expand block can do the following:
 *     the 1st 23b section = { 38..32, 15..0 } of the hash function input
 *     the 2nd 23b section = { 45..39, 31..16 } of the hash function input
 *     the 3rd section does not have an expand.
 *
 * The allocation must then check to see if the bits in multiple 16 bit sections are available
 */
bool IXBar::allocHashDistWideAddress(bitvec post_expand_bits, bitvec possible_shifts,
        int hash_group, bool chained_addr, int &unit_allocated, bitvec &hash_bits_allocated) {
    bitvec units_in_use;
    bitvec hash_bits_in_use;
    determineHashDistInUse(hash_group, units_in_use, hash_bits_in_use);

    bool found = false;

    for (int i = 0; i < HASH_DIST_SLICES - 1; i++) {
        if (units_in_use.getbit(i))
            continue;
        if (chained_addr && units_in_use.getbit(HASH_DIST_SLICES - 1))
            continue;
        unit_allocated = i;
        for (auto possible_shift : possible_shifts) {
            bitvec shifted_bits = post_expand_bits << possible_shift;
            bitvec lower_bits = shifted_bits.getslice(0, HASH_DIST_BITS);
            bitvec upper_bits = shifted_bits.getslice(HASH_DIST_BITS, HASH_DIST_EXPAND_BITS);
            hash_bits_allocated = lower_bits << (i * HASH_DIST_BITS);
            hash_bits_allocated |= upper_bits << (HASH_DIST_BITS * 2 + HASH_DIST_EXPAND_BITS * i);
            if ((hash_bits_allocated & hash_bits_in_use).empty()) {
                found = true;
                break;
            }
        }
        if (found)
            break;
    }
    return found;
}

/**
 * Each HashDistAllocPostExpand coordinates to a single HashDistIRUse.  Both represent a single
 * IR::MAU::HashDist object.   The HashDistIRUse object is a subset of the total
 * hash dist object, (i.e. multiple IR nodes that are 8 bit immediate sections).  The purpose
 * of this is to subset the IR section
 * @param alloc_req
 * @param hdu
 * @param all_reqs
 * @param phv
 * @param hash_group
 * @param hash_bits_used
 *      bit vector covering the bits out of the hash that are used in this hash_dist
 * @param total_post_expand_bits
 *      bit vector of the used bits in the hash_dist after the 'expand' step.  This may
 *      include more upper bits (zero extended) than are in hash_bits_used
 * @param tbl
 * @param name
 */
void IXBar::buildHashDistIRUse(HashDistAllocPostExpand &alloc_req, HashDistUse &hdu,
        IXBar::Use &all_reqs, const PhvInfo &phv, int hash_group, bitvec hash_bits_used,
        bitvec total_post_expand_bits, const IR::MAU::Table* tbl, cstring name) {
    hdu.ir_allocations.emplace_back();
    auto &rv = hdu.ir_allocations.back();
    Use *use = new Use();
    rv.use.reset(use);
    ContByteConversion               map_alloc;
    std::map<cstring, bitvec>        fields_needed;
    safe_vector <IXBar::Use::Byte *> alloced;
    fields_needed.clear();
    KeyInfo ki;
    ki.hash_dist = true;

    for (auto input : alloc_req.func->inputs) {
        safe_vector<const IR::Expression *> flo;
        FieldManagement(&map_alloc, flo, input, &fields_needed, phv, ki, tbl);
    }

    use->field_list_order.insert(use->field_list_order.end(), alloc_req.func->inputs.begin(),
                                   alloc_req.func->inputs.end());
    use->symmetric_keys = alloc_req.func->symmetrically_hashed_inputs;
    // Create pre-allocated bytes of the subset
    create_alloc(map_alloc, *use);

    // Coordinate allocation of those bytes through Container Name/Container Byte.  Only need
    // XBar loc for assembly generation
    for (auto &byte : use->use) {
        bool found = false;
        for (auto &alloc_byte : all_reqs.use) {
            if (!byte.is_subset(alloc_byte)) continue;
            byte.loc = alloc_byte.loc;
            found = true;
            break;
        }
        BUG_CHECK(found, "Byte not found in total allocation for table");
    }

    auto &hdh = use->hash_dist_hash;

    BUG_CHECK(hash_bits_used.popcount() == total_post_expand_bits.popcount() ||
              total_post_expand_bits.popcount() > std::max(hash_bits_used.popcount(), 23),
              "Mismatch in bits input and output for hash_dist post expand");
    int bits_seen = 0;
    int bits_of_my_hash_seen = 0;
    // Coordinate hash positions of an a single IR Use for the whole allocation
    for (auto br : bitranges(hash_bits_used)) {
        le_bitrange galois_range = { br.first , br.second };
        bitvec post_expand_sect_bv;
        int index = 0;
        for (int bit : total_post_expand_bits) {
            if (index >= bits_seen && index < bits_seen + galois_range.size())
                post_expand_sect_bv.setbit(bit);
            index++;
        }
        bits_seen += galois_range.size();

        BUG_CHECK(post_expand_sect_bv.is_contiguous(), "Cannot associate galois matrix region "
            "with hash function");
        // The bits in the 23 bit section of data post expand that coordinate with this
        // allocation
        le_bitrange post_expand_sect
            = { post_expand_sect_bv.min().index(), post_expand_sect_bv.max().index() };
        auto boost_sl = toClosedRange<RangeUnit::Bit, Endian::Little>
                            (alloc_req.bits_in_use.intersectWith(post_expand_sect));
        if (boost_sl == std::nullopt)
            continue;
        le_bitrange overlap = *boost_sl;

        int lo_add = overlap.lo - post_expand_sect.lo;
        int hi_sub = post_expand_sect.hi - overlap.hi;
        // Which part of the galois matrix these bits coordinate with
        galois_range = { galois_range.lo + lo_add, galois_range.hi - hi_sub };
        hdh.galois_matrix_bits.setrange(galois_range.lo, galois_range.size());

        // The range of bits within the P4 Hash
        le_bitrange p4_range
             = { bits_of_my_hash_seen,
                 bits_of_my_hash_seen + static_cast<int>(overlap.size()) - 1 };
        p4_range = p4_range.shiftedByBits(alloc_req.func->hash_bits.lo);
        hdh.galois_start_bit_to_p4_hash[galois_range.lo] = p4_range;

        bits_of_my_hash_seen += overlap.size();
    }
    hdh.algorithm = alloc_req.func->algorithm;
    if (hdh.algorithm.type == IR::MAU::HashFunction::IDENTITY) {
        hdh.hash_gen_expr = alloc_req.func->hash_gen_expr;
    } else {
        // Non-"identity" hashes can't be built from (just) the hash_gen_expr
        hdh.hash_gen_expr = nullptr; }
    hdh.group = hash_group;
    hdh.allocated = true;
    hdh.name = name;
    rv.p4_hash_range = alloc_req.func->hash_bits;
    rv.dest = alloc_req.dest;
    rv.created_hd = alloc_req.created_hd;
    rv.dyn_hash_name = alloc_req.func->dyn_hash_name;
    // The hash_table_inputs value is computed for each HashDistIRUse
    // object vs. HashDistUse, as each HashDistIRUse is output individually.
    // This is the requirement for assembly generation. The HashDistUse
    // requirements are just the OR of all of its HashDistIRUse hash_tables
    use->hash_table_inputs[hash_group] = use->compute_hash_tables();
    if (hdh.hash_gen_expr) {
        for (auto &slice : hdh.galois_start_bit_to_p4_hash) {
            bitvec seed = IXBarExprSeed(hdh.hash_gen_expr, slice.second);
            use->hash_seed[hash_group] |= seed << slice.first; }
    } else {
        use->hash_seed[hash_group]
            |= determine_final_xor(&alloc_req.func->algorithm, phv, hdh.galois_start_bit_to_p4_hash,
                                   use->field_list_order, use->total_input_bits()); }
    use->type = IXBar::Use::HASH_DIST;
    use->used_by = tbl->match_table->externalName();
    use->hash_dist_type = alloc_req.dest;
}

/**
 * Fill in the information of the IXBar Alloc2D structures in order to keep track of
 * hash distribution.
 */
void IXBar::lockInHashDistArrays(safe_vector<Use::Byte *> *alloced, int hash_group,
                                 unsigned hash_table_input, int asm_unit, bitvec hash_bits_used,
                                 HashDistDest_t /* dest */, cstring name) {
    if (alloced)
        fill_out_use(*alloced, false);
    for (int i = 0; i < HASH_TABLES; i++) {
        if (((1 << i) & hash_table_input) == 0) continue;
        hash_dist_use[i][asm_unit % HASH_DIST_SLICES] = name;
        hash_dist_inuse[i].setbit(asm_unit % HASH_DIST_SLICES);
    }

    for (int i = 0; i < HASH_TABLES; i++) {
        if (((1 << i) & hash_table_input) == 0) continue;
        for (auto bit : hash_bits_used) {
            hash_dist_bit_use[i][bit] = name;
            hash_dist_bit_inuse[i].setbit(bit);
        }
    }
    hash_dist_groups[asm_unit / HASH_DIST_SLICES] = hash_group;
    hash_group_use[hash_group] |= hash_table_input;

    hash_used_per_function[hash_group] |= hash_bits_used;
}

/**
 * Hash Distribution, described in uArch section 6.4.3.5.3, is a node in Match Central
 * to distribute galois matrix calculations to many non-RAM matching functions:
 *
 *   - stats_adr, meter_adr, actiondata_adr
 *   - immediate data, (lo and hi 16b)
 *   - selector mod
 *   - meter pre-color
 *
 *
 * The Hash Distribution block inputs 96 bits of hash, bits 0..47 of 2 galois hash functions.
 * These 48 bit sections are further divided into 3 16 bit section of galois matrix calculations.
 * Thus, the hash can be pictured as 6x16 bit sections of hash before the expand block.
 *
 * Because addresses specifically may require to be greater than 16 bits, i.e. a meter_adr
 * could be 23 bits, the expand block is used to combined two of these 16 bit sections into
 * a single 23 bit section in a very strange way.  @sa allocHashDistWideAddress.  If a wide
 * address is not necessary, 7b0 is just append to the msb.  Thus the hash distribution unit
 * at this point can be throught of as 6x23 bits of data.
 *
 * The next step is a mask and shift.  Similar to match central (shift mask default), this
 * per 23 bit section masks out the irrelevant bits, and shifts the bits to the relevant
 * position, which is necessary for many of the address positions.
 *
 * Thus if two IR::MAU::HashDist object were to share the same unit, they would require the
 * same hash function input, the same expand, the same mask, and the same shift.  (A couple
 * exceptions to this:
 *     - Potentially anything headed to immediate, where data can be further masked
 *       and shifted depending on the operation, i.e. deposit-field
 *     - Meter Pre-Color data doesn't go through the Mask/Shift block
 *
 * The terminology for all of these different concepts is difficult.  In reference, a unit
 * refers to one of the 6 23 bit sections.  These come from 2 hash functions inputs.
 *
 * Hash Function [0-7] -> Hash Function Input [0-1].
 * Hash Function Input (48b) -> Hash Dist Inputs (3 * 16b) -> Post Expand (3 * 23b)
 *
 * There is no direct unit in hardware, but instead just an assembly coordination between
 * which hash function input and which 16 bit section of that input (which then becomes a
 * 23 bit section post expand)
 *
 * "unit" = hash_function_input * 3 + 16b hash_dist input of 48b
 *
 * The purpose of this function is given the Hash Function requirement, and eventual
 * destination of a hash function, find the ixbar bytes, galois matrix, and hash distribution
 * unit that can hold this directly.  Multiple IR nodes, (i.e. two 8 bit immediate bytes)
 * might have to go to the same hash dist output, so this tracks both the high level unit
 * allocation as well as the per IR allocation
 */
bool IXBar::allocHashDist(safe_vector<HashDistAllocPostExpand> &alloc_reqs, HashDistUse &use,
        const PhvInfo &phv, cstring name, const IR::MAU::Table* tbl, bool second_try) {
    IXBar::Use all_reqs;
    ContByteConversion               map_alloc;
    std::map<cstring, bitvec>        fields_needed;
    safe_vector <IXBar::Use::Byte *> alloced;
    fields_needed.clear();
    KeyInfo ki;
    ki.hash_dist = true;

    HashDistDest_t dest = alloc_reqs[0].dest;
    bitvec bits_in_use;
    int post_expand_shift = -1;
    bool chained_addr = false;

    LOG3("  Calling allocHashDist on dest " <<  ::IXBar::hash_dist_name(dest));
    // Build a union of all input xbar bytes required, as a single hash distribution output
    // might be sourced from multiple P4 level objects.
    for (auto alloc_req : alloc_reqs) {
        for (auto input : alloc_req.func->inputs) {
            safe_vector<const IR::Expression *> flo;
            FieldManagement(&map_alloc, flo, input, &fields_needed, phv, ki, tbl);
        }
        bits_in_use.setrange(alloc_req.bits_in_use.lo, alloc_req.bits_in_use.size());
        if (post_expand_shift == -1) {
            post_expand_shift = alloc_req.shift;
            chained_addr = alloc_req.chained_addr;
        } else {
            BUG_CHECK(post_expand_shift == alloc_req.shift, "Two hash dist of same type require "
                   "the same shift");
            BUG_CHECK(chained_addr == alloc_req.chained_addr, "Two address allocated at the same "
                   "time aren't chained");
        }
    }

    create_alloc(map_alloc, all_reqs.use);

    bool wide_address = false;
    if (bits_in_use.max().index() >= HASH_DIST_BITS) {
        BUG_CHECK(isHashDistAddress(dest), "Allocating illegal hash_dist");
        wide_address = true;
    }

    hash_matrix_reqs hm_reqs;
    if (second_try) {
        hm_reqs = hash_matrix_reqs::max(true);
    } else {
        hm_reqs.index_groups = 1;
        hm_reqs.hash_dist = true;
    }

    // Determine the xbar requirements of the hash distribution destination
    bool rv = find_alloc(all_reqs.use, false, alloced, hm_reqs);
    if (!rv) {
        use.clear();
        return false;
    }

    bitvec possible_shifts(0, 1);


    unsigned hash_table_input = all_reqs.compute_hash_tables();
    int hash_group_opts[HASH_DIST_UNITS] = {-1, -1};
    getHashDistGroups(hash_table_input, hash_group_opts);

    bool can_allocate_hash = false;
    int three_unit_section = -1;
    int unit = -1;
    int hash_group = -1;
    bitvec hash_bits_used;

    // Determine the galois matrix positions, as well has hash_function (of 8) associated hash
    // functions.
    for (int i = 0; i < HASH_DIST_UNITS; i++) {
        if (hash_group_opts[i] == -1) continue;
        hash_group = hash_group_opts[i];
        three_unit_section = i;
        if (wide_address)
            can_allocate_hash = allocHashDistWideAddress(bits_in_use, possible_shifts, hash_group,
                                    chained_addr, unit, hash_bits_used);
        else
            can_allocate_hash = allocHashDistSection(bits_in_use, possible_shifts, hash_group, unit,
                                    hash_bits_used);
        if (can_allocate_hash)
            break;
    }

    if (!can_allocate_hash) {
        LOG5("failed to allocate hash for hash dist");
        use.clear();
        return false;
    }

    // @seealso Unit described in the HashDistUse code
    int asm_unit = three_unit_section * 3 + unit;
    for (auto &alloc_req : alloc_reqs) {
        buildHashDistIRUse(alloc_req, use, all_reqs, phv, hash_group, hash_bits_used,
            bits_in_use, tbl, name);
    }
    lockInHashDistArrays(&alloced, hash_group, hash_table_input, asm_unit, hash_bits_used, dest,
                         name);

    if (wide_address)
        use.expand = (asm_unit % HASH_DIST_SLICES) * 7;
    use.unit = asm_unit;
    if (dest != HD_PRECOLOR) {
        use.shift = post_expand_shift;
        use.shift += (hash_bits_used.min().index() % HASH_DIST_BITS) - bits_in_use.min().index();
        use.mask = bits_in_use.getslice(0, HASH_DIST_MAX_MASK_BITS);
        if (chained_addr)
            use.outputs.insert("lo"_cs);
    }


    LOG3("  Allocated Hash Dist " << hash_group << " 0x" << hex(hash_table_input)
          << " " << hash_bits_used);
    return true;
}

/**
 * FIXME: Due to certain limitations of the current allocation, in JBay for chained vpns, an
 * address has to go both to an address position and immediate, as described in section
 * 6.2.12.12. of JBay uArch FIFO and Stack Primitives.  Because the current allocation
 * would require separate positions (as the allocation currently doesn't understand when two
 * hash functions are identical), this leads to an excess hash distribution allocation that
 * doesn't currently work for the multistage_fifo.p4 example.  Eventually when the allocation
 * understands that these can be overlaid in the galois matrix (WIP), then this can be separated
 * out.
 */
void IXBar::createChainedHashDist(const HashDistUse &hd_alloc, HashDistUse &chained_hd_alloc,
        cstring name) {
    for (auto &ir_alloc : hd_alloc.ir_allocations) {
        // For updating functions, need to have the same bytes and the same hash_table_inputs
        HashDistIRUse curr;
        Use &curr_use = getUse(curr.use);
        const Use &ir_alloc_use = getUse(ir_alloc.use);
        // FIXME -- why are we not just copying the whole ir_alloc (just curr = ir_alloc)?
        // what needs to be different?  Currently not copying:
        //     use.algorithm, use.galois_matrix_bitsm use, galois_start_bit_to_p4_hash,
        //     dyn_hash_name
        // so they remain uninitialized on curr?
        curr_use.field_list_order.insert(curr_use.field_list_order.end(),
             ir_alloc_use.field_list_order.begin(), ir_alloc_use.field_list_order.end());
        curr_use.use.insert(curr_use.use.end(), ir_alloc_use.use.begin(), ir_alloc_use.use.end());
        curr.p4_hash_range = ir_alloc.p4_hash_range;
        curr.dest = HD_IMMED_HI;
        curr.created_hd = ir_alloc.created_hd;
        for (int i = 0; i < HASH_GROUPS; i++) {
            curr_use.hash_table_inputs[i] = ir_alloc_use.hash_table_inputs[i];
        }
        curr_use.hash_dist_hash.allocated = true;
        curr_use.hash_dist_hash.group = ir_alloc_use.hash_dist_hash.group;
        curr_use.hash_dist_hash.hash_gen_expr = ir_alloc_use.hash_dist_hash.hash_gen_expr;
        curr_use.type = Use::HASH_DIST;
        curr_use.hash_dist_type = curr.dest;
        chained_hd_alloc.ir_allocations.emplace_back(curr);
    }

    chained_hd_alloc.unit = (hd_alloc.unit / HASH_DIST_SLICES) * HASH_DIST_SLICES + 2;
    chained_hd_alloc.shift = hd_alloc.expand;
    chained_hd_alloc.mask = bitvec(chained_hd_alloc.shift, HASH_DIST_EXPAND_BITS);
    chained_hd_alloc.outputs.insert("hi"_cs);

    unsigned ht_inputs = hash_table_inputs(chained_hd_alloc);
    for (int ht = 0; ht < HASH_TABLES; ht++) {
        if (((1 << ht) & ht_inputs) == 0) continue;
        hash_dist_use[ht][chained_hd_alloc.unit % HASH_DIST_SLICES] = name;
        hash_dist_inuse[ht].setbit(chained_hd_alloc.unit % HASH_DIST_SLICES);
    }
}

/**
 * Using the bfn_hash_function, this algorithm determines the necessary final_xor positions
 * and writes them into the seed output.
 */
bitvec IXBar::determine_final_xor(const IR::MAU::HashFunction *hf,
        const PhvInfo& /*unused*/, std::map<int, le_bitrange> &bit_starts,
        safe_vector<const IR::Expression *> field_list, int total_input_bits) {
    safe_vector<hash_matrix_output_t> hash_outputs;
    for (auto &entry : bit_starts) {
        hash_matrix_output_t hash_output;
        hash_output.gfm_start_bit = entry.first;
        hash_output.p4_hash_output_bit = entry.second.lo;
        hash_output.bit_size = entry.second.size();
        hash_outputs.push_back(hash_output);
    }
    bfn_hash_algorithm_t hash_alg;
    hf->build_algorithm_t(&hash_alg);
    hash_seed_t hash_seed;
    hash_seed.hash_seed_value = 0ULL;
    hash_seed.hash_seed_used = 0ULL;

    safe_vector<ixbar_input_t> hash_inputs;
    for (auto it = field_list.rbegin(); it != field_list.rend(); it++) {
        auto &entry = *it;
        ixbar_input_t hash_input;
        if (entry->is<IR::Constant>()) {
            hash_input.type = ixbar_input_type::tCONST;
            if (!entry->to<IR::Constant>()->fitsUint64())
                error(ErrorType::ERR_OVERLIMIT, "The size of constant %1% is too large, "
                        "the maximum supported size is 64 bit", entry);
            hash_input.u.constant = entry->to<IR::Constant>()->asUint64();
        } else {
            hash_input.type = ixbar_input_type::tPHV;
            hash_input.u.valid = true;
        }
        // This is completely irrelevant for any seed calculation.  All field inputs are invalid
        hash_input.ixbar_bit_position = 0;
        hash_input.bit_size = entry->type->width_bits();
        hash_input.symmetric_info.is_symmetric = false;
        hash_inputs.push_back(hash_input);
    }
    determine_seed(hash_outputs.data(), hash_outputs.size(),
                   hash_inputs.data(), hash_inputs.size(), total_input_bits, &hash_alg,
                   &hash_seed);

    unsigned lo_unsigned = hash_seed.hash_seed_value & ((1ULL << 32) - 1);
    unsigned hi_unsigned = (hash_seed.hash_seed_value >> 32) & ((1ULL << 32) - 1);
    bitvec lo_bv(lo_unsigned);
    bitvec hi_bv(hi_unsigned);

    return lo_bv | hi_bv << 32;
}

/*
IXBar::P4HashFunction IXBar::P4HashFunction::split(le_bitrange split) const {
    P4HashFunction rv;
    rv.algorithm = algorithm;
    rv.hash_bits = { split.lo + hash_bits.lo, hash_bits.lo + split.hi };
    int bits_seen = 0;
    if (algorithm == IR::MAU::HashFunction::identity()) {
        for (auto expr : inputs) {
            le_bitrange current_bits = { bits_seen, bits_seen + expr->type->width_bits() - 1 };
            auto boost_sl = toClosedRange<RangeUnit::Bit, Endian::Little>
                                 (split.intersectWith(split));
            if (boost_sl == std::nullopt) {
                bits_seen += expr->type->width_bits();
                continue;
            }

            le_bitrange overlap = *boost_sl;
            overlap = overlap.shiftedByBits(overlap.lo - current_bits.lo);
            rv.inputs.push_back(MakeSlice(expr, overlap.lo, overlap.hi));
            bits_seen += expr->type->width_bits();
        }
    } else {
        rv.inputs.insert(rv.inputs.end(), inputs.begin(), inputs.end());
    }
    return rv;
}
*/

void IXBar::XBarHashDist::build_function(const IR::MAU::HashDist *hd, P4HashFunction **func,
        le_bitrange *bits) {
    BuildP4HashFunction builder(phv);
    hd->expr->apply(builder);
    BUG_CHECK(func != nullptr, "%s: Could not generate hash function in table %s", hd->srcInfo,
              tbl->name);
    *func = builder.func();

    if (bits)
        (*func)->slice(*bits);
}

/**
 * Given a hash distribution object, with it's corresponding Context Node of where the
 * object is going, determine the bits required, the max right shift that is needed,
 * and the P4 Hash Function that will be required for the galois matrix.
 */
void IXBar::XBarHashDist::build_req(const IR::MAU::HashDist *hd, const IR::Node *rel_node,
        bool created_hd) {
    le_bitrange post_expand_bits = { 0, hd->type->width_bits() - 1 };
    HashDistDest_t dest;
    P4HashFunction *func = nullptr;
    build_function(hd, &func, nullptr);
    int shift = 0;
    int color_mapram_shift = -1;
    bool chained_addr = false;
    if (rel_node->is<IR::MAU::Selector>()) {
        dest = IXBar::HD_HASHMOD;
    } else if (rel_node->is<IR::MAU::Meter>()) {
        dest = IXBar::HD_PRECOLOR;
    } else if (auto ba = rel_node->to<IR::MAU::BackendAttached>()) {
        if (auto cntr = ba->attached->to<IR::MAU::Counter>()) {
            dest = IXBar::HD_STATS_ADR;
            int per_word = CounterPerWord(cntr);
            shift = 3 - ceil_log2(per_word);
        } else if (auto salu = ba->attached->to<IR::MAU::StatefulAlu>()) {
            dest = IXBar::HD_METER_ADR;
            // Chained addresses don't need to shift as the input itself is an address
            if (salu->chain_vpn || attached_entries.at(salu).need_more) {
                // Chained address can't be shifted in the hash_dist, as they need to go
                // through the vpn_offset&check unit and will be shifted there
                shift = 0;
                chained_addr = true;
            } else {
                int per_word = RegisterPerWord(salu);
                shift = 7 - ceil_log2(per_word);
            }
            if (!lo->dleft_hash_sizes.empty()) {
                // For dleft tables need to mask the addresses by the way sizes.
                // FIXME -- how to deal with different sizes for the different ways?
                // FIXME -- how do we know this is the dleft addressing hash?  Can a single
                // table have both dleft and another hash_dist use?
                post_expand_bits.hi = ceil_log2(lo->dleft_hash_sizes[0]) + 10 - 1;
            }
        } else if (ba->attached->to<IR::MAU::Meter>()) {
            dest = IXBar::HD_METER_ADR;
            shift = 7;
            color_mapram_shift = 3;
        } else {
            BUG("BackendAttached %s is not a Counter, Meter or StatefulAlu", ba);
        }
    } else {
        BUG("Hash Dist object is not in a valid position");
    }

    alloc_reqs.emplace_back(func, post_expand_bits, dest, shift);
    if (created_hd)
        alloc_reqs.back().created_hd = hd;
    alloc_reqs.back().chained_addr = chained_addr;
    if (color_mapram_shift > -1) {
        alloc_reqs.emplace_back(func, post_expand_bits, IXBar::HD_STATS_ADR, color_mapram_shift);
        if (created_hd)
            alloc_reqs.back().created_hd = hd;
    }
}

/**
 * For ActionData tables, these don't yet exist until after TablePlacement, so no Context node
 * exists for these.  They have to be generated separately.
 */
void IXBar::XBarHashDist::build_action_data_req(const IR::MAU::HashDist *hd) {
    le_bitrange post_expand_bits = { 0, hd->type->width_bits() - 1 };
    HashDistDest_t dest = HD_ACTIONDATA_ADR;
    P4HashFunction *func = nullptr;
    build_function(hd, &func, nullptr);
    int shift = std::min(ceil_log2(lo->layout.action_data_bytes_in_table) + 1, 5);
    alloc_reqs.emplace_back(func, post_expand_bits, dest, shift);
    alloc_reqs.back().created_hd = hd;
}

/**
 * As the HashDist for immediate is done through the new action_format based code, an
 * individual HashDist object in an instruction is no longer able to be linked.  Thus,
 * all parameters of the hash headed to immediate are done in a single operation.
 */
void IXBar::XBarHashDist::immediate_inputs() {
    // Gateway only tables
    if (af == nullptr)
        return;

    // All the hash dists in the immediate HashDists
    auto ram_sect = af->build_locked_in_sect();
    auto ad_positions = ram_sect->parameter_positions(false);

    for (auto pos : ad_positions) {
        auto hash = pos.second->to<ActionData::Hash>();
        if (hash == nullptr)
            continue;
        le_bitrange immed_range = { pos.first, pos.first + hash->size() - 1 };
        int bits_seen = 0;
        for (int i = 0; i < 2; i++) {
            le_bitrange immed_impact = { i * HASH_DIST_BITS, (i+1) * HASH_DIST_BITS - 1 };
            auto boost_sl = toClosedRange<RangeUnit::Bit, Endian::Little>
                                (immed_range.intersectWith(immed_impact));
            if (boost_sl == std::nullopt)
                continue;
            le_bitrange overlap = *boost_sl;
            int hash_bits_shift = (-1 * pos.first);
            le_bitrange hash_range = overlap.shiftedByBits(hash_bits_shift);
            le_bitrange hash_dist_range = overlap.shiftedByBits(-1 * i * HASH_DIST_BITS);
            P4HashFunction *func = new P4HashFunction(hash->func());
            func->slice(hash_range);
            HashDistDest_t dest = static_cast<HashDistDest_t>(i);
            alloc_reqs.emplace_back(func, hash_dist_range, dest, 0);
            bits_seen += overlap.size();
        }
    }
}

/**
 * For hash_action tables, HashDist objects must be created during the allocation of the table,
 * as they will be associated as the address for that table later.  This creates that IR object
 * and uses it as context required for the HashDistPostExpandAllocReq creation.
 */
void IXBar::XBarHashDist::hash_action() {
    if (tbl->conditional_gateway_only())
        return;
    if (!lo || !lo->layout.hash_action)
        return;

    IR::Vector<IR::Expression> components;

    /**
     * When action data is more than 128 bits, the compiler must add extra
     * Huffman bits to the VPN bits as specified in figure 6-34 in uArch doc.
     * Compiler assumes that the first match key field is allocated to the lower bits of the hash output, e.g.:
     *   key = {
     *       f0 : exact   // f0 is bit<4>
     *       f1 : exact.  // f1 is bit<10>
     *   }
     * The HashGenExpression saves the field list as { f0, f1 }
     *
     * Subsequently, the field list is translated to assembly as:
     *   hash:
     *      0..9 : f1
     *      10..13 : f0
     *
     * If a gap for Huffman bits is required, the offset of the gap starts at bit offset 10.
     * If the gap is in the middle of a field, that field would be sliced.
     *
     * { f0, 1b0, f1 }
     *
     * And the corresponding assembly is:
     * hash:
     *   0..9: f1
     *   10: 0
     *   11..14: f0
     */
    int huffman_vpn_bits = ActionDataHuffmanVPNBits(&lo->layout);
    bool add_vpn_gap = huffman_vpn_bits > 0;
    int offset_from_bit_zero = 0;
    for (auto read = tbl->match_key.rbegin(); read != tbl->match_key.rend(); read++) {
        if (!(*read)->for_match())
            continue;
        le_bitrange bits;
        phv.field((*read)->expr, &bits);

        offset_from_bit_zero += bits.size();
        bool add_gap_to_current_field = (offset_from_bit_zero > RAM_LINE_SELECT_BITS);
        if (add_vpn_gap && add_gap_to_current_field) {
            auto num_bits_for_vpn = offset_from_bit_zero - RAM_LINE_SELECT_BITS;
            auto vpn_start = bits.size() - num_bits_for_vpn;
            if (vpn_start - 1 >= 0) {
                auto slice = new IR::Slice((*read)->expr, vpn_start - 1, 0);
                components.insert(components.begin(), slice); }
            BUG_CHECK(huffman_vpn_bits != 0, "vpn shift cannot be zero");
            components.insert(components.begin(),
                    new IR::Constant(IR::Type::Bits::get(huffman_vpn_bits),
                        ActionDataVPNStartPosition(&lo->layout)));
            if (bits.size() - 1 >= vpn_start) {
                auto slice = new IR::Slice((*read)->expr, bits.size() - 1, vpn_start);
                components.insert(components.begin(), slice); }
            add_vpn_gap = false;
            offset_from_bit_zero += huffman_vpn_bits;
        } else {
            components.insert(components.begin(), (*read)->expr);
        }
    }
    IR::ListExpression *field_list = new IR::ListExpression(components);

    auto hge = new IR::MAU::HashGenExpression(tbl->srcInfo,
            IR::Type::Bits::get(offset_from_bit_zero),
            field_list, IR::MAU::HashFunction::identity());

    auto hd = new IR::MAU::HashDist(hge->srcInfo, hge->type, hge);
    for (auto ba : tbl->attached) {
        if (!(ba->attached && ba->attached->direct))
            continue;
        if (attached_entries.at(ba->attached).entries == 0) continue;
        build_req(hd, ba, true);
    }

    if (lo->layout.action_data_bytes_in_table > 0) {
        build_action_data_req(hd->clone());
    }
}

bool IXBar::XBarHashDist::preorder(const IR::MAU::HashDist *hd) {
    if (auto sel = findContext<IR::MAU::Selector>()) {
        build_req(hd, sel);
    } else if (auto mtr = findContext<IR::MAU::Meter>()) {
        build_req(hd, mtr);
    } else if (auto ba = findContext<IR::MAU::BackendAttached>()) {
        if (attached_entries.at(ba->attached).entries > 0) build_req(hd, ba);
    } else if (findContext<IR::MAU::Instruction>()) {
        return false;
    }
    return false;
}

/**
 * The purpose of this class is to gather the requirements that have to go through
 * hash distribution.  The requirement is captured in either the IR::MAU::HashDist, or
 * implicitly as the LayoutOption of the table is a hash action table.  From the IR,
 * we can gather the IXBar Bytes required, as well as the hash matrix requirements.
 *
 * At this point, all requirements have been gathered.  This allocates each requirement
 * on a destination by destination basis.  Thus if multiple HashDistPostExpandAllocReqs are
 * headed to the same destination, they will be allocated simultaneously
 *
 */
bool IXBar::XBarHashDist::allocate_hash_dist() {
    for (int i = HD_IMMED_LO; i < HD_DESTS; i++) {
        bool dynamic_hash = false;
        safe_vector<HashDistAllocPostExpand> dest_reqs;
        ordered_set<HashDistAllocPostExpand> dedup;
        for (auto alloc_req : alloc_reqs) {
            if (alloc_req.dest == static_cast<HashDistDest_t>(i)) {
                if (alloc_req.func && alloc_req.func->is_dynamic())
                    dynamic_hash = true;
                if (dedup.count(alloc_req) != 0)
                    continue;

                dest_reqs.push_back(alloc_req);
                dedup.insert(alloc_req);
            }
        }
        if (dest_reqs.empty())
            continue;

        // Try to find a previously allocated hash dist that would match this new request.
        // Never share dynamic hash or chained address.
        // Share resource based on the same destination (this might be over conservative) since
        // most of the time the shift is different accross destination even if the input is the
        // same. Some destination also have different timing over others.
        if (!dest_reqs[0].chained_addr && !dynamic_hash) {
            bool found_prev_alloc = false;
            for (auto kv : self.tbl_hash_dists) {
                const safe_vector<HashDistUse> *hash_dist_vect = kv.second;
                for (const HashDistUse &hash_dist : *hash_dist_vect) {
                    if (hash_dist.src_reqs.size() == dest_reqs.size() &&
                        hash_dist.src_reqs[0].dest == static_cast<HashDistDest_t>(i)) {
                        bool match = true;
                        for (int j = 0; j < (int)dest_reqs.size(); j++) {
                            const HashDistAllocPostExpand &pe_src = hash_dist.src_reqs[j];
                            const HashDistAllocPostExpand &pe_dst = dest_reqs[j];
                            if (!pe_src.func->equiv_inputs_alg(pe_dst.func) ||
                                pe_src.bits_in_use != pe_dst.bits_in_use ||
                                pe_src.shift != pe_dst.shift ||
                                pe_src.chained_addr != pe_dst.chained_addr) {
                                match = false;
                                break;
                            }
                        }
                        if (match) {
                            // Found HDU previously allocated that exactly match out need.
                            resources->hash_dists.emplace_back(hash_dist);
                            resources->hash_dists.back().used_by = tbl->name;
                            found_prev_alloc = true;
                            break;
                        }
                    }
                }
                if (found_prev_alloc)
                    break;
            }
            // Still have to look for other destination if needed
            if (found_prev_alloc)
                continue;
        }

        HashDistUse hd_alloc;
        hd_alloc.used_by = tbl->name;
        if (!self.allocHashDist(dest_reqs, hd_alloc, phv, tbl->name + "$hash_dist", tbl, false) &&
            !self.allocHashDist(dest_reqs, hd_alloc, phv, tbl->name + "$hash_dist", tbl, true)) {
            LOG5("failed to allocate hash dist for " << hash_dist_name(HashDistDest_t(i)));
            return false;
        }
        // Save source data for reuse by other table of the same stage.
        if (!dest_reqs[0].chained_addr && !dynamic_hash)
            hd_alloc.src_reqs = dest_reqs;

        resources->hash_dists.emplace_back(hd_alloc);

        if (dest_reqs[0].chained_addr && hd_alloc.expand >= 0) {
            HashDistUse chained_hd_alloc;
            chained_hd_alloc.used_by = tbl->name;
            self.createChainedHashDist(hd_alloc, chained_hd_alloc, tbl->name + "$hash_dist");
            resources->hash_dists.emplace_back(chained_hd_alloc);
        }
    }
    return true;
}

/** This is the general algorithm of the input xbar allocation.  This will allocate all
 *  needs for a table's input xbar, specifically xbar needs for the match, the gateway,
 *  selectors, stateful alu, and any hash distribution needs.  The allocation scheme is the
 *  following:
 *     - Find locations on the input xbar for bytes.
 *     - Once those locations are chosen, then make reservations within the hash matrix (if search
 *       data is on SRAM xbar) for these particular tables.
 *
 *  This occurs in two attempts.  First, attempt to share bytes on the ixbar wherever possible.
 *  If this doesn't work, due to hash matrix constraint, redo the allocation with input xbar
 *  locations that have no current hash matrix use
 */
bool IXBar::allocTable(const IR::MAU::Table *tbl, const PhvInfo &phv, TableResourceAlloc &alloc,
                       const LayoutOption *lo, const ActionData::Format::Use *af,
                       const attached_entries_t &attached_entries) {
    if (!tbl) return true;
    if (!lo && !tbl->conditional_gateway_only()) return false;
    /* Determine number of groups needed.  Loop through them, alloc match will be the same
       for these.  Alloc All Hash Ways will required multiple groups, and may need to change  */
    LOG1("IXBar::allocTable(" << tbl->name << ")");
    if (!tbl->conditional_gateway_only() && !lo->layout.no_match_rams() && !lo->layout.atcam &&
        !lo->layout.proxy_hash) {
        bool ternary = tbl->layout.ternary;
        safe_vector<IXBar::Use::Byte *> alloced;
        safe_vector<Use> all_tbl_allocs;
        bool finished = false;
        size_t start = 0; size_t last = 0;
        while (!finished) {
            Use next_alloc;
            layout_option_calculation(lo, start, last);
            /* Essentially a calculation of how much space is potentially available */
            auto hm_reqs = match_hash_reqs(lo, start, last, ternary);
            auto max_hm_reqs = hash_matrix_reqs::max(false, ternary);

            // Used to print initialization information for gtest
            LOG5("hm_reqs.max_search_buses=" << hm_reqs.max_search_buses << ";");
            LOG5("hm_reqs.index_groups=" << hm_reqs.index_groups << ";");
            LOG5("hm_reqs.select_bits=" << hm_reqs.select_bits << ";");
            LOG5("hm_reqs.hash_dist=" << hm_reqs.hash_dist << ";");
            LOG5("hm_reqs.requires_versioning=" << hm_reqs.requires_versioning << ";");

            if (!(allocMatch(ternary, tbl, phv, next_alloc, alloced, hm_reqs)
                && allocAllHashWays(ternary, tbl, next_alloc, lo, start, last, hm_reqs))
                && !(allocMatch(ternary, tbl, phv, next_alloc, alloced, max_hm_reqs)
                && allocAllHashWays(ternary, tbl, next_alloc, lo, start, last, max_hm_reqs))) {
                next_alloc.clear();
                alloc.match_ixbar.reset();
                LOG5("failed to allocate " << (ternary ? "ternary" : "exact") << " match");
                return false;
            } else {
               fill_out_use(alloced, ternary);
            }
            for (auto need : alloced) {
                LOG6("alloced " << need->container << " " << need->lo << " " << need->loc);
            }
            alloced.clear();
            all_tbl_allocs.push_back(next_alloc);
            if (last == lo->way_sizes.size())
                finished = true;
        }
        for (auto a : all_tbl_allocs) {
            getUse(alloc.match_ixbar).add(a);
        }

        // Used to print initialization information for gtest
        for (auto row = 0; row < use(ternary).rows(); row++) {
            for (auto col = 0; col < use(ternary).cols(); col++) {
                if (!use(ternary)[row][col].first)
                    continue;
                LOG5("use[" << row << "][" << col << "]="
                     << "{\"" << use(ternary)[row][col].first << "\", "
                     << use(ternary)[row][col].second << "};"
                     << "  // byte(" << toIXBarOutputByte(ternary, row, col) << ")");
            }
        }

        int index = 0;
        for (auto grp = byte_group_use.begin(); grp != byte_group_use.end(); grp++) {
            LOG5("byte_group_use[" << index << "]={\""
                 << grp->first << "\", " << grp->second << "};");
            index++;
        }
    } else if (tbl->layout.atcam) {
        safe_vector<IXBar::Use::Byte *> alloced;
        hash_matrix_reqs hm_reqs;
        if (!allocMatch(false, tbl, phv, getUse(alloc.match_ixbar), alloced, hm_reqs)) {
            LOG5("failed to allocate atcam match");
            alloc.match_ixbar.reset();
            alloced.clear();
            return false;
        } else {
            fill_out_use(alloced, false);
        }
        LOG3("atcam " << tbl->match_table->name << " " << tbl->layout.partition_count);

        if (!allocPartition(tbl, phv, getUse(alloc.match_ixbar), false)
            && !allocPartition(tbl, phv, getUse(alloc.match_ixbar), true)) {
            LOG5("failed to allocate atcam partition");
            alloc.match_ixbar.reset();
            return false;
        }
    } else if (tbl->layout.proxy_hash) {
        if (!allocProxyHash(tbl, phv, lo, getUse(alloc.match_ixbar),
                            getUse(alloc.proxy_hash_ixbar))) {
            LOG5("failed to allocate proxy hash");
            alloc.clear_ixbar();
            return false;
        }
    }

    for (auto back_at : tbl->attached) {
        auto at_mem = back_at->attached;
        if (attached_entries.at(at_mem).entries <= 0) continue;
        if (auto as = at_mem->to<IR::MAU::Selector>()) {
            if (!allocSelector(as, tbl, phv, getUse(alloc.selector_ixbar), tbl->name)) {
                LOG5("failed to allocate selector");
                alloc.clear_ixbar();
                return false; }
        } else if (auto mtr = at_mem->to<IR::MAU::Meter>()) {
            if (!allocMeter(mtr, tbl, phv, getUse(alloc.meter_ixbar), true) &&
                !allocMeter(mtr, tbl, phv, getUse(alloc.meter_ixbar), false)) {
                LOG5("failed to allocate meter");
                alloc.clear_ixbar();
                return false; }
        } else if (auto salu = at_mem->to<IR::MAU::StatefulAlu>()) {
            if (!allocStateful(salu, tbl, phv, getUse(alloc.salu_ixbar), true) &&
                !allocStateful(salu, tbl, phv, getUse(alloc.salu_ixbar), false)) {
                LOG5("failed to allocate stateful");
                alloc.clear_ixbar();
                return false; } } }

    if (!allocGateway(tbl, phv, getUse(alloc.gateway_ixbar), lo, false) &&
        !allocGateway(tbl, phv, getUse(alloc.gateway_ixbar), lo, true)) {
        LOG5("failed to allocate gateway");
        alloc.clear_ixbar();
        return false; }

    XBarHashDist xbar_hash_dist(*this, phv, tbl, af, lo, &alloc, attached_entries);
    xbar_hash_dist.hash_action();
    xbar_hash_dist.immediate_inputs();
    tbl->attached.apply(xbar_hash_dist);
    if (!xbar_hash_dist.allocate_hash_dist()) {
        alloc.clear_ixbar();
        return false;
    }

    LOG1("Input xbar allocation successful");
    return true;
}

/* Allocate the match table first, then any gateway being merged with it
 * this allows better sharing of search busses?  maybe.  perhaps doesn't matter
 * TODO -- investigate what effect this ordering actually has and see if there's
 * an opportunity for improvement */
bool IXBar::allocTable(const IR::MAU::Table *tbl, const IR::MAU::Table *gw, const PhvInfo &phv,
                       TableResourceAlloc &alloc, const LayoutOption *lo,
                       const ActionData::Format::Use *af,
                       const attached_entries_t &attached_entries) {
    LOG7(*this);
    return allocTable(tbl, phv, alloc, lo, af, attached_entries) &&
           allocTable(gw, phv, alloc, lo, nullptr, attached_entries);
}

/** Fill in the information contained in the IXBar::Use object of a TableResourceAlloc into the
 *  IXBar structure, so that when a new table is allocated to the IXBar, the previous stage table
 *  resources are known
 */
void IXBar::update(cstring name, const ::IXBar::Use &alloc_) {
    auto &alloc = dynamic_cast<const Use &>(alloc_);
    auto &use = alloc.type == Use::TERNARY_MATCH ? ternary_use.base() : exact_use.base();
    auto &fields = alloc.type == Use::TERNARY_MATCH ? ternary_fields : exact_fields;
    cstring xbar_type = alloc.type == Use::TERNARY_MATCH ? "TCAM"_cs : "SRAM"_cs;
    for (auto &byte : alloc.use) {
        if (!byte.loc) continue;
        field_users[byte.container].insert(name);
        if (byte.loc.byte == 5 && alloc.type == Use::TERNARY_MATCH) {
            /* the sixth byte in a ternary group is actually half a byte group it shares with
             * the adjacent ternary group */
            int byte_group = byte.loc.group/2;
            if (byte == byte_group_use[byte_group]) continue;
            if (byte_group_use[byte_group].first)
                BUG("conflicting ixbar allocation at TCAM byte group %d", byte_group);
            byte_group_use[byte_group] = byte;
        } else {
            if (byte == use[byte.loc]) continue;
            if (use[byte.loc].first) {
                BUG("conflicting ixbar allocation at %s xbar group:byte %d:%d", xbar_type,
                    byte.loc.group, byte.loc.byte);
            }
            use[byte.loc] = byte; }
        fields.emplace(byte.container, byte.loc); }
    for (auto &bits : alloc.bit_use) {
        for (int b = 0; b < bits.width; b++) {
            for (auto ht : bitvec(alloc.hash_table_inputs[bits.group])) {
                if (hash_single_bit_use.at(ht, b + bits.bit)) {
                    BUG("conflicting ixbar hash bit allocation");
                }
                hash_single_bit_use.at(ht, b + bits.bit) = name; }
            hash_single_bit_inuse[b + bits.bit] |= alloc.hash_table_inputs[bits.group];
        }

        hash_group_use[bits.group] |= alloc.hash_table_inputs[bits.group];
        update_hash_parity(bits.group);
        hash_group_print_use[bits.group] = name;
        hash_used_per_function[bits.group].setrange(bits.bit + RAM_SELECT_BIT_START, bits.width);
    }
    for (auto &way : alloc.way_use) {
        hash_group_use[way.source] |= alloc.hash_table_inputs[way.source];
        update_hash_parity(way.source);
        hash_group_print_use[way.source] = name;
        hash_used_per_function[way.source].setrange(way.index.lo, way.index.size());
        BUG_CHECK(way.select.lo == RAM_SELECT_BIT_START, "invalid select range for tofino");
        hash_used_per_function[way.source] |= bitvec(way.select_mask) << way.select.lo;
        hash_index_inuse[INDEX_RANGE_SUBGROUP(way.index)] |= alloc.hash_table_inputs[way.source];
        for (int hash : bitvec(alloc.hash_table_inputs[way.source])) {
            if (!hash_index_use[hash][INDEX_RANGE_SUBGROUP(way.index)])
                hash_index_use[hash][INDEX_RANGE_SUBGROUP(way.index)] = name;
            for (auto bit : bitvec(way.select_mask)) {
                hash_single_bit_inuse[bit] |= alloc.hash_table_inputs[way.source];
                if (!hash_single_bit_use[hash][bit])
                    hash_single_bit_use[hash][bit] = name; } } }

    if (alloc.meter_alu_hash.allocated) {
        auto &mah = alloc.meter_alu_hash;
        // The mask is a byte mask, so must reserve the entire byte
        int max_bit = max_bit_to_byte(mah.bit_mask);
        int max_group = max_index_group(max_bit);
        int max_index_bit = max_index_single_bit(max_bit);
        unsigned hash_table_input = alloc.hash_table_inputs[mah.group];
        hash_group_use[mah.group] |= alloc.hash_table_inputs[mah.group];
        update_hash_parity(mah.group);
        hash_group_print_use[mah.group] = name;
        hash_used_per_function[mah.group] |= mah.bit_mask;

        if (!hash_use_free(max_group, max_index_bit, hash_table_input)) {
            BUG("Conflicting hash matrix usage for %s", name);
        }
        write_hash_use(max_group, max_index_bit, hash_table_input, name);
    }
    if (alloc.hash_dist_hash.allocated) {
        auto &hdh = alloc.hash_dist_hash;

        // This alias is used to cover for shared HDU
        cstring local_name = name;
        if (!hdh.name.isNullOrEmpty())
            local_name = hdh.name;

        for (int i = 0; i < HASH_TABLES; i++) {
            if (((1U << i) & alloc.hash_table_inputs[hdh.group]) == 0) continue;
            for (auto bit : bitvec(hdh.galois_matrix_bits)) {
                if (!hash_dist_bit_use[i][bit]) {
                    hash_dist_bit_use[i][bit] = local_name;
                } else if (hash_dist_bit_use[i][bit] != local_name) {
                    BUG("Conflicting hash distribution bit allocation %s and %s",
                        local_name, hash_dist_bit_use[i][bit]);
                }
#if 0
                // FIXME -- should update this here, but doing so causes a BUG_CHECK
                // with arista/obfuscated-l2_dci.p4 and --alt-phv-alloc
                hash_dist_use[i][bit/HASH_DIST_BITS] = local_name;
                hash_dist_inuse[i] |= 1 << bit/HASH_DIST_BITS;
#endif
            }
            hash_dist_bit_inuse[i] |= hdh.galois_matrix_bits;
        }
        hash_used_per_function[hdh.group] |= hdh.galois_matrix_bits;
        hash_group_print_use[hdh.group] = local_name;
        hash_group_use[hdh.group] |= alloc.hash_table_inputs[hdh.group];
        update_hash_parity(hdh.group);
    }

    if (alloc.proxy_hash_key_use.allocated) {
        auto &ph = alloc.proxy_hash_key_use;
        for (int ht = 0; ht < HASH_TABLES; ht++) {
            if (((1U << ht) & alloc.hash_table_inputs[ph.group]) == 0) continue;
            unsigned indexes = index_groups_used(ph.hash_bits);
            for (auto idx : bitvec(indexes)) {
                hash_index_inuse[idx] |= alloc.hash_table_inputs[ph.group];
                if (!hash_index_use[ht][idx])
                    hash_index_use[ht][idx] = name;
                else if (hash_index_use[ht][idx] != name)
                    BUG("Conflicting hash usage between %s and %s", name,
                         hash_index_use[ht][idx]);
            }
            unsigned select_bits = select_bits_used(ph.hash_bits);
            for (auto bit : bitvec(select_bits)) {
                hash_single_bit_inuse[bit] |= alloc.hash_table_inputs[ph.group];
                if (!hash_single_bit_use[ht][bit])
                    hash_single_bit_use[ht][bit] = name;
                else if (hash_single_bit_use[ht][bit] != name)
                    BUG("Conflicting hash usage between %s and %s", name,
                        hash_single_bit_use[ht][bit]);
            }
        }
        hash_group_print_use[ph.group] = name;
        hash_group_use[ph.group] |= alloc.hash_table_inputs[ph.group];
        update_hash_parity(ph.group);
        hash_used_per_function[ph.group] |= ph.hash_bits;
    }
}

void IXBar::update(cstring name, const HashDistUse &hash_dist_alloc) {
    for (auto &ir_alloc : hash_dist_alloc.ir_allocations) {
        update(name, *ir_alloc.use);
    }

    for (int i = 0; i < HASH_TABLES; i++) {
        if (((1U << i) & hash_table_inputs(hash_dist_alloc)) == 0) continue;
        int slice = hash_dist_alloc.unit % HASH_DIST_SLICES;
        if (!hash_dist_use[i][slice].isNull())
            hash_dist_use[i][slice] = name;
        hash_dist_inuse[i].setbit(slice);
    }

    int hash_dist_48_bit_unit = hash_dist_alloc.unit / HASH_DIST_SLICES;
    if (hash_dist_groups[hash_dist_48_bit_unit] != hash_dist_alloc.hash_group()
        && hash_dist_groups[hash_dist_48_bit_unit] != -1)
        BUG("Conflicting hash distribution unit groups");
    hash_dist_groups[hash_dist_48_bit_unit] = hash_dist_alloc.hash_group();
    hash_used_per_function[hash_dist_alloc.hash_group()] |= hash_dist_alloc.galois_matrix_bits();
}

void IXBar::update(const IR::MAU::Table *tbl, const TableResourceAlloc *rsrc) {
    ::IXBar::update(tbl, rsrc);
    auto name = tbl->name;
    int index = 0;
    for (auto &hash_dist : rsrc->hash_dists) {
        update(name + "$hash_dist" + std::to_string(index++), hash_dist);
    }
    tbl_hash_dists.emplace(tbl, &rsrc->hash_dists);
}

void IXBar::update(const IR::MAU::Table *tbl) {
    if (tbl->for_dleft() && tbl->is_placed()) {
        auto orig_name = tbl->name.before(tbl->name.findlast('$'));
        if (dleft_updates.count(orig_name))
            return;
        dleft_updates.emplace(orig_name);
    }
    ::IXBar::update(tbl);
}

/**
 * Because hash functions can share portions of hash tables, this looks for all possible
 * places that cannot be allocated and reserves them.  This is best displayed by an example:
 *
 * Let's say you have two tables A and B with 8 byte keys.  Table A allocates its bytes to
 * byte 0-7 of ixbar group 2, and Table B allocates its bytes to bytes 8-15 of ixbar group 2.
 * Now both A and B both require two ways, so we will say that table A uses bits 0-19 and table
 * B uses bits 20-39.  If these tables were to share the same hash function, then the hash matrix
 * between bytes 0-7 of ixbar group 2 and bits 20-39 would not be off limits, as this would
 * collide with the hash function for table B. The same is true for bits 0-19 of bytes 8-15 of
 * ixbar group 2.  This is to guarantee that this space is reserved.
 */
void IXBar::add_collisions() {
    for (int hg = 0; hg < HASH_GROUPS; hg++) {
        for (int idx = 0; idx < HASH_INDEX_GROUPS; idx++) {
            if ((hash_group_use[hg] & hash_index_inuse[idx]) != 0) {
                for (auto ht : bitvec(hash_group_use[hg])) {
                    if (hash_index_use[ht][idx].isNull())
                        hash_index_use[ht][idx] = "$collision"_cs;
                }
                hash_index_inuse[idx] |= hash_group_use[hg];
            }
        }

        for (int bit = 0; bit < IXBar::get_hash_single_bits(); bit++) {
            if ((hash_group_use[hg] & hash_single_bit_inuse[bit]) != 0) {
                for (auto ht : bitvec(hash_group_use[hg])) {
                    if (hash_single_bit_use[ht][bit].isNull())
                        hash_single_bit_use[ht][bit] = "$collision"_cs;
                }
                hash_single_bit_inuse[bit] |= hash_group_use[hg];
            }
        }
    }
}

/**
 * The purpose of this function is to guarantee no hash collisions on the relevant bits
 * for each hash function.  This is currently only done on non hash dist, though could
 * be extended.  This can only be done after update, as it requires the hash_used_per_function
 * to be set, which is currently only guaranteed on update.
 */
void IXBar::verify_hash_matrix() const {
    for (int hg = 0; hg < HASH_GROUPS; hg++) {
        std::vector<cstring> index_groups(HASH_INDEX_GROUPS, cstring());
        for (int idx = 0; idx < HASH_INDEX_GROUPS; idx++) {
            bitvec idx_in_use = hash_used_per_function[hg].getslice(idx * RAM_LINE_SELECT_BITS,
                                                                    RAM_LINE_SELECT_BITS);
            if (idx_in_use.empty()) continue;
            BUG_CHECK(idx < HASH_INDEX_GROUPS, "Illegal idx %1%", idx);
            unsigned relevant_hash_tables = hash_group_use[hg] & hash_index_inuse[idx];
            for (auto ht : bitvec(relevant_hash_tables)) {
                BUG_CHECK(ht < HASH_TABLES, "Illegal hash table %1%", ht);
                if (index_groups[idx].isNull())
                    index_groups[idx] = hash_index_use[ht][idx];
                else
                    BUG_CHECK(index_groups[idx] == hash_index_use[ht][idx], "Hash table "
                        "collision at ht %1% index %2% between %3% and %4%", ht, idx,
                        index_groups[idx], hash_index_use[ht][idx]);
            }
        }


        std::vector<cstring> single_bits(IXBar::get_hash_single_bits(), cstring());
        for (int bit = 0; bit < IXBar::get_hash_single_bits(); bit++) {
            if (!hash_used_per_function[hg].getbit(bit + RAM_SELECT_BIT_START)) continue;
            unsigned relevant_hash_tables = hash_group_use[hg] & hash_single_bit_inuse[bit];
            BUG_CHECK(bit < IXBar::get_hash_single_bits(), "Illegal bit %1%", bit);
            for (auto ht : bitvec(relevant_hash_tables)) {
                BUG_CHECK(ht < HASH_TABLES, "Illegal hash table %1%", ht);
                if (single_bits[bit].isNull())
                    single_bits[bit] = hash_single_bit_use[ht][bit];
                else
                    BUG_CHECK(single_bits[bit] == hash_single_bit_use[ht][bit], "Hash table "
                        "collision at ht %1% bit %2% between %3% and %4%", ht, bit,
                        single_bits[bit], hash_single_bit_use[ht][bit]);
            }
        }
    }
}

/* IXBarPrinter in .gdbinit should match this */
void IXBar::dbprint(std::ostream &out) const {
    std::map<cstring, std::set<cstring>> field_users_names;
    for (const auto& kv : field_users) {
        field_users_names[kv.first.toString()] = kv.second;
    }
    std::map<cstring, char>     fields;
    add_names(exact_use, fields);
    add_names(ternary_use, fields);
    add_names(byte_group_use, fields);
    sort_names(fields);
    out << "exact ixbar groups                  ternary ixbar groups" << Log::endl;
    for (int r = 0; r < IXBar::EXACT_GROUPS; r++) {
        write_group(out, exact_use[r], fields);
        if (r < IXBar::BYTE_GROUPS) {
            out << "  ";
            write_group(out, ternary_use[2*r], fields);
            out << " ";
            write_one(out, byte_group_use[r], fields);
            out << " ";
            write_group(out, ternary_use[2*r+1], fields); }
        out << Log::endl; }
    for (auto &f : fields) {
        out << "   " << f.second << " " << f.first;
        const char *sep = " (";
        if (field_users_names.count(f.first)) {
            for (auto t : field_users_names.at(f.first)) {
                out << sep << t;
                sep = ", "; } }
        if (*sep == ',') out << ')';
        out << Log::endl; }
    std::map<cstring, char>     tables;
    add_names(hash_index_use, tables);
    add_names(hash_single_bit_use, tables);
    add_names(hash_group_print_use, tables);
    add_names(hash_dist_use, tables);
    add_names(hash_dist_bit_use, tables);
    sort_names(tables);
    out << "hash select bits  Groups  hd  /- hash dist bits --- (Groups: "
        << hash_dist_groups[0] << " " << hash_dist_groups[1] << ")" << Log::endl;
    for (int h = 0; h < IXBar::HASH_TABLES; ++h) {
        write_group(out, hash_index_use[h], tables);
        out << " ";
        write_group(out, hash_single_bit_use[h], tables);
        out << "  ";
        if (h < IXBar::HASH_GROUPS) {
            write_one(out, hash_group_print_use[h], tables);
            out << hex(hash_group_use[h], 4, '0');
        } else {
            out << "     "; }
        out << "  ";
        write_group(out, hash_dist_use[h], tables);
        out << " ";
        write_group(out, hash_dist_bit_use[h], tables);
        out << Log::endl; }
    for (auto &t : tables)
        out << "   " << t.second << " " << t.first << Log::endl;
}

}  // namespace Tofino
