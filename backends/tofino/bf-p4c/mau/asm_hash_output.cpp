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

#include "asm_hash_output.h"

void emit_ixbar_gather_map(const PhvInfo &phv, std::multimap<int, Slice> &match_data_map,
        std::map<le_bitrange, const IR::Constant*> &constant_map,
        const safe_vector<Slice> &match_data,
        const safe_vector<const IR::Expression *> &field_list_order, const LTBitMatrix &sym_keys,
        int &total_size);

bool EmitHashExpression::preorder(const IR::Concat *c) {
    visit(c->right, "right");
    bit += c->right->type->width_bits();
    visit(c->left, "left");
    return false;
}

bool EmitHashExpression::preorder(const IR::BFN::SignExtend *c) {
    le_bitrange     bits;
    if (auto *field = phv.field(c->expr, &bits)) {
        Slice f(field, bits);
        int ext = c->type->width_bits() - bits.size();
        out << indent << bit << ".." << (bit + bits.size() - 1) << ": " << f << std::endl
            << indent << (bit + bits.size());
        if (ext > 1) out << ".." << (bit + c->type->width_bits() - 1);
        out << ": stripe(" << Slice(f, bits.size()-1) << ")" << std::endl;
    } else {
        BUG("%s too complex in EmitHashExpression", c);
    }
    return false;
}

bool EmitHashExpression::preorder(const IR::Constant *) {
    // FIXME -- if the constant is non-zero, it should be included into the 'seed'
    return false;
}

bool EmitHashExpression::preorder(const IR::Expression *e) {
    le_bitrange     bits;
    if (auto *field = phv.field(e, &bits)) {
        Slice sl(field, bits);
        for (auto match_sl : match_data) {
            auto overlap = sl & match_sl;
            if (!overlap) continue;
            auto bit = this->bit + overlap.get_lo() - sl.get_lo();
            out << indent << bit;
            if (overlap.width() > 1)
                out << ".." << (bit + overlap.width() - 1);
            out << ": " << overlap << std::endl; }
    } else if (e->is<IR::Slice>()) {
        // allow for slice on HashGenExpression
        return true;
    } else {
        BUG("%s too complex in EmitHashExpression", e);
    }
    return false;
}

bool EmitHashExpression::preorder(const IR::MAU::HashGenExpression *hge) {
    auto *fl = hge->expr->to<IR::MAU::FieldListExpression>();
    BUG_CHECK(fl, "HashGenExpression not a field list: %s", hge);
    if (hge->algorithm.type == IR::MAU::HashFunction::IDENTITY) {
        // For identity, just output each field individually
        for (auto *el : boost::adaptors::reverse(fl->components)) {
            visit(el, "component");
            bit += el->type->width_bits(); }
        return false; }
    le_bitrange br = { 0, hge->hash_output_width };
    if (auto *sl = getParent<IR::Slice>()) {
        br.lo = sl->getL();
        br.hi = sl->getH(); }
    out << indent << bit << ".." << (bit + br.size() - 1) << ": ";
    safe_vector<const IR::Expression *> field_list_order;
    int total_bits = 0;
    for (auto e : fl->components)
        field_list_order.push_back(e);
    if (hge->algorithm.ordered()) {
        std::multimap<int, Slice> match_data_map;
        std::map<le_bitrange, const IR::Constant*> constant_map;
        LTBitMatrix sym_keys;  // FIXME -- needed?  always empty for now
        emit_ixbar_gather_map(phv, match_data_map, constant_map, match_data,
                                   field_list_order, sym_keys, total_bits);
        out << FormatHash(nullptr, &match_data_map, &constant_map, nullptr,
                          hge->algorithm, total_bits, &br);
    } else {
        // FIXME -- need to set total_bits to something?
        out << FormatHash(&match_data, nullptr, nullptr, nullptr,
                          hge->algorithm, total_bits, &br); }
    out << std::endl;
    return false;
}

/**
 * This funciton is to emit the match data function associated with an SRAM match table.
 */
