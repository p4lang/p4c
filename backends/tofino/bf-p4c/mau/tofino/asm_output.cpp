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

#include <map>
#include <regex>
#include <string>
#include <vector>
#include <iterator>
#include <memory>

#include "action_data_bus.h"
#include "boost/range/adaptor/reversed.hpp"
#include "bf-p4c/common/alias.h"
#include "bf-p4c/common/ir_utils.h"
#include "bf-p4c/common/slice.h"
#include "bf-p4c/lib/error_type.h"
#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/mau/asm_output.h"
#include "bf-p4c/mau/asm_hash_output.h"
#include "bf-p4c/mau/asm_format_hash.h"
#include "bf-p4c/mau/gateway.h"
#include "bf-p4c/mau/payload_gateway.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/table_format.h"
#include "bf-p4c/mau/tofino/asm_output.h"
#include "bf-p4c/mau/tofino/input_xbar.h"
#include "bf-p4c/parde/asm_output.h"
#include "bf-p4c/mau/jbay_next_table.h"
#include "bf-p4c/parde/phase0.h"
#include "bf-p4c/phv/asm_output.h"
#include "lib/algorithm.h"
#include "lib/bitops.h"
#include "lib/bitrange.h"
#include "lib/hex.h"
#include "lib/indent.h"
#include "lib/stringref.h"

