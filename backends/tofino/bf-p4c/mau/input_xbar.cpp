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
#include "bf-p4c/device.h"
#include "bf-p4c/logging/resources.h"
#include "bf-p4c/mau/gateway.h"
#include "bf-p4c/mau/ixbar_expr.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/resource_estimate.h"
#include "bf-p4c/mau/tofino/input_xbar.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-utils/include/dynamic_hash/dynamic_hash.h"
#include "lib/algorithm.h"
#include "lib/bitvec.h"
#include "lib/bitops.h"
#include "lib/bitrange.h"
#include "lib/hex.h"
#include "lib/range.h"
#include "lib/log.h"

unsigned IXBarRandom::seed = 0x0572f1fa;
std::uniform_int_distribution<unsigned> IXBarRandom::distribution10(0, 1023);
std::uniform_int_distribution<unsigned> IXBarRandom::distribution1(0, 1);
std::mt19937 IXBarRandom::mersenne_generator(IXBarRandom::seed);

unsigned IXBarRandom::nextRandomNumber(unsigned numBits) {
    if (numBits == 10)
        return distribution10(mersenne_generator);
    else
        return distribution1(mersenne_generator);
}

IXBar::HashDistDest_t IXBar::dest_location(const IR::Node *node, bool precolor) {
    if (auto ba = node->to<IR::MAU::BackendAttached>()) {
        if (ba->attached->is<IR::MAU::ActionData>())
            return HD_ACTIONDATA_ADR;
        if (ba->attached->is<IR::MAU::Counter>())
            return HD_STATS_ADR;
        if (ba->attached->is<IR::MAU::StatefulAlu>())
            return HD_METER_ADR;
        if (ba->attached->is<IR::MAU::Meter>()) {
            return precolor ? HD_STATS_ADR : HD_METER_ADR;
        }
    }
    if (node->is<IR::MAU::Meter>())
        return HD_PRECOLOR;
    if (node->is<IR::MAU::Selector>())
        return HD_HASHMOD;

    BUG("Invalid call of HashDist dest location");
    return HD_DESTS;
}

std::string IXBar::hash_dist_name(HashDistDest_t dest) {
    std::string type;
    switch (dest) {
        case HD_IMMED_LO: type = "immediate lo"; break;
        case HD_IMMED_HI: type = "immediate hi"; break;
        case HD_STATS_ADR: type = "stats address"; break;
        case HD_METER_ADR: type = "meter address"; break;
        case HD_ACTIONDATA_ADR: type = "action data address"; break;
        case HD_PRECOLOR: type = "meter precolor"; break;
        case HD_HASHMOD: type = "selector mod"; break;
        default: BUG("unknown hash dist user"); break;
    }
    return type;
}

int IXBar::Use::groups() const {
    int rv = 0;
    unsigned counted = 0;
    for (auto &b : use) {
        assert(b.loc.group >= 0 && b.loc.group < 16);
        if (!(1 & (counted >> b.loc.group))) {
            ++rv;
            counted |= 1U << b.loc.group; } }
    return rv;
}

/** Returns each vector of match data.  If multiple hash groups are used, then the allocation,
 *  per hash group is provided
 */
IXBar::Use::TotalBytes IXBar::Use::match_hash(safe_vector<int> *hash_groups) const {
    TotalBytes rv;
    if (type == ATCAM_MATCH) {
        rv = atcam_match();
        if (hash_groups) {
            int atcam_partition_group = -1;
            atcam_partition(&atcam_partition_group);
            hash_groups->push_back(atcam_partition_group);
        }
        return rv;
    }
    return rv;
}

/** Do not count partition indexes within the match bytes and double count repeated bytes
 */
IXBar::Use::TotalBytes IXBar::Use::atcam_match() const {
    TotalBytes single_match;
    single_match.emplace_back(new safe_vector<Byte>());
    for (auto byte : use) {
        if (byte.is_spec(ATCAM_INDEX))
            continue;
        single_match[0]->push_back(byte);
        if (byte.is_spec(ATCAM_DOUBLE)) {
            auto repeat_byte = byte;
            repeat_byte.match_index = 1;
            single_match[0]->push_back(repeat_byte);
        }
    }
    return single_match;
}