void emit_ixbar_match_func(std::ostream &out, indent_t indent,
        safe_vector<Slice> &match_data, Slice *ghost, le_bitrange hash_bits) {
    if (match_data.empty() && ghost == nullptr)
        return;
    out << indent << hash_bits.lo;
    if (hash_bits.size() != 1)
        out << ".." << hash_bits.hi;
    out << ": " << FormatHash(&match_data, nullptr, nullptr, ghost,
                              IR::MAU::HashFunction::random()) << std::endl;
}

/** This function is necessary due to the limits of the driver's handling of ATCAM tables.
 *  The driver requires that the partition index hash be in the same order as the bits
 *  of the partition index.
 *
 *  This will eventually force limitations in the implementation of ATCAM tables, i.e.
 *  multiple fields or potentially a slice being used as the partition index.  The true
 *  way this should be handled is the same way as an exact match table, by generating the
 *  hash from the hash matrix provided to driver.  When this support is provided, this
 *  function becomes unnecessary, and can just go through the same pathway that exact
 *  match goes through.
 */
void emit_ixbar_hash_atcam(std::ostream &out, indent_t indent,
        safe_vector<Slice> &ghost, const ::IXBar::Use *use, int hash_group) {
    safe_vector<Slice> empty;
    BUG_CHECK(use->way_use.size() == 1, "One and only one way necessary for ATCAM tables");
    for (auto ghost_slice : ghost) {
        int start_bit = 0;  int end_bit = 0;
        if (ghost_slice.get_lo() >= TableFormat::RAM_GHOST_BITS)
            continue;
        start_bit = ghost_slice.get_lo();
        Slice adapted_ghost = ghost_slice;
        if (ghost_slice.get_hi() < TableFormat::RAM_GHOST_BITS) {
            end_bit = ghost_slice.get_hi();
        } else {
            int diff = ghost_slice.get_hi() - TableFormat::RAM_GHOST_BITS + 1;
            end_bit = TableFormat::RAM_GHOST_BITS - 1;
            adapted_ghost.shrink_hi(diff);
        }

        le_bitrange hash_bits = { start_bit, end_bit };
        hash_bits = hash_bits.shiftedByBits(use->way_use[0].index.lo);
        emit_ixbar_match_func(out, indent, empty, &adapted_ghost, hash_bits);
    }

    unsigned mask_bits = 0;
    for (auto way : use->way_use) {
        if (way.source != hash_group)
            continue;
        mask_bits |= way.select_mask;
    }

    for (auto ghost_slice : ghost) {
        // int start_bit = 0;  int end_bit = 0;
        if (ghost_slice.get_hi() < TableFormat::RAM_GHOST_BITS)
            continue;

        int bits_seen = TableFormat::RAM_GHOST_BITS;
        for (auto br : bitranges(mask_bits)) {
            le_bitrange ixbar_bits = { bits_seen, bits_seen + (br.second - br.first) };
            le_bitrange ghost_bits = { ghost_slice.get_lo(), ghost_slice.get_hi() };
            bits_seen += ixbar_bits.size();
            auto boost_sl = toClosedRange<RangeUnit::Bit, Endian::Little>
                            (ixbar_bits.intersectWith(ghost_bits));
            if (boost_sl == std::nullopt)
                continue;
            le_bitrange sl_overlap = *boost_sl;
            le_bitrange hash_bits = { br.first + sl_overlap.lo - ixbar_bits.lo,
                                      br.second - (ixbar_bits.hi - sl_overlap.hi) };
            hash_bits = hash_bits.shiftedByBits(Device::ixbarSpec().ramSelectBitStart());
            Slice adapted_ghost = ghost_slice;
            if (ghost_slice.get_lo() < sl_overlap.lo)
                adapted_ghost.shrink_lo(sl_overlap.lo - ghost_slice.get_lo());
            if (ghost_slice.get_hi() > sl_overlap.hi)
                adapted_ghost.shrink_hi(ghost_slice.get_hi() - sl_overlap.hi);
            emit_ixbar_match_func(out, indent, empty, &adapted_ghost, hash_bits);
        }
    }
}