namespace Tofino {

/** Given a bitrange to allocate into the ixbar hash matrix, as well as a list of fields to
 *  be the identity, this coordinates the field slice to a portion of the bit range.  This
 *  really only applies for identity matches.
 */
void emit_ixbar_hash_dist_ident(const PhvInfo &phv, std::ostream &out,
        indent_t indent, safe_vector<Slice> &match_data,
        const Tofino::IXBar::Use::HashDistHash &hdh,
        const safe_vector<const IR::Expression *> & /*field_list_order*/) {
    if (hdh.hash_gen_expr) {
        int hash_gen_expr_width = hdh.hash_gen_expr->type->width_bits();
        BUG_CHECK(hash_gen_expr_width > 0, "zero width hash expression: %s ?", hdh.hash_gen_expr);
        for (auto bit_pos : hdh.galois_start_bit_to_p4_hash) {
            int out_bit = bit_pos.first, in_bit = bit_pos.second.lo;
            while (in_bit <= bit_pos.second.hi && in_bit < hash_gen_expr_width) {
                int width = FormatHash::SliceWidth(phv, hdh.hash_gen_expr, in_bit, match_data);
                le_bitrange slice(in_bit, in_bit + width - 1);
                slice = slice.intersectWith(bit_pos.second);
                out << indent << out_bit;
                if (width > 1 ) out << ".." << (out_bit + slice.size() - 1);
                out << ": ";
                if (!FormatHash::ZeroHash(phv, hdh.hash_gen_expr, slice, match_data)) {
                    FormatHash::Output(phv, out, hdh.hash_gen_expr, slice, match_data);
                } else {
                    out << "0"; }
                out << std::endl;
                in_bit += slice.size();
                out_bit += slice.size(); }
            BUG_CHECK(in_bit == bit_pos.second.hi + 1 || in_bit >= hash_gen_expr_width,
                      "mismatched hash width"); }
        return; }

    BUG("still need this code?");
#if 0
    int bits_seen = 0;
    for (auto it = field_list_order.rbegin(); it != field_list_order.rend(); it++) {
        auto fs = PHV::AbstractField::create(phv, *it);
        for (auto &sl : match_data) {
            if (!(fs->field() && fs->field() == sl.get_field()))
                continue;
            auto boost_sl = toClosedRange<RangeUnit::Bit, Endian::Little>
                                  (fs->range().intersectWith(sl.range()));
            if (boost_sl == std::nullopt)
                continue;
            // Which slice bits of this field are overlapping
            le_bitrange field_overlap = *boost_sl;
            int ident_range_lo = bits_seen + field_overlap.lo - fs->range().lo;
            int ident_range_hi = ident_range_lo + field_overlap.size() - 1;
            le_bitrange identity_range = { ident_range_lo, ident_range_hi };
            for (auto bit_pos : hdh.galois_start_bit_to_p4_hash) {
                // Portion of the p4_output_hash that overlaps with the identity range
                auto boost_sl2 = toClosedRange<RangeUnit::Bit, Endian::Little>
                                 (identity_range.intersectWith(bit_pos.second));
                if (boost_sl2 == std::nullopt)
                    continue;
                le_bitrange ident_overlap = *boost_sl2;
                int hash_lo = bit_pos.first + (ident_overlap.lo - bit_pos.second.lo);
                int hash_hi = hash_lo + ident_overlap.size() - 1;
                int field_range_lo = field_overlap.lo + (ident_overlap.lo - identity_range.lo);
                int field_range_hi = field_range_lo + ident_overlap.size() - 1;
                Slice asm_sl(fs->field(), field_range_lo, field_range_hi);
                safe_vector<Slice> ident_slice;
                ident_slice.push_back(asm_sl);
                out << indent << hash_lo << ".." << hash_hi << ": "
                    << FormatHash(&ident_slice, nullptr, nullptr, nullptr,
                                  IR::MAU::HashFunction::identity())
                    << std::endl;
            }
        }
        bits_seen += fs->size();
    }
#endif
}

void emit_ixbar_meter_alu_hash(const PhvInfo &phv, std::ostream &out, indent_t indent,
        const safe_vector<Slice> &match_data, const Tofino::IXBar::Use::MeterAluHash &mah,
        const safe_vector<const IR::Expression *> &field_list_order,
        const LTBitMatrix &sym_keys) {
    if (mah.algorithm.type == IR::MAU::HashFunction::IDENTITY) {
        auto mask = mah.bit_mask;
        for (auto &el : mah.computed_expressions) {
            el.second->apply(EmitHashExpression(phv, out, indent, el.first, match_data));
            mask.clrrange(el.first, el.second->type->width_bits());
        }
        for (int to_clear = mask.ffs(0); to_clear >= 0;) {
            int end = mask.ffz(to_clear);
            out << indent << to_clear;
            if (end - 1 > to_clear) out << ".." << (end - 1);
            out << ": 0" << std::endl;
            to_clear = mask.ffs(end); }
    } else {
        le_bitrange br = { mah.bit_mask.min().index(), mah.bit_mask.max().index() };
        int total_bits = 0;
        std::multimap<int, Slice> match_data_map;
        std::map<le_bitrange, const IR::Constant*> constant_map;
        bool use_map = false;
        if (mah.algorithm.ordered()) {
            emit_ixbar_gather_map(phv, match_data_map, constant_map, match_data,
                    field_list_order, sym_keys, total_bits);
            use_map = true;
        }
        out << indent << br.lo << ".." << br.hi << ": ";
        if (use_map)
            out << FormatHash(nullptr, &match_data_map, nullptr, nullptr,
                    mah.algorithm, total_bits, &br);
        else
            out << FormatHash(&match_data, nullptr, nullptr, nullptr,
                    mah.algorithm, total_bits, &br);
        out << std::endl;
    }
}

void emit_ixbar_proxy_hash(const PhvInfo &phv, std::ostream &out, indent_t indent,
        safe_vector<Slice> &match_data, const Tofino::IXBar::Use::ProxyHashKey &ph,
        const safe_vector<const IR::Expression *> &field_list_order,
        const LTBitMatrix &sym_keys) {
    int start_bit = ph.hash_bits.ffs();
    do {
        int end_bit = ph.hash_bits.ffz(start_bit);
        le_bitrange br = { start_bit, end_bit - 1 };
        int total_bits = 0;
        out << indent << br.lo << ".." << br.hi << ": ";
        if (ph.algorithm.ordered()) {
            std::multimap<int, Slice> match_data_map;
            std::map<le_bitrange, const IR::Constant*> constant_map;
            emit_ixbar_gather_map(phv, match_data_map, constant_map, match_data,
                    field_list_order, sym_keys, total_bits);
            out << FormatHash(nullptr, &match_data_map, nullptr, nullptr,
                    ph.algorithm, total_bits, &br);
        } else {
            out << FormatHash(&match_data, nullptr, nullptr, nullptr,
                    ph.algorithm, total_bits, &br);
        }
        out << std::endl;
        start_bit = ph.hash_bits.ffs(end_bit);
    } while (start_bit >= 0);
}

/* Generate asm for the hash of a table, specifically either a match, gateway, or selector
   table.  Also used for hash distribution hash */
void emit_ixbar_hash(const PhvInfo &phv, std::ostream &out, indent_t indent,
        safe_vector<Slice> &match_data, safe_vector<Slice> &ghost, const ::IXBar::Use *use_,
        int hash_group, int &ident_bits_prev_alloc) {
    auto *use = dynamic_cast<const Tofino::IXBar::Use *>(use_);
    if (!use) return;
    if (!use->way_use.empty()) {
        emit_ixbar_hash_exact(out, indent, match_data, ghost, use, hash_group,
                              ident_bits_prev_alloc);
    }

    if (use->meter_alu_hash.allocated) {
        emit_ixbar_meter_alu_hash(phv, out, indent, match_data, use->meter_alu_hash,
                                  use->field_list_order, use->symmetric_keys);
    }

    if (use->proxy_hash_key_use.allocated) {
        emit_ixbar_proxy_hash(phv, out, indent, match_data, use->proxy_hash_key_use,
                              use->field_list_order, use->symmetric_keys);
    }


    // Printing out the hash for gateway tables
    for (auto ident : use->bit_use) {
        if (ident.valid) {
            int bit = 40 + ident.bit;
            out << indent << "valid " << bit;
            out << ": ";
            out << "0x" << hex(1 << ident.group) << std::endl;
        } else {
            // Gateway fields in the hash are continuous bitranges, but may not match up
            // with the fields.  So we figure out the overlap between each use and each
            // match field and split them up where they don't match.  Do we really need to
            // do this?
            Slice range_sl(phv, ident.field, ident.lo, ident.lo + ident.width - 1);
            for (auto sl : match_data) {
                auto overlap = range_sl & sl;
                if (!overlap) continue;
                int bit = 40 + ident.bit + overlap.get_lo() - range_sl.get_lo();
                out << indent << bit;
                if (overlap.width() > 1)
                    out << ".." << (bit + overlap.width() - 1);
                out << ": " << overlap << std:: endl;
            }
        }
    }


    if (use->hash_dist_hash.allocated) {
        auto &hdh = use->hash_dist_hash;
        if (hdh.algorithm.type == IR::MAU::HashFunction::IDENTITY) {
            emit_ixbar_hash_dist_ident(phv, out, indent, match_data, hdh, use->field_list_order);
            return;
        }
        std::multimap<int, Slice> match_data_map;
        std::map<le_bitrange, const IR::Constant*> constant_map;
        bool use_map = false;
        int total_bits = 0;
        if (hdh.algorithm.ordered()) {
            emit_ixbar_gather_map(phv, match_data_map, constant_map, match_data,
                    use->field_list_order, use->symmetric_keys, total_bits);
            use_map = true;
        }

        for (auto bit_start : hdh.galois_start_bit_to_p4_hash) {
            int start_bit = bit_start.first;
            le_bitrange br = bit_start.second;
            int end_bit = start_bit + br.size() - 1;
            out << indent << start_bit << ".." << end_bit;
            if (use_map)
                out << ": " << FormatHash(nullptr, &match_data_map, &constant_map,
                        nullptr, hdh.algorithm, total_bits, &br);
            else
                out << ": " << FormatHash(&match_data, nullptr, nullptr,
                        nullptr, hdh.algorithm, total_bits, &br);
            out << std::endl;
        }
    }
}

void Tofino::IXBar::Use::emit_ixbar_asm(const PhvInfo &phv, std::ostream &out, indent_t indent,
        const TableMatch *fmt, const IR::MAU::Table *tbl) const {
    std::map<int, std::map<int, Slice>> sort;
    std::map<int, std::map<int, Slice>> midbytes;
    emit_ixbar_gather_bytes(phv, use, sort, midbytes, tbl,
                            type == IXBar::Use::TERNARY_MATCH);
    cstring group_type = type == IXBar::Use::TERNARY_MATCH ? "ternary"_cs : "exact"_cs;
    for (auto &group : sort)
        out << indent << group_type << " group "
            << group.first << ": " << group.second << std::endl;
    for (auto &midbyte : midbytes)
        out << indent << "byte group "
            << midbyte.first << ": " << midbyte.second << std::endl;
    if (type == IXBar::Use::ATCAM_MATCH) {
        sort.clear();
        midbytes.clear();
        emit_ixbar_gather_bytes(phv, use, sort, midbytes, tbl,
                                type == IXBar::Use::TERNARY_MATCH,
                                type == IXBar::Use::ATCAM_MATCH);
    }
    for (int hash_group = 0; hash_group < IXBar::HASH_GROUPS; hash_group++) {
        unsigned hash_table_input = hash_table_inputs[hash_group];
        bitvec hash_seed = this->hash_seed[hash_group];
        int ident_bits_prev_alloc = 0;
        if (hash_table_input || hash_seed) {
            for (int ht : bitvec(hash_table_input)) {
                out << indent++ << "hash " << ht << ":" << std::endl;
                safe_vector<Slice> match_data;
                safe_vector<Slice> ghost;
                emit_ixbar_hash_table(ht, match_data, ghost, fmt, sort);
                // FIXME: This is obviously an issue for larger selector tables,
                //  whole function needs to be replaced
                emit_ixbar_hash(phv, out, indent, match_data, ghost, this, hash_group,
                                ident_bits_prev_alloc);
                if (is_parity_enabled())
                    out << indent << IXBar::HASH_PARITY_BIT << ": parity" << std::endl;
                --indent;
            }
            out << indent++ << "hash group " << hash_group << ":" << std::endl;
            if (hash_table_input)
                out << indent << "table: [" << emit_vector(bitvec(hash_table_input), ", ") << "]"
                    << std::endl;
            out << indent << "seed: 0x" << hash_seed << std::endl;
            if (is_parity_enabled())
                out << indent << "seed_parity: true" << std::endl;
            --indent;
        }
    }
}

bool Tofino::ActionDataBus::Use::emit_adb_asm(std::ostream &out, const IR::MAU::Table *tbl,
                                              bitvec source) const {
    auto &format = tbl->resources->action_format;
    auto &meter_use = tbl->resources->meter_format;

    bool first = true;
    for (auto &rs : action_data_locs) {
        if (!source.getbit(rs.source)) continue;
        auto source_is_immed = (rs.source == ActionData::IMMEDIATE);
        auto source_is_adt = (rs.source == ActionData::ACTION_DATA_TABLE);
        auto source_is_meter = (rs.source == ActionData::METER_ALU);
        BUG_CHECK(source_is_immed || source_is_adt || source_is_meter,
                  "bad action data source %1%", rs.source);
        if (source_is_meter &&
            !meter_use.contains_adb_slot(rs.location.type, rs.byte_offset)) continue;
        bitvec total_range(0, ActionData::slot_type_to_bits(rs.location.type));
        int byte_sz = ActionData::slot_type_to_bits(rs.location.type) / 8;
        if (!first)
            out << ", ";
        first = false;
        out << rs.location.byte;
        if (byte_sz > 1)
            out << ".." << (rs.location.byte + byte_sz - 1);
        out << " : ";

        // For emitting hash distribution sections on the action_bus directly.  Must find
        // which slices of hash distribution are to go to which bytes, requiring coordination
        // from the input xbar and action format allocation
        if (source_is_immed
            && format.is_byte_offset<ActionData::Hash>(rs.byte_offset)) {
            safe_vector<int> all_hash_dist_units = tbl->resources->hash_dist_immed_units();
            bitvec slot_hash_dist_units;
            int immed_lo = rs.byte_offset * 8;
            int immed_hi = immed_lo + (8 << rs.location.type) - 1;
            le_bitrange immed_range = { immed_lo, immed_hi };
            for (int i = 0; i < 2; i++) {
                le_bitrange immed_impact = { i * IXBar::HASH_DIST_BITS,
                                             (i + 1) * IXBar::HASH_DIST_BITS - 1 };
                if (!immed_impact.overlaps(immed_range))
                    continue;
                slot_hash_dist_units.setbit(i);
            }

            out << "hash_dist(";
            // Find the particular hash dist units (if 32 bit, still potentially only one if)
            // only certain bits are allocated
            std::string sep = "";
            for (auto bit : slot_hash_dist_units) {
                if (all_hash_dist_units.at(bit) < 0) continue;
                out << sep << all_hash_dist_units.at(bit);
                sep = ", ";
            }

            // Byte slots need a particular byte range of hash dist
            if (rs.location.type == ActionData::BYTE) {
                int slot_range_shift = (immed_range.lo / IXBar::HASH_DIST_BITS);
                slot_range_shift *= IXBar::HASH_DIST_BITS;
                le_bitrange slot_range = immed_range.shiftedByBits(-1 * slot_range_shift);
                out << ", " << slot_range.lo << ".." << slot_range.hi;
            }
            // 16 bit hash dist in a 32 bit slot have to determine whether the hash distribution
            // unit goes in the lo section or the hi section
            if (slot_hash_dist_units.popcount() == 1) {
                cstring lo_hi = slot_hash_dist_units.getbit(0) ? "lo"_cs : "hi"_cs;
                out << ", " << lo_hi;
            }
            out << ")";
        } else if (source_is_immed
                   && format.is_byte_offset<ActionData::RandomNumber>(rs.byte_offset)) {
            int rng_unit = tbl->resources->rng_unit();
            out << "rng(" << rng_unit << ", ";
            int lo = rs.byte_offset * 8;
            int hi = lo + byte_sz * 8 - 1;
            out << lo << ".." << hi << ")";
        } else if (source_is_immed
                   && format.is_byte_offset<ActionData::MeterColor>(rs.byte_offset)) {
            for (auto back_at : tbl->attached) {
                auto at = back_at->attached;
                auto *mtr = at->to<IR::MAU::Meter>();
                if (mtr == nullptr) continue;
                out << MauAsmOutput::find_attached_name(tbl, mtr) << " color";
                break;
            }
        } else if (source_is_adt || source_is_immed) {
            out << format.get_format_name(rs.location.type, rs.source, rs.byte_offset);
        } else if (source_is_meter) {
            auto *at = tbl->get_attached<IR::MAU::MeterBus2Port>();
            BUG_CHECK(at != nullptr, "Trying to emit meter alu without meter alu user");
            cstring ret_name = MauAsmOutput::find_attached_name(tbl, at);
            out << ret_name;
            out << "(" << (rs.byte_offset * 8) << ".." << ((rs.byte_offset + byte_sz) * 8 - 1)
                << ")";
        } else {
            BUG("unhandled case in emit_adb_asm");
        }
    }
    return !first;
}

void MauAsmOutput::emit_table_format(std::ostream &out, indent_t indent,
        const TableFormat::Use &use, const TableMatch *tm,
        bool ternary, bool no_match) const {
    ::MauAsmOutput::emit_table_format(out, indent, use, tm, ternary, no_match);

    if (!use.match_group_map.empty()) {
        out << indent << "match_group_map: [ ";
        std::string sep = "";
        for (auto ram_entries : use.match_group_map) {
            out << sep << "[ " << emit_vector(ram_entries) << " ]";
            sep = ", ";
        }
        out << " ]" << std::endl;
        indent--;
    }
}

void MauAsmOutput::emit_memory(std::ostream &out, indent_t indent, const Memories::Use &mem,
         const IR::MAU::Table::Layout *layout, const TableFormat::Use *format) const {
    ::MauAsmOutput::emit_memory(out, indent, mem, layout, format);
}

}  // end Tofino namespace