/** Provides the bytes and hash group location of the partition index of an atcam table
 */
safe_vector<IXBar::Use::Byte> IXBar::Use::atcam_partition(int *) const {
    safe_vector<IXBar::Use::Byte> partition;
    for (auto byte : use) {
        if (!byte.is_spec(ATCAM_INDEX))
            continue;
        partition.push_back(byte);
    }
    return partition;
}

/** Base matching on search buses rather than ixbar groups, as potentially an input xbar
 *  group for an ATCAM table has multiple matches in it.
 */
int IXBar::Use::search_buses_single() const {
    unsigned counted = 0;
    int rv = 0;

    for (auto &b : *match_hash()[0]) {
        assert(b.loc.group >= 0 && b.loc.group < 16);
        assert(b.search_bus >= 0 && b.search_bus < 16);
        if (((1 << b.search_bus) & counted) == 0) {
            ++rv;
            counted |= 1U << b.search_bus;
        }
    }
    return rv;
}

/** Returns the ixbar group that the gateway is allocated to */
int IXBar::Use::gateway_group() const {
    int rv = -1;
    bool unset = true;
    for (auto &b : use) {
        if (unset) {
            rv = b.loc.group;
            unset = false;
        } else if (rv != b.loc.group)
            BUG("Gateway can only currently be allocated to one ixbar group");
    }
    return rv;
}

/** Provides information per search bus of how many bytes/bits a particular section of
 *  table uses in order to determine what section is the best candidate to ghost off
 */
safe_vector<IXBar::Use::TotalInfo> IXBar::Use::bits_per_search_bus() const {
    BUG("bits_per_search_bus not done for target");
    return {};
}

unsigned IXBar::Use::compute_hash_tables() {
    BUG("compute_hash_tables not done for target");
    return 0;
}

/* Combining the allocation of multiple separately allocated hash groups of the same
   table.  Done if the table requires two sources */
void IXBar::Use::add(const IXBar::Use &alloc) {
    type = alloc.type;
    used_by = alloc.used_by;

    for (auto old_byte : use) {
        for (auto new_byte : alloc.use) {
            if (old_byte.loc.group == new_byte.loc.group
                && old_byte.loc.byte == new_byte.loc.byte)
                BUG("Two combined input xbar groups are using the same byte location");
        }
    }
    for (auto old_way : way_use) {
        for (auto new_way : alloc.way_use) {
            if (old_way.source == new_way.source)
                BUG("Ways from supposedly different sources have same sources?");
        }
    }

    use.insert(use.end(), alloc.use.begin(), alloc.use.end());
    way_use.insert(way_use.end(), alloc.way_use.begin(), alloc.way_use.end());;
}

/** Visualization Information of Bytes and their corresponding Hash Matrix Bits
 */
std::string IXBar::Use::used_for() const {
    if (type == EXACT_MATCH) {
        return "exact_match";
    } else if (type == ATCAM_MATCH) {
        return "atcam_match";
    } else if (type == TERNARY_MATCH) {
        return "ternary_match";
    } else if (type == TRIE_MATCH) {
        return "trie_match";
    } else if (type == GATEWAY) {
        return "gateway";
    } else if (type == SELECTOR) {
        return "selection";
    } else if (type == METER) {
        return "meter";
    } else if (type == STATEFUL_ALU) {
        return "stateful_alu";
    } else if (type == HASH_DIST) {
        return hash_dist_used_for();
    }
    BUG("Invalid type of input xbar: %d", type);
    return "";
}