void ixbar_hash_exact_bitrange(Slice ghost_slice, int min_way_size,
        le_bitrange non_rotated_slice, le_bitrange comp_slice, int initial_lo_bit,
        safe_vector<std::pair<le_bitrange, Slice>> &ghost_positions) {
    auto boost_sl = toClosedRange<RangeUnit::Bit, Endian::Little>
                        (non_rotated_slice.intersectWith(comp_slice));
    if (boost_sl == std::nullopt)
        return;
    le_bitrange overlap = *boost_sl;
    int lo = overlap.lo - initial_lo_bit;
    int hi = overlap.hi - initial_lo_bit;
    le_bitrange hash_position = overlap;
    if (hash_position.lo >= min_way_size)
        hash_position = hash_position.shiftedByBits(-1 * min_way_size);
    ghost_positions.emplace_back(hash_position, ghost_slice(lo, hi));
}

/**
 * Fills in the slice_to_select_bits map, described in emit_ixbar_hash_exact
 */
void ixbar_hash_exact_info(int &min_way_size, int &min_way_slice,
        const ::IXBar::Use *use, int hash_group,
        std::map<int, bitvec> &slice_to_select_bits) {
    for (auto way : use->way_use) {
        if (way.source != hash_group)
            continue;
        bitvec local_select_mask = bitvec(way.select_mask);
        int curr_way_size = Device::ixbarSpec().ramLineSelectBits() + local_select_mask.popcount();
        min_way_size = std::min(min_way_size, curr_way_size);
        min_way_slice = std::min(way.index.lo / 10, min_way_slice);

        // Guarantee that way object that have the same slice bits also use a
        // similar pattern of select bits
        auto mask_p = slice_to_select_bits.find(way.index.lo / 10);
        if (mask_p != slice_to_select_bits.end()) {
            bitvec curr_mask = mask_p->second;
            BUG_CHECK(curr_mask.min().index() == local_select_mask.min().index()
                      || local_select_mask.empty(), "Shared line select bits are not coordinated "
                      "to shared ram select index");
            slice_to_select_bits[way.index.lo / 10] |= local_select_mask;
        } else {
            slice_to_select_bits[way.index.lo / 10] = local_select_mask;
        }
    }

    bitvec verify_overlap;
    for (auto kv : slice_to_select_bits) {
        BUG_CHECK((verify_overlap & kv.second).empty(), "The RAM select bits are not unique per "
                  "way");
        verify_overlap |= kv.second;
        BUG_CHECK(kv.second.empty() || kv.second.is_contiguous(), "The RAM select bits must "
                  "currently be contiguous for the context JSON");
    }
}

/**
 * The purpose of this code is to output the hash matrix specifically for exact tables.
 * This code classifies all of the ghost bits for this particular hash table.  The ghost bits
 * are the bits that appear in the hash but not in the table format.  This reduces the
 * number of bits actually needed to match against.
 *
 * The ghost bits themselves are spread through an identity hash, while the bits that appear
 * in the match are randomized.  Thus each ghost bit is assigned a corresponding bit in the
 * way bits or select bits within the match format.
 *
 * The hash matrix is specified on a hash table by hash table basis.  16 x 64b hash tables
 * at most are specified, and if the ghost bits do not appear in that particular hash table,
 * they will not be output.  The ident_bits_prev_alloc is used to track how where to start
 * the identity of the ghost bits within this particular hash table.
 *
 * The hash for an exact match function is the following:
 *
 *     1.  Random for all of the match related bits.
 *     2.  An identity hash for the ghost bits across each way.
 *
 * Each way is built up of both RAM line select bits and RAM select bits.  The RAM line select
 * bits is a 10 bit window ranging from 0-4 way * 10 bit sections of the 52 bit hash bus. The
 * RAM select bits is any section of the upper 12 bits of the hash.
 *
 * In order to increase randomness, the identity hash across different ways is different.
 * Essentially each identity hash is shifted by 1 bit per way, so that the same identity hash
 * does not end up on the same RAM line.
 */