void IXBar::Use::update_resources(int stage, BFN::Resources::StageResources &stageResource) const {
    LOG_FEATURE("resources", 2, "add_xbar_bytes_usage (stage=" << stage <<
                                "), table: " << used_by);
    for (auto &byte : use) {
        bool ternary = type == IXBar::Use::TERNARY_MATCH;
        LOG_FEATURE("resources", 3, "\tadding resource: xbar bytes " << byte.loc.getOrd(ternary));
        stageResource.xbarBytes[byte.loc.getOrd(ternary)].emplace(used_by, used_for(), byte);
    }

    using UsageType = BFN::Resources::HashBitResource::UsageType;

    // Used for the bits to do exact match/atcam match
    int wayIndex = 0;
    for (auto &way : way_use) {
        for (int bit = way.index.lo; bit <= way.index.hi; bit++) {
            auto key = std::make_pair(bit, way.source);

            LOG_FEATURE("resources", 3, "\tadding resource hash_bits from way_use(" << bit <<
                                        ", " << way.source << "): {" << used_by << " --> " <<
                                        used_for() << ": " << "Hash Way " << wayIndex <<
                                        " RAM line select" << "}");
            stageResource.hashBits[key].append(
                used_by,
                used_for(),
                UsageType::WayLineSelect,
                wayIndex);
        }

        for (auto bit : bitvec(way.select_mask)) {
            bit += way.select.lo;
            auto key = std::make_pair(bit, way.source);

            LOG_FEATURE("resources", 3, "\tadding resource hash_bits from way_use(" << bit <<
                                        ", " << way.source << "): {" << used_by << " --> " <<
                                        used_for() << ": " << "Hash Way " << wayIndex <<
                                        " RAM select" << "}");
            stageResource.hashBits[key].append(
                used_by,
                used_for(),
                UsageType::WaySelect,
                wayIndex);
        }

        wayIndex++;
    }
}



/** Visualization Information on Hash Distribution Units
 */
std::ostream &operator<<(std::ostream &out, IXBar::Use::type_t type) {
    static const char *types[] = { "Exact", "ATCam", "Ternary", "Trie", "Gateway", "Action",
        "ProxyHash", "Selector", "Meter", "StatefulAlu", "HashDist" };
    if (size_t(type) < sizeof(types)/sizeof(types[0]))
        out << types[type];
    else
        out << "<type " << int(type) << ">";
    return out;
}

void IXBar::Use::dbprint(std::ostream &out) const {
    out << type;
    for (auto &b : use)
        out << Log::endl << b;
    for (auto &w : way_use)
        out << Log::endl << "[ " << w.source << ", " << w.index << ", " << w.select << ", 0x"
            << hex(w.select_mask) << " ]";
}

void dump(const IXBar::Use *use) {
    std::cout << *use << std::endl;
}
void dump(const IXBar::Use &use) {
    std::cout << use << std::endl;
}
void dump(const IXBar::Use::Byte *ub) {
    std::cout << *ub << std::endl;
}
void dump(const IXBar::Use::Byte &ub) {
    std::cout << ub << std::endl;
}

std::string IXBar::FieldInfo::visualization_detail() const {
    std::string rv = get_use_name() + "";
    rv += "[" + std::to_string(lo) + ":" + std::to_string(hi) + "]";
    return rv;
}

PHV::FieldSlice IXBar::FieldInfo::field_slice(const PhvInfo &phv) const {
    le_bitrange bits(lo, hi);
    auto finfo = phv.field(field);
    BUG_CHECK(finfo, "%s is not a field?", field);
    return PHV::FieldSlice(finfo, bits);
}

/** Visualization Details on what bits are used within a particular byte are used within a
 *  table.  Unused bits are printed out as unused
 */
std::string IXBar::Use::Byte::visualization_detail() const {
    const auto slices = get_slices_for_visualization();
    return "{" + std::accumulate(
            slices.begin() + 1,
            slices.end(),
            slices.begin()->visualization_detail(),
            [] (const std::string &str, const FieldInfo &slice) {
                return str + ", " + slice.visualization_detail();
            }) + "}";
}

std::vector<IXBar::FieldInfo> IXBar::Use::Byte::get_slices_for_visualization() const {
    std::vector<IXBar::FieldInfo> result;

    bitvec byte(0, 8);
    bitvec unused_bv = byte - bit_use;
    int unused_start_bit = 0;
    int used_start_bit = bit_use.ffs();
    int unused_end_bit = used_start_bit;
    bool first_time = true;

    auto it = field_bytes.begin();

    do {
        int used_end_bit = bit_use.ffz(used_start_bit);
        if (unused_start_bit < unused_end_bit) {
            int lo = 0; int hi = (unused_end_bit - unused_start_bit) - 1;
            result.push_back(FieldInfo("unused"_cs, lo, hi, 0, std::nullopt));
        }

        BUG_CHECK(used_start_bit != used_end_bit, "The bit_use object in %s is incorrectly "
                 "configured", *this);

        while (it != field_bytes.end()) {
            if (it->cont_lo > used_end_bit) break;
            result.push_back(*it);
            it++;
        }

        unused_start_bit = used_end_bit;
        used_start_bit = bit_use.ffs(used_end_bit);
        unused_end_bit = used_start_bit;
        first_time = false;
    } while (used_start_bit >= 0);

    if (unused_start_bit < 8) {
        BUG_CHECK(!first_time, "Byte %s has no field slices", *this);

        int lo = 0; int hi = 7 - unused_start_bit;
        result.push_back(FieldInfo("unused"_cs, lo, hi, 0, std::nullopt));
    }

    return result;
}

bool IXBar::Use::Byte::is_subset(const Byte &b) const {
    if (container != b.container && lo != b.lo)
        return false;
    for (auto fi : field_bytes) {
        bool is_subset = false;
        for (auto b_fi : b.field_bytes) {
            if (fi.field != b_fi.field) continue;
            if (!b_fi.range().contains(fi.range())) continue;
            is_subset = true;
            break;
        }
        if (!is_subset)
            return false;
    }
    return true;
}

bool IXBar::Use::Byte::can_add_info(const FieldInfo &fi) const {
    bitvec overlap_bits = bit_use & fi.cont_loc();
    for (auto br : bitranges(overlap_bits)) {
        le_bitrange field_bits = { br.first, br.second };
        field_bits = field_bits.shiftedByBits(fi.lo - fi.cont_lo);
        bool is_subset = false;
        for (auto c_fi : field_bytes) {
            if (c_fi.field == fi.field && c_fi.range().contains(field_bits)) {
                is_subset = true;
                break;
            }
        }
        if (!is_subset) {
            return false;
        }
    }
    return true;
}

void IXBar::Use::Byte::add_info(const FieldInfo &fi) {
    safe_vector<FieldInfo> add_fi;
    bitvec non_overlap_bits = fi.cont_loc() - bit_use;
    for (auto br : bitranges(non_overlap_bits)) {
        le_bitrange field_bits = { br.first, br.second };
        field_bits = field_bits.shiftedByBits(fi.lo - fi.cont_lo);
        add_fi.emplace_back(fi.field, field_bits.lo, field_bits.hi, br.first, fi.aliasSource);
    }

    field_bytes.insert(field_bytes.end(), add_fi.begin(), add_fi.end());
    std::sort(field_bytes.begin(), field_bytes.end(), [](const FieldInfo &a, const FieldInfo &b) {
        return a.cont_lo < b.cont_lo;
    });

    BUG_CHECK(field_bytes.size() > 0, "Should be at least one field slice on a byte");
    for (size_t i = 0; i < field_bytes.size() - 1; i++) {
        auto &field_a = field_bytes[i];
        auto &field_b = field_bytes[i+1];
        if (field_a.field != field_b.field)
            continue;
        if (field_a.cont_hi() + 1 != field_b.cont_lo)
            continue;
        if (field_a.hi + 1 != field_b.lo)
            continue;
        field_a.hi += field_b.width();
        field_bytes.erase(field_bytes.begin() + i + 1);
        i--;
    }
    bit_use |= fi.cont_loc();
}

static int need_align_flags[4][4] = {
    { 0, 0, 0, 0 },  // 8bit -- no alignment needed
    { IXBar::Use::Align16lo, IXBar::Use::Align16hi, IXBar::Use::Align16lo, IXBar::Use::Align16hi },
    { IXBar::Use::Align16lo | IXBar::Use::Align32lo,
      IXBar::Use::Align16hi | IXBar::Use::Align32lo,
      IXBar::Use::Align16lo | IXBar::Use::Align32hi,
      IXBar::Use::Align16hi | IXBar::Use::Align32hi },
    { 0, 0, 0, 0, },  // Not yet allocated -- assume no alignment required
};