void emit_ixbar_hash_exact(std::ostream &out, indent_t indent,
        safe_vector<Slice> &match_data, safe_vector<Slice> &ghost, const ::IXBar::Use *use,
        int hash_group, int &ident_bits_prev_alloc) {
    if (use->type == IXBar::Use::ATCAM_MATCH) {
        emit_ixbar_hash_atcam(out, indent, ghost, use, hash_group);
        return;
    }

    int min_way_size = INT_MAX;
    int min_way_slice = INT_MAX;
    // key is way.slice i.e. RAM line select, value is the RAM select mask.  Due to an optimization
    // multiple IXBar::Use::Way may have the same way.slice, and different way.mask values.
    std::map<int, bitvec> slice_to_select_bits;
    auto &ixbSpec = Device::ixbarSpec();

    ixbar_hash_exact_info(min_way_size, min_way_slice, use, hash_group, slice_to_select_bits);
    bool select_bits_needed = min_way_size > ixbSpec.ramLineSelectBits();
    bitvec ways_done;

    for (auto way : use->way_use) {
        if (ways_done.getbit(way.index.lo / 10))
            continue;
        if (way.source != hash_group)
            continue;
        ways_done.setbit(way.index.lo / 10);
        // pair is portion of identity function, slice of PHV field
        safe_vector<std::pair<le_bitrange, Slice>> ghost_line_select_positions;
        safe_vector<std::pair<le_bitrange, Slice>> ghost_ram_select_positions;
        int ident_pos = ident_bits_prev_alloc;
        for (auto ghost_slice : ghost) {
            int ident_pos_shifted = ident_pos + way.index.lo / 10 - min_way_slice;
            // This is the identity bits starting at bit 0 that this ghost slice will impact on
            // a per way basis.
            le_bitrange non_rotated_slice = { ident_pos_shifted,
                                              ident_pos_shifted + ghost_slice.width() - 1 };

            // The bits in the RAM line select that begin at the ident_pos_shifted bit up to 10
            bool pre_rotation_line_sel_needed = ident_pos_shifted < ixbSpec.ramLineSelectBits();
            if (pre_rotation_line_sel_needed) {
                le_bitrange ident_bits = { ident_pos_shifted, ixbSpec.ramLineSelectBits() - 1};
                ixbar_hash_exact_bitrange(ghost_slice, min_way_size, non_rotated_slice,
                                          ident_bits, ident_pos_shifted,
                                          ghost_line_select_positions);
            }

            // The bits in the RAM select, i.e. the upper 12 bits
            if (select_bits_needed) {
                le_bitrange ident_select_bits = { ixbSpec.ramLineSelectBits(), min_way_size - 1};
                ixbar_hash_exact_bitrange(ghost_slice, min_way_size, non_rotated_slice,
                                          ident_select_bits, ident_pos_shifted,
                                          ghost_ram_select_positions);
            }

            // The bits that start at bit 0 of the RAM line select
            bool post_rotation_needed = ident_pos_shifted + ghost_slice.width() > min_way_size;
            if (post_rotation_needed) {
                le_bitrange post_rotation_bits = { min_way_size,
                                                   min_way_size + ident_pos_shifted - 1 };
                ixbar_hash_exact_bitrange(ghost_slice, min_way_size, non_rotated_slice,
                                          post_rotation_bits, ident_pos_shifted,
                                          ghost_line_select_positions);
            }
            ident_pos += ghost_slice.width();
        }

        bitvec used_line_select_range;
        for (auto ghost_pos : ghost_line_select_positions) {
            used_line_select_range.setrange(ghost_pos.first.lo, ghost_pos.first.size());
        }

        safe_vector<le_bitrange> non_ghosted;
        bitvec no_ghost_line_select_bits
            = bitvec(0, ixbSpec.ramLineSelectBits())
              - used_line_select_range.getslice(0, ixbSpec.ramLineSelectBits());

        // Print out the portions that have no ghost impact, but have hash impact due to the
        // random hash on the normal match data (RAM line select)
        for (auto br : bitranges(no_ghost_line_select_bits)) {
            le_bitrange hash_bits = { br.first, br.second };
            hash_bits = hash_bits.shiftedByBits(way.index.lo);
            emit_ixbar_match_func(out, indent, match_data, nullptr, hash_bits);
        }

        // Print out the portions that have both match data and ghost data (RAM line select)
        for (auto ghost_pos : ghost_line_select_positions) {
            le_bitrange hash_bits = ghost_pos.first;
            hash_bits = hash_bits.shiftedByBits(way.index.lo);
            emit_ixbar_match_func(out, indent, match_data, &(ghost_pos.second), hash_bits);
        }

        bitvec ram_select_mask = slice_to_select_bits.at(way.index.lo / 10);
        bitvec used_ram_select_range;
        for (auto ghost_pos : ghost_ram_select_positions) {
            used_ram_select_range.setrange(ghost_pos.first.lo, ghost_pos.first.size());
        }
        used_ram_select_range >>= ixbSpec.ramLineSelectBits();

        bitvec no_ghost_ram_select_bits =
            bitvec(0, ram_select_mask.popcount()) - used_ram_select_range;

        // Print out the portions of that have no ghost data in RAM select
        for (auto br : bitranges(no_ghost_ram_select_bits)) {
            le_bitrange hash_bits = { br.first, br.second };
            // start at bit 40
            int shift = ixbSpec.ramSelectBitStart() + ram_select_mask.min().index();
            hash_bits = hash_bits.shiftedByBits(shift);
            emit_ixbar_match_func(out, indent, match_data, nullptr, hash_bits);
        }


        // Print out the portions that have ghost impact.  Assumed at the point that
        // the select bits are contiguous, not a hardware requirement, but a context JSON req.
        for (auto ghost_pos : ghost_ram_select_positions) {
            le_bitrange hash_bits = ghost_pos.first;
            int shift = ixbSpec.ramSelectBitStart() + ram_select_mask.min().index() -
                        ixbSpec.ramLineSelectBits();
            hash_bits = hash_bits.shiftedByBits(shift);
            emit_ixbar_match_func(out, indent, match_data, &(ghost_pos.second), hash_bits);
        }
    }

    for (auto ghost_slice : ghost) {
        ident_bits_prev_alloc += ghost_slice.width();
    }
}