/* Add the pre-allocated bytes to the Use structure */
void IXBar::add_use(ContByteConversion &map_alloc, const PHV::Field *field,
                    const PhvInfo &phv, const IR::MAU::Table *ctxt,
                    std::optional<cstring> aliasSourceName, const le_bitrange *bits,
                    int flags, byte_type_t byte_type, unsigned extra_align, int range_index,
                    int ixbar_group_num) {
    LOG5("Adding IXBar Use for field - " << field << Log::endl << "  on table : " << ctxt->name
            << ", flags : " << flags << ", byte_type: " << byte_type
            << ", extra_align: " << extra_align << ", range_index: " << range_index
            << ", ixbar_group_num: " << ixbar_group_num);

    bool ok = false;
    int index = 0;
    PHV::FieldUse use(PHV::FieldUse::READ);
    // FIXME: This will currently not work before PHV allocation, because the foreach_byte
    // over alloc_slices only works if the slices have been allocated, according to Cole.
    // If we want to move TablePlacement before PHV allocation in the future, this will have
    // to change
    BUG_CHECK(ctxt, "Context not declared");
    field->foreach_byte(bits, ctxt, &use, [&](const PHV::AllocSlice &sl) {
        ok = true;  // FIXME -- better sanity check?
        // FIXME: This will not work if moved before PHV allocation
        IXBar::FieldInfo fi(field->name, sl.field_slice().lo, sl.field_slice().hi,
                            sl.container_slice().lo % 8, aliasSourceName);

        // FIXME: Unclear if this is too constrained, as the bits that aren't live at the same
        // time may still be non-zero even though they aren't live.  This will always mark
        // all bits that are ever allocated
        bitvec all_bits = phv.bits_allocated(sl.container());
        IXBar::Use::Byte byte(sl.container(), (sl.container_slice().lo/8U) * 8U);

        byte.non_zero_bits = all_bits.getslice((sl.container_slice().lo / 8U) * 8U, 8);
        byte.flags =
            flags | need_align_flags[sl.container().log2sz()][(sl.container_slice().lo/8U) & 3]
                  | need_align_flags[extra_align][index & 3];
        // FIXME -- for (jbay) 128-bit salu, extra_align ends up being 3, so we're not adding
        // any extra alignment here as it falls into the 'not yet allocated'.  This is either
        // a happy accident, or incorrect -- I'm not sure if we need extra alignment for this
        // case.  It will generally fill the entire 128-bit group, or use hash anyways.
        if (byte_type == IXBar::ATCAM) {
            byte.set_spec(IXBar::ATCAM_DOUBLE);
        } else if (byte_type == IXBar::PARTITION_INDEX) {
            byte.set_spec(IXBar::ATCAM_INDEX);
        }

        if (ixbar_group_num != -1) {
            byte.ixbar_group_num = ixbar_group_num;
            LOG3("initialize ixbar_group_num " << ixbar_group_num << " for field " << field->name);
        }

        if (byte_type == IXBar::RANGE) {
            byte.range_index = range_index;
            if ((sl.container_slice().lo % 8) < 4) {
                byte.set_spec(IXBar::RANGE_LO);
                map_alloc[byte].push_back(fi);
            }
            if ((sl.container_slice().hi % 8) > 3) {
                byte.clear_spec(IXBar::RANGE_LO);
                byte.set_spec(IXBar::RANGE_HI);
                map_alloc[byte].push_back(fi);
            }
        } else {
            map_alloc[byte].push_back(fi);
        }

        index++;
    });
    if (!ok) {
        // should be ok as we'll backtrack and redo layout after final PHV alloc.
        LOG1("field " << field->name << " is not allocated in the PHV, ixbar alloc will be "
             "incorrect"); }
}

/** In order to prevent some overlay bugs by the driver, this guarantees that if a table matches
 *  on multiple overlaid bits, that these bits appear twice in the match key.  One could in
 *  theory have a ternary match table that has the following:
 *
 *  key {
 *       h1.f1 : ternary;
 *       h2.f1 : ternary;
 *  }
 *
 *  where if h1.f1 and h2.f1 are never live at the same time, could be that a don't care
 *  match is always turned on for at least one of the two fields.  This could potentially
 *  be a save on logical tables.
 *
 *  However, the driver currently writes the fields in the order in the context JSON, not in
 *  the order of write don't care before do care, and in this instance, if these fields
 *  were overlaid on the match, could potentially overwrite one of the fields.
 *
 *  Thus currently, the fields have to appear multiple times within the match, and as of
 *  right now, also need to appear multiple times on the IXBar.  In some cases this might
 *  not be true.  For ternary table, a single byte in the match must be a single byte in the
 *  ixbar.  However, for exact match, a byte can be swizzled multiple times, which we take
 *  advantage of in an ATCAM match to save room.  However the compiler will not do this for
 *  multiple appearances of overlaid fields.
 *
 *  Furthermore, each byte is classified by a byte_speciaility_t.  Bytes with different
 *  specialities themselves will not be overlaid during the allocation process.  This management
 *  just becomes difficult
 *
 */
void IXBar::create_alloc(ContByteConversion &map_alloc, safe_vector<Use::Byte> &bytes) {
    for (auto &entry : map_alloc) {
        safe_vector<IXBar::Use::Byte> created_bytes;

        for (auto &fi : entry.second) {
            bool add_new_byte = true;
            int index = 0;
            safe_vector<FieldInfo> non_overlap_field_info;
            for (auto c_byte : created_bytes) {
                if (c_byte.can_add_info(fi)) {
                    add_new_byte = false;
                    break;
                }
                index++;
            }
            if (add_new_byte)
                created_bytes.emplace_back(entry.first);
            created_bytes[index].add_info(fi);
        }
        bytes.insert(bytes.end(), created_bytes.begin(), created_bytes.end());
    }

    // Putting the fields in container order so the visualization prints them out in
    // le_bitrange order
    for (auto &byte : bytes) {
        std::sort(byte.field_bytes.begin(), byte.field_bytes.end(),
            [](const FieldInfo &a, const FieldInfo &b) {
            return a.cont_lo < b.cont_lo;
        });
    }

    for (auto &byte : bytes) {
        LOG5("Allocate " << byte);
        // Used to print initialization information for gtest
        LOG_FEATURE("gtest", 2, "Allocate " << byte.container << " lo " << byte.lo
             << " bit_use " << byte.bit_use
             << " flag " << byte.flags
             << " non_zero_bits " << byte.non_zero_bits
             << " specialities " << byte.specialities
             << " search_bus " << byte.search_bus
             << " match_index " << byte.match_index
             << " range_index " << byte.range_index
             << " proxy_hash " << byte.proxy_hash);
    }
}

void IXBar::create_alloc(ContByteConversion &map_alloc, IXBar::Use &alloc) {
    create_alloc(map_alloc, alloc.use);
}

/** IXBar::FieldManagement: This is for adding fields to be allocated in the ixbar
  * allocation scheme.  Used by match tables, selectors, and hash distribution */
// BEWARE: The preorder implementations for ListExpression and StructExpression
//         MUST be kept in sync. For future extensions, getListExprComponents()
//         may come in handy.
bool IXBar::FieldManagement::preorder(const IR::ListExpression *) {
    if (!ki.hash_dist)
        BUG("A field list is somehow contained within the reads in table %s", tbl->name);
    return true;
}

// See the preorder implementation for ListExpression.
bool IXBar::FieldManagement::preorder(const IR::StructExpression *) {
    if (!ki.hash_dist)
        BUG("A field list is somehow contained within the reads in table %s", tbl->name);
    return true;
}

bool IXBar::FieldManagement::preorder(const IR::Mask *) {
    BUG("Masks should have been converted to Slices before input xbar allocation");
    return true;
}

bool IXBar::FieldManagement::preorder(const IR::MAU::TableKey *read) {
    if (ki.is_atcam) {
        if (ki.partition != read->partition_index)
            return false; }
    return true;
}

bool IXBar::FieldManagement::preorder(const IR::Constant *c) {
    field_list_order.push_back(c);
    return true;
}

bool IXBar::FieldManagement::preorder(const IR::MAU::ActionArg *aa) {
    error(ErrorType::ERR_INVALID, "Can't use action argument %1% in a hash in the same action;"
          " try splitting the action", aa);
    return false;
}

static std::string db_slices(const IR::MAU::Table *tbl, const PHV::Field *field, le_bitrange bits) {
    std::stringstream rv;
    bool first = true;
    PHV::FieldUse READ(PHV::FieldUse::READ);
    field->foreach_alloc(bits, tbl, &READ, [&](const PHV::AllocSlice &slice) {
        if (first)
            first = false;
        else
            rv << ", ";
        rv << slice.container() << "[" << slice.container_slice().hi << ":"
            << slice.container_slice().lo << "]"; });
    return rv.str();
}