/**
 * Given an order for an allocation, will determine the input position of the slice in
 * the allocation, and save it in the match_data_map
 *
 * When two keys are considered symmetric, currently they are given the same bit stream position
 * in the crc calculation in order to guarantee that their hash algorithm is identical for
 * those two keys.  This change, however, notes that the output of the function is not truly
 * the CRC function they requested, but a variation on it
 */
void emit_ixbar_gather_map(const PhvInfo &phv,
        std::multimap<int, Slice> &match_data_map,
        std::map<le_bitrange, const IR::Constant*> &constant_map,
        const safe_vector<Slice> &match_data,
        const safe_vector<const IR::Expression *> &field_list_order, const LTBitMatrix &sym_keys,
        int &total_size) {
    std::map<int, int> field_start_bits;
    std::map<int, int> reverse_sym_keys;
    int bits_seen = 0;
    for (int i = field_list_order.size() - 1; i >= 0; i--) {
        auto fs = PHV::AbstractField::create(phv, field_list_order.at(i));
        field_start_bits[i] = bits_seen;
        bitvec sym_key = sym_keys[i];
        if (!sym_key.empty()) {
            BUG_CHECK(sym_key.popcount() == 1 && reverse_sym_keys.count(sym_key.min().index()) == 0,
                "Symmetric hash broken in the backend");
            reverse_sym_keys[sym_key.min().index()] = i;
        }
        bits_seen += fs->size();
    }

    for (auto sl : match_data) {
        int order_bit = 0;
        // Traverse field list in reverse order. For a field list the convention
        // seems to indicate the field offsets are determined based on first
        int index = field_list_order.size();
        for (auto fs_itr = field_list_order.rbegin(); fs_itr != field_list_order.rend(); fs_itr++) {
            index--;
            auto fs = PHV::AbstractField::create(phv, *fs_itr);
            if (fs->is<PHV::Constant>()) {
                auto cons = fs->to<PHV::Constant>();
                le_bitrange br = { order_bit, order_bit + fs->size() - 1 };
                constant_map[br] = cons->value;
                order_bit += fs->size();
                continue;
            }

            if (fs->field() != sl.get_field()) {
                order_bit += fs->size();
                continue;
            }

            auto half_open_intersect = fs->range().intersectWith(sl.range());
            if (half_open_intersect.empty()) {
                order_bit += fs->size();
                continue;
            }

            le_bitrange intersect = { half_open_intersect.lo, half_open_intersect.hi - 1 };
            Slice adapted_sl = sl;


            int lo_adjust = std::max(intersect.lo - sl.range().lo, sl.range().lo - intersect.lo);
            int hi_adjust = std::max(intersect.hi - sl.range().hi, sl.range().hi - intersect.hi);
            adapted_sl.shrink_lo(lo_adjust);
            adapted_sl.shrink_hi(hi_adjust);
            int offset = adapted_sl.get_lo() - fs->range().lo;

            int sym_order_bit = reverse_sym_keys.count(index) > 0 ?
                                field_start_bits.at(reverse_sym_keys.at(index)) : order_bit;
            match_data_map.emplace(sym_order_bit + offset, adapted_sl);
            order_bit += fs->size();
        }
    }

    total_size = 0;
    for (auto fs : field_list_order) {
        total_size += fs->type->width_bits();
    }
}