bool IXBar::FieldManagement::preorder(const IR::Expression *e) {
    LOG3("IXBar::FieldManagement preorder expression : " << e);
    le_bitrange bits = { };
    auto *finfo = phv.field(e, &bits);
    if (!finfo) return true;
    LOG4("  " << db_slices(tbl, finfo, bits));
    field_list_order.push_back(e);
    bitvec field_bits(bits.lo, bits.hi - bits.lo + 1);
    // Currently, due to driver, only one field is allowed to be the partition index
    if (map_alloc == nullptr || fields_needed == nullptr) {
        BUG_CHECK(map_alloc == nullptr && fields_needed == nullptr, "Invalid call of the "
            "IXBar::FieldManagement pass");
        return false;
    }

    byte_type_t byte_type = NO_BYTE_TYPE;
    int ixbar_group_num = -1;
    if (auto *read = findContext<IR::MAU::TableKey>()) {
        if (ki.is_atcam) {
            if (read->partition_index)
                byte_type = PARTITION_INDEX;
            else if (read->match_type.name == "ternary" || read->match_type.name == "lpm")
                byte_type = ATCAM;
        }
        if (read->match_type.name == "range")
            byte_type = RANGE;

        ixbar_group_num = read->ixbar_group_num;
        LOG3("  " << read->match_type.name << " " << byte_type << " " << ixbar_group_num);
    }
    if (byte_type == PARTITION_INDEX) {
        int diff = bits.size() - ki.partition_bits;
        if (diff > 0)
            bits.hi -= diff;
    }

    if (fields_needed->count(finfo->name)) {
        auto &allocated_bits = fields_needed->at(finfo->name);
        if ((allocated_bits & field_bits).popcount() == field_bits.popcount())
            return false;
        (*fields_needed)[finfo->name] |= field_bits;
    } else {
        (*fields_needed)[finfo->name] = field_bits;
    }

    std::optional<cstring> aliasSourceName = phv.get_alias_name(e);
    add_use(*map_alloc, finfo, phv, tbl, aliasSourceName, &bits, 0, byte_type, 0,
            ki.range_index, ixbar_group_num);
    if (byte_type == RANGE) {
        ki.range_index++;
    }
    return false;
}

void IXBar::FieldManagement::postorder(const IR::BFN::SignExtend *c) {
    BUG_CHECK(!field_list_order.empty(), "SignExtend on nonexistant field");
    BUG_CHECK(field_list_order.back() == c->expr, "SignExtend mismatch");
    int size = c->expr->type->width_bits();
    for (int i = c->type->width_bits(); i > size; --i) {
        field_list_order.insert(field_list_order.end() - 1,
            MakeSlice(c->expr, size - 1, size - 1)); }
}

/**
 * FIXME: This a nasty hack due to the way dynamic hashing is currently handled in
 * the backend.  If multiple field_lists exists, which is true for the dyn_hash test,
 * then this algorithm removes the duplicated fields so that the dynamic hashing
 * doesn't crash .  When the dynamic hashing is correctly handled in the backend,
 * this can go away.
 */
void IXBar::FieldManagement::end_apply() {
    if (ki.repeats_allowed)
        return;
    std::map<cstring, bitvec> field_list_check;
    for (auto it = field_list_order.begin(); it != field_list_order.end(); it++) {
        le_bitrange bits;
        auto field = phv.field(*it, &bits);
        if (field == nullptr)
            continue;

        bitvec used_bits(bits.lo, bits.size());

        bitvec overlap;
        if (field_list_check.count(field->name) > 0)
            overlap = field_list_check.at(field->name) & used_bits;
        if (!overlap.empty()) {
            if (overlap == used_bits) {
                it = field_list_order.erase(it);
                it--;
            } else {
                error(ErrorType::ERR_INVALID,
                        "Overlapping field %2% in table %1% not supported with the hashing "
                        "algorithm", tbl, field->name);
            }
        }
        field_list_check[field->name] |= used_bits;
    }
}

void dump(const IXBar *ixbar) {
    std::cout << *ixbar;
}
void dump(const IXBar &ixbar) {
    std::cout << ixbar;
}