/* Calculate the hash tables used by an individual P4 table in the IXBar */
void emit_ixbar_gather_bytes(const PhvInfo &phv,
        const safe_vector<::IXBar::Use::Byte> &use,
        std::map<int, std::map<int, Slice>> &sort, std::map<int, std::map<int, Slice>> &midbytes,
        const IR::MAU::Table *tbl, bool ternary, bool atcam) {
    PHV::FieldUse f_use(PHV::FieldUse::READ);
    for (auto &b : use) {
        BUG_CHECK(b.loc.allocated(), "Byte not allocated by assembly");
        int byte_loc = Device::ixbarSpec().ternaryBytesPerGroup();
        if (atcam && !b.is_spec(::IXBar::ATCAM_INDEX))
            continue;
        for (auto &fi : b.field_bytes) {
            auto field = phv.field(fi.get_use_name());
            BUG_CHECK(field, "Field not found");
            le_bitrange field_bits = { fi.lo, fi.hi };
            // It is not a guarantee, especially in Tofino2 due to live ranges being different
            // that a FieldInfo is not corresponding to a single alloc_slice object
            field->foreach_alloc(field_bits, tbl, &f_use, [&](const PHV::AllocSlice &sl) {
                if (b.loc.byte == byte_loc && ternary) {
                    Slice asm_sl(phv, fi.get_use_name(), sl.field_slice().lo, sl.field_slice().hi);
                    auto n = midbytes[b.loc.group/2].emplace(asm_sl.bytealign(), asm_sl);
                    BUG_CHECK(n.second, "duplicate byte use in ixbar");
                } else {
                    Slice asm_sl(phv, fi.get_use_name(), sl.field_slice().lo, sl.field_slice().hi);
                    auto n = sort[b.loc.group].emplace(b.loc.byte*8 + asm_sl.bytealign(), asm_sl);
                    BUG_CHECK(n.second, "duplicate byte use in ixbar");
                }
            }, PHV::SliceMatch::REF_PHYS_LR);
        }
    }

    // join together adjacent slices
    for (auto &group : sort) {
        auto it = group.second.begin();
        while (it != group.second.end()) {
            auto next = it;
            if (++next != group.second.end()) {
                Slice j = it->second.join(next->second);
                if (j && it->first + it->second.width() == next->first) {
                    it->second = j;
                    group.second.erase(next);
                    continue;
                }
            }
            it = next;
        }
    }
}