void IXBar::update(const IR::MAU::Table *tbl, const TableResourceAlloc *rsrc) {
    const IR::MAU::Selector *as = nullptr;
    const IR::MAU::Meter *mtr = nullptr;
    const IR::MAU::StatefulAlu *salu = nullptr;

    for (auto ba : tbl->attached) {
        if (auto as_p = ba->attached->to<IR::MAU::Selector>())
            as = as_p;
        if (auto mtr_p = ba->attached->to<IR::MAU::Meter>())
            mtr = mtr_p;
        if (auto salu_p = ba->attached->to<IR::MAU::StatefulAlu>())
            salu = salu_p;
    }

    auto name = tbl->name;
    if (as && (allocated_attached.count(as) == 0)) {
        if (rsrc->selector_ixbar) {
            update(name + "$select", *rsrc->selector_ixbar);
            allocated_attached.emplace(as, *rsrc->selector_ixbar); }
    }
    if (mtr && (allocated_attached.count(mtr) == 0)) {
        if (rsrc->meter_ixbar) {
            update(name + "$mtr", *rsrc->meter_ixbar);
            allocated_attached.emplace(mtr, *rsrc->meter_ixbar); }
    }
    if (salu && (allocated_attached.count(salu) == 0)) {
        if (!tbl->for_dleft() && salu->for_dleft()) {
            // FIXME -- if this SALU is used for dleft, then it must be accounted for
            // with its dleft match table; otherwise the dleft hash won't be allocated
            // properly.  So we hack it by simply skipping it when we see it with a
            // non-dleft table
        } else {
            if (rsrc->salu_ixbar) {
                update(name + "$salu", *rsrc->salu_ixbar);
                allocated_attached.emplace(salu, *rsrc->salu_ixbar); }
        }
    }

    if (rsrc->proxy_hash_ixbar)
        update(name + "$proxy_hash", *rsrc->proxy_hash_ixbar);
    if (rsrc->gateway_ixbar)
        update(name + "$gw", *rsrc->gateway_ixbar);
    if (rsrc->action_ixbar)
        update(name + "$act", *rsrc->action_ixbar);
    if (rsrc->match_ixbar)
        update(name, *rsrc->match_ixbar);
}

void IXBar::update(const IR::MAU::Table *tbl) {
    if (tbl->is_placed())
        update(tbl, tbl->resources);
}

IXBar *IXBar::create() {
    return new Tofino::IXBar();
}

void IXBar::add_names(cstring n, std::map<cstring, char> &names) {
    if (n && !names.count(n)) {
        if (names.size() >= 52)
            names.emplace(n, '?');
        else if (names.size() >= 26)
            names.emplace(n, 'a' + names.size() - 26);
        else
            names.emplace(n, 'A' + names.size()); }
}
void IXBar::add_names(const std::pair<PHV::Container, int>& c, std::map<cstring, char> &names) {
    add_names(c.first ? c.first.toString() : cstring(), names); }
void IXBar::add_names(PHV::Container c, std::map<cstring, char> &names) {
    add_names(c ? c.toString() : cstring(), names); }

void IXBar::sort_names(std::map<cstring, char> &names) {
    char a = 'A' - 1;
    for (auto &n : names) {
        n.second = ++a;
        if (a == 'Z') a = 'a' - 1;
        if (a == 'z') a = '0' - 1;
        if (a == '9' || a == '?') a = '?' - 1; }
}

void IXBar::write_one(std::ostream &out, const std::pair<cstring, int> &f,
                      std::map<cstring, char> &fields) {
    if (f.first) {
        out << fields.at(f.first) << hex(f.second/8);
    } else {
        out << ".."; }
}
void IXBar::write_one(std::ostream &out, cstring n, std::map<cstring, char> &names) {
    if (n) {
        out << names.at(n);
    } else {
        out << '.'; }
}
void IXBar::write_one(std::ostream &out, const std::pair<PHV::Container, int> &f,
                      std::map<cstring, char> &fields) {
    write_one(out, std::make_pair(f.first ? f.first.toString() : cstring(), f.second), fields);
}
void IXBar::write_one(std::ostream &out, PHV::Container f, std::map<cstring, char> &fields) {
    write_one(out, std::make_pair(f ? f.toString() : cstring(), 0), fields);
}
